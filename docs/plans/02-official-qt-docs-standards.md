# Implementation Plan 02: Official Qt Documentation Standards & Qt Creator QCH Help

## 1. Goal Description

Establish strict adherence to **official Qt documentation conventions** across all Qtui reference materials, while generating **Qt Compressed Help (`.qch`)** files so developers get native **F1 Contextual Help inside Qt Creator**.

---

## 2. Standard Qt QML Type Documentation Template

Every component in [`examples/components/`](file:///home/yosri/qtui/examples/components) will be documented using the official Qt QML Type layout:

```markdown
# <TypeName> QML Type

<Brief one-line summary of what the type does>. More...

| Item | Details |
| :--- | :--- |
| **Import Statement** | `import Qtui` (or `import Qtui.Controls` / `import Qtui.Extras`) |
| **Since** | Qtui 0.1 |
| **Inherits** | [`Qtui.Item`](./item.md) |
| **C++ Class** | [`qtui::<ClassName>`](file:///path/to/header.h) (in `Qtui::<Module>`) |

## Properties

| Property | Type | Default | Access | Description |
| :--- | :--- | :--- | :--- | :--- |
| `propertyName` | `type` | `default` | Read-write / Read-only | Clear description of property behavior |

## Attached Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `<TypeName>.attachedProperty` | `type` | `default` | Contextual attached value |

## Signals

| Signal | Parameters | Description |
| :--- | :--- | :--- |
| `signalName(type param)` | `type param` | Emitted under specific condition |

## Methods

| Method | Returns | Parameters | Description |
| :--- | :--- | :--- | :--- |
| `methodName(type arg)` | `returnType` | `type arg` | Description of operation |

## Detailed Description

Comprehensive explanation of component lifecycle, keyboard focus handling, clipping, and integration with models.

### Example

```qml
import QtQml
import Qtui
import Qtui.Controls

<TypeName> {
    // Runnable declarative snippet
}
```
```

---

## 3. Standard C++ Class Documentation Template

For C++ extension APIs (such as `qtui::Item`, `qtui::Color`, `qtui::TestRunner`, and custom items):

```markdown
# qtui::<ClassName> Class

The `<ClassName>` class provides ... More...

- **Header**: `#include <Qtui/<ClassName>.h>`
- **CMake Target**: `Qtui::Core`
- **Inherits**: `QObject` (or `qtui::Item`)
- **Inherited By**: ...

## Properties
- `<type> <propertyName>`: Detailed description, `READ`, `WRITE`, `NOTIFY` signal.

## Public Functions
- `<returnType> <functionName>(<args>)`: Description, thread safety, ownership transfer.

## Signals
- `void <signalName>(<params>)`: Condition under which signal is emitted.

## Protected Functions (Item Subclassing)
- `void paint(TextGridBuffer &buffer, int x, int y) override`: Description of rendering contract.
```

---

## 4. Qt Creator F1 Help Integration (`.qch`)

Qt Creator indexes installed documentation archives (`.qch`) and provides instant context help when a developer highlights any QML or C++ symbol and presses **F1**.

### CMake Integration (`cmake/QtuiDoc.cmake`)
```cmake
find_package(Qt6 COMPONENTS Tools QUIET)

if(TARGET Qt6::qhelpgenerator OR TARGET Qt6::qdoc)
    add_custom_target(docs_qch
        COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::qhelpgenerator
            "${CMAKE_CURRENT_SOURCE_DIR}/docs/qtui.qhp"
            -o "${CMAKE_CURRENT_BINARY_DIR}/qtui.qch"
        COMMENT "Generating Qt Compressed Help (qtui.qch) for Qt Creator"
    )

    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/qtui.qch"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/doc/qt6"
        COMPONENT Documentation
    )
endif()
```

### Installation Verification in Qt Creator
When `cmake --install build` is executed, Qt Creator automatically discovers `qtui.qch` under `share/doc/qt6/` and enables F1 contextual help for:
- `Terminal`, `Item`, `Text`, `TableView`, `TreeView`, `ListView`, `Button`, `Gauge`, etc.
