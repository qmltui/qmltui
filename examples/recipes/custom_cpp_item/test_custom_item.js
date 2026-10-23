function main(app) {
    app.expectText("Custom C++ Item")
    app.expectText("0%")
    app.expectRegex("┌─+┐")
    app.expectRegex("└─+┘")
    app.captureText("custom-item.txt")
}
