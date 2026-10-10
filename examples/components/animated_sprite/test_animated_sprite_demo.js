function main(terminal) {
    terminal.waitForText("AnimatedSprite");
    terminal.waitForText("Animation: idle");
    // No kitty or sixel under the headless backend: the sprite falls back.
    terminal.waitForText("half-block fallback");
    terminal.captureText("animated_sprite/idle.txt");

    // Switch the animation row and pause the clock.
    terminal.press("2");
    terminal.waitForText("Animation: wave");
    terminal.press("space");
    terminal.waitForText("paused");
    terminal.captureText("animated_sprite/wave_paused.txt");

    // Reverse plays the frames last to first.
    terminal.press("r");
    terminal.waitForText("reverse");

    terminal.press("4");
    terminal.waitForText("Animation: sleep");
    terminal.waitForText("/3");          // sleep row has three frames
    terminal.captureText("animated_sprite/sleep.txt");

    terminal.finish();
}
