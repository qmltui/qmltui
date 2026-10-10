// Demo-mode smoke scenario (120x36). The Runner-linked executable selects the
// procedural demo source when no media is given (the production binary starts idle),
// so this runs identically with and without Qt Multimedia. The microphone key is deliberately never pressed: whether
// an input device exists differs between CI and developer machines.
function main(terminal) {
    terminal.waitForText("NEURAL AUDIO MATRIX");
    terminal.sleep(300);                      // let the demo synth build up some levels
    terminal.expectText("DEMO");
    terminal.expectText("SYNTHWAVE RUNNER");
    terminal.expectText("24-BAND NEON EQUALIZER");
    terminal.expectText("BRAILLE OSCILLOSCOPE · LP off");
    terminal.expectText("STEREO VU");
    terminal.expectText("PLAYING");
    terminal.expectNoText("TRACK");           // no playlist in demo mode
    terminal.expectNoText("MODE");            // layout modes were replaced by widget toggles
    terminal.expectText("EQ");
    terminal.expectText("Scope");
    terminal.expectText("VU");
    terminal.expectNoText("Demo");            // the demo has no key of its own
    terminal.expectText("Quit");
    terminal.captureText("cyber_audio/initial.txt");

    // 1 hides the equalizer: the lower panel grows into the freed space
    terminal.typeText("1");
    terminal.expectNoText("24-BAND NEON EQUALIZER");
    terminal.expectText("BRAILLE OSCILLOSCOPE");
    terminal.captureText("cyber_audio/equalizer_hidden.txt");
    terminal.typeText("1");
    terminal.expectText("24-BAND NEON EQUALIZER");

    // 2 hides the oscilloscope: the panel keeps the VU meters and retitles itself
    terminal.typeText("2");
    terminal.expectNoText("BRAILLE OSCILLOSCOPE");
    terminal.expectText("STEREO VU METERS");
    terminal.typeText("2");
    terminal.expectText("BRAILLE OSCILLOSCOPE");

    // 3 hides the VU deck, leaving the oscilloscope alone in its panel
    terminal.typeText("3");
    terminal.expectNoText("STEREO VU");
    terminal.expectText("BRAILLE OSCILLOSCOPE");
    terminal.captureText("cyber_audio/vu_hidden.txt");
    terminal.typeText("3");
    terminal.expectText("STEREO VU");

    // Everything off shows the hint; everything back on restores both panels
    terminal.typeText("1");
    terminal.typeText("2");
    terminal.typeText("3");
    terminal.expectText("All panels hidden");
    terminal.typeText("1");
    terminal.typeText("2");
    terminal.typeText("3");
    terminal.expectNoText("All panels hidden");
    terminal.expectText("24-BAND NEON EQUALIZER");
    terminal.expectText("BRAILLE OSCILLOSCOPE");

    // File-only controls are inert without a playlist
    terminal.press("Left");
    terminal.typeText("n");
    terminal.typeText("+");
    terminal.expectText("DEMO");

    // Transport toggle
    terminal.press("Space");
    terminal.expectText("PAUSED");
    terminal.expectText("Play");
    terminal.press("Space");
    terminal.expectText("PLAYING");


    // Scope low-pass presets: [ lowers the cutoff, ] raises it back to off
    terminal.typeText("[");
    terminal.expectText("LP 8 kHz");
    terminal.typeText("[");
    terminal.expectText("LP 4 kHz");
    terminal.typeText("]");
    terminal.typeText("]");
    terminal.expectText("LP off");

    // V cycles the oscilloscope art: trace -> glow -> ridge -> nebula -> trace
    terminal.typeText("v");
    terminal.expectText("PHOSPHOR SCOPE");
    terminal.typeText("v");
    terminal.sleep(600);                       // wall time: the C++ synth timer must fill the ridge history
    terminal.expectText("RIDGE PLOT");
    terminal.captureText("cyber_audio/ridge.txt");
    terminal.typeText("v");
    terminal.sleep(300);
    terminal.expectText("NEON NEBULA");
    terminal.expectRegex("[⠁-⣿]");
    terminal.captureText("cyber_audio/nebula_panel.txt");
    terminal.typeText("v");
    terminal.expectText("BRAILLE OSCILLOSCOPE");

    // Blend toggle for shared oscilloscope cells
    terminal.typeText("a");
    terminal.expectText("Blend");
    terminal.typeText("a");

    // Immersive art preserves the dashboard toggles and freezes on pause.
    terminal.typeText("f");
    terminal.sleep(450);                       // wall time: the C++ synth timer must draw immersive art
    terminal.expectText("NEON NEBULA");
    terminal.expectNoText("24-BAND NEON EQUALIZER");
    terminal.expectNoText("STEREO VU");
    terminal.expectRegex("[⠁-⣿]");
    const artRows = () => terminal.frameText().split("\n").filter(row => /[\u2801-\u28ff]/.test(row)).join("\n");
    const moving = artRows();
    terminal.sleep(300);
    if (artRows() === moving)
        terminal.fail("Nebula did not animate during playback");
    terminal.press("Space");
    terminal.sleep(150);
    const paused = artRows();
    terminal.sleep(200);
    if (!paused || artRows() !== paused)
        terminal.fail("Nebula did not freeze on pause");
    terminal.captureText("cyber_audio/nebula.txt");
    terminal.captureStyled("cyber_audio/nebula_styled.txt");
    terminal.press("Space");
    terminal.sleep(150);
    if (artRows() === paused)
        terminal.fail("Nebula did not resume");
    terminal.press("Escape");
    terminal.expectText("24-BAND NEON EQUALIZER");
    terminal.expectText("BRAILLE OSCILLOSCOPE");
    terminal.expectText("STEREO VU");
    terminal.captureText("cyber_audio/final.txt");
}
