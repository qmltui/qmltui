// Gallery smoke scenario: the searchable GridView with component previews renders,
// inline header search filters tiles, Enter opens the full-width Example stage,
// and Escape returns to the GridView.
function main(app) {
    app.waitForText("QmlTui Gallery");
    app.expectText("Text");
    app.expectText("Rectangle");
    app.expectText("/ Search");
    // The tile count depends on the build (Behavior, QmlTui.Image, QmlTui.PtyView), so read it.
    app.waitForRegex("1 / [0-9]+");
    const total = parseInt(/1 \/ ([0-9]+)/.exec(app.frameText())[1], 10);
    const at = n => n + " / " + total;
    app.captureText("gallery/grid.txt");

    // Inline search in Panel titleTrailing: inactive until '/' or click activates it.
    app.press("/");
    app.typeText("Sparkline");
    app.waitForText("1 / 1");
    app.expectText("Sparkline");
    app.expectNoText("Rectangle");
    app.captureText("gallery/search.txt");
    app.press("ESCAPE");
    app.waitForText(at(1));
    app.expectText("/ Search");

    // Right moves to the second tile, Down to the next row.
    app.press("RIGHT");
    app.waitForText(at(2));
    app.press("DOWN");
    app.waitForRegex("[2-9] / " + total);
    app.press("HOME");
    app.waitForText(at(1));

    // Filter to Button and press Enter to open the Button demo.
    app.press("/");
    app.typeText("Button");
    app.waitForText("Button");
    app.press("ENTER");
    app.waitForText("No clicks yet");
    app.expectText("examples/Button.qml");
    app.captureText("gallery/button.txt");

    // The first button has focus: Space activates it.
    app.press("SPACE");
    app.waitForText("Clicked 1 time");

    // Escape goes back to the grid, and a second Escape clears the search filter.
    app.press("ESCAPE");
    app.waitForText("QmlTui Gallery");
    app.expectNoText("Clicked 1 time");
    app.press("ESCAPE");
    app.waitForText(at(1));

    // Clicking TextInput on the top row opens it: Q typed into a focused TextInput
    // is text, not quit.
    app.clickText("TextInput");
    app.waitForText("Hello, stranger!");
    app.typeText("quinn");
    app.waitForText("Hello, quinn!");
    app.press("ESCAPE");
    app.waitForText("QmlTui Gallery");

    // Search for Slider and open it.
    app.press("/");
    app.typeText("Slider");
    app.waitForText("1 / 1");
    app.press("ENTER");
    app.waitForText("Volume  42%");
    app.press("RIGHT");
    app.waitForText("Volume  43%");
    app.captureText("gallery/slider.txt");
    app.press("ESCAPE");
    app.waitForText("QmlTui Gallery");
    app.press("ESCAPE");
    app.waitForText(at(1));

    // Escape is contextual: it closes an open Dialog before it leaves the demo.
    app.press("/");
    app.typeText("Dialog");
    app.waitForText("1 / 1");
    app.press("ENTER");
    app.waitForText("No decision yet");
    app.press("ENTER");
    app.waitForText("report.pdf will be removed");
    app.press("ESCAPE");
    app.waitForText("Kept report.pdf");
    app.expectText("Esc back to grid");
    app.press("ENTER");
    app.waitForText("report.pdf will be removed");
    app.press("ENTER");   // the accept button has initial focus
    app.waitForText("Deleted report.pdf");
    app.press("ESCAPE");
    app.waitForText("QmlTui Gallery");
    app.press("ESCAPE");
    app.waitForText(at(1));

    // End jumps to the last tile at the bottom of the grid.
    app.press("END");
    app.waitForText(at(total));
    app.captureText("gallery/grid_end.txt");
    app.press("HOME");
    app.waitForText(at(1));

    // The Image tile defaults to the embedded bike-picsum.jpg source and lets
    // the user enter a custom URL or path in its source TextInput.
    app.press("/");
    app.typeText("Image");
    if (!app.frameText().includes("0 / 0")) {
        app.press("ENTER");
        app.waitForText("qrc:/gallery/assets/bike-picsum.jpg");
        app.expectText("Source 4912x3264 px");
        app.press("TAB");
        app.press("a", ["Ctrl"]);
        app.press("BACKSPACE");
        app.typeText("qrc:/gallery/assets/loading.png");
        app.press("ENTER");
        app.waitForText("Source 256x64 px");
        app.press("TAB");
        app.press("a", ["Ctrl"]);
        app.press("BACKSPACE");
        app.press("ENTER");
        app.waitForText("qrc:/gallery/assets/bike-picsum.jpg");
        app.expectText("Source 4912x3264 px");
        app.press("ESCAPE");
        app.waitForText("QmlTui Gallery");
    }
    app.press("ESCAPE");
    app.waitForText(at(1));

    // The AnimatedSprite tile (in a QmlTui.Image build) plays a sprite sheet:
    // Space pauses the frame clock, Right picks the next animation.
    app.press("/");
    app.typeText("AnimatedSprite");
    if (!app.frameText().includes("0 / 0")) {
        app.press("ENTER");
        app.waitForRegex("loading: frame [0-9]+/4");
        app.expectText("of 256x64 sheet");
        app.press("SPACE");
        app.waitForText("paused");
        app.press("RIGHT");
        app.waitForRegex("loading tail: frame [0-9]+/2");
        app.typeText("r");
        app.waitForText("reverse");
        app.captureText("gallery/animated_sprite.txt");
        app.press("ESCAPE");
        app.waitForText("QmlTui Gallery");
    }
    app.press("ESCAPE");
    app.waitForText(at(1));

    // Every page must instantiate without error: open each one in turn.
    for (let i = 0; i < total; ++i) {
        app.press("ENTER");
        app.waitForText("back to grid");
        app.expectNoText("✗ Error");
        app.expectNoText("failed to create object");
        if (app.rootObject().codeError !== "")
            app.fail(app.rootObject().codeError);
        app.sleep(200);   // let timers and animations paint at least once
        app.captureText("gallery/page_" + String(i).padStart(2, "0") + ".txt");
        app.press("ESCAPE");
        app.waitForText("QmlTui Gallery");
        app.press("RIGHT");
    }
    app.captureText("gallery/after_tour.txt");

    app.finish();
}
