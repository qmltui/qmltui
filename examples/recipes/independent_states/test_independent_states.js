/** @param {Runner} app */
function main(app) {
    // 1. ENTRY: Initial state check (offline, Card 1 focused)
    app.expectText("STATUS: Offline (Disconnected)");
    app.expectText("[Card 1: Primary Node]");
    app.expectText("[Card 2: Secondary Node]");
    app.expectStyled("snapshots/entry.txt");

    // 2. TRIGGER & MIDPOINT: Initiate connection with 200ms ColorAnimation
    app.press("c");
    app.advanceTime(100); // 100ms: midpoint of transition
    app.expectText("STATUS: Connecting...");
    app.expectStyled("snapshots/midpoint.txt");

    // 3. COMPLETION: Finish connection transition and connectTimer
    app.advanceTime(100); // completes connecting transition
    app.advanceTime(200); // timer triggers "connected" + 200ms transition
    app.expectText("STATUS: Connected (Cluster Healthy)");
    app.expectStyled("snapshots/completion.txt");

    // 4. INTERRUPTION: Disconnect, then interrupt mid-flight by reconnecting
    app.press("c"); // starts disconnect transition
    app.advanceTime(50); // interrupted at 50ms
    app.press("c"); // interrupts disconnect, begins reconnect
    app.expectText("STATUS: Connecting...");
    app.expectStyled("snapshots/interruption.txt");
    app.advanceTime(400); // allow connecting transition + timer to settle
    app.expectText("STATUS: Connected (Cluster Healthy)");

    // 5. RESIZE: Verify layout and frame stability under viewport change
    app.resize(100, 26);
    app.expectText("INDEPENDENT STATES & TRANSITIONS DEMO");
    app.expectStyled("snapshots/resize.txt");
    app.resize(80, 24);

    // 6. RESTORATION: Reset to base state and verify restored styles and text
    app.press("r");
    app.advanceTime(200);
    app.expectText("STATUS: Offline (Disconnected)");
    app.expectStyled("snapshots/restoration.txt");

    // 7. CONTEXTUAL QUIT PATH:
    // When connecting, Escape contextually cancels in-flight connection
    app.press("c");
    app.expectText("STATUS: Connecting...");
    app.press("Escape");
    app.expectText("STATUS: Offline (Disconnected)");

    // When idle/offline, Escape exits the application
    app.press("Escape");
}
