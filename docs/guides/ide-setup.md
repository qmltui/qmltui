# IDE Setup and Tooling

A productive QmlTui developer experience requires autocompletion, real-time
linting, hover documentation, and context-sensitive help.

Because QmlTui builds on `QtQml` without `QtQuick`, configuring the QML Language
Server (`qmlls`) and Qt Help (`.qch`) ensures you receive instant completions
and zero false-positive diagnostics.

---

## 1. Visual Studio Code

### Recommended Extensions
- **Qt** (`TheQtCompany.qt`): Official extension providing QML syntax, completion, and debugging.
- **clangd** (`llvm-vs-code-extensions.vscode-clangd`): Fast C++20 autocompletion, hover docs, and diagnostics.
- **CMake Tools** (`ms-vscode.cmake-tools`): Configures and builds CMake targets.

### Workspace Configuration (`.vscode/settings.json`)

To enable `qmlls` autocompletion for QmlTui modules, point `qml.importPaths` to your
build directory's generated QML folder:

```json
{
    // Point QML language server to QmlTui's generated module definitions (.qmltypes & qmldir)
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

Qt Creator provides native CMake and QML integration out of the box:

1. Open **Preferences $\rightarrow$ Qt Quick $\rightarrow$ QML Language Server**.
2. Ensure **Enable QML Language Server** is checked.
3. When building a QmlTui project, Qt Creator automatically feeds CMake's
   `QT_QML_OUTPUT_DIRECTORY` (`build/qml`) to `qmlls`. Types like `Terminal`,
   `Button`, `TableView`, and `Rectangle` will auto-complete with full properties.

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

QmlTui enforces strict QML practices (explicit types, `pragma ComponentBehavior: Bound`,
and no `QtQuick` imports).

You can run `qmllint` from your terminal:

```bash
qmllint -I build/qml Main.qml
```

Any attempt to import `QtQuick` or assign non-existent properties will be flagged
immediately.

---

## 5. Runner Scenario Autocomplete (`qmltui-runner.d.ts`)

JavaScript test and recording scenarios driven by [`Runner`](testing.md) receive a typed `terminal` facade via `qmltui-runner.d.ts`. Enable editor autocomplete in your project's `CMakeLists.txt`:

```cmake
if(COMMAND qmltui_scenario_editor_support)
    qmltui_scenario_editor_support()
endif()
```

Then annotate your scenario entry point with JSDoc so VS Code, Qt Creator, Neovim (`ts_ls`), and Helix provide method completion and hover documentation:

```javascript
/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Ready");
}
```
