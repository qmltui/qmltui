/** @param {Terminal} terminal */
function main(terminal) {
    // 1. ENTRY: Initial state check (offline, Card 1 focused)
    terminal.expectText("STATUS: Offline (Disconnected)");
    terminal.expectText("[Card 1: Primary Node]");
    terminal.expectText("[Card 2: Secondary Node]");
    terminal.expectStyled("snapshots/entry.txt");

    // 2. TRIGGER & MIDPOINT: Initiate connection with 200ms ColorAnimation
    terminal.press("c");
    terminal.advanceTime(100); // 100ms: midpoint of transition
    terminal.expectText("STATUS: Connecting...");
    terminal.expectStyled("snapshots/midpoint.txt");

    // 3. COMPLETION: Finish connection transition and connectTimer
    terminal.advanceTime(100); // completes connecting transition
    terminal.advanceTime(200); // timer triggers "connected" + 200ms transition
    terminal.expectText("STATUS: Connected (Cluster Healthy)");
    terminal.expectStyled("snapshots/completion.txt");

    // 4. INTERRUPTION: Disconnect, then interrupt mid-flight by reconnecting
    terminal.press("c"); // starts disconnect transition
    terminal.advanceTime(50); // interrupted at 50ms
    terminal.press("c"); // interrupts disconnect, begins reconnect
    terminal.expectText("STATUS: Connecting...");
    terminal.expectStyled("snapshots/interruption.txt");
    terminal.advanceTime(400); // allow connecting transition + timer to settle
    terminal.expectText("STATUS: Connected (Cluster Healthy)");

    // 5. RESIZE: Verify layout and frame stability under viewport change
    terminal.resize(100, 26);
    terminal.expectText("INDEPENDENT STATES & TRANSITIONS DEMO");
    terminal.expectStyled("snapshots/resize.txt");
    terminal.resize(80, 24);

    // 6. RESTORATION: Reset to base state and verify restored styles and text
    terminal.press("r");
    terminal.advanceTime(200);
    terminal.expectText("STATUS: Offline (Disconnected)");
    terminal.expectStyled("snapshots/restoration.txt");

    // 7. CONTEXTUAL QUIT PATH:
    // When connecting, Escape contextually cancels in-flight connection
    terminal.press("c");
    terminal.expectText("STATUS: Connecting...");
    terminal.press("Escape");
    terminal.expectText("STATUS: Offline (Disconnected)");

    // When idle/offline, Escape exits the application
    terminal.press("Escape");
}
