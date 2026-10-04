// 1. Modern showcase verification on initial load
Test.waitForText("Modern Style Showcase");
Test.expectText("Let’s talk about artichokes");
Test.expectText("He holds him with a skinny hand");

// 2. Tab heading navigation into table and code sections
Test.press("Tab");
Test.expectText("Section: Let’s talk about artichokes");
Test.press("Tab");
Test.expectText("Section: Other foods worth mentioning");
Test.press("Tab");
Test.expectText("Section: Things to eat today");
Test.press("Tab");
Test.expectText("Section: Power levels of the aforementioned foods");
Test.expectText("9001");

Test.press("Tab");
Test.expectText("Section: Currying Artichokes");
Test.expectText("module Main where");
Test.expectText("intercalate");

// 3. Line numbers toggle
Test.press("l");
Test.waitForText("Line numbers: ON");
Test.expectRegex("(^|\\n)\\s*\\d+\\s+│");
Test.press("l");
Test.waitForText("Line numbers: OFF");

// 4. Style toggle (modern -> classic -> minimal -> custom -> modern)
Test.press("s");
Test.waitForText("Style: classic");
Test.press("s");
Test.waitForText("Style: minimal");
Test.press("s");
Test.waitForText("Style: custom");
Test.expectText("LIVE STYLE EDITOR");
Test.press("Down");
Test.press("Right");
Test.press("s");
Test.waitForText("Style: modern");

// 5. Search query execution and theme-aware highlight
Test.press("/");
Test.typeText("   ");
Test.waitForText("No matches");
Test.press("Backspace");
Test.press("Backspace");
Test.press("Backspace");

Test.typeText("Haskell");
Test.waitForText("Match 1 of");
let haskellMatches = Test.findText("Haskell");
if (haskellMatches.length === 0)
    Test.fail("Search did not scroll the active Haskell match into view");
const beforeTheme = Test.cell(haskellMatches[0].x, haskellMatches[0].y).background;
if (beforeTheme === "#00000000")
    Test.fail("Active search match has no highlight background");
Test.press("Escape");

// 6. Theme cycling
Test.press("t");
Test.waitForText("tokyo_night");
Test.press("t");
Test.waitForText("dracula");

// 7. Sample document cycling and link hit-testing
Test.press("o");
Test.waitForText("Welcome to Qtui Markdown Viewer");
Test.clickText("md4qt");
Test.expectText("Activated Link: https://github.com/KDE/md4qt");

Test.captureText("markdown_viewer_final.txt");
