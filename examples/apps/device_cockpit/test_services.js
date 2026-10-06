// Services page scenario, run against the mock providers.

Test.waitForText("DEVICE COCKPIT")
Test.press("F3")
Test.waitForCondition((frameText) =>
    frameText.includes("device-agent.service")
    && frameText.includes("mosquitto.service")
    && frameText.includes("chronyd.service")
    && frameText.includes("MANAGED"), 3000)

// Filter narrows the table down to sshd.
Test.press("/")
Test.typeText("ssh")
Test.waitForCondition((frameText) =>
    frameText.includes("sshd.service")
    && !frameText.includes("mosquitto.service"), 3000)
Test.press("Escape")

// Stop the selected unit through the confirmation dialog.
Test.press("Down")
Test.press("X")
Test.waitForText("Stop sshd.service?")
Test.press("Y")
Test.waitForText("stop succeeded", 3000)
Test.waitForCondition((frameText) =>
    frameText.includes("inactive") && frameText.includes("dead"), 3000)
Test.captureText("cockpit/services.txt")

// The rejected path leaves the unit untouched.
// The provider replaces its QVariantList after stopping; select the refreshed row.
Test.press("Down")
Test.press("S")
Test.waitForText("Start sshd.service?")
Test.press("N")
Test.expectNoText("CONFIRM")
Test.finish()
