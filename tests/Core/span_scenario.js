function main(terminal) {
    terminal.expectText("Status: Active\nWorkers: 4");
    terminal.expectStyle(0, 0, {bold: false, dim: true, foreground: "#00FFFFFF"});
    terminal.expectStyle(8, 0, {bold: true, foreground: "#00FF00FF"});
    terminal.captureStyled("initial.txt");
    terminal.injectKey("s");
    terminal.expectRepaint();
    terminal.expectStyle(0, 0, {bold: false, dim: true, foreground: "#FF0000FF"});
    terminal.expectStyle(8, 0, {bold: false, foreground: "#00FF00FF"});
    terminal.captureStyled("inherited.txt");
    terminal.injectKey("r");
    terminal.expectRepaint();
    terminal.expectText("Replaced");
    terminal.expectNoText("Workers");
    terminal.expectStyle(0, 0, {underline: true, foreground: "#FF0000FF"});
    terminal.captureStyled("replaced.txt");
    terminal.injectKey("c");
    terminal.expectRepaint();
    terminal.expectText("Fallback");
    terminal.expectStyle(0, 0, {underline: false});
    terminal.captureStyled("fallback.txt");
}
