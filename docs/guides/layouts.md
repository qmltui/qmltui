# Layouts and Cell Geometry

All coordinates and dimensions in QmlTui are measured in integer character cells: `x`, `y`, `width`, `height`, `spacing`, `padding`, and `margins` correspond directly to terminal columns and rows.

## Borders and Interior Insets

The grouped `border` property belongs to [`Rectangle`](../qml-types/rectangle.md) and `Rectangle`-derived controls such as [`Panel`](../qml-types/panel.md). Supported [`Border`](../qml-types/border.md) styles are `Border.Single`, `Border.Double`, `Border.Rounded`, `Border.Heavy`, `Border.Ascii`, and `Border.None`.

A visible border occupies 1 cell on each outer edge:

- `Row`, `Column`, and `*Layout` containers placed inside a bordered `Rectangle` receive 1 cell of implicit interior padding on all four sides.
- When anchoring a child directly to a bordered `Rectangle` with `anchors.fill: parent`, set `anchors.margins: 1` so the child does not paint over the border cells.
- [`TableView`](../qml-types/tableview.md) draws connected box-drawing intersections (`┬`, `┼`, `┴`, `├`, `┤`) through its own `showOuterBorder`, `showColumnGrid`, `showRowGrid`, and `gridStyle` (`TableView.Single`, `TableView.Double`, `TableView.Ascii`) properties rather than `Rectangle.border`. Wrap other non-`Rectangle` items in a `Rectangle` or `Panel` when a frame is needed.

## Positioners (`Row`, `Column`) and Layout Engines (`*Layout`)

Use [`Row`](../qml-types/row.md) and [`Column`](../qml-types/column.md) when stacking items of known or implicit size with uniform `spacing` and `padding`. Use [`RowLayout`](../qml-types/rowlayout.md), [`ColumnLayout`](../qml-types/columnlayout.md), [`GridLayout`](../qml-types/gridlayout.md), and [`StackLayout`](../qml-types/stacklayout.md) when children need proportional stretching or attached [`Layout`](../qml-types/layout.md) constraints (`Layout.fillWidth`, `Layout.fillHeight`, `Layout.preferredWidth`, `Layout.minimumWidth`, `Layout.maximumWidth`).

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

## Anchors and Text Wrapping

[`Anchors`](../qml-types/anchors.md) position an item relative to its parent or sibling edge lines (`anchors.fill`, `anchors.centerIn`, `left`, `right`, `top`, `bottom`, `margins`). Do not set anchors on an item that is already positioned by a `Row`, `Column`, or `*Layout`.

On [`Text`](../qml-types/text.md), set `wrapMode: Text.WordWrap` (or `Text.WrapAnywhere`) together with an explicit `width` or `Layout.fillWidth: true` to reflow multi-line strings across rows. For inline multi-style runs on a single `Text` item, bind `spans` to an array of `QmlTui.span(text, { bold, italic, underline, dim, color, backgroundColor })` values.

See [`Layouts.qml`](../../examples/gallery/examples/Layouts.qml), [`Rectangle.qml`](../../examples/gallery/examples/Rectangle.qml), and [`Text.qml`](../../examples/gallery/examples/Text.qml), and the [`RowLayout`](../qml-types/rowlayout.md) and [`Text`](../qml-types/text.md) references.
