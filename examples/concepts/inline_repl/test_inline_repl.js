function main(terminal) {
    terminal.expectText("Enter a message.")
    terminal.typeText("hello inline")
    terminal.expectText("hello inline")
    terminal.captureText("typed.txt")
    terminal.press("Enter")
    terminal.expectNoText("hello inline")
    terminal.expectText("Enter a message.")
    terminal.typeText("clear me")
    terminal.press("Escape")
    terminal.expectNoText("clear me")
    terminal.captureText("cleared.txt")
}
