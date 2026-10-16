# Six-Part Tutorial

Welcome to the hands-on QmlTui tutorial! Each chapter builds a complete, runnable terminal application under [`examples/tutorials/`](../../examples/tutorials/).

Start with [Getting started](../getting-started.md) to verify your Qt 6.8+ and CMake setup, then work through the chapters in order. All six chapters build identically whether optional Private Qt features (`QMLTUI_ENABLE_PRIVATE_QT`) are enabled or disabled.

---

## Chapter Overview

| Chapter | QML Types & Concepts Introduced | CMake Target |
| :--- | :--- | :--- |
| **[01: First terminal](01-first-terminal.md)** | `pragma ComponentBehavior: Bound`, `Terminal` root, integer cell `x`/`y` coordinates, explicit `Escape` quit handling, `QCoreApplication` + `QQmlApplicationEngine` bootstrap. | `qmltui_tutorial_01_first_terminal` |
| **[02: Layout and text](02-layout-and-text.md)** | `Rectangle` borders, `anchors.fill` & `anchors.margins`, `Column`/`Row` positioners vs. `RowLayout`/`Layout.fillWidth`, `Text.WordWrap` on resize, rich-text `spans`. | `qmltui_tutorial_02_layout_and_text` |
| **[03: Input and focus](03-input-and-focus.md)** | Initial `focus: true`, Tab focus traversal between `TextInput` and `Button`, reactive property bindings, focus-independent `Shortcut`, `Keys.onPressed` bubbling. | `qmltui_tutorial_03_input_and_focus` |
| **[04: Models and views](04-models-and-views.md)** | `ListModel` & `ListElement`, type-safe `required property` delegates, `ListView.isCurrentItem` selection styling, multi-column `TableView`. | `qmltui_tutorial_04_models_and_views` |
| **[05: Controls and styling](05-controls-and-styling.md)** | `CheckBox`, `Switch`, `Slider` + `ProgressBar` bindings, inline `component AppButton: Button` for app-owned styling without a theme singleton, modal `Dialog` consuming `Escape`. | `qmltui_tutorial_05_controls_and_styling` |
| **[06: Testing with runner](06-testing-with-runner.md)** | Conditional `qmltui::runner` linking, `qmltui_add_scenario`, `CLOCK VIRTUAL`, full-frame `expectSnapshot()`, `.cast` recording with `RECORD`, and the `qmltui-runner` CLI launcher. | `qmltui_tutorial_06_testing_with_runner` |

---

## Building and Running the Tutorials

Configure the repository with examples enabled:

```sh
cmake -S . -B build -DQMLTUI_BUILD_EXAMPLES=ON
```

Build and launch any chapter interactively by its target name:

```sh
cmake --build build --target qmltui_tutorial_01_first_terminal
./build/examples/tutorials/01_first_terminal/qmltui_tutorial_01_first_terminal
```

Ready? Start with **[Chapter 01: First terminal →](01-first-terminal.md)**.
