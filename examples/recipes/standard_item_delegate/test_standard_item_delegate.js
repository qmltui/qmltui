function main(app) {
    // All three views show the same styled model: glyph, check box, colors.
    app.expectText("ListView + StandardItemDelegate")
    app.expectText("* [x] Freeze API")
    app.expectText("[-] Run benchmarks")
    app.expectText("! [ ] Sign installers")
    app.expectText("1 of 5 tasks done")
    // TableView header comes from headerData(), no columns descriptors set.
    app.expectText("Task")
    app.expectText("Owner")
    // The disabled row paints dimmed in the ListView delegate.
    app.expectStyle(2, 8, { dim: true })
    app.captureText("initial.txt")
    app.sleep(800)

    // Space toggles the current row through setData(); every view follows.
    app.press("Down")
    app.sleep(600)
    app.press("Space")
    app.expectText("[x] Update docs")
    app.expectText("2 of 5 tasks done")
    app.expectNoText("[ ] Update docs")
    app.captureText("toggled.txt")
    app.sleep(800)

    // A disabled item refuses to toggle.
    app.press("Down")
    app.sleep(400)
    app.press("Down")
    app.sleep(400)
    app.press("Down")
    app.sleep(400)
    app.press("Space")
    app.expectText("[ ] Announce")
    app.expectText("2 of 5 tasks done")
    app.sleep(900)
}
