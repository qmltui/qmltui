/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("Cluster Overview — us-east-prod");
    terminal.expectText("currentIndex: 0");
    terminal.expectText("active: Overview");

    // Press RIGHT on focused TabBar to switch to Processes
    terminal.press("RIGHT");
    terminal.waitForText("Active Processes (Top CPU)");
    terminal.expectText("currentIndex: 1");
    terminal.expectText("active: Processes");

    // Press RIGHT to switch to Network
    terminal.press("RIGHT");
    terminal.waitForText("Network Interfaces & Throughput");
    terminal.expectText("currentIndex: 2");
    terminal.expectText("active: Network");

    // Click Next Tab button in Inspector to switch to Storage
    terminal.clickText("Next Tab");
    terminal.waitForText("Storage Volumes & IOPS");
    terminal.expectText("currentIndex: 3");
    terminal.expectText("active: Storage");

    // Toggle selectionColor checkbox in Inspector
    terminal.clickText("selectionColor");
    terminal.waitForText("style: Reverse");
}
