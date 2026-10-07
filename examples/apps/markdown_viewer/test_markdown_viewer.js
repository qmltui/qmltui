// 1. Modern showcase verification on initial load
function main(terminal) {
    terminal.waitForText("Modern Style Showcase");
    terminal.expectText("Let’s talk about artichokes");
    terminal.expectText("He holds him with a skinny hand");

    // 2. Tab heading navigation into table and code sections
    terminal.press("Tab");
    terminal.expectText("Section: Let’s talk about artichokes");
    terminal.press("Tab");
    terminal.expectText("Section: Other foods worth mentioning");
    terminal.press("Tab");
    terminal.expectText("Section: Things to eat today");
    terminal.press("Tab");
    terminal.expectText("Section: Power levels of the aforementioned foods");
    terminal.expectText("9001");

    terminal.press("Tab");
    terminal.expectText("Section: Currying Artichokes");
    terminal.expectText("module Main where");
    terminal.expectText("intercalate");

    // 3. Line numbers toggle
    terminal.press("l");
    terminal.waitForText("Line numbers: ON");
    terminal.expectRegex("(^|\\n)\\s*\\d+\\s+│");
    terminal.press("l");
    terminal.waitForText("Line numbers: OFF");

    // 4. Style toggle (modern -> classic -> minimal -> custom -> modern)
    terminal.press("s");
    terminal.waitForText("Style: classic");
    terminal.press("s");
    terminal.waitForText("Style: minimal");
    terminal.press("s");
    terminal.waitForText("Style: custom");
    terminal.expectText("LIVE STYLE EDITOR");
    terminal.press("Down");
    terminal.press("Right");
    terminal.press("s");
    terminal.waitForText("Style: modern");

    // 5. Search query execution and theme-aware highlight
    terminal.press("/");
    terminal.typeText("   ");
    terminal.waitForText("No matches");
    terminal.press("Backspace");
    terminal.press("Backspace");
    terminal.press("Backspace");

    terminal.typeText("Haskell");
    terminal.waitForText("Match 1 of");
    let haskellMatches = terminal.findText("Haskell");
    if (haskellMatches.length === 0)
        terminal.fail("Search did not scroll the active Haskell match into view");
    const beforeTheme = terminal.cell(haskellMatches[0].x, haskellMatches[0].y).background;
    if (beforeTheme === "#00000000")
        terminal.fail("Active search match has no highlight background");
    terminal.press("Escape");

    // 6. Theme cycling
    terminal.press("t");
    terminal.waitForText("tokyo_night");
    terminal.press("t");
    terminal.waitForText("dracula");

    // 7. Sample document cycling and link hit-testing
    terminal.press("o");
    terminal.waitForText("Welcome to Qtui Markdown Viewer");
    terminal.clickText("md4qt");
    terminal.expectText("Activated Link: https://github.com/KDE/md4qt");

    terminal.captureText("markdown_viewer_final.txt");
}
