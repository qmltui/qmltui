/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Inline mode");
    terminal.expectScrollbackText("QmlTui");
    terminal.expectScrollbackText("Inline Agent CLI");
    terminal.expectText("Enter a message.");
    terminal.typeText("hello inline");
    terminal.expectText("hello inline");
    terminal.captureText("typed.txt");
    terminal.press("Enter");
    terminal.expectText("printed: 1");
    terminal.expectNoText("hello inline");            // gone from the live region...
    terminal.expectScrollbackText("hello inline");    // ...and committed to the scrollback
    terminal.captureScrollbackStyled("scrollback_styled.txt");
    terminal.expectText("Enter a message.");
    terminal.typeText("clear me");
    terminal.press("Escape");
    terminal.expectNoText("clear me");
    terminal.captureText("cleared.txt");
}
