function main(terminal) {
    // The root level is fetched on load; nothing below it is.
    terminal.expectText("Lazy model")
    terminal.expectText("> dir-0")
    terminal.expectText("fetchMore calls: 1")
    terminal.expectText("file-0.txt  4 KiB")
    terminal.expectNoText("file-0.txt  17 KiB")
    terminal.captureText("root.txt")

    // Expanding the current branch fetches exactly one more level.
    terminal.press("Right")
    terminal.expectText("fetchMore calls: 2")
    terminal.expectText("  > dir-0")
    terminal.expectText("file-0.txt  17 KiB")
    terminal.captureText("expanded.txt")

    // Collapsing and re-expanding does not fetch again.
    terminal.press("Left")
    terminal.expectNoText("  > dir-0")
    terminal.press("Right")
    terminal.expectText("fetchMore calls: 2")
}
