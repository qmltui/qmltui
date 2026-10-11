import QtQml
import Qamui
import Qamui.Examples.MarkdownViewer 1.0

Terminal {
    id: root
    color: theme.bg
    mouseReporting: false

    Theme {
        id: theme
        activeTheme: "aurora"
    }

    property bool isSearching: false
    property var searchMatches: []
    property int currentMatchIndex: 0
    property int currentSectionIndex: -1
    property bool sectionJumpInProgress: false
    property string statusToast: ""
    property int currentSampleIndex: 0
    property bool showLineNumbers: false
    property string currentStyleName: "modern"
    property bool editorOpen: false

    readonly property var sampleFiles: [":/qt/qml/Qamui/Examples/MarkdownViewer/resources/modern_showcase.md", ":/qt/qml/Qamui/Examples/MarkdownViewer/resources/welcome.md", ":/qt/qml/Qamui/Examples/MarkdownViewer/resources/qamui_guide.md", ":/qt/qml/Qamui/Examples/MarkdownViewer/resources/cheatsheet.md", ":/qt/qml/Qamui/Examples/MarkdownViewer/resources/edge_cases.md"]

    ModernDocStyle {
        id: modernDocStyle
    }
    ClassicDocStyle {
        id: classicDocStyle
    }
    MinimalDocStyle {
        id: minimalDocStyle
    }

    // Live editable custom style instance
    MdDocStyle {
        id: liveCustomDocStyle
        h1Format: MdDocStyle.Badge
        h1BadgePadding: 3
        headingFormat: MdDocStyle.Bullets
        headingBanners: true
        codeBlockFormat: MdDocStyle.Indented
        codeBlockIndent: 4
        tableFormat: MdDocStyle.Open
        horizontalRuleFormat: MdDocStyle.Dashed
        horizontalRuleText: "───────── ❖ ─────────"
        listBulletLevel1: "✦ "
        listBulletLevel2: "▸ "
        taskListChecked: "[✔] "
        taskListUnchecked: "[ ] "
        blockquotePrefix: "▍ "
    }

    readonly property var activeDocStyle: (root.currentStyleName === "modern") ? modernDocStyle : (root.currentStyleName === "classic") ? classicDocStyle : (root.currentStyleName === "minimal") ? minimalDocStyle : liveCustomDocStyle

    readonly property bool isEditorActive: (root.currentStyleName === "custom" || root.editorOpen)

    MdViewStyle {
        id: mdViewStyle
        showLineNumbers: root.showLineNumbers

        palette.text: theme.textPrimary
        palette.textMuted: theme.textMuted
        palette.heading: theme.h1Fg
        palette.heading1Bg: theme.h1Bg
        palette.heading2: theme.accent
        palette.heading3: theme.accent
        palette.heading4: theme.accent
        palette.heading5: theme.accent
        palette.code: theme.warning
        palette.codeBg: theme.surface
        palette.inlineCode: theme.inlineCodeFg
        palette.inlineCodeBg: theme.inlineCodeBg
        palette.quote: theme.accent
        palette.border: theme.border
        palette.link: theme.accent
        palette.success: theme.success
        palette.warning: theme.warning
        palette.syntaxKeyword: theme.syntaxKeyword
        palette.syntaxType: theme.syntaxType
        palette.syntaxFunction: theme.syntaxFunction
        palette.syntaxString: theme.syntaxString
        palette.syntaxComment: theme.syntaxComment
        palette.syntaxNumber: theme.syntaxNumber
        palette.syntaxPreprocessor: theme.syntaxPreprocessor
        palette.syntaxSymbol: theme.syntaxSymbol
        palette.searchMatchBg: theme.warning
        palette.searchMatchFg: theme.bg
        palette.activeMatchBg: theme.accent
        palette.activeMatchFg: theme.bg
    }

    MdDoc {
        id: mdDoc
        source: root.sampleFiles[root.currentSampleIndex]
        style: root.activeDocStyle
    }

    Component.onCompleted: {
        if (typeof initialCliFile !== "undefined" && initialCliFile && initialCliFile.length > 0) {
            mdDoc.source = initialCliFile;
        }
    }

    Keys.onPressed: event => {
        // Esc handling: close search bar or editor, else quit
        if (event.key === Qt.Key_Escape) {
            if (root.isSearching) {
                root.isSearching = false;
                root.clearSearch();
                event.accepted = true;
                return;
            }
            if (root.editorOpen) {
                root.editorOpen = false;
                event.accepted = true;
                return;
            }
            Qt.quit();
            event.accepted = true;
            return;
        }

        // Q key to quit (unless typing in search bar)
        if (event.key === Qt.Key_Q && !root.isSearching) {
            Qt.quit();
            event.accepted = true;
            return;
        }

        // Tab and Shift+Tab jump between document headings.
        if (event.key === Qt.Key_Tab || event.key === Qt.Key_Backtab) {
            root.jumpSection((event.key === Qt.Key_Backtab || (event.modifiers & Qt.ShiftModifier)) ? -1 : 1);
            event.accepted = true;
            return;
        }

        // If typing in search bar, let search input handle keys
        if (root.isSearching) {
            if (event.key === Qt.Key_Enter || event.key === Qt.Key_Return) {
                root.nextSearchMatch();
                event.accepted = true;
                return;
            }
            return;
        }

        // When live custom style editor is active, handle arrow navigation and adjustments
        if (root.isEditorActive) {
            if (event.key === Qt.Key_Up) {
                styleEditor.selectPrev();
                root.statusToast = "Editing style: row " + (styleEditor.selectedRow + 1);
                event.accepted = true;
                return;
            }
            if (event.key === Qt.Key_Down) {
                styleEditor.selectNext();
                root.statusToast = "Editing style: row " + (styleEditor.selectedRow + 1);
                event.accepted = true;
                return;
            }
            if (event.key === Qt.Key_Left) {
                styleEditor.adjustSelected(-1);
                root.statusToast = "Adjusted property live!";
                event.accepted = true;
                return;
            }
            if (event.key === Qt.Key_Right || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                styleEditor.adjustSelected(1);
                root.statusToast = "Adjusted property live!";
                event.accepted = true;
                return;
            }
        }

        // Navigation keys
        if (event.key === Qt.Key_J || (!root.isEditorActive && event.key === Qt.Key_Down)) {
            mdView.scrollBy(1);
            event.accepted = true;
            return;
        }

        if (event.key === Qt.Key_K || (!root.isEditorActive && event.key === Qt.Key_Up)) {
            mdView.scrollBy(-1);
            event.accepted = true;
            return;
        }

        if (event.key === Qt.Key_PageDown || event.key === Qt.Key_Space) {
            mdView.pageDown();
            event.accepted = true;
            return;
        }

        if (event.key === Qt.Key_PageUp) {
            mdView.pageUp();
            event.accepted = true;
            return;
        }

        if (event.key === Qt.Key_Home || (event.key === Qt.Key_G && !(event.modifiers & Qt.ShiftModifier))) {
            mdView.scrollToLine(0);
            event.accepted = true;
            return;
        }

        if (event.key === Qt.Key_End || (event.key === Qt.Key_G && (event.modifiers & Qt.ShiftModifier))) {
            mdView.scrollToLine(mdDoc.lineCount);
            event.accepted = true;
            return;
        }

        // Live Editor toggle 'e'
        if (event.key === Qt.Key_E) {
            if (root.currentStyleName !== "custom")
                root.currentStyleName = "custom";
            root.editorOpen = !root.editorOpen;
            root.statusToast = "Live Style Editor: " + (root.isEditorActive ? "ACTIVE" : "CLOSED");
            event.accepted = true;
            return;
        }

        // Search trigger '/'
        if (event.key === Qt.Key_Slash) {
            root.isSearching = true;
            event.accepted = true;
            return;
        }

        // Next / Previous match
        if (event.key === Qt.Key_N) {
            if (event.modifiers & Qt.ShiftModifier) {
                root.previousSearchMatch();
            } else {
                root.nextSearchMatch();
            }
            event.accepted = true;
            return;
        }

        // Theme toggle 't'
        if (event.key === Qt.Key_T) {
            theme.cycleTheme();
            root.statusToast = "Theme: " + theme.activeTheme;
            event.accepted = true;
            return;
        }

        // Style toggle 's' (cycles modern -> classic -> minimal -> custom -> modern)
        if (event.key === Qt.Key_S) {
            if (root.currentStyleName === "modern")
                root.currentStyleName = "classic";
            else if (root.currentStyleName === "classic")
                root.currentStyleName = "minimal";
            else if (root.currentStyleName === "minimal")
                root.currentStyleName = "custom";
            else
                root.currentStyleName = "modern";
            root.statusToast = "Style: " + root.currentStyleName;
            event.accepted = true;
            return;
        }

        // Line numbers toggle 'l'
        if (event.key === Qt.Key_L) {
            root.showLineNumbers = !root.showLineNumbers;
            root.statusToast = "Line numbers: " + (root.showLineNumbers ? "ON" : "OFF");
            event.accepted = true;
            return;
        }

        // Sample switcher 'o'
        if (event.key === Qt.Key_O) {
            root.cycleSample();
            event.accepted = true;
            return;
        }
    }

    function cycleSample() {
        currentSampleIndex = (currentSampleIndex + 1) % sampleFiles.length;
        mdDoc.source = sampleFiles[currentSampleIndex];
        mdView.scrollToLine(0);
        currentSectionIndex = -1;
        clearSearch();
        statusToast = "Loaded sample: " + mdDoc.documentTitle;
    }

    function jumpSection(direction) {
        var sections = mdDoc.tableOfContents;
        if (sections.length === 0) {
            statusToast = "No sections in this document";
            return;
        }

        var targetIndex = -1;
        if (currentSectionIndex >= 0 && currentSectionIndex < sections.length) {
            targetIndex = (currentSectionIndex + direction + sections.length) % sections.length;
        } else if (direction > 0) {
            for (var i = 0; i < sections.length; ++i) {
                if (sections[i].line > mdView.scrollY) {
                    targetIndex = i;
                    break;
                }
            }
            if (targetIndex < 0)
                targetIndex = 0;
        } else {
            for (var j = sections.length - 1; j >= 0; --j) {
                if (sections[j].line < mdView.scrollY) {
                    targetIndex = j;
                    break;
                }
            }
            if (targetIndex < 0)
                targetIndex = sections.length - 1;
        }

        var section = sections[targetIndex];
        sectionJumpInProgress = true;
        mdView.scrollToLine(section.line);
        sectionJumpInProgress = false;
        currentSectionIndex = targetIndex;
        statusToast = "Section: " + section.title + " (line " + (section.line + 1) + ")";
    }

    function clearSearch() {
        searchMatches = [];
        currentMatchIndex = 0;
        searchBar.queryText = "";
        mdView.searchQuery = "";
        mdView.activeMatchIndex = 0;
    }

    function runSearch(query) {
        searchMatches = mdDoc.search(query);
        currentMatchIndex = 0;
        mdView.searchQuery = query;
        mdView.activeMatchIndex = 0;
        if (searchMatches.length > 0) {
            var match = searchMatches[0];
            mdView.scrollToLine(match.line);
        }
    }

    function nextSearchMatch() {
        if (searchMatches.length === 0)
            return;
        currentMatchIndex = (currentMatchIndex + 1) % searchMatches.length;
        mdView.activeMatchIndex = currentMatchIndex;
        var match = searchMatches[currentMatchIndex];
        mdView.scrollToLine(match.line);
    }

    function previousSearchMatch() {
        if (searchMatches.length === 0)
            return;
        currentMatchIndex = (currentMatchIndex - 1 + searchMatches.length) % searchMatches.length;
        mdView.activeMatchIndex = currentMatchIndex;
        var match = searchMatches[currentMatchIndex];
        mdView.scrollToLine(match.line);
    }

    Column {
        anchors.fill: parent
        spacing: 0

        // 1. Header Bar
        HeaderBar {
            id: headerBar
            width: parent.width
            theme: theme
            title: mdDoc.documentTitle
            currentLine: mdView.scrollY + 1
            totalLines: Math.max(1, mdDoc.lineCount)
            headingCount: mdDoc.headingCount
            codeBlockCount: mdDoc.codeBlockCount
            tableCount: mdDoc.tableCount
            activeThemeName: theme.activeTheme
            activeStyleName: root.currentStyleName + (root.isEditorActive ? " [EDIT]" : "")
        }

        // 2. Main content area (Split layout when Editor is active)
        Item {
            id: body
            width: parent.width
            height: Math.max(1, parent.height - headerBar.height - statusBar.height - (root.isSearching ? searchBar.height : 0))

            Row {
                anchors.fill: parent
                spacing: 0

                // Live Style Editor Panel
                StyleEditor {
                    id: styleEditor
                    visible: root.isEditorActive
                    width: root.isEditorActive ? Math.min(36, Math.floor(body.width * 0.45)) : 0
                    height: parent.height
                    theme: theme
                    docStyle: liveCustomDocStyle
                }

                // Markdown document Viewport
                MdView {
                    id: mdView
                    width: body.width - (styleEditor.visible ? styleEditor.width : 0)
                    height: parent.height
                    document: mdDoc
                    style: mdViewStyle

                    onScrollYChanged: {
                        if (!root.sectionJumpInProgress)
                            root.currentSectionIndex = -1;
                    }

                    onLinkActivated: url => {
                        root.statusToast = "🔗 Activated Link: " + url;
                    }
                }
            }
        }

        // 3. Search Bar (when searching)
        SearchBar {
            id: searchBar
            visible: root.isSearching
            width: parent.width
            theme: theme
            isSearching: root.isSearching
            totalMatches: root.searchMatches.length
            currentMatch: root.currentMatchIndex

            onQueryChanged: query => {
                root.runSearch(query);
            }

            onNextMatch: {
                root.nextSearchMatch();
            }

            onPreviousMatch: {
                root.previousSearchMatch();
            }

            onCloseSearch: {
                root.isSearching = false;
                root.clearSearch();
            }
        }

        // 4. Status Bar
        StatusBar {
            id: statusBar
            width: parent.width
            theme: theme
            statusMessage: root.statusToast
            activeSearchInfo: (root.isSearching && root.searchMatches.length > 0) ? ("Match " + (root.currentMatchIndex + 1) + "/" + root.searchMatches.length) : ""
        }
    }
}
