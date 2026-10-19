# Layouts and Cell Geometry

All coordinates and dimensions in QmlTui are measured in **integer terminal character cells**: `x`, `y`, `width`, `height`, `spacing`, `padding`, and `margins` correspond directly to terminal columns and rows. There are no fractional pixels or floating-point sub-cell coordinates.

---

## 1. Borders and Interior Insets

The grouped `border` property belongs to [`Rectangle`](../qml-types/rectangle.md) and `Rectangle`-derived controls such as [`Panel`](../qml-types/panel.md). Available box-drawing styles on [`Border`](../qml-types/border.md) are `Border.Single`, `Border.Double`, `Border.Rounded`, `Border.Heavy`, `Border.Ascii`, and `Border.None`.

- **Layout & Positioner Children**: A bordered `Rectangle` automatically contributes 1 cell of implicit interior padding on all four edges for `Row`, `Column`, and `*Layout` children.
- **Anchored Children**: When anchoring a child directly to a bordered `Rectangle` with `anchors.fill: parent`, set `anchors.margins: 1` so the child does not overwrite the 1-cell border glyphs.
- **Connected Table Grids**: [`TableView`](../qml-types/tableview.md) draws its own connected box-drawing intersections via `showOuterBorder`, `showColumnGrid`, `showRowGrid`, and `gridStyle` (`TableView.Single`, `TableView.Double`, `TableView.Ascii`) rather than using `Rectangle.border`.

---

## 2. Positioners (`Row`, `Column`) vs. Layout Engines (`*Layout`)

- Use **[`Row`](../qml-types/row.md)** and **[`Column`](../qml-types/column.md)** when stacking items of known or implicit size with uniform `spacing` and `padding`.
- Use **[`RowLayout`](../qml-types/rowlayout.md)**, **[`ColumnLayout`](../qml-types/columnlayout.md)**, **[`GridLayout`](../qml-types/gridlayout.md)**, and **[`StackLayout`](../qml-types/stacklayout.md)** when children need proportional stretching (`Layout.fillWidth`, `Layout.fillHeight`, `Layout.preferredWidth`, `Layout.minimumWidth`).

```qml
pragma ComponentBehavior: Bound
import QtQml
import QmlTui

Terminal {
    id: root

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit();
            event.accepted = true;
        }
    }

    RowLayout {
        anchors {
            fill: parent
            margins: 1
        }
        spacing: 1

        Rectangle {
            Layout.preferredWidth: 24
            Layout.fillHeight: true
            border {
                style: Border.Rounded
                color: "cyan"
            }

            Column {
                spacing: 1
                Text {
                    text: "Sidebar (24 cols)"
                    bold: true
                }
                Text {
                    width: 20
                    wrapMode: Text.WordWrap
                    text: "Word-wrapped description inside the fixed-width sidebar."
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            border {
                style: Border.Single
                color: "gray"
            }

            Text {
                anchors.centerIn: parent
                spans: [
                    QmlTui.span("Status: ", { dim: true }),
                    QmlTui.span("Ready", { bold: true, color: "cyan" })
                ]
            }
        }
    }
}
```

---

## 3. Anchors and Responsive `Text`

- **Anchors (`anchors.*`)**: Position an item relative to its parent or sibling edge lines (`anchors.fill`, `anchors.centerIn`, `left`, `right`, `top`, `bottom`, `margins`). Never set anchors along an axis that is already managed by a `Row`, `Column`, or `*Layout`.
- **Wrapping & Rich Spans (`Text`)**: Set `wrapMode: Text.WordWrap` (or `Text.WrapAnywhere`) together with an explicit `width` (or `Layout.fillWidth: true`) to reflow multi-line text automatically, or bind `spans` to an array of `QmlTui.span(text, { bold, italic, underline, dim, color, backgroundColor })` runs for inline styling.

See the gallery examples [`Layouts.qml`](../../examples/gallery/examples/Layouts.qml), [`Rectangle.qml`](../../examples/gallery/examples/Rectangle.qml), and [`Text.qml`](../../examples/gallery/examples/Text.qml), as well as the [`RowLayout`](../qml-types/rowlayout.md) and [`Text`](../qml-types/text.md) references.
