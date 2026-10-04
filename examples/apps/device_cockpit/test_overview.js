// Overview smoke scenario, run against the mock providers
// (main.cpp defaults scenario runs to QTUI_COCKPIT_MOCK=1).

// Shell and overview render with the mock identity.
Test.waitForCondition((frameText) =>
    frameText.includes("DEVICE COCKPIT")
    && frameText.includes("cockpit-demo")
    && frameText.includes("MOCK"), 3000)

// Overview panels show the deterministic mock inventory.
Test.waitForCondition((frameText) =>
    frameText.includes("SYSTEM")
    && frameText.includes("CPU")
    && frameText.includes("MEMORY")
    && frameText.includes("STORAGE"), 3000)
Test.waitForText("6.6.0-mock")
Test.waitForText("eth0")
Test.waitForText("192.168.7.2")
Test.waitForText("/data")
Test.captureText("cockpit/overview.txt")

// Page switching reaches every page and returns.
Test.press("F2")
Test.waitForText("INTERFACES")
Test.press("F3")
Test.waitForText("mosquitto.service")
Test.press("F4")
Test.waitForText("LIVE JOURNAL")
Test.press("1")
Test.waitForText("STORAGE")
Test.finish()
