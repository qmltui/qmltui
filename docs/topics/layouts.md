# Layouts and cell geometry

QmlTui measures position and size in integer terminal cells. Use `Row` and
`Column` to arrange children with spacing and padding. Use `RowLayout`,
`ColumnLayout`, or `GridLayout` when siblings need `Layout.fillWidth`,
`Layout.fillHeight`, or stretch constraints.

Anchors place a child relative to its parent or siblings. Do not anchor a child
along an axis already managed by a layout. Inset content by one cell inside a
bordered rectangle. `Text` wraps text to its width when `wrapMode` is enabled,
or keeps explicit line breaks and styled spans.

See [layout examples](../../examples/topics/layouts/), [anchors](../../examples/topics/anchors/),
[RowLayout](../qml-types/rowlayout.md), and [Text](../qml-types/text.md).
