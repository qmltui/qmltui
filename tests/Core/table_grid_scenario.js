function main(terminal) {
    terminal.expectSnapshot("table_grid_snapshot.txt");
    terminal.captureText("comparison.txt");
    terminal.captureStyled("comparison_styled.txt");
    terminal.press("r");
    terminal.expectTextInRegion(0, 8, 164, 1, "└");
    terminal.expectNoText("Public traction");
    terminal.clickText("Approach");
    terminal.press("PageDown");
    terminal.expectTextInRegion(1, 3, 17, 1, "API users write");
    terminal.expectTextInRegion(1, 7, 17, 1, "Commits");
    terminal.expectStyle(2, 7, {foreground: "#00FF00FF", background: "#102030FF", bold: true});
    terminal.expectStyle(18, 7, {character: "│", foreground: "default", background: "#102030FF", bold: false});
    terminal.captureStyled("paged_styled.txt");
    terminal.press("g");
    // Toggling the row grid grows the capacity. The viewport is remembered
    // now rather than derived from the selection, so it stays where PageDown
    // left it instead of snapping back to pin the selection to the last line;
    // dropping the dividers just packs the same rows tighter.
    terminal.expectTextInRegion(1, 3, 17, 1, "API users write");
    terminal.expectTextInRegion(1, 5, 17, 1, "Commits");
    terminal.press("c");
    terminal.expectStyle(18, 3, {character: " "})
    terminal.expectStyle(19, 3, {character: "D"});   // "Declarative", the row the view is on
    terminal.press("b");
    terminal.expectTextInRegion(1, 2, 17, 1, "API users write");
    terminal.captureText("compact.txt");
    terminal.press("b"); terminal.press("c"); terminal.press("g"); terminal.press("w");
    terminal.expectSnapshot("table_grid_snapshot.txt");
    terminal.press("d");
    terminal.expectStyle(0, 0, {character: "╔"});
    terminal.expectStyle(18, 2, {character: "╬"});
    terminal.captureText("double.txt");
    terminal.press("a");
    terminal.expectStyle(0, 0, {character: "+"});
    terminal.expectStyle(1, 0, {character: "-"});
    terminal.captureText("ascii.txt");
    terminal.press("s");
    terminal.expectSnapshot("table_grid_snapshot.txt");
}
