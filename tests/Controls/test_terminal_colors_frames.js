function main(terminal) {
    terminal.expectStyle(3, 0, { character: "D", foreground: "#00000000", background: "#00000000" });
    terminal.expectStyle(4, 2, { character: "C", foreground: "#123456ff", background: "#202830ff" });
    terminal.expectStyle(3, 4, { character: "R", foreground: "#123456ff" });
    terminal.expectStyle(0, 6, { character: "[", foreground: "#00000000", background: "#00000000", reversed: true });
    terminal.expectStyle(0, 8, { character: "H", foreground: "#00000000", dim: true });
    terminal.expectStyle(0, 10, { character: "▀", foreground: "#00000000" });
    terminal.expectStyle(1, 10, { character: "▄", foreground: "#00000000" });
    terminal.expectStyle(2, 10, { character: "█", foreground: "#00000000" });
    terminal.captureText("defaults.txt");
    terminal.press("f");
    terminal.expectStyle(4, 2, { foreground: "#123456ff", bold: true });
    terminal.press("t");
    terminal.expectStyle(1, 3, { character: "T", foreground: "#ffffffff", background: "#654321ff" });
    terminal.press("d");
    terminal.expectStyle(4, 2, { foreground: "#123456ff", dim: true });
    terminal.captureText("disabled.txt");
    terminal.injectKey("c");
    terminal.expectRepaint();
    terminal.expectStyle(4, 2, { foreground: "#abcdefff", dim: true });
    terminal.expectStyle(3, 4, { foreground: "#abcdefff" });
    terminal.expectStyle(3, 0, { foreground: "#00000000" });
    terminal.captureText("customized.txt");
}
