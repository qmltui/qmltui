pragma ComponentBehavior: Bound
import QtQml
import QtQml.Models
import Qt.labs.qmlmodels
import Qamui

Terminal {
    id: root

    enum MessageType {
        BannerMessage,
        UserMessage,
        ThoughtMessage,
        ToolCallMessage,
        ToolResultMessage,
        DiffMessage,
        PermissionDecisionMessage,
        AgentMessage,
        SystemMessage,
        ErrorMessage,
        SwarmTableMessage,
        RepoTreeMessage
    }

    enum AgentState {
        ReadyState,
        WorkingState,
        PermissionState
    }

    mode: Terminal.Inline
    title: {
        if (root.awaitingPermission) {
            return "⚠️ [ACTION REQUIRED] " + root.currentToolName + "(" + root.currentToolTarget + ") — Qt AI Studio";
        }
        if (modelModal && modelModal.visible) {
            return "⚙️ Select Neural Model — Qt AI Studio (" + root.activeModel + ")";
        }
        if (diffInspector && diffInspector.visible) {
            return "📝 Diff Inspector: " + diffInspector.targetFile + " — Qt AI Studio";
        }
        if (root.working) {
            return root.spinnerFrames[root.spinnerIndex] + " " + root.workingLabel + " — Qt AI Studio (" + root.activeModel + " · " + root.sessionCostStr + ")";
        }
        return "● Ready — Qt AI Studio (" + root.activeModel + " · " + root.sessionCostStr + ")";
    }
    mouseReporting: false

    property int agentState: Main.ReadyState
    property int currentScenario: 1
    property int agentStep: 0
    property int permissionIndex: 0
    property int spinnerIndex: 0
    property int elapsedDeciseconds: 0
    property bool sessionAlwaysAllow: false
    property string activeModel: "claude-3-7-sonnet"
    property string sessionCostStr: "$0.014"
    property int totalTokens: 1420
    property string currentToolName: ""
    property string currentToolTarget: ""
    property string currentToolAction: ""
    property string currentDiffFile: ""
    property string requestText: ""

    // History buffer for navigation
    property var inputHistory: []
    property int historyIndex: -1

    readonly property bool ready: agentState === Main.ReadyState
    readonly property bool working: agentState === Main.WorkingState
    readonly property bool awaitingPermission: agentState === Main.PermissionState
    readonly property var spinnerFrames: ["⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"]

    readonly property string elapsedTimeStr: (elapsedDeciseconds / 10).toFixed(1) + "s"

    readonly property string workingLabel: {
        if (currentToolName.length > 0)
            return currentToolName + " (" + elapsedTimeStr + ")...";
        return "Reasoning (" + elapsedTimeStr + ")...";
    }

    function addMessage(type, content, title, extra, detail, tag, approved) {
        chatHistory.append({
            "type": type,
            "content": content !== undefined ? content : "",
            "title": title !== undefined ? title : "",
            "extra": extra !== undefined ? extra : "",
            "detail": detail !== undefined ? detail : "",
            "tag": tag !== undefined ? tag : "",
            "approved": approved !== undefined ? approved : true
        });
    }

    function handleSlashCommand(cmd) {
        const trimmed = cmd.trim();
        const parts = trimmed.split(" ");
        const mainCmd = parts[0].toLowerCase();

        if (mainCmd === "/help") {
            addMessage(Main.SystemMessage, "Commands & Workflows:\n" + "  /demo [1-4]   Execute agent workflows (1: Refactor, 2: Swarm, 3: Patch, 4: Tree)\n" + "  /swarm        Show subagent swarm status (TableView)\n" + "  /tree         Inspect repository workspace hierarchy (TreeView)\n" + "  /cost         Show session token metrics and capacity (ProgressBar & Sparkline)\n" + "  /model        Toggle active neural model (Claude 3.7 / GPT-4.5 / Gemini 2.0)\n" + "  /doctor       Run system environment diagnostics\n" + "  /clear        Clear terminal history\n" + "  /exit, /quit  Exit studio session\n\n" + "Keys: Enter (submit), 1-4 (select), Ctrl+C (abort), Ctrl+Q (quit), Esc (close or quit)", "Help & Workflows");
            return;
        }

        if (mainCmd === "/swarm") {
            addMessage(Main.SwarmTableMessage, "", "Active Mesh Status");
            return;
        }

        if (mainCmd === "/tree") {
            addMessage(Main.RepoTreeMessage, "", "~/Qamui Workspace");
            return;
        }

        if (mainCmd === "/cost" || mainCmd === "/usage") {
            addMessage(Main.SystemMessage, "Session Metrics:\n" + "  • Input Tokens:   " + (totalTokens * 4) + "\n" + "  • Output Tokens:  " + totalTokens + "\n" + "  • Total Cost:     " + sessionCostStr + "\n" + "  • Active Model:   " + activeModel, "Usage & Cost");
            return;
        }

        if (mainCmd === "/diff" || mainCmd === "/patch") {
            diffInspector.open();
            return;
        }

        if (mainCmd === "/model") {
            modelModal.open();
            return;
        }

        if (mainCmd === "/doctor") {
            addMessage(Main.SystemMessage, "System Status:\n" + "  ✓ Repository:   ~/Qamui (branch: main*)\n" + "  ✓ Runtime:      C++20 & Qt 6.11.1\n" + "  ✓ Stream:       Zero-Copy Atomic Buffer\n" + "  ✓ Components:   TableView, TreeView, ProgressBar, Sparkline, StatusBadge\n" + "  ✓ Status:       All systems nominal", "Diagnostics");
            return;
        }

        if (mainCmd === "/clear") {
            chatHistory.clear();
            showBanner();
            return;
        }

        if (mainCmd === "/exit" || mainCmd === "/quit") {
            Qt.quit();
            return;
        }

        if (mainCmd.startsWith("/demo")) {
            let demoId = 1;
            const match = trimmed.match(/\/demo\s*(\d+)/i);
            if (match && match[1]) {
                const parsed = parseInt(match[1]);
                if (!isNaN(parsed) && parsed >= 1 && parsed <= 4)
                    demoId = parsed;
            }
            startScenario(demoId);
            return;
        }

        addMessage(Main.ErrorMessage, "Unknown command: " + cmd + ". Type /help for available workflows.", "Error");
    }

    function startScenario(scenarioId, customPrompt) {
        agentTimer.stop();
        workingTimer.stop();

        currentScenario = scenarioId;
        agentStep = 0;
        elapsedDeciseconds = 0;
        currentToolName = "Planning";
        agentState = Main.WorkingState;

        let promptText = customPrompt;
        if (!promptText || promptText.length === 0) {
            if (scenarioId === 1)
                promptText = "Optimize StreamRenderer buffer resizing and verify zero-copy safety";
            else if (scenarioId === 2)
                promptText = "Dispatch parallel subagent swarm to audit memory leaks and concurrency locks";
            else if (scenarioId === 3)
                promptText = "Run security sandbox exploit audit and generate regression patch";
            else
                promptText = "Explore repository workspace hierarchy and file structure";
        }

        requestText = promptText;
        addMessage(Main.UserMessage, promptText);

        totalTokens += 380;
        sessionCostStr = "$" + ((totalTokens * 0.000008) + 0.012).toFixed(3);

        workingTimer.restart();
        agentTimer.restart();
    }

    function submitRequest(text) {
        const query = text ? text.trim() : promptBar.inputText.trim();
        if (query.length === 0)
            return;

        // Allow slash commands in any state
        if (query.startsWith("/")) {
            inputHistory.push(query);
            historyIndex = inputHistory.length;
            promptBar.inputText = "";
            handleSlashCommand(query);
            return;
        }

        if (!ready)
            return;
        inputHistory.push(query);
        historyIndex = inputHistory.length;
        promptBar.inputText = "";

        const lower = query.toLowerCase();
        if (lower.indexOf("swarm") !== -1 || lower.indexOf("audit") !== -1 || lower.indexOf("subagent") !== -1) {
            startScenario(2, query);
        } else if (lower.indexOf("sec") !== -1 || lower.indexOf("vuln") !== -1 || lower.indexOf("patch") !== -1) {
            startScenario(3, query);
        } else if (lower.indexOf("tree") !== -1 || lower.indexOf("files") !== -1 || lower.indexOf("arch") !== -1) {
            startScenario(4, query);
        } else {
            startScenario(1, query);
        }
    }

    function choosePermission(index) {
        permissionIndex = index;
        if (index === 2) {
            addMessage(Main.PermissionDecisionMessage, "Execution denied: " + currentToolName + "(" + currentToolTarget + ")", "Denied", "", "", "", false);
            addMessage(Main.ErrorMessage, "Agent task halted by user.", "Aborted");
            agentState = Main.ReadyState;
            agentTimer.stop();
            workingTimer.stop();
            return;
        }

        if (index === 3) {
            addMessage(Main.ErrorMessage, "Cancelled request.", "Cancelled");
            agentState = Main.ReadyState;
            agentTimer.stop();
            workingTimer.stop();
            return;
        }

        if (index === 1) {
            sessionAlwaysAllow = true;
            addMessage(Main.PermissionDecisionMessage, "Always allow: " + currentToolName + "(" + currentToolTarget + ")", "Allowed", "", "", "", true);
        } else {
            addMessage(Main.PermissionDecisionMessage, "Approved: " + currentToolName + "(" + currentToolTarget + ")", "Approved", "", "", "", true);
        }

        agentStep = 6;
        agentState = Main.WorkingState;
        currentToolName = "Bash";
        agentTimer.restart();
    }

    function interruptAgent() {
        agentTimer.stop();
        workingTimer.stop();
        addMessage(Main.ErrorMessage, "Execution interrupted by user (Ctrl+C).", "Interrupted");
        agentState = Main.ReadyState;
    }

    function showBanner() {
        addMessage(Main.BannerMessage, "", "Qt", activeModel);
    }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_F2) {
            modelModal.open();
            event.accepted = true;
            return;
        }
        if (event.key === Qt.Key_F3) {
            diffInspector.open();
            event.accepted = true;
            return;
        }

        // Handle Ctrl+Q to quit
        if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_Q) {
            Qt.quit();
            event.accepted = true;
            return;
        }

        // Handle Escape to quit ONLY if prompt is empty and no popup/modal is open
        if (root.ready && event.key === Qt.Key_Escape && promptBar.inputText.length === 0 && !promptBar.popupVisible && !modelModal.visible && !diffInspector.visible) {
            Qt.quit();
            event.accepted = true;
            return;
        }

        // Ctrl+C handler
        if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_C) {
            if (root.working) {
                root.interruptAgent();
            } else if (root.awaitingPermission) {
                root.choosePermission(3);
            } else {
                if (promptBar.inputText.length > 0)
                    promptBar.inputText = "";
                else
                    Qt.quit();
            }
            event.accepted = true;
            return;
        }

        // Permission state controls
        if (root.awaitingPermission) {
            if (event.key === Qt.Key_1) {
                root.choosePermission(0);
                event.accepted = true;
            } else if (event.key === Qt.Key_2) {
                root.choosePermission(1);
                event.accepted = true;
            } else if (event.key === Qt.Key_3) {
                root.choosePermission(2);
                event.accepted = true;
            } else if (event.key === Qt.Key_4) {
                root.choosePermission(3);
                event.accepted = true;
            } else if (event.key === Qt.Key_Y) {
                root.choosePermission(0);
                event.accepted = true;
            } else if (event.key === Qt.Key_N) {
                root.choosePermission(2);
                event.accepted = true;
            } else if (event.key === Qt.Key_Up) {
                root.permissionIndex = Math.max(0, root.permissionIndex - 1);
                event.accepted = true;
            } else if (event.key === Qt.Key_Down) {
                root.permissionIndex = Math.min(3, root.permissionIndex + 1);
                event.accepted = true;
            } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                root.choosePermission(root.permissionIndex);
                event.accepted = true;
            } else if (event.key === Qt.Key_Escape) {
                root.choosePermission(3);
                event.accepted = true;
            }
            return;
        }

        // History Navigation
        if (root.ready) {
            if (event.key === Qt.Key_Up) {
                if (root.inputHistory.length > 0) {
                    if (root.historyIndex > 0) {
                        root.historyIndex--;
                        promptBar.inputText = root.inputHistory[root.historyIndex];
                        promptBar.inputCursorPosition = promptBar.inputText.length;
                    } else if (root.historyIndex === -1) {
                        root.historyIndex = root.inputHistory.length - 1;
                        promptBar.inputText = root.inputHistory[root.historyIndex];
                        promptBar.inputCursorPosition = promptBar.inputText.length;
                    }
                }
                event.accepted = true;
            } else if (event.key === Qt.Key_Down) {
                if (root.historyIndex >= 0 && root.historyIndex < root.inputHistory.length - 1) {
                    root.historyIndex++;
                    promptBar.inputText = root.inputHistory[root.historyIndex];
                    promptBar.inputCursorPosition = promptBar.inputText.length;
                } else if (root.historyIndex >= root.inputHistory.length - 1) {
                    root.historyIndex = root.inputHistory.length;
                    promptBar.inputText = "";
                }
                event.accepted = true;
            }
        }
    }

    ListModel {
        id: chatHistory
    }

    Scrollback {
        model: chatHistory
        spacing: 1

        delegate: DelegateChooser {
            role: "type"

            DelegateChoice {
                roleValue: Main.BannerMessage
                BannerCard {
                    activeModel: root.activeModel
                }
            }

            DelegateChoice {
                roleValue: Main.UserMessage
                UserPromptCard {
                    required property var model
                    content: model.content
                }
            }

            DelegateChoice {
                roleValue: Main.ThoughtMessage
                ThoughtCard {
                    required property var model
                    title: model.title
                    content: model.content
                }
            }

            DelegateChoice {
                roleValue: Main.ToolCallMessage
                ToolCallCard {
                    required property var model
                    title: model.title
                    content: model.content
                }
            }

            DelegateChoice {
                roleValue: Main.ToolResultMessage
                ToolResultCard {
                    required property var model
                    content: model.content
                }
            }

            DelegateChoice {
                roleValue: Main.DiffMessage
                DiffCard {
                    required property var model
                    title: model.title
                }
            }

            DelegateChoice {
                roleValue: Main.PermissionDecisionMessage
                PermissionCard {
                    required property var model
                    content: model.content
                    approved: model.approved
                }
            }

            DelegateChoice {
                roleValue: Main.AgentMessage
                AgentResponseCard {
                    required property var model
                    title: model.title
                    content: model.content
                    extra: model.extra
                }
            }

            DelegateChoice {
                roleValue: Main.SystemMessage
                SystemMessageCard {
                    required property var model
                    title: model.title
                    content: model.content
                }
            }

            DelegateChoice {
                roleValue: Main.ErrorMessage
                ErrorMessageCard {
                    required property var model
                    content: model.content
                }
            }

            DelegateChoice {
                roleValue: Main.SwarmTableMessage
                SwarmTableCard {
                    required property var model
                    title: model.title
                }
            }

            DelegateChoice {
                roleValue: Main.RepoTreeMessage
                RepoTreeCard {
                    required property var model
                    title: model.title
                }
            }
        }
    }

    // Pinned inline prompt
    PinnedPromptBar {
        id: promptBar
        ready: root.ready
        working: root.working
        awaitingPermission: root.awaitingPermission
        modalActive: (modelModal && modelModal.visible) || (diffInspector && diffInspector.visible)
        activeModel: root.activeModel
        sessionCostStr: root.sessionCostStr
        currentToolName: root.currentToolName
        currentToolTarget: root.currentToolTarget
        currentToolAction: root.currentToolAction
        permissionIndex: root.permissionIndex
        spinnerFrames: root.spinnerFrames
        spinnerIndex: root.spinnerIndex
        workingLabel: root.workingLabel

        onRequestSubmitted: text => root.submitRequest(text)
        onPermissionChosen: index => root.choosePermission(index)
        onOpenModelPickerRequested: () => modelModal.open()
        onOpenDiffInspectorRequested: () => diffInspector.open()
    }

    // Interactive Neural Model Picker Dialog (F2 / /model)
    ModelPickerModal {
        id: modelModal
        x: 0
        y: 0
        width: root.width
        currentModel: root.activeModel
        onModelSelected: modelId => {
            root.activeModel = modelId;
            toast.notify("Model: " + modelId, "info");
            root.addMessage(Main.SystemMessage, "Switched neural model to: " + modelId, "Model");
        }
    }

    // Side-by-Side Patch Diff Inspector Dialog (F3 / /diff)
    DiffInspectorModal {
        id: diffInspector
        x: 0
        y: 0
        width: root.width
    }

    // Floating Background Task Notification Toast
    ToastNotification {
        id: toast
        x: Math.max(0, root.width - width - 2)
        y: 1
    }

    // Unified Animation Timer
    Timer {
        id: workingTimer
        interval: 100
        running: root.working
        repeat: true
        onTriggered: {
            root.elapsedDeciseconds += 1;
            root.spinnerIndex = (root.spinnerIndex + 1) % root.spinnerFrames.length;
        }
    }

    // Multi-Step Agent Scenario Simulation Timer
    Timer {
        id: agentTimer
        interval: 600
        repeat: true

        onTriggered: {
            // Scenario 1: Refactor
            if (root.currentScenario === 1) {
                if (root.agentStep === 0) {
                    root.currentToolName = "Planner";
                    root.addMessage(Main.ThoughtMessage, "Analyzing StreamRenderer buffer logic to eliminate resize flicker and clamp bounds.", "0.8s");
                } else if (root.agentStep === 1) {
                    root.currentToolName = "Grep";
                    root.addMessage(Main.ToolCallMessage, "query=\"promptRowToAnsi\", path=\"src/Qamui/engine/\"", "Grep");
                } else if (root.agentStep === 2) {
                    root.addMessage(Main.ToolResultMessage, "Found 3 matches across 2 files (StreamRenderer.h, StreamRenderer.cpp)");
                } else if (root.agentStep === 3) {
                    root.currentToolName = "ViewFile";
                    root.addMessage(Main.ToolCallMessage, "path=\"src/Qamui/engine/StreamRenderer.cpp\", lines=70-105", "ViewFile");
                } else if (root.agentStep === 4) {
                    root.addMessage(Main.ToolResultMessage, "Read 35 lines from StreamRenderer.cpp");
                } else if (root.agentStep === 5) {
                    root.currentDiffFile = "src/Qamui/engine/StreamRenderer.cpp";
                    root.currentToolName = "EditFile";
                    root.currentToolTarget = "src/Qamui/engine/StreamRenderer.cpp";
                    root.currentToolAction = "Clamp safeHeight and emit atomic buffer flush (+3, -1 lines)";

                    root.addMessage(Main.DiffMessage, "", root.currentDiffFile);

                    if (!root.sessionAlwaysAllow) {
                        root.permissionIndex = 0;
                        root.agentState = Main.PermissionState;
                        agentTimer.stop();
                        return;
                    } else {
                        root.addMessage(Main.PermissionDecisionMessage, "Auto-approved: EditFile(" + root.currentToolTarget + ")", "Approved", "", "", "", true);
                    }
                } else if (root.agentStep === 6) {
                    root.currentToolName = "Bash";
                    root.addMessage(Main.ToolCallMessage, "command=\"ninja -C build qamui_agent_stream_demo\"", "Bash");
                } else if (root.agentStep === 7) {
                    root.addMessage(Main.ToolResultMessage, "Built target qamui_agent_stream_demo (exit 0, 0.28s)");
                } else if (root.agentStep === 8) {
                    root.currentToolName = "CTest";
                    root.addMessage(Main.ToolCallMessage, "command=\"ctest --test-dir build -R agent_stream\"", "CTest");
                } else if (root.agentStep === 9) {
                    root.addMessage(Main.ToolResultMessage, "Passed 2/2 tests: zero regressions (0.08s)");
                } else {
                    root.addMessage(Main.AgentMessage, "StreamRenderer optimization complete:\n" + "• Implemented in-memory atomic double buffering in StreamRenderer.cpp\n" + "• Replaced multi-step clears with single device.write() flushes\n" + "• Rebuilt and validated: 100% tests passed", "Agent", "2.1k tokens · 7.7s");
                    toast.notify("Refactor patch applied & tested", "success");
                    root.agentState = Main.ReadyState;
                    agentTimer.stop();
                    workingTimer.stop();
                }
            } else
            // Scenario 2: Swarm Audit (Displays TableView)
            if (root.currentScenario === 2) {
                if (root.agentStep === 0) {
                    root.currentToolName = "Swarm";
                    root.addMessage(Main.ThoughtMessage, "Spawning 3 parallel subagent workers (Memory, QML bindings, Concurrency).", "0.6s");
                } else if (root.agentStep === 1) {
                    root.currentToolName = "Subagent(MemoryAuditor)";
                    root.addMessage(Main.ToolCallMessage, "scope=\"src/Qamui/engine/\", check=\"leaks\"", "Subagent");
                } else if (root.agentStep === 2) {
                    root.addMessage(Main.ToolResultMessage, "MemoryAuditor: 0 heap leaks, 0 use-after-free conditions (checked 18 classes)");
                } else if (root.agentStep === 3) {
                    root.currentToolName = "Subagent(QmlProfiler)";
                    root.addMessage(Main.ToolCallMessage, "scope=\"examples/concepts/agent_stream/\", check=\"bindings\"", "Subagent");
                } else if (root.agentStep === 4) {
                    root.addMessage(Main.ToolResultMessage, "QmlProfiler: 100% typed delegate roles; 0 binding loops detected");
                } else if (root.agentStep === 5) {
                    root.addMessage(Main.SwarmTableMessage, "", "Parallel Audit Results");
                } else {
                    root.addMessage(Main.AgentMessage, "Swarm audit finished with 100% clean verification across all 3 workers.", "Agent", "3.1k tokens · 1.4s");
                    toast.notify("Swarm audit complete: 100% clean", "success");
                    root.agentState = Main.ReadyState;
                    agentTimer.stop();
                    workingTimer.stop();
                }
            } else
            // Scenario 3: Security Sandbox
            if (root.currentScenario === 3) {
                if (root.agentStep === 0) {
                    root.currentToolName = "SecSandbox";
                    root.addMessage(Main.ThoughtMessage, "Scanning text buffer bounds against UTF-8 overflow vectors.", "0.5s");
                } else if (root.agentStep === 1) {
                    root.currentToolName = "FuzzTester";
                    root.addMessage(Main.ToolCallMessage, "input=\"malformed_utf8_sequences.bin\"", "FuzzTester");
                } else if (root.agentStep === 2) {
                    root.addMessage(Main.ToolResultMessage, "Identified edge case in wide-character column clipping");
                } else if (root.agentStep === 3) {
                    root.currentToolName = "SecurityPatch";
                    root.currentToolTarget = "src/Qamui/engine/TextGridBuffer.cpp";
                    root.currentToolAction = "Add displayWidth boundary clamp (+4, -1 lines)";
                    root.addMessage(Main.DiffMessage, "", "src/Qamui/engine/TextGridBuffer.cpp");

                    if (!root.sessionAlwaysAllow) {
                        root.permissionIndex = 0;
                        root.agentState = Main.PermissionState;
                        agentTimer.stop();
                        return;
                    }
                } else if (root.agentStep === 4) {
                    root.addMessage(Main.ToolResultMessage, "Passed 24/24 buffer stress tests with 0 faults (0.12s)");
                } else {
                    root.addMessage(Main.AgentMessage, "Security patch applied and verified with stress tests.", "Agent", "2.4k tokens · 2.1s");
                    toast.notify("Security patch verified", "success");
                    root.agentState = Main.ReadyState;
                    agentTimer.stop();
                    workingTimer.stop();
                }
            } else
            // Scenario 4: Workspace Hierarchy (Displays TreeView)
            {
                if (root.agentStep === 0) {
                    root.currentToolName = "RepoIndexer";
                    root.addMessage(Main.ThoughtMessage, "Scanning repository workspace hierarchy and file layout...", "0.4s");
                } else if (root.agentStep === 1) {
                    root.addMessage(Main.RepoTreeMessage, "", "~/Qamui Workspace");
                } else {
                    root.addMessage(Main.AgentMessage, "Indexed workspace tree:\n" + "• Engine core: StreamRenderer (ANSI stream) & TextGridBuffer\n" + "• Stream components: TableView, TreeView, ProgressBar, Sparkline, StatusBadge\n" + "• Inline session: agent output stays in terminal scrollback", "Agent", "1.2k tokens · 0.9s");
                    toast.notify("Workspace tree indexed", "info");
                    root.agentState = Main.ReadyState;
                    agentTimer.stop();
                    workingTimer.stop();
                }
            }

            root.agentStep += 1;
        }
    }

    Component.onCompleted: {
        root.showBanner();
    }
}
