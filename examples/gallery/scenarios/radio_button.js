/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("Build Profile Selector");
    terminal.expectText("profile: Debug");
    terminal.expectText("flags: -O0 -g");
    terminal.expectText("size: 14.8 MB");
    terminal.expectText("time: 1.2s");

    // Tab to Release (-O2) and select with Space
    terminal.press("TAB");
    terminal.press("SPACE");
    terminal.waitForText("profile: Release");
    terminal.expectText("flags: -O2");
    terminal.expectText("size: 4.2 MB");
    terminal.expectText("time: 3.8s");

    // Click Ultra (-O3 -flto)
    terminal.clickText("Ultra (-O3 -flto)");
    terminal.waitForText("profile: Ultra");
    terminal.expectText("flags: -O3 -flto");
    terminal.expectText("size: 5.1 MB");
    terminal.expectText("time: 9.4s");

    // Toggle strip (-s) in Inspector
    terminal.clickText("strip (-s)");
    terminal.waitForText("flags: -O3 -flto -s");
    terminal.expectText("size: 4.1 MB");

    // Click Cycle Profile button in Inspector to wrap back to Debug
    terminal.clickText("Cycle Profile");
    terminal.waitForText("profile: Debug");
    terminal.expectText("flags: -O0 -g -s");
}
