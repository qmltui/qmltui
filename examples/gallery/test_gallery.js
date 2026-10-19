// Gallery smoke scenario: the searchable GridView with component previews renders,
// inline header search filters tiles, Enter opens the split view (Example Panel +
// Editable Code Panel), editing code auto-reloads after the 500ms timeout, and
// Escape returns to the GridView.
function main(terminal) {
    terminal.waitForText("QmlTui Gallery");
    terminal.expectText("Button");
    terminal.expectText("[ Primary ]");
    terminal.expectText("/ Search");
    // The tile count depends on the build (Behavior, QmlTui.Image, QmlTui.PtyView), so read it.
    terminal.waitForRegex("1 / [0-9]+");
    const total = parseInt(/1 \/ ([0-9]+)/.exec(terminal.frameText())[1], 10);
    const at = n => n + " / " + total;
    terminal.captureText("gallery/grid.txt");

    // Inline search in Panel titleTrailing: inactive until '/' or click activates it.
    terminal.press("/");
    terminal.typeText("Sparkline");
    terminal.waitForText("1 / 1");
    terminal.expectText("Sparkline");
    terminal.expectNoText("[ Primary ]");
    terminal.captureText("gallery/search.txt");
    terminal.press("ESCAPE");
    terminal.waitForText(at(1));
    terminal.expectText("/ Search");

    // Right moves to the second tile, Down to the next row.
    terminal.press("RIGHT");
    terminal.waitForText(at(2));
    terminal.press("DOWN");
    terminal.waitForRegex("[2-9] / " + total);
    terminal.press("HOME");
    terminal.waitForText(at(1));

    // Enter opens the Button demo alongside the editable QML Code Panel.
    terminal.press("ENTER");
    terminal.waitForText("No clicks yet");
    terminal.expectText("examples/Button.qml");
    terminal.expectText("(editable)");
    terminal.expectText("Auto-reload");
    terminal.captureText("gallery/button.txt");

    // The first button has focus: Space activates it.
    terminal.press("SPACE");
    terminal.waitForText("Clicked 1 time");

    // Clicking the Code Panel focuses the code editor; clicking the Example
    // Panel moves focus back to the live example rendering frame.
    terminal.clickText("(editable)");
    terminal.press("END");
    terminal.press("ENTER");
    terminal.typeText("    // Live edited comment");
    terminal.expectText("Reloading");
    terminal.sleep(600);
    terminal.waitForText("No clicks yet");
    terminal.expectText("Live edited comment");
    terminal.clickText("● Live");
    terminal.press("SPACE");
    terminal.waitForText("Clicked 1 time");
    terminal.captureText("gallery/button_live_edit.txt");

    // Escape goes back to the grid.
    terminal.press("ESCAPE");
    terminal.waitForText("QmlTui Gallery");
    terminal.expectNoText("Clicked 1 time");

    // End jumps to the last tile (InlineMode in CLI) and scrolls the grid to it.
    terminal.press("END");
    terminal.waitForText(at(total));
    terminal.expectText("InlineMode");
    terminal.captureText("gallery/grid_end.txt");

    // Topic TabBar: clicking or pressing Tab filters by topic (Controls, Layout,
    // Display, Views, Input, Animation, CLI) and Escape resets back to All.
    terminal.press("HOME");
    terminal.waitForText(at(1));
    terminal.press("TAB");
    terminal.waitForText("Components · Controls (14/");
    terminal.press("ESCAPE");
    terminal.waitForText(at(1));

    // Clicking a tile opens it directly.
    terminal.clickText("Slider");
    terminal.waitForText("Volume  42%");
    terminal.press("RIGHT");
    terminal.waitForText("Volume  43%");
    terminal.captureText("gallery/slider.txt");
    terminal.press("ESCAPE");
    terminal.waitForText("QmlTui Gallery");

    // Escape is contextual: it closes an open Dialog before it leaves the demo.
    terminal.clickText("Dialog");
    terminal.waitForText("No decision yet");
    terminal.press("ENTER");
    terminal.waitForText("report.pdf will be removed");
    terminal.press("ESCAPE");
    terminal.waitForText("Kept report.pdf");
    terminal.expectText("Esc back to grid");
    terminal.press("ENTER");
    terminal.waitForText("report.pdf will be removed");
    terminal.press("ENTER");   // the accept button has initial focus
    terminal.waitForText("Deleted report.pdf");
    terminal.press("ESCAPE");
    terminal.waitForText("QmlTui Gallery");

    // Switching to the Input topic tab selects TextInput: Q typed into a focused
    // TextInput is text, not quit.
    terminal.clickText("Input");
    terminal.waitForText("Components · Input (4/");
    terminal.expectText("TextInput");
    terminal.press("ENTER");
    terminal.waitForText("Hello, stranger!");
    terminal.typeText("quinn");
    terminal.waitForText("Hello, quinn!");
    terminal.press("ESCAPE");
    terminal.waitForText("QmlTui Gallery");
    terminal.press("ESCAPE");
    terminal.waitForText(at(1));

    // The AnimatedSprite tile in the Display topic (only in a QmlTui.Image build)
    // plays a sprite sheet: Space pauses the frame clock, Right picks the next animation.
    terminal.clickText("Display");
    terminal.waitForText("Components · Display");
    terminal.press("END");
    if (terminal.frameText().includes("AnimatedSprite")) {
        terminal.press("ENTER");
        terminal.waitForRegex("loading: frame [0-9]+/4");
        terminal.expectText("of 256x64 sheet");
        terminal.press("SPACE");
        terminal.waitForText("paused");
        terminal.press("RIGHT");
        terminal.waitForRegex("loading tail: frame [0-9]+/2");
        terminal.typeText("r");
        terminal.waitForText("reverse");
        terminal.captureText("gallery/animated_sprite.txt");
        terminal.press("ESCAPE");
        terminal.waitForText("QmlTui Gallery");
    }
    terminal.press("ESCAPE");
    terminal.waitForText(at(1));

    // Every page must instantiate without error: open each one in turn.
    for (let i = 0; i < total - 1; ++i) {
        terminal.press("ENTER");
        terminal.waitForText("back to grid");
        terminal.expectNoText("✗ Error");
        terminal.sleep(200);   // let timers and animations paint at least once
        terminal.captureText("gallery/page_" + String(i).padStart(2, "0") + ".txt");
        terminal.press("ESCAPE");
        terminal.waitForText("QmlTui Gallery");
        terminal.press("RIGHT");
    }
    terminal.captureText("gallery/after_tour.txt");

    // The last tile switches the whole app to Terminal.Inline; the prompt
    // prints into the scrollback and Escape restores full-screen mode.
    terminal.press("END");
    terminal.waitForText(at(total));
    terminal.press("ENTER");
    terminal.waitForText("Inline mode");
    terminal.expectNoText("QmlTui Gallery");
    terminal.typeText("hello from inline");
    terminal.press("ENTER");
    terminal.waitForText("printed: 1");
    terminal.captureText("gallery/inline.txt");
    terminal.press("ESCAPE");
    terminal.waitForText("QmlTui Gallery");
    terminal.captureText("gallery/after_inline.txt");

    // Full-screen mode is fully restored: another demo opens and closes.
    terminal.clickText("Animation");
    terminal.waitForText("Components · Animation");
    terminal.press("END");
    terminal.press("ENTER");
    terminal.waitForRegex("Easing.OutBounce|Behavior on x");
    terminal.press("ESCAPE");
    terminal.waitForText("QmlTui Gallery");

    terminal.finish();
}
