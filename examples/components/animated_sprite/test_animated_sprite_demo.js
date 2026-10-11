function main(terminal) {
    terminal.waitForText("AnimatedSprite");
    terminal.waitForText("Animation: idle");
    terminal.waitForText("/6");          // idle row has six frames
    // No kitty or sixel under the headless backend: the sprite falls back.
    terminal.waitForText("half-block fallback");
    terminal.captureText("animated_sprite/idle.txt");

    // Switch the animation row and pause the clock.
    terminal.press("4");
    terminal.waitForText("Animation: wave");
    terminal.waitForText("/4");
    terminal.press("space");
    terminal.waitForText("paused");
    terminal.captureText("animated_sprite/wave_paused.txt");

    // Reverse plays the frames last to first.
    terminal.press("r");
    terminal.waitForText("reverse");

    // Row 5 holds two runs: sleep starts at column 0, faint at column 5.
    terminal.press("6");
    terminal.waitForText("Animation: sleep");
    terminal.waitForText("/5");
    terminal.captureText("animated_sprite/sleep.txt");
    terminal.press("7");
    terminal.waitForText("Animation: faint");
    terminal.waitForText("/3");

    // 0 selects the tenth animation; Right wraps back to the first.
    terminal.press("0");
    terminal.waitForText("Animation: cube");
    terminal.press("Right");
    terminal.waitForText("Animation: idle");
    terminal.press("Left");
    terminal.waitForText("Animation: cube");

    terminal.finish();
}
