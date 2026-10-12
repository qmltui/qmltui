# IDE Setup and Tooling

A productive Qamui developer experience requires autocompletion, real-time
linting, hover documentation, and context-sensitive help.

Because Qamui builds on `QtQml` without `QtQuick`, configuring the QML Language
Server (`qmlls`) and Qt Help (`.qch`) ensures you receive instant completions
and zero false-positive diagnostics.

---

## 1. Visual Studio Code

### Recommended Extensions
- **Qt** (`TheQtCompany.qt`): Official extension providing QML syntax, completion, and debugging.
- **clangd** (`llvm-vs-code-extensions.vscode-clangd`): Fast C++20 autocompletion, hover docs, and diagnostics.
- **CMake Tools** (`ms-vscode.cmake-tools`): Configures and builds CMake targets.

### Workspace Configuration (`.vscode/settings.json`)

To enable `qmlls` autocompletion for Qamui modules, point `qml.importPaths` to your
build directory's generated QML folder:

```json
{
    // Point QML language server to Qamui's generated module definitions (.qmltypes & qmldir)
    "qml.importPaths": [
        "${workspaceFolder}/build/qml",
        "${workspaceFolder}/../../build/qml"
    ],

    // Enable real-time QML linting to catch syntax and binding errors
    "qml.qmllint.enabled": true,

    // C++20 autocompletion via compile_commands.json
    "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
    "clangd.arguments": [
        "--compile-commands-dir=${workspaceFolder}/build",
        "--header-insertion=never"
    ]
}
```

Pre-configured `.vscode/settings.json` files are included in `examples/` and
`examples/quickstart/`.

---

## 2. Qt Creator

Qt Creator provides native CMake and QML integration out of the box.

### A. QML Language Server (`qmlls`)
1. Open **Preferences $\rightarrow$ Qt Quick $\rightarrow$ QML Language Server**.
2. Ensure **Enable QML Language Server** is checked.
3. When building a Qamui project, Qt Creator automatically feeds CMake's
   `QT_QML_OUTPUT_DIRECTORY` (`build/qml`) to `qmlls`. Types like `Terminal`,
   `Button`, `TableView`, and `Rectangle` will auto-complete with full properties.

### B. Offline F1 Help (`qamui.qch`)
You can view Qamui documentation directly inside Qt Creator's Help pane and press
**F1** on any Qamui symbol to jump to its reference page.

1. Build the help archive (requires `qhelpgenerator`):
   ```bash
   cmake --build build --target docs_qch
   ```
   Or download `qamui.qch` from the latest GitHub release.
2. In Qt Creator, go to **Preferences $\rightarrow$ Help $\rightarrow$ Documentation**.
3. Click **Add...** and select `build/doc/qamui.qch` (or your downloaded copy).
4. Place the cursor over `Button`, `Terminal`, or `qamui::Item` in your editor and
   press **F1**.

---

## 3. Neovim & Helix (LSP Setup)

If you use Neovim with `nvim-lspconfig`, configure `qmlls` by pointing it to
your build directory's QML import path:

```lua
local lspconfig = require('lspconfig')

lspconfig.qmlls.setup{
  cmd = { "qmlls", "-I", vim.fn.getcwd() .. "/build/qml" },
  filetypes = { "qml", "qmljs" },
  root_dir = lspconfig.util.root_pattern("CMakeLists.txt", ".git"),
}
```

For C++20 support, `clangd` will automatically discover `build/compile_commands.json`
when generated with `cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`.

---

## 4. Linting with `qmllint`

Qamui enforces strict QML practices (explicit types, `pragma ComponentBehavior: Bound`,
and no `QtQuick` imports).

You can run `qmllint` from your terminal:

```bash
qmllint -I build/qml Main.qml
```

Any attempt to import `QtQuick` or assign non-existent properties will be flagged
immediately.
