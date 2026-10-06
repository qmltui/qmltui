import QtQml
import Qtui.Examples.MarkdownViewer 1.0

MdDocStyle {
    id: root

    h1Format: MdDocStyle.Plain
    h1BadgePadding: 0
    headingFormat: MdDocStyle.Plain
    headingBanners: false
    codeBlockFormat: MdDocStyle.Borderless
    codeBlockIndent: 0
    tableFormat: MdDocStyle.Minimal
    horizontalRuleFormat: MdDocStyle.Dashed
    horizontalRuleText: "---"
    listBulletLevel1: "* "
    listBulletLevel2: "* "
    taskListChecked: "[x] "
    taskListUnchecked: "[ ] "
    blockquotePrefix: "  "
}
