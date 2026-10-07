// Overview smoke scenario, run against the mock providers
// (main.cpp defaults scenario runs to QTUI_COCKPIT_MOCK=1).

// Shell and overview render with the mock identity.
function main(terminal) {
    terminal.waitForCondition((frameText) =>
        frameText.includes("DEVICE COCKPIT")
        && frameText.includes("cockpit-demo")
        && frameText.includes("MOCK"), 3000)

    // Overview panels show the deterministic mock inventory.
    terminal.waitForCondition((frameText) =>
        frameText.includes("SYSTEM")
        && frameText.includes("CPU")
        && frameText.includes("MEMORY")
        && frameText.includes("STORAGE"), 3000)
    terminal.waitForText("6.6.0-mock")
    terminal.waitForText("eth0")
    terminal.waitForText("192.168.7.2")
    terminal.waitForText("/data")
    terminal.captureText("cockpit/overview.txt")

    // Page switching reaches every page and returns.
    terminal.press("F2")
    terminal.waitForText("INTERFACES")
    terminal.press("F3")
    terminal.waitForText("mosquitto.service")
    terminal.press("F4")
    terminal.waitForText("LIVE JOURNAL")
    terminal.press("1")
    terminal.waitForText("STORAGE")
    terminal.finish()
}
