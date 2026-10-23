/** @param {Runner} app */
function main(app) {
    app.expectText("Search:");

    // 1. JSON Mode (C++ Highlighter)
    const keyword = app.findText("true")[0];
    app.expectStyle(keyword.x, keyword.y, { character: "t", foreground: "blue", bold: true });
    const number = app.findText("8080")[0];
    app.expectStyle(number.x, number.y, { character: "8", foreground: "cyan" });

    // The search layer (priority 10) stacks over syntax: adds background & bold
    app.clickText("search (e.g. timeout)...");
    app.typeText("port");
    const match = app.findText("port").find(m => m.y >= 3);
    if (!match)
        app.fail("search term not found in the editor");
    app.expectStyle(match.x, match.y,
        { character: "p", foreground: "green", background: "yellow", bold: true });

    // Palette toggle via mouse click
    app.clickText("[Toggle Palette]");
    app.expectStyle(match.x, match.y, { foreground: "magenta", background: "yellow" });

    // Palette toggle via keyboard shortcut (F2)
    app.press("F2");
    app.expectStyle(match.x, match.y, { foreground: "green", background: "yellow" });

    // 2. Switch to QML Mode via keyboard shortcut (F3) -> activates Pure QML Highlighter
    app.press("F3");
    app.expectText("Mode: QML");

    // Pure QML highlighter: 'import' keyword is bold blue, 'Terminal' type is bold cyan
    const qmlKw = app.findText("import")[0];
    app.expectStyle(qmlKw.x, qmlKw.y, { character: "i", foreground: "blue", bold: true });
    const qmlType = app.findText("Terminal")[0];
    app.expectStyle(qmlType.x, qmlType.y, { character: "T", foreground: "cyan", bold: true });

    // Palette toggle via F2 in QML Mode
    const qmlStr = app.findText("Hello from QmlTui QML!")[0];
    app.expectStyle(qmlStr.x, qmlStr.y, { foreground: "green" });
    app.press("F2");
    app.expectStyle(qmlStr.x, qmlStr.y, { foreground: "magenta" });

    // 3. Switch back to JSON mode via F3
    app.press("F3");
    app.expectText("Mode: JSON");

    // 4. Tab indentation in TextArea: soft tab (2 spaces) without losing focus
    app.clickText("fallback");
    app.press("Home");
    app.press("Tab");
    app.expectText("    \"fallback\": null");

    app.captureText("text_area_highlighting.txt");
    app.captureStyled("text_area_highlighting_styled.txt");
}
