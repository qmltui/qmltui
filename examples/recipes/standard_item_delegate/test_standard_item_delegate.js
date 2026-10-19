function main(terminal) {
    // All three views show the same styled model: glyph, check box, colors.
    terminal.expectText("ListView + StandardItemDelegate")
    terminal.expectText("* [x] Freeze API")
    terminal.expectText("[-] Run benchmarks")
    terminal.expectText("! [ ] Sign installers")
    terminal.expectText("1 of 5 tasks done")
    // TableView header comes from headerData(), no columns descriptors set.
    terminal.expectText("Task")
    terminal.expectText("Owner")
    // The disabled row paints dimmed in the ListView delegate.
    terminal.expectStyle(2, 8, { dim: true })
    terminal.captureText("initial.txt")

    // Space toggles the current row through setData(); every view follows.
    terminal.press("Down")
    terminal.press("Space")
    terminal.expectText("[x] Update docs")
    terminal.expectText("2 of 5 tasks done")
    terminal.expectNoText("[ ] Update docs")
    terminal.captureText("toggled.txt")

    // A disabled item refuses to toggle.
    terminal.press("Down")
    terminal.press("Down")
    terminal.press("Down")
    terminal.press("Space")
    terminal.expectText("[ ] Announce")
    terminal.expectText("2 of 5 tasks done")
}
