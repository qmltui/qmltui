// Overview smoke scenario, run against the mock providers
// (main.cpp defaults scenario runs to QTUI_COCKPIT_MOCK=1).

// Shell and overview render with the mock identity.
import { waitForShell, openPage } from "./lib/cockpit.mjs";

/** @param {Terminal} terminal */
export function main(terminal) {
    waitForShell(terminal)
    terminal.waitForText("MOCK")

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
    openPage(terminal, "F2", "INTERFACES")
    openPage(terminal, "F3", "mosquitto.service")
    openPage(terminal, "F4", "LIVE JOURNAL")
    openPage(terminal, "1", "STORAGE")
    terminal.finish()
}
