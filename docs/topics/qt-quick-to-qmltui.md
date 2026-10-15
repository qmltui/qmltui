# QmlTui for Qt Quick Developers

If you already know Qt Quick and QML, you will feel immediately at home in QmlTui.
However, because QmlTui renders retained-mode terminal character grids rather than
GPU/software pixels, a few fundamental assumptions change.

This guide bridges the gap between Qt Quick and QmlTui.

---

## 1. Imports: No `QtQuick`

QmlTui is built on `QtQml`, **not** `QtQuick`. Never import `QtQuick` or
`QtQuick.Controls` in a QmlTui application:

```qml
// ❌ WRONG (Qt Quick)
// import QtQuick
// import QtQuick.Controls

// ✅ CORRECT (QmlTui)
// import QtQml
// import QmlTui
// import QmlTui.Controls
```

QmlTui provides its own `Item`, `Rectangle`, `Text`, `MouseArea`, `Loader`, and
Controls suite tailored specifically for terminal character geometry.

---

## 2. Integer Cell Geometry vs Pixels

In Qt Quick, coordinates and dimensions are real numbers (`qreal`) representing
screen device pixels. In QmlTui:

- **1 unit = 1 character cell**. `width: 40` means 40 character columns;
  `height: 10` means 10 rows.
- **No floating-point geometry**. Fractional coordinates (`x: 10.5`) do not exist.
  All layout math resolves to integer columns and rows.
- **Non-square aspect ratio**. Terminal character cells are typically twice as
  tall as they are wide (e.g., an 80×24 terminal window). Sizing and diagonal lines
  must account for cell proportions.

```qml
Terminal {
    width: 80     // 80 columns wide
    height: 24    // 24 rows tall

    Rectangle {
        x: 2; y: 1
        width: 30
        height: 5
        border.style: Border.Rounded
    }
}
```

---

## 3. Borders and Styling

In Qt Quick, a rectangle's border is specified by pixel width and color. In
terminal cells, a border is drawn using Unicode box-drawing characters:

```qml
Rectangle {
    color: "#0d1117"
    border.style: Border.Single   // or Border.Double, Border.Rounded, Border.Ascii, Border.None
    border.color: "cyan"
}
```

| Border Style | Visual Output in Terminal |
| :--- | :--- |
| `"single"` (`Border.Single`) | `┌───┐` / `│   │` / `└───┘` |
| `"double"` (`Border.Double`) | `╔═══╗` / `║   ║` / `╚═══╝` |
| `"rounded"` (`Border.Rounded`) | `╭───╮` / `│   │` / `╰───╯` |
| `"ascii"` (`Border.Ascii`) | `+---+` / `|   |` / `+---+` |
| `"none"` (`Border.None`) | No border characters drawn |

Borders consume exactly 1 cell on each outer edge. A 10×5 rectangle with a border
has an interior content area of 8 columns by 3 rows.

---

## 4. Colors: Terminal Defaults vs Themes

Qt Quick Controls often relies on palette singletons (like `Material` or `Universal`).
QmlTui intentionally **does not provide a global theme singleton**:

- By default, all controls and items inherit `"default"`, which is the user's
  host terminal background and foreground color.
- You can specify named ANSI colors (`"black"`, `"red"`, `"green"`, `"yellow"`,
  `"blue"`, `"magenta"`, `"cyan"`, `"white"`), ANSI 256 indices, or TrueColor
  24-bit hex strings (`"#2CDE85"`).
- QmlTui automatically degrades TrueColor hex values to 256-color or 16-color
  palettes if the host terminal lacks 24-bit direct color support.
- Custom styling is declared directly via component properties or reusable QML
  wrappers.

---

## 5. Keyboard Navigation and the Quit Path

In a GUI desktop application, the window manager provides a close button (`[X]`).
In a terminal application, the root `Terminal` does not automatically map `Escape`
or `Q` to exit. **Applications must provide an explicit keyboard quit path**:

```qml
Terminal {
    id: root

    // Central application quit handler
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit();
            event.accepted = true;
        }
    }
}
```

If your application has modals or popups, consume `Escape` in the modal first
to close it, and only exit the process when no focused control consumes it.

---

## 6. Models and Delegates

Like Qt Quick, QmlTui uses Qt's model/view architecture (`QAbstractItemModel`,
`QAbstractListModel`, `QStringList`, etc.) with `ListView`, `TableView`, and
`TreeView`.

In QmlTui, modern QML conventions are strictly followed:
1. Always enable `pragma ComponentBehavior: Bound` at the top of your QML file.
2. Declare delegate roles as **required properties** rather than accessing them
   via dynamic context properties.

```qml
ListView {
    anchors.fill: parent
    model: processModel

    delegate: Item {
        required property string processName
        required property int cpuUsage
        required property int index

        width: ListView.view.width
        height: 1

        Text {
            text: processName + " (" + cpuUsage + "%)"
            color: index % 2 === 0 ? "white" : "gray"
        }
    }
}
```

---

## Quick Reference / Translation Cheatsheet

| Qt Quick | QmlTui Equivalent | Notes |
| :--- | :--- | :--- |
| `import QtQuick` | `import QtQml` + `import QmlTui` | No Qt Quick dependency |
| `Item { width: 100.0 }` | `Item { width: 10 }` | Integer cells, no decimals |
| `Rectangle { border.width: 1 }` | `Rectangle { border.style: Border.Single }` | Uses box-drawing glyphs |
| `font.pixelSize: 14` | `N/A (use bold, dim, italic, underline)` | Implicit cell height |
| `color: Material.accent` | `color: "cyan"` or `color: "#2CDE85"` | No theme singletons |
| `focus: true` | `focus: true`, `focusable: true` | Explicit Tab navigation |
| Window Manager `[X]` | `Keys.onPressed: event => ...` | Explicit keyboard quit path |
