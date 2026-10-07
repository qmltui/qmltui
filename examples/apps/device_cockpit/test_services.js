// Services page scenario, run against the mock providers.

function main(terminal) {
    terminal.waitForText("DEVICE COCKPIT")
    terminal.press("F3")
    terminal.waitForCondition((frameText) =>
        frameText.includes("device-agent.service")
        && frameText.includes("mosquitto.service")
        && frameText.includes("chronyd.service")
        && frameText.includes("MANAGED"), 3000)

    // Filter narrows the table down to sshd.
    terminal.press("/")
    terminal.typeText("ssh")
    terminal.waitForCondition((frameText) =>
        frameText.includes("sshd.service")
        && !frameText.includes("mosquitto.service"), 3000)
    terminal.press("Escape")

    // Stop the selected unit through the confirmation dialog.
    terminal.press("Down")
    terminal.press("X")
    terminal.waitForText("Stop sshd.service?")
    terminal.press("Y")
    terminal.waitForText("stop succeeded", 3000)
    terminal.waitForCondition((frameText) =>
        frameText.includes("inactive") && frameText.includes("dead"), 3000)
    terminal.captureText("cockpit/services.txt")

    // The rejected path leaves the unit untouched.
    // The provider replaces its QVariantList after stopping; select the refreshed row.
    terminal.press("Down")
    terminal.press("S")
    terminal.waitForText("Start sshd.service?")
    terminal.press("N")
    terminal.expectNoText("CONFIRM")
    terminal.finish()
}
