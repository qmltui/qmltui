function main(terminal) {
    terminal.waitForText("NEURAL AUDIO MATRIX");
    terminal.expectText("24-BAND NEON EQUALIZER");
    terminal.expectText("STEREO PEAK VU METERS");
    terminal.captureText("cyber_audio/initial.txt");

    // Switch to Spectrum focus mode (triggers State transition with OutBounce)
    terminal.typeText("2");
    terminal.sleep(450);
    terminal.captureText("cyber_audio/spectrum_focus.txt");

    // Switch to Scope focus mode (Lissajous 3D phase scope)
    terminal.typeText("3");
    terminal.sleep(450);
    terminal.captureText("cyber_audio/scope_focus.txt");

    // Toggle Bass Boost
    terminal.typeText("b");
    terminal.sleep(200);

    // Toggle Glitch overdrive mode
    terminal.typeText("g");
    terminal.sleep(200);

    // Return to default Split mode
    terminal.typeText("1");
    terminal.sleep(450);
    terminal.captureText("cyber_audio/final.txt");

    terminal.finish();
}
