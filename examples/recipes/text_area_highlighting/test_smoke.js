/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Search:");

    // 1. JSON Mode (C++ Highlighter)
    const keyword = terminal.findText("true")[0];
    terminal.expectStyle(keyword.x, keyword.y, { character: "t", foreground: "blue", bold: true });
    const number = terminal.findText("8080")[0];
    terminal.expectStyle(number.x, number.y, { character: "8", foreground: "cyan" });

    // The search layer (priority 10) stacks over syntax: adds background & bold
    terminal.clickText("search (e.g. timeout)...");
    terminal.typeText("port");
    const match = terminal.findText("port").find(m => m.y >= 3);
    if (!match)
        terminal.fail("search term not found in the editor");
    terminal.expectStyle(match.x, match.y,
        { character: "p", foreground: "green", background: "yellow", bold: true });

    // Palette toggle via mouse click
    terminal.clickText("[Toggle Palette]");
    terminal.expectStyle(match.x, match.y, { foreground: "magenta", background: "yellow" });

    // Palette toggle via keyboard shortcut (F2)
    terminal.press("F2");
    terminal.expectStyle(match.x, match.y, { foreground: "green", background: "yellow" });

    // 2. Switch to QML Mode via keyboard shortcut (F3) -> activates Pure QML Highlighter
    terminal.press("F3");
    terminal.expectText("Mode: QML");

    // Pure QML highlighter: 'import' keyword is bold blue, 'Terminal' type is bold cyan
    const qmlKw = terminal.findText("import")[0];
    terminal.expectStyle(qmlKw.x, qmlKw.y, { character: "i", foreground: "blue", bold: true });
    const qmlType = terminal.findText("Terminal")[0];
    terminal.expectStyle(qmlType.x, qmlType.y, { character: "T", foreground: "cyan", bold: true });

    // Palette toggle via F2 in QML Mode
    const qmlStr = terminal.findText("Hello from QmlTui QML!")[0];
    terminal.expectStyle(qmlStr.x, qmlStr.y, { foreground: "green" });
    terminal.press("F2");
    terminal.expectStyle(qmlStr.x, qmlStr.y, { foreground: "magenta" });

    // 3. Switch back to JSON mode via F3
    terminal.press("F3");
    terminal.expectText("Mode: JSON");

    // 4. Tab indentation in TextArea: soft tab (2 spaces) without losing focus
    terminal.clickText("fallback");
    terminal.press("Home");
    terminal.press("Tab");
    terminal.expectText("    \"fallback\": null");

    terminal.captureText("text_area_highlighting.txt");
    terminal.captureStyled("text_area_highlighting_styled.txt");
}
