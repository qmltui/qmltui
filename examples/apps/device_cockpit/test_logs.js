// Logs page scenario, run against the mock providers.

Test.waitForText("DEVICE COCKPIT")
Test.press("F4")
Test.waitForCondition((frameText) =>
    frameText.includes("LIVE JOURNAL")
    && frameText.includes("STREAMING")
    && frameText.includes("FOLLOW: ON"), 3000)

// The deterministic mock backlog is replayed into the view.
Test.waitForText("telemetry batch published (42 points)")
Test.waitForText("ERR chronyd[201]: exiting: could not step system clock")
Test.waitForText("WRN chronyd[201]: no reachable NTP sources")
Test.captureText("cockpit/logs.txt")

// Follow toggling.
Test.press("F")
Test.waitForText("FOLLOW: OFF")
Test.press("F")
Test.waitForText("FOLLOW: ON")

// Clear empties the view; new lines keep arriving afterwards.
Test.press("C")
Test.expectNoText("telemetry batch published (42 points)")
Test.waitForCondition((frameText) =>
    frameText.includes("device-agent[312]")
    || frameText.includes("mosquitto[220]")
    || frameText.includes("watchdog-feeder[98]")
    || frameText.includes("kernel"), 5000)
Test.finish()
