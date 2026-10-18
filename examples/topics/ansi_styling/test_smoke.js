/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Concept Demo: Text Styling & ANSI Parser");
    terminal.expectText("4. Terminal Hyperlinks (OSC 8 Browser Links & In-App Clicks):");
    terminal.expectText("Last activated link: none");

    terminal.clickText("Qt Project");
    terminal.waitForText("Last activated link: https://qt.io");

    terminal.clickText("QmlTui Docs");
    terminal.waitForText("Last activated link: https://qmltui.org");

    terminal.advanceTime(200);
    terminal.captureText("ansi_styling.txt");
}
