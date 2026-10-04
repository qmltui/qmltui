import QtQml 2.15
import Qtui
import Qtui.Examples.MarkdownViewer 1.0

Terminal {
    id: root
    fillTerminal: true
    color: theme.bg

    Theme {
        id: theme
        activeTheme: "tokyo_night"
    }

    property bool isSearching: false
    property var searchMatches: []
    property int currentMatchIndex: 0
    property int currentSectionIndex: -1
    property bool sectionJumpInProgress: false
    property string statusToast: ""
    property int currentSampleIndex: 0
    readonly property var sampleFiles: [
        ":/qt/qml/Qtui/Examples/MarkdownViewer/resources/welcome.md",
        ":/qt/qml/Qtui/Examples/MarkdownViewer/resources/qtui_guide.md",
        ":/qt/qml/Qtui/Examples/MarkdownViewer/resources/cheatsheet.md",
        ":/qt/qml/Qtui/Examples/MarkdownViewer/resources/edge_cases.md"
    ]

    MarkdownDocument {
        id: mdDoc
        source: root.sampleFiles[root.currentSampleIndex]
    }

    Component.onCompleted: {
        if (typeof initialCliFile !== "undefined" && initialCliFile && initialCliFile.length > 0) {
            mdDoc.source = initialCliFile
        }
    }

    Keys.onPressed: (event) => {
        // Esc handling: close search bar if open, else quit
        if (event.key === Qt.Key_Escape) {
            if (root.isSearching) {
                root.isSearching = false
                root.clearSearch()
                event.accepted = true
                return
            }
            Qt.quit()
            event.accepted = true
            return
        }

        // Q key to quit (unless typing in search bar)
        if (event.key === Qt.Key_Q && !root.isSearching) {
            Qt.quit()
            event.accepted = true
            return
        }

        // Tab and Shift+Tab jump between document headings.
        if (event.key === Qt.Key_Tab || event.key === Qt.Key_Backtab) {
            root.jumpSection((event.key === Qt.Key_Backtab
                              || (event.modifiers & Qt.ShiftModifier)) ? -1 : 1)
            event.accepted = true
            return
        }

        // If typing in search bar, let search input handle keys
        if (root.isSearching) {
            if (event.key === Qt.Key_Enter || event.key === Qt.Key_Return) {
                root.nextSearchMatch()
                event.accepted = true
                return
            }
            return
        }

        // Navigation keys
        if (event.key === Qt.Key_J || event.key === Qt.Key_Down) {
            markdownView.scrollBy(1)
            event.accepted = true
            return
        }

        if (event.key === Qt.Key_K || event.key === Qt.Key_Up) {
            markdownView.scrollBy(-1)
            event.accepted = true
            return
        }

        if (event.key === Qt.Key_PageDown || event.key === Qt.Key_Space) {
            markdownView.pageDown()
            event.accepted = true
            return
        }

        if (event.key === Qt.Key_PageUp) {
            markdownView.pageUp()
            event.accepted = true
            return
        }

        if (event.key === Qt.Key_Home || event.key === Qt.Key_G && !(event.modifiers & Qt.ShiftModifier)) {
            markdownView.scrollToLine(0)
            event.accepted = true
            return
        }

        if (event.key === Qt.Key_End || (event.key === Qt.Key_G && (event.modifiers & Qt.ShiftModifier))) {
            markdownView.scrollToLine(mdDoc.lineCount)
            event.accepted = true
            return
        }

        // Search trigger '/'
        if (event.key === Qt.Key_Slash) {
            root.isSearching = true
            event.accepted = true
            return
        }

        // Next / Previous match
        if (event.key === Qt.Key_N) {
            if (event.modifiers & Qt.ShiftModifier) {
                root.previousSearchMatch()
            } else {
                root.nextSearchMatch()
            }
            event.accepted = true
            return
        }

        // Theme toggle 't'
        if (event.key === Qt.Key_T) {
            theme.cycleTheme()
            root.statusToast = "Switched theme to: " + theme.activeTheme
            event.accepted = true
            return
        }

        // Sample switcher 'o'
        if (event.key === Qt.Key_O) {
            root.cycleSample()
            event.accepted = true
            return
        }

    }

    function cycleSample() {
        currentSampleIndex = (currentSampleIndex + 1) % sampleFiles.length
        mdDoc.source = sampleFiles[currentSampleIndex]
        markdownView.scrollToLine(0)
        currentSectionIndex = -1
        clearSearch()
        statusToast = "Loaded sample: " + mdDoc.documentTitle
    }

    function jumpSection(direction) {
        var sections = mdDoc.tableOfContents
        if (sections.length === 0) {
            statusToast = "No sections in this document"
            return
        }

        var targetIndex = -1
        if (currentSectionIndex >= 0 && currentSectionIndex < sections.length) {
            targetIndex = (currentSectionIndex + direction + sections.length) % sections.length
        } else if (direction > 0) {
            for (var i = 0; i < sections.length; ++i) {
                if (sections[i].line > markdownView.scrollY) {
                    targetIndex = i
                    break
                }
            }
            if (targetIndex < 0)
                targetIndex = 0
        } else {
            for (var j = sections.length - 1; j >= 0; --j) {
                if (sections[j].line < markdownView.scrollY) {
                    targetIndex = j
                    break
                }
            }
            if (targetIndex < 0)
                targetIndex = sections.length - 1
        }

        var section = sections[targetIndex]
        sectionJumpInProgress = true
        markdownView.scrollToLine(section.line)
        sectionJumpInProgress = false
        currentSectionIndex = targetIndex
        statusToast = "Section: " + section.title + " (line " + (section.line + 1) + ")"
    }

    function clearSearch() {
        searchMatches = []
        currentMatchIndex = 0
        searchBar.queryText = ""
        markdownView.searchQuery = ""
        markdownView.activeMatchIndex = 0
    }

    function runSearch(query) {
        searchMatches = mdDoc.search(query)
        currentMatchIndex = 0
        markdownView.searchQuery = query
        markdownView.activeMatchIndex = 0
        if (searchMatches.length > 0) {
            var match = searchMatches[0]
            markdownView.scrollToLine(match.line)
        }
    }

    function nextSearchMatch() {
        if (searchMatches.length === 0) return
        currentMatchIndex = (currentMatchIndex + 1) % searchMatches.length
        markdownView.activeMatchIndex = currentMatchIndex
        var match = searchMatches[currentMatchIndex]
        markdownView.scrollToLine(match.line)
    }

    function previousSearchMatch() {
        if (searchMatches.length === 0) return
        currentMatchIndex = (currentMatchIndex - 1 + searchMatches.length) % searchMatches.length
        markdownView.activeMatchIndex = currentMatchIndex
        var match = searchMatches[currentMatchIndex]
        markdownView.scrollToLine(match.line)
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
            currentLine: markdownView.scrollY + 1
            totalLines: Math.max(1, mdDoc.lineCount)
            headingCount: mdDoc.headingCount
            codeBlockCount: mdDoc.codeBlockCount
            tableCount: mdDoc.tableCount
            activeThemeName: theme.activeTheme
        }

        // 2. Markdown document body
        Item {
            id: body
            width: parent.width
            height: Math.max(1, parent.height - headerBar.height - statusBar.height - (root.isSearching ? searchBar.height : 0))

            MarkdownView {
                id: markdownView
                width: body.width
                height: parent.height
                document: mdDoc
                borderStyle: "none"

                onScrollYChanged: {
                    if (!root.sectionJumpInProgress)
                        root.currentSectionIndex = -1
                }

                palette.text: theme.textPrimary
                palette.textMuted: theme.textMuted
                palette.heading: theme.accent
                palette.heading2: theme.warning
                palette.heading3: theme.accentSecondary
                palette.heading4: theme.success
                palette.heading5: theme.accent
                palette.code: theme.warning
                palette.codeBg: theme.surface
                palette.quote: theme.accentSecondary
                palette.border: theme.border
                palette.link: theme.accent
                palette.success: theme.success
                palette.warning: theme.warning
                palette.syntaxKeyword: theme.accentSecondary
                palette.syntaxType: theme.accent
                palette.syntaxString: theme.success
                palette.syntaxComment: theme.textMuted
                palette.syntaxNumber: theme.warning
                palette.syntaxPreprocessor: theme.warning
                palette.syntaxSymbol: theme.accentSecondary
                palette.searchMatchBg: theme.warning
                palette.searchMatchFg: theme.bg
                palette.activeMatchBg: theme.accent
                palette.activeMatchFg: theme.bg

                onLinkActivated: (url) => {
                    root.statusToast = "🔗 Activated Link: " + url
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

            onQueryChanged: (query) => {
                root.runSearch(query)
            }

            onNextMatch: {
                root.nextSearchMatch()
            }

            onPreviousMatch: {
                root.previousSearchMatch()
            }

            onCloseSearch: {
                root.isSearching = false
                root.clearSearch()
            }
        }

        // 4. Status Bar
        StatusBar {
            id: statusBar
            width: parent.width
            theme: theme
            statusMessage: root.statusToast
            activeSearchInfo: (root.isSearching && root.searchMatches.length > 0) ?
                              ("Match " + (root.currentMatchIndex + 1) + "/" + root.searchMatches.length) : ""
        }
    }
}
