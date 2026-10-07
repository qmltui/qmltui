// Logs page scenario, run against the mock providers.

function main(terminal) {
    terminal.waitForText("DEVICE COCKPIT")
    terminal.press("F4")
    terminal.waitForCondition((frameText) =>
        frameText.includes("LIVE JOURNAL")
        && frameText.includes("STREAMING")
        && frameText.includes("FOLLOW: ON"), 3000)

    // The deterministic mock backlog is replayed into the view.
    terminal.waitForText("telemetry batch published (42 points)")
    terminal.waitForText("ERR chronyd[201]: exiting: could not step system clock")
    terminal.waitForText("WRN chronyd[201]: no reachable NTP sources")
    terminal.captureText("cockpit/logs.txt")

    // Follow toggling.
    terminal.press("F")
    terminal.waitForText("FOLLOW: OFF")
    terminal.press("F")
    terminal.waitForText("FOLLOW: ON")

    // Clear empties the view; new lines keep arriving afterwards.
    terminal.press("C")
    terminal.expectNoText("telemetry batch published (42 points)")
    terminal.waitForCondition((frameText) =>
        frameText.includes("device-agent[312]")
        || frameText.includes("mosquitto[220]")
        || frameText.includes("watchdog-feeder[98]")
        || frameText.includes("kernel"), 5000)
    terminal.finish()
}
