// The cockpit shell must survive a runtime resize: content reflows into the
// narrow viewport and back without losing the header, footer, or pages.
import { waitForShell, openPage } from "./lib/cockpit.mjs";

/** @param {Terminal} terminal */
export function main(terminal) {
    waitForShell(terminal)
    terminal.waitForText("STORAGE")

    const wide = terminal.size();
    if (wide.columns !== 100 || wide.rows !== 30)
        terminal.fail("Expected the configured 100x30 viewport");
    terminal.captureText("cockpit/resize_wide.txt")

    // Narrow: the shell chrome and overview panels must still be reachable.
    terminal.resize(60, 20)
    terminal.expectText("DEVICE COCKPIT")
    terminal.expectText("SYSTEM")
    terminal.captureText("cockpit/resize_narrow.txt")

    // Page switching still works at the narrow size. The shortened table
    // shows its first units; mosquitto.service is below the visible rows.
    terminal.press("F3")
    terminal.waitForText("device-agent.service", 3000)
    // Back on the overview the narrow viewport fits the first panels only;
    // STORAGE returns with the wide layout below.
    terminal.press("1")
    terminal.waitForText("SYSTEM", 3000)

    // And back to the default: the wide layout is restored in full.
    terminal.resize(100, 30)
    terminal.expectText("DEVICE COCKPIT")
    terminal.waitForText("STORAGE", 3000)
    terminal.waitForText("192.168.7.2", 3000)
    terminal.finish()
}
