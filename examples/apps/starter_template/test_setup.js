// Boot to Qtui device setup smoke scenario.

// Boots into the network page with brand header and tab bar.
function main(terminal) {
    terminal.waitForCondition((frameText) =>
        frameText.includes("Boot to Qtui")
        && frameText.includes("[F1] Network")
        && frameText.includes("Wi-Fi Authentication")
        && frameText.includes("Remote Access")
        && frameText.includes("SSH:")
        && frameText.includes("Network Interfaces"), 3000)
    terminal.captureText("starter_template/network.txt")

    // The deploy command can be copied when an interface has an address.
    // (Glyph-agnostic: the marker before "Copied" depends on Theme.simpleGlyphs.)
    if (terminal.frameText().includes("[ Copy ]")) {
        terminal.clickText("[ Copy ]")
        terminal.waitForText("Copied")
    }

    // The password field takes focus on Enter and accepts input.
    terminal.press("Enter")
    terminal.typeText("hunter22")
    terminal.waitForText("********")

    // About page shows device, software, and network summaries.
    terminal.press("F2")
    terminal.waitForCondition((frameText) =>
        frameText.includes("Hostname:")
        && frameText.includes("UI Toolkit:")
        && frameText.includes("Wi-Fi:"), 3000)
    terminal.captureText("starter_template/about.txt")

    // Back to the network page.
    terminal.press("F1")
    terminal.waitForText("Wi-Fi Authentication")

    terminal.finish()
}
