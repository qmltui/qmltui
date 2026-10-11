function main(terminal) {
    terminal.expectText("DUAL TERMINAL WORKFLOW")
    terminal.waitForText("[RUNNING]")
    terminal.press("F1")
    terminal.typeText("printf 'QAMUI_%s\\n' LEFT_READY")
    terminal.press("Enter")
    terminal.waitForText("QAMUI_LEFT_READY")

    terminal.press("F2")
    terminal.typeText("printf 'QAMUI_%s\\n' RIGHT_READY")
    terminal.press("Enter")
    terminal.waitForText("QAMUI_RIGHT_READY")
    terminal.expectText("QAMUI_LEFT_READY")
    terminal.captureText("shells.txt")

    terminal.press("Escape")
    terminal.expectText("Focused Command Input")
    terminal.press("Enter")
    terminal.waitForCondition(frame => frame.split("\n").slice(3, 26)
        .some(line => line.includes("│Hello Qamui Dual Terminal!")))
    terminal.captureText("dispatcher.txt")
}
