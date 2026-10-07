// Network page scenario, run against the mock providers.

function main(terminal) {
    terminal.waitForText("DEVICE COCKPIT")
    terminal.press("F2")
    terminal.waitForCondition((frameText) =>
        frameText.includes("INTERFACES")
        && frameText.includes("eth0")
        && frameText.includes("wlan0")
        && frameText.includes("192.168.7.2"), 3000)
    terminal.waitForCondition((frameText) =>
        frameText.includes("192.168.7.2/24")
        && frameText.includes("192.168.7.1")
        && frameText.includes("02:42:c0:a8:07:02"), 3000)

    // Apply a static configuration through the edit dialog.
    terminal.press("E")
    terminal.waitForText("STATIC IPV4 — eth0")
    terminal.press("A", ["Ctrl"])
    terminal.typeText("10.0.0.5")
    terminal.press("Tab")
    terminal.press("A", ["Ctrl"])
    terminal.typeText("16")
    terminal.press("Tab")
    terminal.press("A", ["Ctrl"])
    terminal.typeText("10.0.0.1")
    terminal.press("Enter")
    terminal.waitForText("static configuration applied", 3000)
    terminal.waitForCondition((frameText) =>
        frameText.includes("10.0.0.5/16")
        && frameText.includes("static"), 3000)
    terminal.captureText("cockpit/network.txt")

    // Cancelling the DHCP confirmation leaves the static config in place.
    terminal.press("D")
    terminal.waitForText("Request a DHCP lease on eth0?")
    terminal.press("N")
    terminal.waitForText("10.0.0.5/16")
    terminal.finish()
}
