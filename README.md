# Arkhe

**Arkhe** is a visual explorer and editor for formal mathematical structures. It combines the MSC2020 classification, the Lean 4 Mathlib library, and custom academic syllabi into a single navigable graph — rendered through a force-directed physics engine with a high-performance 2D renderer.

---

## Features

### Three Navigation Modes
| Mode | Description |
|------|-------------|
| **MSC2020** | AMS Mathematics Subject Classification — 97 top-level areas, ~6,000 topics |
| **Mathlib** | Lean 4 formal library — folders, files, and individual declarations |
| **Standard** | Custom academic syllabus linking MSC2020 topics to Mathlib modules |

### Dependency Graph
Visualizes directed dependency relationships between nodes using a real-time force-directed simulation. Supports pan, zoom, keyboard navigation, and a pivot mode for deep graph exploration.

### Integrated Entry Editor
Per-node documentation editor with four sections:
- **Basic Info** — note, texture key, MSC refs
- **LaTeX Body** — full textarea with preview (requires TeX Live)
- **Cross-References** — typed links between nodes (`requires`, `extends`, `example_of`, `generalizes`, `equivalent_to`, `see_also`)
- **Resources** — links, books, papers, and other references

All data persists to `assets/entries/<code>.json` and `assets/entries/<code>.tex`.

### Search
- **Local fuzzy search** across the active tree (up to 2,000 results, paginated)
- **Loogle integration** — search Mathlib declarations by name, type, or signature (requires internet)

### Keyboard Navigation
Full keyboard control across all panels. `Tab` cycles focus zones: Canvas → Toolbar → Search → Info.

---

## Getting Started

### Requirements
- Windows 10/11 x64
- No additional installs required

### Installation
1. Download `Arkhe-vX.X.zip` from [Releases](../../releases)
2. Extract anywhere
3. Run `Arkhe.exe`

On first launch, open **Ubicaciones** (folder icon in the toolbar) to configure your asset paths.

### Optional: LaTeX Preview
To enable the LaTeX body preview in the Entry Editor:
1. Install [TeX Live](https://tug.org/texlive/)
2. Set the `pdflatex.exe` and `pdftoppm.exe` paths in Ubicaciones

### Optional: Mathlib Integration
To generate the Mathlib tree from source:
1. Clone [Mathlib4](https://github.com/leanprover-community/mathlib4)
2. Set the `Mathlib src` path in Ubicaciones
3. Use the **Generadores Mathlib** buttons to build `mathlib_layout.json` and `deps_mathlib.json`

---

## Navigation

| Input | Action |
|-------|--------|
| Click bubble | Navigate into node (if it has children) |
| Drag background | Pan camera |
| Scroll wheel | Zoom |
| `ESC` | Go up one level |
| `Tab` | Cycle focus zones |
| `←` / `→` | Switch view mode |
| `D` | Toggle dependency graph view |
| `S` | Toggle bubble ↔ dependency view |
| `C` | Return to root |
| `+` / `-` | Zoom in / out |

In the **dependency graph**:

| Input | Action |
|-------|--------|
| Click node | Re-focus graph on that node |
| `Backspace` | Navigate back in history |
| `Shift` + arrows | Pan camera |
| `Ctrl` | Activate pivot mode |

---

## Project Structure

```
Arkhe/
├── assets/
│   ├── fonts/              # Main typeface
│   ├── graphics/           # Textures, nine-patch skin, icon
│   ├── entries/            # Per-node .tex and .json files (user data)
│   ├── msc2020_tree.json   # Full MSC2020 classification tree
│   ├── crossref.json       # Mathlib module → MSC + Standard mapping
│   ├── deps.json           # MSC dependency graph
│   └── deps_mathlib.json   # Mathlib dependency graph
└── data/
└── ui/
    
```

Or open the folder directly in Visual Studio and select the `x64-Release` configuration.

---

## License

MIT — see [LICENSE](LICENSE)
