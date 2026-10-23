// Gallery smoke scenario: the searchable GridView with component previews renders,
// inline header search filters tiles, Enter opens the full-width Example Panel by
// default, F2 toggles the editable Code Panel, editing code auto-reloads after the
// 500ms timeout, and Escape returns to the GridView.
function main(app) {
    app.waitForText("QmlTui Gallery");
    app.expectText("Button");
    app.expectText("[ Primary ]");
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
    app.expectNoText("[ Primary ]");
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

    // Enter opens the Button demo with the QML Code Panel hidden by default.
    app.press("ENTER");
    app.waitForText("No clicks yet");
    app.expectText("examples/Button.qml");
    app.expectNoText("(editable)");
    app.captureText("gallery/button.txt");

    // The first button has focus: Space activates it.
    app.press("SPACE");
    app.waitForText("Clicked 1 time");

    // F2 reveals the editable QML Code Panel; clicking the Code Panel focuses
    // the code editor; clicking the Example Panel moves focus back to the live
    // example rendering frame.
    app.press("F2");
    app.waitForText("(editable)");
    app.expectText("Auto-reload");
    app.clickText("(editable)");
    app.press("END");
    app.press("ENTER");
    app.typeText("    // Live edited comment");
    app.expectText("Reloading");
    app.sleep(600);
    app.waitForText("No clicks yet");
    app.expectText("Live edited comment");
    app.clickText("● Live");
    app.press("SPACE");
    app.waitForText("Clicked 1 time");
    app.captureText("gallery/button_live_edit.txt");

    // F2 hides the Code Panel again before returning to the grid.
    app.press("F2");
    app.expectNoText("(editable)");

    // Escape goes back to the grid.
    app.press("ESCAPE");
    app.waitForText("QmlTui Gallery");
    app.expectNoText("Clicked 1 time");

    // End jumps to the last tile in CLI and scrolls the grid to it.
    app.press("END");
    app.waitForText(at(total));
    app.expectText("TerminalCapabilities");
    app.captureText("gallery/grid_end.txt");

    // Topic TabBar: clicking or pressing Tab filters by topic (Controls, Layout,
    // Display, Views, Input, Animation, CLI) and Escape resets back to All.
    app.press("HOME");
    app.waitForText(at(1));
    app.press("TAB");
    app.waitForText("Components · Controls (14/");
    app.press("ESCAPE");
    app.waitForText(at(1));

    // Clicking a tile opens it directly.
    app.clickText("Slider");
    app.waitForText("Volume  42%");
    app.press("RIGHT");
    app.waitForText("Volume  43%");
    app.captureText("gallery/slider.txt");
    app.press("ESCAPE");
    app.waitForText("QmlTui Gallery");

    // Escape is contextual: it closes an open Dialog before it leaves the demo.
    app.clickText("Dialog");
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

    // Switching to the Input topic tab selects TextInput: Q typed into a focused
    // TextInput is text, not quit.
    app.clickText("Input");
    app.waitForText("Components · Input (4/");
    app.expectText("TextInput");
    app.press("ENTER");
    app.waitForText("Hello, stranger!");
    app.typeText("quinn");
    app.waitForText("Hello, quinn!");
    app.press("ESCAPE");
    app.waitForText("QmlTui Gallery");
    app.press("ESCAPE");
    app.waitForText(at(1));

    // The AnimatedSprite tile in the Display topic (only in a QmlTui.Image build)
    // plays a sprite sheet: Space pauses the frame clock, Right picks the next animation.
    app.clickText("Display");
    app.waitForText("Components · Display");
    app.press("END");
    if (app.frameText().includes("AnimatedSprite")) {
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
        app.sleep(200);   // let timers and animations paint at least once
        app.captureText("gallery/page_" + String(i).padStart(2, "0") + ".txt");
        app.press("ESCAPE");
        app.waitForText("QmlTui Gallery");
        app.press("RIGHT");
    }
    app.captureText("gallery/after_tour.txt");

    app.finish();
}
