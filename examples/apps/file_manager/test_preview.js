// test_preview.js - Tests preview column for directory, text, and binary items

function main(terminal) {
    terminal.waitForText("PREVIEW", 2000);

    // Initial focus on assets folder -> Directory Preview
    terminal.expectText("assets");
    terminal.expectText("logo.png");

    // Move to docs folder
    terminal.press("j");
    terminal.expectText("docs");
    terminal.expectText("architecture.md");

    // Move down to README.md text file (last item in list)
    terminal.press("End");
    terminal.waitForText("README.md", 2000);

    // Verify text preview has line numbers and markdown content
    terminal.expectText("1 │");
    terminal.expectText("Yazi-Style");

    // Navigate to assets to test binary and image preview
    terminal.press("Home"); // Top: assets
    terminal.press("l"); // Enter assets
    terminal.waitForText("archive.tar.gz", 2000);

    // 1. Binary Archive Preview
    terminal.expectText("Binary / Archive");
    terminal.expectText("application/gzip");

    // 2. Image Asset Preview (move down to banner.jpg or logo.png)
    terminal.press("j");
    terminal.waitForText("banner.jpg", 2000);
    terminal.expectText("Image Asset");
    terminal.expectText("image/jpeg");

    terminal.captureText("preview.txt");
}
