// Boot to Qtui device setup smoke scenario.

// Boots into the network page with brand header and tab bar.
Test.waitForCondition((frameText) =>
    frameText.includes("Boot to Qtui")
    && frameText.includes("[F1] Network")
    && frameText.includes("Wi-Fi Authentication")
    && frameText.includes("Remote Access")
    && frameText.includes("SSH:")
    && frameText.includes("Network Interfaces"), 3000)
Test.captureText("starter_template/network.txt")

// The deploy command can be copied when an interface has an address.
// (Glyph-agnostic: the marker before "Copied" depends on Theme.simpleGlyphs.)
if (Test.frameText().includes("[ Copy ]")) {
    Test.clickText("[ Copy ]")
    Test.waitForText("Copied")
}

// The password field takes focus on Enter and accepts input.
Test.press("Enter")
Test.typeText("hunter22")
Test.waitForText("********")

// About page shows device, software, and network summaries.
Test.press("F2")
Test.waitForCondition((frameText) =>
    frameText.includes("Hostname:")
    && frameText.includes("UI Toolkit:")
    && frameText.includes("Wi-Fi:"), 3000)
Test.captureText("starter_template/about.txt")

// Back to the network page.
Test.press("F1")
Test.waitForText("Wi-Fi Authentication")

Test.finish()
