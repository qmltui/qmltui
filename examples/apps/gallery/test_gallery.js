// Gallery smoke scenario: the grid renders, the keyboard moves the selection,
// Enter opens a demo, Escape returns, and a mouse click opens a tile.
function main(terminal) {
    terminal.waitForText("Qtui Gallery");
    terminal.expectText("Button");
    // The tile count depends on the build (Behavior, Qtui.Image), so read it.
    terminal.waitForRegex("1 / [0-9]+");
    const total = parseInt(/1 \/ ([0-9]+)/.exec(terminal.frameText())[1], 10);
    const at = n => n + " / " + total;
    terminal.captureText("gallery/grid.txt");

    // Right moves to the second tile, Down to the next row.
    terminal.press("RIGHT");
    terminal.waitForText(at(2));
    terminal.press("DOWN");
    terminal.waitForRegex("[2-9] / " + total);
    terminal.press("HOME");
    terminal.waitForText(at(1));

    // Enter opens the Button demo with its snippet panel.
    terminal.press("ENTER");
    terminal.waitForText("No clicks yet");
    terminal.expectText("Esc");
    terminal.captureText("gallery/button.txt");

    // The first button has focus: Space activates it.
    terminal.press("SPACE");
    terminal.waitForText("Clicked 1 time");

    // Escape goes back to the grid.
    terminal.press("ESCAPE");
    terminal.waitForText("Qtui Gallery");
    terminal.expectNoText("No clicks yet");

    // End jumps to the last tile and scrolls the grid to it.
    terminal.press("END");
    terminal.waitForText(at(total));
    terminal.expectText("Inline mode");
    terminal.captureText("gallery/grid_end.txt");
    terminal.press("LEFT");
    terminal.waitForText(at(total - 1));
    terminal.press("ENTER");
    terminal.waitForRegex("Easing.OutBounce|Behavior on x");
    terminal.press("ESCAPE");
    terminal.waitForText("Qtui Gallery");

    // Clicking a tile opens it directly.
    terminal.press("HOME");
    terminal.waitForText(at(1));
    terminal.clickText("Slider");
    terminal.waitForText("Volume  42%");
    terminal.press("RIGHT");
    terminal.waitForText("Volume  43%");
    terminal.captureText("gallery/slider.txt");
    terminal.press("ESCAPE");
    terminal.waitForText("Qtui Gallery");

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
    terminal.waitForText("Qtui Gallery");

    // Q typed into a focused TextInput is text, not quit. The tile sits on a
    // lower row, so scroll the grid to the end before clicking it.
    terminal.press("HOME");
    for (let i = 0; i < 19; ++i)
        terminal.press("RIGHT");
    terminal.waitForText(at(20));
    terminal.expectText("TextInput");
    terminal.press("ENTER");
    terminal.waitForText("Hello, stranger!");
    terminal.typeText("quinn");
    terminal.waitForText("Hello, quinn!");
    terminal.press("ESCAPE");
    terminal.waitForText("Qtui Gallery");

    // The AnimatedSprite tile (only in a Qtui.Image build) plays a sprite
    // sheet: Space pauses the frame clock, Right picks the next animation.
    terminal.press("HOME");
    for (let i = 0; i < 25; ++i)
        terminal.press("RIGHT");
    terminal.waitForText(at(26));
    if (terminal.frameText().includes("Frames out of one sheet")) {
        terminal.press("ENTER");
        terminal.waitForRegex("idle: frame [0-9]+/6");
        terminal.expectText("of a 840x1024 sheet");
        terminal.press("SPACE");
        terminal.waitForText("paused");
        terminal.press("RIGHT");
        terminal.waitForRegex("walk right: frame [0-9]+/8");
        terminal.typeText("r");
        terminal.waitForText("reverse");
        terminal.captureText("gallery/animated_sprite.txt");
        terminal.press("ESCAPE");
        terminal.waitForText("Qtui Gallery");
    }

    // Every page must instantiate without error: open each one in turn.
    terminal.press("HOME");
    terminal.waitForText(at(1));
    for (let i = 0; i < total - 1; ++i) {
        terminal.press("ENTER");
        terminal.waitForText("back to grid");
        terminal.sleep(200);   // let timers and animations paint at least once
        terminal.captureText("gallery/page_" + String(i).padStart(2, "0") + ".txt");
        terminal.press("ESCAPE");
        terminal.waitForText("Qtui Gallery");
        terminal.press("RIGHT");
    }
    terminal.captureText("gallery/after_tour.txt");

    // The last tile switches the whole app to Terminal.Inline; the prompt
    // prints into the scrollback and Escape restores full-screen mode.
    terminal.press("END");
    terminal.waitForText(at(total));
    terminal.press("ENTER");
    terminal.waitForText("Inline mode");
    terminal.expectNoText("Qtui Gallery");
    terminal.typeText("hello from inline");
    terminal.press("ENTER");
    terminal.waitForText("printed: 1");
    terminal.captureText("gallery/inline.txt");
    terminal.press("ESCAPE");
    terminal.waitForText("Qtui Gallery");
    terminal.captureText("gallery/after_inline.txt");

    // Full-screen mode is fully restored: another demo opens and closes.
    terminal.press("LEFT");
    terminal.waitForText(at(total - 1));
    terminal.press("ENTER");
    terminal.waitForRegex("Easing.OutBounce|Behavior on x");
    terminal.press("ESCAPE");
    terminal.waitForText("Qtui Gallery");

    terminal.finish();
}
