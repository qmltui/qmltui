import QtQml
import Qtui.Examples.MarkdownViewer 1.0

MdDocStyle {
    id: root

    h1Format: MdDocStyle.Badge
    h1BadgePadding: 1
    headingFormat: MdDocStyle.Bullets
    headingBanners: false
    codeBlockFormat: MdDocStyle.Indented
    codeBlockIndent: 2
    tableFormat: MdDocStyle.Open
    horizontalRuleFormat: MdDocStyle.Dashed
    horizontalRuleText: "--------"
    listBulletLevel1: "• "
    listBulletLevel2: "▪ "
    taskListChecked: "[✓] "
    taskListUnchecked: "[ ] "
    blockquotePrefix: "│ "
}
