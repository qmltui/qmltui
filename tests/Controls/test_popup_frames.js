/** @param {Terminal} terminal */
function main(terminal) {
    // 0. Initial frame
    terminal.expectStyled("snapshots/popup_frames_initial.txt");

    // 1. Transparency: open transparent popup
    terminal.press("1");
    terminal.expectStyled("snapshots/popup_frames_transparent.txt");

    // Restoration after closing transparent popup via Escape
    terminal.press("Escape");
    terminal.expectStyled("snapshots/popup_frames_initial.txt");

    // 2. Occlusion: open opaque popup with composed Rectangle
    terminal.press("2");
    terminal.expectStyled("snapshots/popup_frames_opaque.txt");

    // Resize opaque popup
    terminal.press("R");
    terminal.expectStyled("snapshots/popup_frames_resized.txt");

    // Restoration after closing resized opaque popup
    terminal.press("Escape");
    terminal.expectStyled("snapshots/popup_frames_initial.txt");

    // 3. Nested overlays: open parent, then open child overlay
    terminal.press("3");
    terminal.expectStyled("snapshots/popup_frames_nested_parent.txt");
    terminal.press("C");
    terminal.expectStyled("snapshots/popup_frames_nested_child.txt");

    // Escape closes child first, leaving parent open
    terminal.press("Escape");
    terminal.expectStyled("snapshots/popup_frames_nested_parent.txt");

    // Escape closes parent, returning to initial frame
    terminal.press("Escape");
    terminal.expectStyled("snapshots/popup_frames_initial.txt");

    // 4. Default Dialog surface (composed background rectangle, double border, terminal default colors)
    terminal.press("4");
    terminal.expectStyled("snapshots/popup_frames_dialog_default.txt");
    terminal.press("Escape");
    terminal.expectStyled("snapshots/popup_frames_initial.txt");

    // 5. Customized Dialog surface (custom background.color, single green border)
    terminal.press("5");
    terminal.expectStyled("snapshots/popup_frames_dialog_custom.txt");
    terminal.press("Escape");
    terminal.expectStyled("snapshots/popup_frames_initial.txt");
}
