/** @param {Runner} app */
function main(app) {
    // 0. Initial frame
    app.expectStyled("snapshots/popup_frames_initial.txt");

    // 1. Transparency: open transparent popup
    app.press("1");
    app.expectStyled("snapshots/popup_frames_transparent.txt");

    // Restoration after closing transparent popup via Escape
    app.press("Escape");
    app.expectStyled("snapshots/popup_frames_initial.txt");

    // 2. Occlusion: open opaque popup with composed Rectangle
    app.press("2");
    app.expectStyled("snapshots/popup_frames_opaque.txt");

    // Resize opaque popup
    app.press("R");
    app.expectStyled("snapshots/popup_frames_resized.txt");

    // Restoration after closing resized opaque popup
    app.press("Escape");
    app.expectStyled("snapshots/popup_frames_initial.txt");

    // 3. Nested overlays: open parent, then open child overlay
    app.press("3");
    app.expectStyled("snapshots/popup_frames_nested_parent.txt");
    app.press("C");
    app.expectStyled("snapshots/popup_frames_nested_child.txt");

    // Escape closes child first, leaving parent open
    app.press("Escape");
    app.expectStyled("snapshots/popup_frames_nested_parent.txt");

    // Escape closes parent, returning to initial frame
    app.press("Escape");
    app.expectStyled("snapshots/popup_frames_initial.txt");

    // 4. Default Dialog surface (composed background rectangle, double border, terminal default colors)
    app.press("4");
    app.expectStyled("snapshots/popup_frames_dialog_default.txt");
    app.press("Escape");
    app.expectStyled("snapshots/popup_frames_initial.txt");

    // 5. Customized Dialog surface (custom background.color, single green border)
    app.press("5");
    app.expectStyled("snapshots/popup_frames_dialog_custom.txt");
    app.press("Escape");
    app.expectStyled("snapshots/popup_frames_initial.txt");
}
