import QtQml
import Qtui
import Qtui.Examples.MarkdownViewer 1.0

Rectangle {
    id: editor
    color: theme.surface
    border.style: "single"
    border.color: theme.accent

    required property var theme
    required property MdDocStyle docStyle
    property int selectedRow: 0

    readonly property var hrPresets: [
        "───────── ❖ ─────────",
        "────────────────────",
        "--------------------",
        "════════════════════",
        "· · · · · · · · · · ",
        "* * * * * * * * * * ",
        "- - - - - - - - - - ",
        "~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ",
        "- - - - - ◈ - - - - -"
    ]
    property int hrPresetIndex: 0

    readonly property var bullet1Presets: [
        "✦ ", "• ", "▸ ", "◆ ", "■ ", "- ", "❯ ", "→ ", "* "
    ]
    property int bullet1Index: 0

    readonly property var bullet2Presets: [
        "▸ ", "▪ ", "▫ ", "* ", "› ", "– ", "- "
    ]
    property int bullet2Index: 0

    readonly property var taskCheckedPresets: [
        "[✔] ", "[✓] ", "[x] ", "[X] ", "[+] ", "☑ ", "[ok] "
    ]
    property int taskCheckedIndex: 0

    readonly property var taskUncheckedPresets: [
        "[ ] ", "( ) ", "☐ ", "[·] ", "[?] "
    ]
    property int taskUncheckedIndex: 0

    readonly property var quotePresets: [
        "▍ ", "│ ", "┃ ", "▌ ", "▎ ", "> ", "» ", "  "
    ]
    property int quoteIndex: 0

    readonly property int totalRows: 14

    function selectNext() {
        selectedRow = (selectedRow + 1) % totalRows
    }

    function selectPrev() {
        selectedRow = (selectedRow - 1 + totalRows) % totalRows
    }

    function applyBasePreset(presetName) {
        if (presetName === "Modern") {
            docStyle.h1Format = MdDocStyle.Badge
            docStyle.h1BadgePadding = 1
            docStyle.headingFormat = MdDocStyle.Bullets
            docStyle.headingBanners = false
            docStyle.codeBlockFormat = MdDocStyle.Indented
            docStyle.codeBlockIndent = 2
            docStyle.tableFormat = MdDocStyle.Open
            docStyle.horizontalRuleText = "--------"
            docStyle.listBulletLevel1 = "• "
            docStyle.listBulletLevel2 = "▪ "
            docStyle.taskListChecked = "[✓] "
            docStyle.taskListUnchecked = "[ ] "
            docStyle.blockquotePrefix = "│ "
        } else if (presetName === "Classic") {
            docStyle.h1Format = MdDocStyle.Banner
            docStyle.h1BadgePadding = 0
            docStyle.headingFormat = MdDocStyle.Hashes
            docStyle.headingBanners = true
            docStyle.codeBlockFormat = MdDocStyle.Boxed
            docStyle.codeBlockIndent = 0
            docStyle.tableFormat = MdDocStyle.Boxed
            docStyle.horizontalRuleText = "════════════════════"
            docStyle.listBulletLevel1 = "- "
            docStyle.listBulletLevel2 = "  * "
            docStyle.taskListChecked = "[x] "
            docStyle.taskListUnchecked = "[ ] "
            docStyle.blockquotePrefix = "> "
        } else if (presetName === "Minimal") {
            docStyle.h1Format = MdDocStyle.Plain
            docStyle.h1BadgePadding = 0
            docStyle.headingFormat = MdDocStyle.Plain
            docStyle.headingBanners = false
            docStyle.codeBlockFormat = MdDocStyle.Borderless
            docStyle.codeBlockIndent = 0
            docStyle.tableFormat = MdDocStyle.Minimal
            docStyle.horizontalRuleText = "---"
            docStyle.listBulletLevel1 = "* "
            docStyle.listBulletLevel2 = "* "
            docStyle.taskListChecked = "[x] "
            docStyle.taskListUnchecked = "[ ] "
            docStyle.blockquotePrefix = "  "
        } else { // Fancy / Cyber
            docStyle.h1Format = MdDocStyle.Badge
            docStyle.h1BadgePadding = 3
            docStyle.headingFormat = MdDocStyle.Bullets
            docStyle.headingBanners = true
            docStyle.codeBlockFormat = MdDocStyle.Indented
            docStyle.codeBlockIndent = 4
            docStyle.tableFormat = MdDocStyle.Open
            docStyle.horizontalRuleText = "───────── ❖ ─────────"
            docStyle.listBulletLevel1 = "✦ "
            docStyle.listBulletLevel2 = "▸ "
            docStyle.taskListChecked = "[✔] "
            docStyle.taskListUnchecked = "[ ] "
            docStyle.blockquotePrefix = "▍ "
        }
    }

    function adjustSelected(delta) {
        switch (selectedRow) {
        case 0: // Quick base preset
            if (delta > 0)
                applyBasePreset("Fancy")
            else
                applyBasePreset("Modern")
            break

        case 1: // H1 Format
            if (docStyle.h1Format === MdDocStyle.Badge)
                docStyle.h1Format = delta > 0 ? MdDocStyle.Banner : MdDocStyle.Plain
            else if (docStyle.h1Format === MdDocStyle.Banner)
                docStyle.h1Format = delta > 0 ? MdDocStyle.Plain : MdDocStyle.Badge
            else
                docStyle.h1Format = delta > 0 ? MdDocStyle.Badge : MdDocStyle.Banner
            break

        case 2: // H1 Padding
            docStyle.h1BadgePadding = Math.max(0, Math.min(8, docStyle.h1BadgePadding + delta))
            break

        case 3: // Heading Format
            if (docStyle.headingFormat === MdDocStyle.Bullets)
                docStyle.headingFormat = delta > 0 ? MdDocStyle.Hashes : MdDocStyle.Plain
            else if (docStyle.headingFormat === MdDocStyle.Hashes)
                docStyle.headingFormat = delta > 0 ? MdDocStyle.Plain : MdDocStyle.Bullets
            else
                docStyle.headingFormat = delta > 0 ? MdDocStyle.Bullets : MdDocStyle.Hashes
            break

        case 4: // Heading Banners
            docStyle.headingBanners = !docStyle.headingBanners
            break

        case 5: // Code Block Format
            if (docStyle.codeBlockFormat === MdDocStyle.Indented)
                docStyle.codeBlockFormat = delta > 0 ? MdDocStyle.Boxed : MdDocStyle.Borderless
            else if (docStyle.codeBlockFormat === MdDocStyle.Boxed)
                docStyle.codeBlockFormat = delta > 0 ? MdDocStyle.Borderless : MdDocStyle.Indented
            else
                docStyle.codeBlockFormat = delta > 0 ? MdDocStyle.Indented : MdDocStyle.Boxed
            break

        case 6: // Code Block Indent
            docStyle.codeBlockIndent = Math.max(0, Math.min(8, docStyle.codeBlockIndent + delta))
            break

        case 7: // Table Format
            if (docStyle.tableFormat === MdDocStyle.Open)
                docStyle.tableFormat = delta > 0 ? MdDocStyle.Boxed : MdDocStyle.Minimal
            else if (docStyle.tableFormat === MdDocStyle.Boxed)
                docStyle.tableFormat = delta > 0 ? MdDocStyle.Minimal : MdDocStyle.Open
            else
                docStyle.tableFormat = delta > 0 ? MdDocStyle.Open : MdDocStyle.Boxed
            break

        case 8: // HR text preset
            hrPresetIndex = (hrPresetIndex + delta + hrPresets.length) % hrPresets.length
            docStyle.horizontalRuleText = hrPresets[hrPresetIndex]
            break

        case 9: // Bullet L1
            bullet1Index = (bullet1Index + delta + bullet1Presets.length) % bullet1Presets.length
            docStyle.listBulletLevel1 = bullet1Presets[bullet1Index]
            break

        case 10: // Bullet L2
            bullet2Index = (bullet2Index + delta + bullet2Presets.length) % bullet2Presets.length
            docStyle.listBulletLevel2 = bullet2Presets[bullet2Index]
            break

        case 11: // Task Checked
            taskCheckedIndex = (taskCheckedIndex + delta + taskCheckedPresets.length) % taskCheckedPresets.length
            docStyle.taskListChecked = taskCheckedPresets[taskCheckedIndex]
            break

        case 12: // Task Unchecked
            taskUncheckedIndex = (taskUncheckedIndex + delta + taskUncheckedPresets.length) % taskUncheckedPresets.length
            docStyle.taskListUnchecked = taskUncheckedPresets[taskUncheckedIndex]
            break

        case 13: // Quote prefix
            quoteIndex = (quoteIndex + delta + quotePresets.length) % quotePresets.length
            docStyle.blockquotePrefix = quotePresets[quoteIndex]
            break
        }
    }

    Column {
        id: mainColumn
        anchors.fill: parent
        anchors.margins: 1
        spacing: 0

        // Header Title
        Rectangle {
            width: mainColumn.width
            height: 2
            color: theme.bg

            Text {
                anchors.fill: parent
                text: " ⚙ LIVE STYLE EDITOR "
                color: theme.accent
                bold: true
                verticalAlignment: Qt.AlignVCenter
            }
        }

        Text {
            width: mainColumn.width
            text: " [↑/↓] Select  [←/→/Enter] Tweak"
            color: theme.textMuted
        }

        Rectangle {
            width: mainColumn.width
            height: 1
            color: theme.border
        }

        // Options List
        Column {
            id: optionsColumn
            width: mainColumn.width
            spacing: 0

            Repeater {
                model: [
                    { label: "1. Quick Base", value: "Modern / Cyber" },
                    { label: "2. H1 Format", value: (docStyle.h1Format === MdDocStyle.Badge ? "Badge" : (docStyle.h1Format === MdDocStyle.Banner ? "Banner" : "Plain")) },
                    { label: "3. H1 Padding", value: docStyle.h1BadgePadding + " sp" },
                    { label: "4. Headings", value: (docStyle.headingFormat === MdDocStyle.Bullets ? "Bullets" : (docStyle.headingFormat === MdDocStyle.Hashes ? "Hashes" : "Plain")) },
                    { label: "5. H-Banners", value: docStyle.headingBanners ? "ON" : "OFF" },
                    { label: "6. Code Format", value: (docStyle.codeBlockFormat === MdDocStyle.Indented ? "Indented" : (docStyle.codeBlockFormat === MdDocStyle.Boxed ? "Boxed" : "Borderless")) },
                    { label: "7. Code Indent", value: docStyle.codeBlockIndent + " sp" },
                    { label: "8. Table Format", value: (docStyle.tableFormat === MdDocStyle.Open ? "Open" : (docStyle.tableFormat === MdDocStyle.Boxed ? "Boxed" : "Minimal")) },
                    { label: "9. Divider HR", value: docStyle.horizontalRuleText.substring(0, Math.min(8, docStyle.horizontalRuleText.length)) + "…" },
                    { label: "10. Bullet L1", value: "'" + docStyle.listBulletLevel1 + "'" },
                    { label: "11. Bullet L2", value: "'" + docStyle.listBulletLevel2 + "'" },
                    { label: "12. Task [x]", value: "'" + docStyle.taskListChecked.trim() + "'" },
                    { label: "13. Task [ ]", value: "'" + docStyle.taskListUnchecked.trim() + "'" },
                    { label: "14. Quote Pre", value: "'" + docStyle.blockquotePrefix + "'" }
                ]

                Rectangle {
                    width: optionsColumn.width
                    height: 1
                    color: (index === editor.selectedRow) ? theme.accentSecondary : "transparent"

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 1
                        anchors.rightMargin: 1

                        Text {
                            width: 15
                            text: (index === editor.selectedRow ? "▶ " : "  ") + modelData.label
                            color: (index === editor.selectedRow) ? theme.bg : theme.textPrimary
                            bold: (index === editor.selectedRow)
                        }

                        Text {
                            text: " ‹ " + modelData.value + " ›"
                            color: (index === editor.selectedRow) ? theme.bg : theme.warning
                            bold: (index === editor.selectedRow)
                        }
                    }
                }
            }
        }
    }
}
