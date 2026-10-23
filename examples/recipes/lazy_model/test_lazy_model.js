function main(app) {
    // The root level is fetched on load; nothing below it is.
    app.expectText("Lazy model")
    app.expectText("> dir-0")
    app.expectText("fetchMore calls: 1")
    app.expectText("file-0.txt  4 KiB")
    app.expectNoText("file-0.txt  17 KiB")
    app.captureText("root.txt")

    // Expanding the current branch fetches exactly one more level.
    app.press("Right")
    app.expectText("fetchMore calls: 2")
    app.expectText("  > dir-0")
    app.expectText("file-0.txt  17 KiB")
    app.captureText("expanded.txt")

    // Collapsing and re-expanding does not fetch again.
    app.press("Left")
    app.expectNoText("  > dir-0")
    app.press("Right")
    app.expectText("fetchMore calls: 2")
}
