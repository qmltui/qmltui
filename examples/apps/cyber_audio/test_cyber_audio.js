function main(terminal) {
    terminal.waitForText("NEURAL AUDIO MATRIX");
    terminal.expectText("24-BAND NEON EQUALIZER");
    terminal.expectText("BRAILLE OSCILLOSCOPE");
    terminal.expectText("STEREO VU");
    terminal.expectText("MODE SPLIT");
    // Footer key map is visible and lists every shortcut
    terminal.expectText("Spectrum");
    terminal.expectText("Glitch");
    terminal.expectText("Quit");
    terminal.captureText("cyber_audio/initial.txt");

    // Spectrum focus: equalizer takes most of the height (OutBounce transition)
    terminal.typeText("2");
    terminal.sleep(450);
    terminal.expectText("MODE SPECTRUM");
    terminal.captureText("cyber_audio/spectrum_focus.txt");

    // Scope focus: Lissajous phase scope
    terminal.typeText("3");
    terminal.sleep(450);
    terminal.expectText("MODE SCOPE");
    terminal.expectText("LISSAJOUS PHASE SCOPE");
    terminal.captureText("cyber_audio/scope_focus.txt");

    // Effects are independent of the layout and show as header badges
    terminal.typeText("b");
    terminal.sleep(100);
    terminal.expectText("BASS+");
    terminal.typeText("g");
    terminal.sleep(100);
    terminal.expectText("GLITCH");
    terminal.expectText("MODE SCOPE");

    // Transport toggle
    terminal.typeText(" ");
    terminal.sleep(100);
    terminal.expectText("PAUSED");
    terminal.expectText("Play");
    terminal.typeText(" ");
    terminal.sleep(100);
    terminal.expectText("PLAYING");

    // Back to the default split layout
    terminal.typeText("1");
    terminal.sleep(450);
    terminal.expectText("MODE SPLIT");
    terminal.captureText("cyber_audio/final.txt");

    terminal.finish();
}
