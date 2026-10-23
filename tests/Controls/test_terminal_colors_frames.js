function main(app) {
    app.expectStyle(3, 0, { character: "D", foreground: "#00000000", background: "#00000000" });
    app.expectStyle(4, 2, { character: "C", foreground: "#123456ff", background: "#202830ff" });
    app.expectStyle(3, 4, { character: "R", foreground: "#123456ff" });
    app.expectStyle(0, 6, { character: "[", foreground: "#00000000", background: "#00000000", reversed: true });
    app.expectStyle(0, 8, { character: "H", foreground: "#00000000", dim: true });
    app.expectStyle(0, 10, { character: "▀", foreground: "#00000000" });
    app.expectStyle(1, 10, { character: "▄", foreground: "#00000000" });
    app.expectStyle(2, 10, { character: "█", foreground: "#00000000" });
    app.captureText("defaults.txt");
    app.press("f");
    app.expectStyle(4, 2, { foreground: "#123456ff", bold: true });
    app.press("t");
    app.expectStyle(1, 3, { character: "T", foreground: "#ffffffff", background: "#654321ff" });
    app.press("d");
    app.expectStyle(4, 2, { foreground: "#123456ff", dim: true });
    app.captureText("disabled.txt");
    app.injectKey("c");
    app.expectRepaint();
    app.expectStyle(4, 2, { foreground: "#abcdefff", dim: true });
    app.expectStyle(3, 4, { foreground: "#abcdefff" });
    app.expectStyle(3, 0, { foreground: "#00000000" });
    app.captureText("customized.txt");
}
