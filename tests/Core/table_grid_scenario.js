function main(app) {
    app.expectSnapshot("table_grid_snapshot.txt");
    app.captureText("comparison.txt");
    app.captureStyled("comparison_styled.txt");
    app.press("r");
    app.expectTextInRegion(0, 8, 164, 1, "└");
    app.expectNoText("Public traction");
    app.clickText("Approach");
    app.press("PageDown");
    app.expectTextInRegion(1, 3, 17, 1, "API users write");
    app.expectTextInRegion(1, 7, 17, 1, "Commits");
    app.expectStyle(2, 7, {foreground: "#00FF00FF", background: "#102030FF", bold: true});
    app.expectStyle(18, 7, {character: "│", foreground: "default", background: "#102030FF", bold: false});
    app.captureStyled("paged_styled.txt");
    app.press("g");
    // Toggling the row grid grows the capacity. The viewport is remembered
    // now rather than derived from the selection, so it stays where PageDown
    // left it instead of snapping back to pin the selection to the last line;
    // dropping the dividers just packs the same rows tighter.
    app.expectTextInRegion(1, 3, 17, 1, "API users write");
    app.expectTextInRegion(1, 5, 17, 1, "Commits");
    app.press("c");
    app.expectStyle(18, 3, {character: " "})
    app.expectStyle(19, 3, {character: "D"});   // "Declarative", the row the view is on
    app.press("b");
    app.expectTextInRegion(1, 2, 17, 1, "API users write");
    app.captureText("compact.txt");
    app.press("b"); app.press("c"); app.press("g"); app.press("w");
    app.expectSnapshot("table_grid_snapshot.txt");
    app.press("d");
    app.expectStyle(0, 0, {character: "╔"});
    app.expectStyle(18, 2, {character: "╬"});
    app.captureText("double.txt");
    app.press("a");
    app.expectStyle(0, 0, {character: "+"});
    app.expectStyle(1, 0, {character: "-"});
    app.captureText("ascii.txt");
    app.press("s");
    app.expectSnapshot("table_grid_snapshot.txt");
}
