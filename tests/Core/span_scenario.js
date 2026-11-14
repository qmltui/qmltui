function main(app) {
    app.expectText("Status: Active\nWorkers: 4");
    app.expectStyle(0, 0, {bold: false, dim: true, foreground: "#00FFFFFF"});
    app.expectStyle(8, 0, {bold: true, foreground: "#00FF00FF"});
    app.captureStyled("initial.txt");
    app.injectKey("s");
    app.expectRepaint();
    app.expectStyle(0, 0, {bold: false, dim: true, foreground: "#FF0000FF"});
    app.expectStyle(8, 0, {bold: false, foreground: "#00FF00FF"});
    app.captureStyled("inherited.txt");
    app.injectKey("r");
    app.expectRepaint();
    app.expectText("Replaced");
    app.expectNoText("Workers");
    app.expectStyle(0, 0, {underline: true, foreground: "#FF0000FF"});
    app.captureStyled("replaced.txt");
    app.injectKey("c");
    app.expectRepaint();
    app.expectText("Fallback");
    app.expectStyle(0, 0, {underline: false});
    app.captureStyled("fallback.txt");
    app.injectKey("k");
    app.expectRepaint();
    app.expectStyle(0, 2, {character: "X", strikethrough: true, blink: false, conceal: false});
}
