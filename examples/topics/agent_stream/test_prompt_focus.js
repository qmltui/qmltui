/** @param {Terminal} terminal */
function main(terminal) {
    function expectPromptAcceptsText(text) {
        terminal.expectText("Ask anything")
        terminal.typeText(text)
        terminal.expectText(text)
    }

    // Completing successive requests must leave the next prompt ready to type.
    for (let i = 0; i < 2; ++i) {
        expectPromptAcceptsText("list files")
        terminal.press("Enter")
        terminal.advanceTime(2000)
        terminal.expectScrollbackText("Indexed workspace tree")
    }

    // Interrupting work must restore focus too.
    expectPromptAcceptsText("audit workspace")
    terminal.press("Enter")
    terminal.press("C", ["Ctrl"])
    expectPromptAcceptsText("security patch")
    terminal.press("Enter")
    terminal.advanceTime(2500)
    terminal.expectText("Allow once")
    terminal.press("3")

    // Denying permission and dismissing a modal must both restore the prompt.
    expectPromptAcceptsText("next request")
    terminal.press("F2")
    terminal.expectText("Neural model selector")
    terminal.press("Escape")
    terminal.typeText(" after modal")
    terminal.expectText("next request after modal")
    terminal.captureText("focused-prompt.txt")
}
