
/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Ask anything")
    terminal.expectNoText("shell")
    terminal.captureText("inline.txt")

    // The session banner lives in the scrollback, never in the live frame.
    terminal.expectScrollbackText("Qamui")
    terminal.expectScrollbackText("agent stream demo")
    terminal.expectNoScrollbackText("Ask anything")
    terminal.captureScrollback("banner.txt")

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

    console.log("Waiting for the agent to be ready...")

    terminal.sleep(3000)

    terminal.typeText("/demo 1")
    terminal.press("Enter")
    terminal.waitForText("Allow once", 7000)
    terminal.captureText("permission.txt")
    terminal.press("3")
    terminal.expectText("Ask anything")
    terminal.captureText("ready.txt")
}
