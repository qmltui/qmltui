// Records the default rendering of Button, CheckBox, RadioButton and Switch
// (see ButtonLookFixture.qml). Re-record with captureStyled() only for an
// intentional change to the default look, and say so in the commit.
function main(app) {
    app.expectStyled("snapshots/button_look_initial.txt");
    app.press("Tab");            // focus moves to the plain button
    app.expectStyled("snapshots/button_look_tab.txt");
    app.press("Tab");            // the one-row button shows focus by bold only
    app.press("Tab");            // the disabled button is skipped: first checkbox
    app.expectStyled("snapshots/button_look_checkbox_focus.txt");
    app.press("Space");          // toggles it off
    app.press("Tab");
    app.press("Space");          // second checkbox on
    app.press("Tab");
    app.press("Tab");            // second radio
    app.press("Space");          // checks it, unchecking the first
    app.expectStyled("snapshots/button_look_toggled.txt");
    app.press("Tab");            // third radio
    app.press("Tab");            // first switch
    app.expectStyled("snapshots/button_look_switch_focus.txt");
}
