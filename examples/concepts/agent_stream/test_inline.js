function main(terminal) {
    terminal.expectText("Ask anything")
    terminal.expectNoText("shell")
    terminal.captureText("inline.txt")

    terminal.typeText("persistent prompt")
    for (const key of ["F1", "T", "B"]) {
        terminal.press(key, key === "F1" ? [] : ["Ctrl"])
        terminal.expectText("persistent prompt")
        terminal.expectText("[F2] model")
        terminal.expectNoText("[Shift+PgUp/PgDn]")
    }
    terminal.captureText("prompt.txt")

    terminal.press("C", ["Ctrl"])
    terminal.press("F2")
    terminal.expectText("Neural model selector")
    terminal.captureText("model.txt")
    terminal.press("Escape")
    terminal.expectText("Ask anything")

    terminal.typeText("/demo 1")
    terminal.press("Enter")
    terminal.waitForText("Allow once", 7000)
    terminal.captureText("permission.txt")
    terminal.press("3")
    terminal.expectText("Ask anything")
    terminal.captureText("ready.txt")
}
