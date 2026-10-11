# Plan 04: Qt Documentation Conventions and Qt Creator Integration

## 1. Goal

Reference pages that look and read like Qt's own, and a Qt Creator experience
where F1 on a Qamui symbol opens Qamui help and QML completion knows every Qamui
type. Both are generated from the single markdown source defined in Plan 01.

---

## Implementation status

Section 2 skeletons exist for all 71 exported QML types (2026-09-14), written
by `scripts/component_docs.py generate` from the build's `.qmltypes` and
`qmldir` metadata: frontmatter, the import/inherits table, and every
property, signal, method, enum and attached property as table rows, with
`_TODO_` where prose belongs. `scripts/component_docs.py check` runs as
`test_component_docs`: a type without a page or a page whose member tables
differ from the build fails the suite, and `--strict` will fail on remaining
placeholders once the prose is written. Slugs are the lowercase type name
(`tabbar.md`). Sections 3 to 5 are not started.

## 2. QML Type Page Template (`docs/components/<type>.md`)

```markdown
---
title: Button
kind: qml
import: Qamui.Controls
since: "0.1"
inherits: Item
cpp: qamui::Item
category: inputs
summary: A focusable, clickable action button.
---

# Button QML Type

A focusable, clickable action button. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui.Controls` |
| **Since** | Qamui 0.1 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `text` | `string` | `""` | Label drawn inside the border. |
| `pressed` | `bool` | `false` | Read-only. True while Enter or the mouse button is held. |
| `focusBorderStyle` | `string` | `"double"` | Border style used while the button has active focus. |

## Signals

| Signal | Description |
| :--- | :--- |
| `clicked()` | Emitted on Enter, Space, or a mouse click inside the item. |

## Methods

| Method | Description |
| :--- | :--- |
| `void click()` | Triggers `clicked()` programmatically. |

## Detailed Description

Focus handling, keyboard interaction, sizing rules, and how the border
reflects state.

### Example

```qml
import QtQml
import Qamui
import Qamui.Controls

Terminal {
    Button {
        anchors.centerIn: parent
        text: "[ OK ]"
        focus: true
        onClicked: Qt.quit()
    }
}
```
```

Rules:

- Every example compiles. `scripts/check_doc_examples.py` extracts each
  complete `qml` block (one with an `import` line) into a temporary
  `Main.qml`, builds it against the tree, and runs it with
  `QAMUI_CAPTURE_AFTER_MS` set. CI fails on any block that does not load or
  renders nothing. A `qml` block without an import line is an illustrative
  snippet and is not run.
- Property names follow the Plan 00 naming pass. The template says `text`, not
  `label`.
- Frontmatter drives the catalog grid, the search index, and the `.qhp`
  keyword list. Nothing is typed twice.

---

## 3. C++ Class Page Template (`src/Qamui/<module>/doc/<Class>.md`)

The existing files already follow a close variant of this. Normalize them to:

```markdown
---
title: qamui::Item
kind: cpp
header: <Qamui/Item.h>
target: Qamui::Core
inherits: QObject
category: core
summary: Base class for every terminal item.
---

# qamui::Item Class

- **Header**: `#include <Qamui/Item.h>`
- **CMake**: `target_link_libraries(app PRIVATE Qamui::Core)`
- **Inherits**: `QObject`
- **Inherited By**: `Rectangle`, `Text`, `Loader`, ...

## Properties
## Public Functions
## Signals
## Protected Functions
```

Protected functions get the subclassing contract spelled out, including the
`const` `paint()` and `paintOverlay()` signatures and `CanvasPainter`.

---

## 4. Qt Creator: F1 Help via `.qch`

### What has to be true

Qt Creator's help engine indexes `.qch` archives. A `.qch` is built by
`qhelpgenerator` from a `.qhp` project file that lists **HTML** files, a table
of contents, and keywords. Markdown is not an input. Qt Creator also does
**not** scan an arbitrary install prefix for `.qch` files; it registers the
ones in the Qt kit's documentation directory automatically and everything else
through Preferences.

### Pipeline (`scripts/build_qch.py` + `cmake/QamuiDoc.cmake`)

1. Render every markdown page (components, guides, C++ classes) to
   self-contained HTML with `python-markdown` and a small Qt-like stylesheet.
2. Emit `qamui.qhp`: namespace `org.qamui.qamui.0.5.0`, virtual folder `qamui`, a TOC
   mirroring the site sidebar, and one `<keyword>` per type, property, signal,
   and method. Keyword `id` values follow Qt's scheme, `QML.Button` and
   `QML.Button.text`, so F1 resolves the symbol under the cursor.
3. Run `qhelpgenerator`.

```cmake
# cmake/QamuiDoc.cmake
find_program(QAMUI_QHELPGENERATOR qhelpgenerator
    HINTS "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_LIBEXECS}" "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_BINS}")
find_package(Python3 COMPONENTS Interpreter)

if(QAMUI_QHELPGENERATOR AND Python3_Interpreter_FOUND)
    set(qamui_doc_dir "${CMAKE_BINARY_DIR}/doc")
    add_custom_target(docs_qch
        COMMAND ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/scripts/build_qch.py"
                --source "${CMAKE_SOURCE_DIR}" --out "${qamui_doc_dir}"
        COMMAND ${QAMUI_QHELPGENERATOR} "${qamui_doc_dir}/qamui.qhp" -o "${qamui_doc_dir}/qamui.qch"
        COMMENT "Generating qamui.qch for Qt Creator")
    install(FILES "${qamui_doc_dir}/qamui.qch"
        DESTINATION "${CMAKE_INSTALL_DOCDIR}" COMPONENT Documentation OPTIONAL)
endif()
```

`qhelpgenerator` lives in Qt's `libexec` directory in Qt 6, so `find_program`
with those hints is more reliable than looking for a CMake target.

### Getting it into Qt Creator

Ship `qamui.qch` as a GitHub release asset and document both paths on the
`guide/qt-creator` page:

- **GUI**: Edit, Preferences, Help, Documentation, Add, pick `qamui.qch`.
- **Kit docs folder**: copy `qamui.qch` next to the kit's Qt docs
  (`$(qmake -query QT_INSTALL_DOCS)`), which Qt Creator registers on start.

Neither is automatic after `cmake --install`, so the page must say so.

---

## 5. Qt Creator: QML Completion and `qmllint`

This matters more to a newcomer than F1. Completion and lint depend on the
`qmldir` and `plugins.qmltypes` that `qt_add_qml_module` already generates for
`Qamui`, `Qamui.Controls`, `Qamui.Extras`, `Qamui.Canvas`, `Qamui.Image`, and
`Qamui.TerminalHost`.

- Verify the install step places them under `QAMUI_INSTALL_QMLDIR` and that
  `QamuiConfig.cmake` exports that path.
- Add a `qmlls` section to the `qt-creator` guide: enable the QML language
  server in Preferences, set the build directory, and completion works for
  every Qamui type. Verify with a screenshot in the guide.
- Run `qmllint` over `examples/` in CI with the Qamui import path so the
  shipped examples are warning-free, which is what a newcomer copies.

---

## 6. Acceptance

- Every `docs/components/*.md` example block builds and loads in CI.
- `cmake --build build --target docs_qch` produces `qamui.qch`; opening it in
  Qt Creator and pressing F1 on `Button` in a QML file opens the Button page.
- `qmlls` completes `Qamui.Controls` types in a fresh Qt Creator project that
  consumes Qamui via `FetchContent`.
- `qmllint` passes on `examples/`.
