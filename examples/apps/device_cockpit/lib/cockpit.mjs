// Shared navigation for the device_cockpit scenarios: module scenarios
// (.mjs) import these helpers instead of repeating the shell handshake and
// page switching in every file.

/** @param {Terminal} terminal */
export function waitForShell(terminal) {
    terminal.waitForCondition((frameText) =>
        frameText.includes("DEVICE COCKPIT")
        && frameText.includes("cockpit-demo"), 3000);
}

/**
 * @param {Terminal} terminal
 * @param {string} key
 * @param {string} marker
 */
export function openPage(terminal, key, marker) {
    terminal.press(key);
    terminal.waitForText(marker, 3000);
}

/**
 * @param {Terminal} terminal
 * @param {string} prompt
 * @param {string} key
 */
export function confirmDialog(terminal, prompt, key) {
    terminal.waitForText(prompt);
    terminal.press(key);
}
