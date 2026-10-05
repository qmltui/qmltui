pragma ComponentBehavior: Bound

import QtQml
import Qtui
import Qtui.TerminalHost
import Qtui.Examples.FileManager

Terminal {
    id: root

    title: "Yazi Terminal File Manager"
    fillTerminal: true
    useAlternateScreen: true
    color: Theme.bgMain
    focus: true

    FileManagerBackend {
        id: backend
    }

    property bool isFilterMode: false
    property bool isEditorOpen: false
    property string editorFilePath: ""

    readonly property string modeName: {
        if (root.isFilterMode) return "FILTER";
        if (backend.markedCount > 0) return "SELECT (" + backend.markedCount + ")";
        return "NORMAL";
    }

    readonly property string modeColor: {
        if (root.isFilterMode) return Theme.badgeFilter;
        if (backend.markedCount > 0) return Theme.badgeSelect;
        return Theme.badgeNormal;
    }

    // MANDATORY QTUI RULE: Explicit key handling for Q/Escape and contextual actions
    Keys.onPressed: (event) => {
        // If embedded editor is open, let TerminalView handle all keys
        if (root.isEditorOpen) {
            return;
        }

        // If a popup modal is open, let the popup handle keys
        if (helpDialog.visible || inputDialog.visible || confirmDialog.visible) {
            return;
        }

        // Filter Mode Key Handling
        if (root.isFilterMode) {
            if (event.key === Qt.Key_Escape) {
                root.isFilterMode = false;
                backend.filterText = "";
                filterTextInput.text = "";
                root.focus = true;
                event.accepted = true;
                return;
            }
            if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                root.isFilterMode = false;
                root.focus = true;
                event.accepted = true;
                return;
            }
            return;
        }

        // 1. Exit / Quit Handling
        if (event.key === Qt.Key_Q) {
            Qt.quit();
            event.accepted = true;
            return;
        }

        if (event.key === Qt.Key_Escape) {
            if (backend.filterText !== "") {
                backend.filterText = "";
                filterTextInput.text = "";
                event.accepted = true;
                return;
            }
            if (backend.markedCount > 0) {
                backend.clearMarks();
                event.accepted = true;
                return;
            }
            Qt.quit();
            event.accepted = true;
            return;
        }

        // 2. Navigation Keys
        if (event.key === Qt.Key_J || event.key === Qt.Key_Down) {
            if (backend.currentEntries.length > 0) {
                backend.currentIndex = Math.min(backend.currentEntries.length - 1, backend.currentIndex + 1);
            }
            event.accepted = true;
            return;
        }

        if (event.key === Qt.Key_K || event.key === Qt.Key_Up) {
            if (backend.currentEntries.length > 0) {
                backend.currentIndex = Math.max(0, backend.currentIndex - 1);
            }
            event.accepted = true;
            return;
        }

        if (event.key === Qt.Key_H || event.key === Qt.Key_Left) {
            backend.navigateUp();
            event.accepted = true;
            return;
        }

        if (event.key === Qt.Key_L || event.key === Qt.Key_Right || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            if (backend.currentEntries.length > 0) {
                if (backend.isCurrentItemDir) {
                    backend.navigateIntoCurrent();
                } else if (!backend.isMock) {
                    const targetFile = backend.currentItemPath;
                    if (targetFile !== "") {
                        root.editorFilePath = targetFile;
                        root.isEditorOpen = true;
                        terminalEditor.start();
                        terminalEditor.focus = true;
                    }
                } else {
                    backend.navigateIntoCurrent();
                }
            }
            event.accepted = true;
            return;
        }

        if (event.key === Qt.Key_G && (event.modifiers & Qt.ShiftModifier)) {
            // Capital G -> Bottom
            if (backend.currentEntries.length > 0) {
                backend.currentIndex = backend.currentEntries.length - 1;
            }
            event.accepted = true;
            return;
        }

        if (event.key === Qt.Key_G || event.key === Qt.Key_Home) {
            backend.currentIndex = 0;
            event.accepted = true;
            return;
        }

        if (event.key === Qt.Key_End) {
            if (backend.currentEntries.length > 0) {
                backend.currentIndex = backend.currentEntries.length - 1;
            }
            event.accepted = true;
            return;
        }

        if (event.key === Qt.Key_PageDown) {
            if (backend.currentEntries.length > 0) {
                backend.currentIndex = Math.min(backend.currentEntries.length - 1, backend.currentIndex + 8);
            }
            event.accepted = true;
            return;
        }

        if (event.key === Qt.Key_PageUp) {
            if (backend.currentEntries.length > 0) {
                backend.currentIndex = Math.max(0, backend.currentIndex - 8);
            }
            event.accepted = true;
            return;
        }

        // 3. Selection & Marking
        if (event.key === Qt.Key_Space || event.key === Qt.Key_V) {
            if (event.modifiers & Qt.ShiftModifier) {
                backend.selectAll();
            } else {
                backend.toggleMarkCurrent();
            }
            event.accepted = true;
            return;
        }

        // 4. File Operations
        if (event.key === Qt.Key_Y) {
            backend.copyMarked();
            event.accepted = true;
            return;
        }

        if (event.key === Qt.Key_X) {
            backend.cutMarked();
            event.accepted = true;
            return;
        }

        if (event.key === Qt.Key_P) {
            backend.pasteClipboard();
            event.accepted = true;
            return;
        }

        if (event.key === Qt.Key_A || event.key === Qt.Key_N) {
            inputDialog.dialogTitle = "Create New Item";
            inputDialog.promptText = "Enter name for new file or folder:";
            inputDialog.inputValue = "";
            inputDialog.isDirectory = false;
            inputDialog.allowToggleDir = true;
            inputDialog.open();
            event.accepted = true;
            return;
        }

        if (event.key === Qt.Key_R || event.key === Qt.Key_F2) {
            if (backend.previewData && backend.previewData.name) {
                inputDialog.dialogTitle = "Rename Item";
                inputDialog.promptText = "Enter new name:";
                inputDialog.inputValue = backend.previewData.name;
                inputDialog.isDirectory = backend.previewData.isDir || false;
                inputDialog.allowToggleDir = false;
                inputDialog.open();
            }
            event.accepted = true;
            return;
        }

        if (event.key === Qt.Key_D || event.key === Qt.Key_Delete) {
            confirmDialog.dialogTitle = "Delete Item(s)";
            if (backend.markedCount > 0) {
                confirmDialog.message = "Delete " + backend.markedCount + " marked item(s)?";
                confirmDialog.targetItem = backend.markedCount + " items selected";
            } else if (backend.previewData && backend.previewData.name) {
                confirmDialog.message = "Delete item permanently?";
                confirmDialog.targetItem = backend.previewData.name;
            }
            confirmDialog.open();
            event.accepted = true;
            return;
        }

        // 5. Search / Filter
        if (event.key === Qt.Key_Slash || event.key === Qt.Key_F) {
            root.isFilterMode = true;
            filterTextInput.focus = true;
            event.accepted = true;
            return;
        }

        // 6. Dotfiles & Sorting
        if (event.key === Qt.Key_Period) {
            backend.toggleShowHidden();
            event.accepted = true;
            return;
        }

        if (event.key === Qt.Key_S) {
            backend.cycleSort();
            event.accepted = true;
            return;
        }

        // 7. Help Modal
        if (event.key === Qt.Key_Question || event.key === Qt.Key_F1) {
            helpDialog.open();
            event.accepted = true;
            return;
        }
    }

    // Main App Layout
    Column {
        anchors.fill: parent
        spacing: 0

        // 1. Top Header Bar
        HeaderBar {
            width: parent.width
            userHost: backend.userHost
            breadcrumbs: backend.breadcrumbs
            diskUsage: backend.diskUsage
            modeName: root.modeName
            modeColor: root.modeColor
        }

        // Inline Filter Bar (appears when / or f is pressed)
        Rectangle {
            visible: root.isFilterMode
            width: parent.width
            height: 1
            color: Theme.bgCursor
            border.style: "none"

            Row {
                x: 1
                anchors.verticalCenter: parent.verticalCenter
                spacing: 1

                Text {
                    text: "🔍 Filter: "
                    color: Theme.accentPink
                    bold: true
                }

                TextInput {
                    id: filterTextInput
                    width: parent.width - 20
                    color: Theme.textHighlight
                    cursorVisible: true
                    cursorColor: Theme.accentPink
                    onTextChanged: backend.filterText = filterTextInput.text
                    onAccepted: {
                        root.isFilterMode = false;
                        root.focus = true;
                    }
                }
            }
        }

        // 2. Center 3-Column Miller Layout
        Row {
            width: parent.width
            height: Math.max(6, parent.height - (root.isFilterMode ? 4 : 3))
            spacing: 0

            // Column 1: Parent Directory (22% width)
            ParentColumn {
                id: parentCol
                width: Math.max(12, Math.floor(parent.width * 0.22))
                height: parent.height
                parentEntries: backend.parentEntries
                currentPath: backend.currentPath
                parentPath: backend.parentPath
            }

            // Column 2: Current Directory / Explorer (38% width)
            CurrentColumn {
                id: currentCol
                width: Math.max(20, Math.floor(parent.width * 0.38))
                height: parent.height
                currentEntries: backend.currentEntries
                currentIndex: backend.currentIndex
                filterText: backend.filterText
                isFilterActive: root.isFilterMode
            }

            // Column 3: Preview Pane (remaining width)
            PreviewColumn {
                id: previewCol
                width: Math.max(20, parent.width - (parentCol.width + currentCol.width))
                height: parent.height
                previewData: backend.previewData
                backend: backend
            }
        }

        // 3. Bottom Status Bar
        StatusBar {
            width: parent.width
            previewData: backend.previewData
            currentIndex: backend.currentIndex
            totalCount: backend.currentEntries ? backend.currentEntries.length : 0
            markedCount: backend.markedCount
            sortField: backend.sortField
            sortAscending: backend.sortAscending
            statusMessage: backend.statusMessage
        }
    }

    // Modal Dialogs
    HelpDialog {
        id: helpDialog
    }

    InputDialog {
        id: inputDialog
        onDialogAccepted: (name, isDir) => {
            if (dialogTitle.indexOf("Rename") !== -1) {
                backend.renameCurrent(name);
            } else {
                backend.createItem(name, isDir);
            }
            root.focus = true;
        }
        onDialogRejected: {
            root.focus = true;
        }
    }

    ConfirmDialog {
        id: confirmDialog
        onDialogConfirmed: {
            backend.deleteCurrentOrMarked();
            root.focus = true;
        }
        onDialogRejected: {
            root.focus = true;
        }
    }

    // Fullscreen Embedded Terminal Editor View (vim / nvim / nano / emacs)
    Rectangle {
        id: editorOverlay
        visible: root.isEditorOpen
        anchors.fill: parent
        color: Theme.bgMain
        focus: root.isEditorOpen

        Column {
            anchors.fill: parent
            spacing: 0

            // Editor Header Bar
            Rectangle {
                width: parent.width
                height: 1
                color: Theme.bgHeader
                border.style: "none"

                Row {
                    anchors.left: parent.left
                    anchors.leftMargin: 1
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 1

                    Text {
                        text: "📝 " + backend.editorProgram.toUpperCase()
                        color: Theme.accentPeach
                        bold: true
                    }

                    Text {
                        text: "· " + root.editorFilePath
                        color: Theme.textPrimary
                    }
                }

                Text {
                    anchors.right: parent.right
                    anchors.rightMargin: 1
                    anchors.verticalCenter: parent.verticalCenter
                    text: "[Exit " + backend.editorProgram + " to return to file manager]"
                    color: Theme.textDim
                }
            }

            // Embedded Unix PTY Terminal View
            TerminalView {
                id: terminalEditor
                width: parent.width
                height: Math.max(1, parent.height - 1)
                program: backend.editorProgram
                arguments: root.editorFilePath ? [root.editorFilePath] : []
                workingDirectory: backend.currentPath
                autoStart: false
                focus: root.isEditorOpen

                onFinished: (exitCode) => {
                    root.isEditorOpen = false;
                    root.focus = true;
                    backend.refreshCurrentAndPreview();
                    backend.showToast("Closed " + backend.editorProgram + " (exit code " + exitCode + ")");
                }

                onErrorOccurred: (err) => {
                    root.isEditorOpen = false;
                    root.focus = true;
                    backend.showToast("Editor error: " + err);
                }
            }
        }
    }
}
