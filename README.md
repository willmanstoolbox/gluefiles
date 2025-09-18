# GlueFiles — Willman’s Toolbox

> Drag-and-drop file concatenator (Qt6). Free and open.

- **Website:** https://www.willmanstoolbox.com
- **Project page:** https://www.willmanstoolbox.com/gluefiles/
- **Repo:** https://github.com/willmanstoolbox/gluefiles
- **Donate:** https://www.willmanstoolbox.com/donate

## Downloads

> Pending: upload the binaries to make these links actually work

- **Windows (x64):** https://www.willmanstoolbox.com/gluefiles/downloads/gluefiles-win64.zip
- **Linux (x64):** https://www.willmanstoolbox.com/gluefiles/downloads/gluefiles-linux-x64.tar.gz
- **Source:** https://github.com/willmanstoolbox/gluefiles

*Checksums will be published with each release. Verify before running.*

---

## What it is

GlueFiles is a small desktop utility (Qt 6 / Widgets) that concatenates multiple files—in a specific order—into a single output file. Browse folders, drag-and-drop files, reorder them, and optionally include each file’s absolute path as a header before its contents. You can also paste a JSON list of paths.

## Key features

- Two-pane UI: file browser (left) and build list (right).
- Drag-and-drop, multi-select, and reordering.
- Optional “include file path header”.
- Group-by-folder sort; progress bar while building.
- Basic i18n (Qt base translations + Spanish app strings if available).
- App icon via Qt resource; desktop ID for Linux integration.

---

## Build from source

### Linux / macOS

Requires **Qt 6 (Widgets)** and **CMake ≥ 3.16**.

```bash
# Clone
git clone https://github.com/willmanstoolbox/gluefiles
cd gluefiles

# Configure + build
cmake -S . -B build
cmake --build build -j

# Run
./build/gluefiles
