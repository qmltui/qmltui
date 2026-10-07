function main(terminal) {
    terminal.expectText("Custom C++ Item")
    terminal.expectText("0%")
    terminal.expectRegex("┌─+┐")
    terminal.expectRegex("└─+┘")
    terminal.captureText("custom-item.txt")
}
