# ─────────────────────────────────────────────────────────────────────────────
# tools/bridge_watch.ps1
#
# Lado VS Code del bridge Arkhe.
#
# Modo 1 — "push" (VS Code → Arkhe):
#   Detecta el archivo activo en VS Code y escribe arkhe_bridge.json.
#   Arkhe lo lee en el siguiente frame con bridge_poll() y navega al nodo.
#
# Modo 2 — "pull" (Arkhe → VS Code):
#   Lee arkhe_bridge.json (escrito por Arkhe con bridge_write()).
#   Si hay un tex_file válido, lo abre en VS Code.
#
# Uso desde terminal de VS Code (o Task):
#   # Iniciar watcher (polling cada 2s, modo push):
#   .\tools\bridge_watch.ps1 -Mode push -BridgeFile "arkhe_bridge.json"
#
#   # Enviar archivo activo manualmente (modo push-once):
#   .\tools\bridge_watch.ps1 -Mode push-once -File "assets/entries/foo.tex"
#
#   # Abrir lo que Arkhe mandó (modo pull-once):
#   .\tools\bridge_watch.ps1 -Mode pull-once
#
# Integración recomendada: añadir en .vscode/tasks.json (ver al final del script)
# ─────────────────────────────────────────────────────────────────────────────

param(
    [ValidateSet("push", "push-once", "pull-once", "watch")]
    [string]$Mode       = "watch",

    [string]$BridgeFile = "arkhe_bridge.json",   # relativo al workspace
    [string]$File       = "",                     # para push-once
    [int]   $PollSecs   = 2
)

# ── Helper: resolver ruta del workspace ──────────────────────────────────────
$WorkspaceRoot = if ($env:VSCODE_CWD) { $env:VSCODE_CWD }
                 elseif ($PSScriptRoot) { Split-Path $PSScriptRoot -Parent }
                 else { Get-Location }

$BridgePath = Join-Path $WorkspaceRoot $BridgeFile

function Write-Bridge($direction, $mode, $nodeCode, $nodeLabel, $texFile) {
    $ts = [DateTimeOffset]::UtcNow.ToUnixTimeSeconds()
    $obj = [ordered]@{
        direction  = $direction
        mode       = $mode
        node_code  = $nodeCode
        node_label = $nodeLabel
        tex_file   = $texFile
        timestamp  = $ts
    }
    $obj | ConvertTo-Json -Depth 2 | Set-Content -Path $BridgePath -Encoding UTF8
    Write-Host "[bridge] Escrito: direction=$direction node=$nodeCode"
}

function Read-Bridge {
    if (-not (Test-Path $BridgePath)) { return $null }
    try   { return Get-Content $BridgePath -Raw | ConvertFrom-Json }
    catch { return $null }
}

# ── Inferir código de nodo a partir de ruta .tex ─────────────────────────────
# Convierte "assets/entries/Mathlib_Algebra_Group_Basic.tex"
# → "Mathlib.Algebra.Group.Basic"
# Si el archivo está en entries_index.json, usa ese mapeo inverso.
function Resolve-NodeCode($texPath) {
    # Intentar entries_index.json primero
    $indexPath = Join-Path $WorkspaceRoot "assets\entries\entries_index.json"
    if (Test-Path $indexPath) {
        $idx = Get-Content $indexPath -Raw | ConvertFrom-Json
        $filename = Split-Path $texPath -Leaf
        foreach ($prop in $idx.PSObject.Properties) {
            if ($prop.Value -eq $filename) { return $prop.Name }
        }
    }
    # Fallback: revertir safe_filename (reemplazar _ por .)
    $base = [System.IO.Path]::GetFileNameWithoutExtension($texPath)
    return $base -replace '_', '.'
}

# ─────────────────────────────────────────────────────────────────────────────
# Modos de operación
# ─────────────────────────────────────────────────────────────────────────────

switch ($Mode) {

    # ── push-once: enviar un archivo específico a Arkhe ──────────────────────
    "push-once" {
        if (-not $File) { Write-Error "push-once requiere -File <ruta>"; exit 1 }
        $absFile  = if ([System.IO.Path]::IsPathRooted($File)) { $File }
                    else { Join-Path $WorkspaceRoot $File }
        $nodeCode = Resolve-NodeCode $absFile
        Write-Bridge "vscode" "Mathlib" $nodeCode "" $absFile
    }

    # ── pull-once: abrir en VS Code lo que Arkhe mandó ───────────────────────
    "pull-once" {
        $b = Read-Bridge
        if ($null -eq $b)                  { Write-Host "[bridge] No hay bridge."; exit 0 }
        if ($b.direction -ne "arkhe")      { Write-Host "[bridge] Nada nuevo de Arkhe."; exit 0 }
        if ([string]::IsNullOrEmpty($b.tex_file)) {
            Write-Host "[bridge] Sin tex_file en bridge."; exit 0
        }
        Write-Host "[bridge] Abriendo: $($b.tex_file)"
        & code $b.tex_file
    }

    # ── push: watcher continuo — detecta cambios en el editor ────────────────
    # VS Code no expone el archivo activo fácilmente desde fuera.
    # Este modo hace polling de un archivo sentinela que el usuario puede
    # actualizar con el atajo de teclado configurado en keybindings.json:
    #
    #   { "key": "ctrl+shift+b", "command": "workbench.action.tasks.runTask",
    #     "args": "Arkhe: send current file" }
    #
    # La task "Arkhe: send current file" en tasks.json llama:
    #   bridge_watch.ps1 -Mode push-once -File "${file}"
    "push" {
        Write-Host "[bridge] Watcher activo (Ctrl+C para salir)..."
        $lastTs = 0
        while ($true) {
            Start-Sleep -Seconds $PollSecs
            # Leer si Arkhe tiene algo nuevo para abrir
            $b = Read-Bridge
            if ($b -and $b.direction -eq "arkhe" -and $b.timestamp -gt $lastTs) {
                $lastTs = $b.timestamp
                if (-not [string]::IsNullOrEmpty($b.tex_file)) {
                    Write-Host "[bridge] Arkhe → abriendo $($b.tex_file)"
                    & code $b.tex_file
                }
            }
        }
    }

    # ── watch: alias de push (default) ───────────────────────────────────────
    "watch" {
        & $PSCommandPath -Mode push -BridgeFile $BridgeFile -PollSecs $PollSecs
    }
}

# ─────────────────────────────────────────────────────────────────────────────
# Fragmento sugerido para .vscode/tasks.json:
# ─────────────────────────────────────────────────────────────────────────────
# {
#   "version": "2.0.0",
#   "tasks": [
#     {
#       "label": "Arkhe: send current file",
#       "type": "shell",
#       "command": "powershell",
#       "args": [
#         "-ExecutionPolicy", "Bypass",
#         "-File", "${workspaceFolder}/tools/bridge_watch.ps1",
#         "-Mode", "push-once",
#         "-File", "${file}"
#       ],
#       "presentation": { "reveal": "silent", "panel": "shared" },
#       "problemMatcher": []
#     },
#     {
#       "label": "Arkhe: open current node",
#       "type": "shell",
#       "command": "powershell",
#       "args": [
#         "-ExecutionPolicy", "Bypass",
#         "-File", "${workspaceFolder}/tools/bridge_watch.ps1",
#         "-Mode", "pull-once"
#       ],
#       "presentation": { "reveal": "silent", "panel": "shared" },
#       "problemMatcher": []
#     },
#     {
#       "label": "Arkhe: start bridge watcher",
#       "type": "shell",
#       "command": "powershell",
#       "args": [
#         "-ExecutionPolicy", "Bypass",
#         "-File", "${workspaceFolder}/tools/bridge_watch.ps1",
#         "-Mode", "watch"
#       ],
#       "isBackground": true,
#       "presentation": { "reveal": "always", "panel": "dedicated" },
#       "problemMatcher": []
#     }
#   ]
# }
# ─────────────────────────────────────────────────────────────────────────────
