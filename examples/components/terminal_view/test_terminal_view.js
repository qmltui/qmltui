function main(terminal) {
    terminal.expectText("DUAL TERMINAL WORKFLOW")
    terminal.waitForText("[RUNNING]")
    terminal.press("F1")
    terminal.typeText("printf 'QMLTUI_%s\\n' LEFT_READY")
    terminal.press("Enter")
    terminal.waitForText("QMLTUI_LEFT_READY")

    terminal.press("F2")
    terminal.typeText("printf 'QMLTUI_%s\\n' RIGHT_READY")
    terminal.press("Enter")
    terminal.waitForText("QMLTUI_RIGHT_READY")
    terminal.expectText("QMLTUI_LEFT_READY")
    terminal.captureText("shells.txt")

    terminal.press("Escape")
    terminal.expectText("Focused Command Input")
    terminal.press("Enter")
    terminal.waitForCondition(frame => frame.split("\n").slice(3, 26)
        .some(line => line.includes("│Hello QmlTui Dual Terminal!")))
    terminal.captureText("dispatcher.txt")
}
