const fs = require('fs');
const path = require('path');

// Asegurar que recibimos la ruta
const filePath = process.argv[2];
if (!filePath) {
    console.error("Debes proporcionar una ruta absoluta.");
    process.exit(1);
}

// Raíz del proyecto Arkhe (suponiendo que el script se aloja en <raiz>/.agent/scripts/)
const workspaceRoot = path.resolve(__dirname, '..', '..');

const filename = path.basename(filePath);
const ext = path.extname(filePath).toLowerCase();

let nodeCode = '';
let mode = 'MSC2020';
let found = false;

// Intentar leer desde entries_index.json primero
const indexPath = path.join(workspaceRoot, 'assets', 'entries', 'entries_index.json');
if (fs.existsSync(indexPath)) {
    try {
        const index = JSON.parse(fs.readFileSync(indexPath, 'utf8'));
        for (const [code, fname] of Object.entries(index)) {
            if (fname === filename) {
                mode = code.startsWith('Mathlib') ? 'Mathlib' : 'MSC2020';
                nodeCode = code;
                found = true;
                break;
            }
        }
    } catch (_) {}
}

if (!found) {
    if (ext === '.lean') {
        const normalized = filePath.replace(/\\/g, '/');
        const mathlibIdx = normalized.lastIndexOf('/Mathlib/');
        if (mathlibIdx !== -1) {
            const relative = normalized.slice(mathlibIdx + 1);
            nodeCode = relative.replace(/\//g, '.').replace(/\.lean$/, '');
            mode = 'Mathlib';
        } else {
            const base = path.basename(filePath, '.lean');
            nodeCode = base.replace(/_/g, '.');
            mode = 'Mathlib';
        }
    } else {
        const base = path.basename(filePath, ext);
        nodeCode = base.replace(/_/g, '.');
        mode = 'MSC2020';
    }
}

// Escribir bridge en la raíz
const bridgePath = path.join(workspaceRoot, 'arkhe_bridge.json');
const payload = {
    direction: 'antigravity',
    mode,
    node_code: nodeCode,
    node_label: '',
    tex_file: filePath,
    timestamp: Math.floor(Date.now() / 1000)
};

const payloadJson = JSON.stringify(payload, null, 2);

try {
    fs.writeFileSync(bridgePath, payloadJson, 'utf8');
    
    // También intentar escribir en out/build/x64-debug para evitar problemas con CWD de CMake/Visual Studio
    const debugDir = path.join(workspaceRoot, 'out', 'build', 'x64-debug');
    if (fs.existsSync(debugDir)) {
        fs.writeFileSync(path.join(debugDir, 'arkhe_bridge.json'), payloadJson, 'utf8');
    }
    
    const releaseDir = path.join(workspaceRoot, 'out', 'build', 'x64-release');
    if (fs.existsSync(releaseDir)) {
        fs.writeFileSync(path.join(releaseDir, 'arkhe_bridge.json'), payloadJson, 'utf8');
    }

    console.log(`✅ [Antigravity Bridge] Enviado con éxito → ${nodeCode} (${mode})\n${filePath}`);
} catch (err) {
    console.error(`Error al escribir el puente: ${err.message}`);
    process.exit(1);
}
