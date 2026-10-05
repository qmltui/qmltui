// test_preview.js - Tests preview column for directory, text, and binary items

function run() {
    Test.waitForText("PREVIEW", 2000);

    // Initial focus on assets folder -> Directory Preview
    Test.expectText("assets");
    Test.expectText("logo.png");

    // Move to docs folder
    Test.press("j");
    Test.expectText("docs");
    Test.expectText("architecture.md");

    // Move down to README.md text file (last item in list)
    Test.press("End");
    Test.waitForText("README.md", 2000);

    // Verify text preview has line numbers and markdown content
    Test.expectText("1 │");
    Test.expectText("Yazi-Style");

    // Navigate to assets to test binary and image preview
    Test.press("Home"); // Top: assets
    Test.press("l"); // Enter assets
    Test.waitForText("archive.tar.gz", 2000);

    // 1. Binary Archive Preview
    Test.expectText("Binary / Archive");
    Test.expectText("application/gzip");

    // 2. Image Asset Preview (move down to banner.jpg or logo.png)
    Test.press("j");
    Test.waitForText("banner.jpg", 2000);
    Test.expectText("Image Asset");
    Test.expectText("image/jpeg");

    Test.captureText("preview.txt");
}

run();
