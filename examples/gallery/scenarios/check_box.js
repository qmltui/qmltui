/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("Service Security & Logging Policy");
    terminal.expectText("2 / 4 enabled");
    terminal.expectText("--tls --audit");

    // Toggle focused TLS 1.3 checkbox off with Space
    terminal.press("SPACE");
    terminal.waitForText("1 / 4 enabled");
    terminal.expectText("TLS 1.3 (off)");

    // Click Verbose Trace and Strict CORS checkboxes
    terminal.clickText("Verbose Trace");
    terminal.waitForText("2 / 4 enabled");
    terminal.expectText("--audit --trace");

    terminal.clickText("Strict CORS");
    terminal.waitForText("3 / 4 enabled");
    terminal.expectText("--audit --trace --cors");

    // Click All button in Inspector
    terminal.clickText("All");
    terminal.waitForText("4 / 4 enabled");
    terminal.expectText("HARDENED");
    terminal.expectText("--tls --audit --trace --cors");

    // Click None button in Inspector
    terminal.clickText("None");
    terminal.waitForText("0 / 4 enabled");
    terminal.expectText("UNPROTECTED");
}
