// ─────────────────────────────────────────────────────────────────────────────
// tools/arkhe-bridge/extension.ts
//
// Extensión VS Code — bridge unidireccional VS Code → Arkhe.
//
// Comando: arkhe.sendToArkhe  (Ctrl+Shift+Alt+A)
//   1. Lee el archivo activo en el editor.
//   2. Resuelve su node_code consultando entries_index.json (o por nombre).
//   3. Escribe arkhe_bridge.json con direction="vscode".
//   4. Si Arkhe no está corriendo y hay executablePath configurado, lo lanza.
// ─────────────────────────────────────────────────────────────────────────────

import * as vscode from 'vscode';
import * as fs from 'fs';
import * as path from 'path';
import { execSync, spawn } from 'child_process';

// ── Interfaces de Datos ───────────────────────────────────────────────────────

interface Declaration {
    kind: string;
    name: string;
    line: number;
}

interface MathFile {
    code: string;
    label: string;
    path: string;
    declarations?: Declaration[];
}

interface Folder {
    code: string;
    label: string;
    path: string;
    children?: Folder[];
    files?: MathFile[];
}

interface Layout {
    root: string;
    folders: Folder[];
}

interface IndexEntries {
    [code: string]: string;
}

// Ruta del bridge relativa a la raíz del workspace
const BRIDGE_RELATIVE = 'arkhe_bridge.json';

// Cache de assets
let cachedLayout: Layout | null = null;
let cachedIndex: IndexEntries | null = null;
let lastWorkspaceRoot: string | null = null;

// ── Activación ────────────────────────────────────────────────────────────────

export function activate(context: vscode.ExtensionContext) {
    context.subscriptions.push(
        vscode.commands.registerCommand('arkhe.sendToArkhe', sendToArkhe),
        vscode.commands.registerCommand('arkhe.setMathlibPath', setMathlibPath),
        vscode.commands.registerCommand('arkhe.setArkhePath', setArkhePath)
    );
}

// ── Comando principal ─────────────────────────────────────────────────────────

async function sendToArkhe(uri?: vscode.Uri) {
    let filePath: string;
    
    if (uri && uri.fsPath) {
        filePath = uri.fsPath;
    } else {
        const editor = vscode.window.activeTextEditor;
        if (!editor) {
            vscode.window.showWarningMessage('Arkhe Bridge: no hay archivo activo y no se proporcionó URI.');
            return;
        }
        filePath = editor.document.uri.fsPath;
    }

    const workspaceRoot = getWorkspaceRoot();
    if (!workspaceRoot) {
        vscode.window.showErrorMessage('Arkhe Bridge: no hay workspace abierto.');
        return;
    }

    const bridgePath = path.join(workspaceRoot, BRIDGE_RELATIVE);

    // Asegurar que directorio existe
    fs.mkdirSync(path.dirname(bridgePath), { recursive: true });

    // Cargar assets si es necesario
    await loadAssets(workspaceRoot);

    // Resolver nodo y modo
    const editor = vscode.window.activeTextEditor;
    const currentLine = editor ? editor.selection.active.line + 1 : 0;
    
    const { nodeCode, nodeLabel, texFilename, mode } = resolveNodeWithAssets(workspaceRoot, filePath, currentLine);

    // Escribir bridge
    const payload = {
        direction:  'vscode',
        mode,
        node_code:  nodeCode,
        node_label: nodeLabel,
        tex_file:   texFilename,
        timestamp:  Math.floor(Date.now() / 1000)
    };
    
    const payloadJson = JSON.stringify(payload, null, 2);
    try {
        console.log(`Escribiendo bridge en: ${bridgePath}`);
        fs.writeFileSync(bridgePath, payloadJson, 'utf8');
        vscode.window.showInformationMessage(`Bridge escrito en: ${bridgePath}`);
        
        // También escribir en CWD del binario (CMake) si existe, para cuando se corre desde Visual Studio
        const debugDir = path.join(workspaceRoot, 'out', 'build', 'x64-debug');
        if (fs.existsSync(debugDir)) {
            console.log(`Escribiendo bridge en debugDir: ${debugDir}`);
            fs.writeFileSync(path.join(debugDir, BRIDGE_RELATIVE), payloadJson, 'utf8');
        }
        
        const releaseDir = path.join(workspaceRoot, 'out', 'build', 'x64-release');
        if (fs.existsSync(releaseDir)) {
            fs.writeFileSync(path.join(releaseDir, BRIDGE_RELATIVE), payloadJson, 'utf8');
        }
        vscode.window.setStatusBarMessage(`$(check) Arkhe Bridge: ${nodeCode}`, 5000);
    } catch (err: any) {
        vscode.window.showErrorMessage(`Arkhe Bridge: no se pudo escribir el bridge.\n${err.message}`);
        return;
    }

    // Lanzar Arkhe si no está corriendo
    const config   = vscode.workspace.getConfiguration('arkhe');
    const execPath = config.get<string>('executablePath', '').trim();
    const cwd      = config.get<string>('workingDirectory', '').trim() || workspaceRoot;

    if (isArkheRunning()) {
        // Arkhe ya está corriendo; leerá el bridge en el siguiente frame
        vscode.window.showInformationMessage(`Arkhe Bridge: enviado → ${nodeCode}`);
    } else if (execPath) {
        if (!fs.existsSync(execPath)) {
            vscode.window.showErrorMessage(
                `Arkhe Bridge: ejecutable no encontrado en "${execPath}".`
            );
            return;
        }
        launchArkhe(execPath, cwd);
        vscode.window.showInformationMessage(`Arkhe Bridge: Arkhe lanzado → ${nodeCode}`);
    } else {
        // Arkhe no está corriendo y no hay ruta configurada
        vscode.window.showWarningMessage(
            'Arkhe Bridge: bridge escrito, pero Arkhe no está corriendo. ' +
            'Configura "arkhe.executablePath" para lanzarlo automáticamente.'
        );
    }
}

// ── Helpers ───────────────────────────────────────────────────────────────────

// Devuelve la raíz del repositorio de Arkhe.
// Prioridad: 
// 1. Configuración arkhe.arkheSourcePath
// 2. Carpeta del workspace que se llame "Arkhe" o contenga "arkhe_bridge.json"
// 3. Primera carpeta del workspace
function getWorkspaceRoot(): string | null {
    const config = vscode.workspace.getConfiguration('arkhe');
    const configPath = config.get<string>('arkheSourcePath', '').trim();
    
    if (configPath && fs.existsSync(configPath)) {
        return configPath;
    }

    const folders = vscode.workspace.workspaceFolders;
    if (!folders || folders.length === 0) return null;

    // Buscar carpeta que parezca ser de Arkhe
    for (const folder of folders) {
        const folderPath = folder.uri.fsPath;
        if (folder.name.toLowerCase().includes('arkhe')) return folderPath;
        if (fs.existsSync(path.join(folderPath, BRIDGE_RELATIVE))) return folderPath;
    }

    return folders[0].uri.fsPath;
}

// ── Gestión de Assets ─────────────────────────────────────────────────────────

async function loadAssets(workspaceRoot: string) {
    if (cachedLayout && cachedIndex && lastWorkspaceRoot === workspaceRoot) return;

    // Priorizar carpetas de salida de compilación
    const buildAssets = path.join(workspaceRoot, 'out', 'build', 'x64-debug', 'assets');
    const releaseAssets = path.join(workspaceRoot, 'out', 'build', 'x64-release', 'assets');
    const rootAssets = path.join(workspaceRoot, 'assets');

    let assetDir = rootAssets;
    if (fs.existsSync(path.join(buildAssets, 'mathlib_layout.json'))) {
        assetDir = buildAssets;
    } else if (fs.existsSync(path.join(releaseAssets, 'mathlib_layout.json'))) {
        assetDir = releaseAssets;
    }

    const layoutPath = path.join(assetDir, 'mathlib_layout.json');
    const indexPath = path.join(assetDir, 'entries', 'entries_index.json');

    try {
        if (fs.existsSync(layoutPath)) {
            const data = fs.readFileSync(layoutPath, 'utf8');
            cachedLayout = JSON.parse(data);
        }
        if (fs.existsSync(indexPath)) {
            const data = fs.readFileSync(indexPath, 'utf8');
            cachedIndex = JSON.parse(data);
        }
        lastWorkspaceRoot = workspaceRoot;
    } catch (err) {
        console.error('Error cargando assets:', err);
    }
}

function resolveNodeWithAssets(workspaceRoot: string, filePath: string, line: number): { nodeCode: string, nodeLabel: string, texFilename: string, mode: string } {
    const config = vscode.workspace.getConfiguration('arkhe');
    const mathlibSource = config.get<string>('mathlibSourcePath', '').replace(/\\/g, '/');
    const normalizedPath = filePath.replace(/\\/g, '/');

    let mode = 'MSC2020';
    let nodeCode = '';
    let nodeLabel = '';
    let texFilename = '';

    // Intentar resolver en Mathlib
    if (cachedLayout) {
        // Encontrar archivo en el layout
        const fileNode = findFileInLayout(cachedLayout.folders, normalizedPath, mathlibSource);
        if (fileNode) {
            mode = 'Mathlib';
            nodeCode = fileNode.code;
            nodeLabel = fileNode.label;

            // Buscar declaración por línea
            if (fileNode.declarations && line > 0) {
                let bestDecl: Declaration | null = null;
                for (const decl of fileNode.declarations) {
                    if (decl.line <= line) {
                        if (!bestDecl || decl.line > bestDecl.line) {
                            bestDecl = decl;
                        }
                    }
                }
                if (bestDecl) {
                    nodeCode = `${bestDecl.kind} ${bestDecl.name}`;
                    nodeLabel = bestDecl.name;
                }
            }
        }
    }

    // Fallback si no hay layout o no se encontró
    if (!nodeCode) {
        const res = resolveNodeLegacy(workspaceRoot, filePath);
        nodeCode = res.nodeCode;
        mode = res.mode;
    }

    // Buscar en entries_index
    if (cachedIndex && cachedIndex[nodeCode]) {
        texFilename = cachedIndex[nodeCode];
    } else {
        // Generar placeholder con guiones bajos (evita puntos y espacios)
        texFilename = nodeCode.replace(/\./g, '_').replace(/ /g, '_') + ".tex";
    }

    return { nodeCode, nodeLabel, texFilename, mode };
}

function findFileInLayout(folders: Folder[], fullPath: string, mathlibSource: string): MathFile | null {
    // Normalizar path relativo a Mathlib
    let relPath = fullPath;
    if (mathlibSource && fullPath.toLowerCase().startsWith(mathlibSource.toLowerCase())) {
        relPath = fullPath.slice(mathlibSource.length);
        if (relPath.startsWith('/')) relPath = relPath.slice(1);
    } else {
        const idx = fullPath.lastIndexOf('/Mathlib/');
        if (idx !== -1) relPath = fullPath.slice(idx + 1);
    }

    // Búsqueda recursiva
    function search(folders: Folder[]): MathFile | null {
        for (const f of folders) {
            if (f.files) {
                for (const file of f.files) {
                    if (file.path === relPath) return file;
                }
            }
            if (f.children) {
                const found = search(f.children);
                if (found) return found;
            }
        }
        return null;
    }

    return search(folders);
}

function resolveNodeLegacy(workspaceRoot: string, filePath: string): { nodeCode: string; mode: string } {
    const filename = path.basename(filePath);
    const ext      = path.extname(filePath).toLowerCase();
    const config   = vscode.workspace.getConfiguration('arkhe');
    const mathlibSource = config.get<string>('mathlibSourcePath', '').replace(/\\/g, '/');

    // ── 1. entries_index.json ──────────────────────────────────────────────
    const indexPath = path.join(workspaceRoot, 'assets', 'entries', 'entries_index.json');
    if (fs.existsSync(indexPath)) {
        try {
            const index = JSON.parse(fs.readFileSync(indexPath, 'utf8'));
            for (const [code, fname] of Object.entries(index) as [string, unknown][]) {
                if (fname === filename) {
                    const mode = code.startsWith('Mathlib') ? 'Mathlib' : 'MSC2020';
                    return { nodeCode: code, mode };
                }
            }
        } catch (_) {}
    }

    // ── 2. Archivo .lean → Mathlib ─────────────────────────────────────────
    if (ext === '.lean') {
        const normalized = filePath.replace(/\\/g, '/');
        
        if (mathlibSource && normalized.toLowerCase().startsWith(mathlibSource.toLowerCase())) {
            let relative = normalized.slice(mathlibSource.length);
            if (relative.startsWith('/')) relative = relative.slice(1);
            const nodeCode = relative.replace(/\//g, '.').replace(/\.lean$/, '');
            return { nodeCode, mode: 'Mathlib' };
        }

        const mathlibIdx = normalized.lastIndexOf('/Mathlib/');
        if (mathlibIdx !== -1) {
            const relative = normalized.slice(mathlibIdx + 1);
            const nodeCode = relative.replace(/\//g, '.').replace(/\.lean$/, '');
            return { nodeCode, mode: 'Mathlib' };
        }

        const base = path.basename(filePath, '.lean');
        return { nodeCode: base.replace(/_/g, '.'), mode: 'Mathlib' };
    }

    const base = path.basename(filePath, ext);
    return { nodeCode: base.replace(/_/g, '.'), mode: 'MSC2020' };
}

// Comprueba si hay un proceso llamado "arkhe" (o "arkhe.exe") corriendo.
function isArkheRunning(): boolean {
    try {
        if (process.platform === 'win32') {
            const out = execSync(
                'tasklist /FI "IMAGENAME eq arkhe.exe" /NH 2>NUL',
                { encoding: 'utf8', timeout: 3000 }
            );
            return out.toLowerCase().includes('arkhe.exe');
        } else {
            // Linux / macOS
            execSync('pgrep -x arkhe', { encoding: 'utf8', timeout: 3000 });
            return true;
        }
    } catch (_) {
        return false;
    }
}

// Lanza Arkhe como proceso independiente (detached) sin bloquear VS Code.
function launchArkhe(execPath: string, cwd: string): void {
    const child = spawn(execPath, [], {
        cwd,
        detached: true,
        stdio:    'ignore'
    });
    child.unref(); // VS Code no espera a que Arkhe termine
}

// ── Desactivación ─────────────────────────────────────────────────────────────

export function deactivate() {}

// ── Comando para configurar ruta de Mathlib ──────────────────────────────────

async function setMathlibPath() {
    const config = vscode.workspace.getConfiguration('arkhe');
    const currentPath = config.get<string>('mathlibSourcePath', '');

    const newPath = await vscode.window.showInputBox({
        title: 'Configurar ruta de repositorio Mathlib',
        value: currentPath,
        placeHolder: 'Ej: C:/Users/USUARIO/Documents/mathlib_clone',
        prompt: 'Esta ruta se usará para resolver archivos .lean fuera del workspace.'
    });

    if (newPath !== undefined) {
        await config.update('mathlibSourcePath', newPath.trim(), vscode.ConfigurationTarget.Global);
        vscode.window.showInformationMessage(`Ruta de Mathlib actualizada a: ${newPath}`);
    }
}

// ── Comando para configurar ruta de Arkhe ────────────────────────────────────

async function setArkhePath() {
    const config = vscode.workspace.getConfiguration('arkhe');
    const currentPath = config.get<string>('arkheSourcePath', '');

    const newPath = await vscode.window.showInputBox({
        title: 'Configurar ruta raíz de Arkhe',
        value: currentPath,
        placeHolder: 'Ej: C:/Users/USUARIO/source/repos/Arkhe',
        prompt: 'Aquí es donde se escribirá el archivo arkhe_bridge.json.'
    });

    if (newPath !== undefined) {
        await config.update('arkheSourcePath', newPath.trim(), vscode.ConfigurationTarget.Global);
        vscode.window.showInformationMessage(`Ruta de Arkhe actualizada a: ${newPath}`);
    }
}
