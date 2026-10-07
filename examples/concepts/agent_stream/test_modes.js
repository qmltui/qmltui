function main(terminal) {
    terminal.expectText("Ask anything")
    terminal.captureText("inline.txt")
    terminal.press("F1")
    terminal.expectText("return")
    terminal.expectNoText("Ask anything")
    terminal.captureText("shell.txt")
    terminal.press("F1")
    terminal.expectText("Ask anything")
    terminal.expectNoText("[Shift+PgUp/PgDn]")
    terminal.captureText("resumed.txt")
    terminal.typeText("persistent prompt")
    terminal.expectText("persistent prompt")
}
