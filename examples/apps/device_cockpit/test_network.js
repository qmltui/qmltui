// Network page scenario, run against the mock providers.

Test.waitForText("DEVICE COCKPIT")
Test.press("F2")
Test.waitForCondition((frameText) =>
    frameText.includes("INTERFACES")
    && frameText.includes("eth0")
    && frameText.includes("wlan0")
    && frameText.includes("192.168.7.2"), 3000)
Test.waitForCondition((frameText) =>
    frameText.includes("192.168.7.2/24")
    && frameText.includes("192.168.7.1")
    && frameText.includes("02:42:c0:a8:07:02"), 3000)

// Apply a static configuration through the edit dialog.
Test.press("E")
Test.waitForText("STATIC IPV4 — eth0")
Test.press("A", ["Ctrl"])
Test.typeText("10.0.0.5")
Test.press("Tab")
Test.press("A", ["Ctrl"])
Test.typeText("16")
Test.press("Tab")
Test.press("A", ["Ctrl"])
Test.typeText("10.0.0.1")
Test.press("Enter")
Test.waitForText("static configuration applied", 3000)
Test.waitForCondition((frameText) =>
    frameText.includes("10.0.0.5/16")
    && frameText.includes("static"), 3000)
Test.captureText("cockpit/network.txt")

// Cancelling the DHCP confirmation leaves the static config in place.
Test.press("D")
Test.waitForText("Request a DHCP lease on eth0?")
Test.press("N")
Test.waitForText("10.0.0.5/16")
Test.finish()
