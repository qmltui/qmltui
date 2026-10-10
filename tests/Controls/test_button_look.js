// Records the default rendering of Button, CheckBox, RadioButton and Switch
// (see ButtonLookFixture.qml). Re-record with captureStyled() only for an
// intentional change to the default look, and say so in the commit.
function main(terminal) {
    terminal.expectStyled("snapshots/button_look_initial.txt");
    terminal.press("Tab");            // focus moves to the plain button
    terminal.expectStyled("snapshots/button_look_tab.txt");
    terminal.press("Tab");            // the one-row button shows focus by bold only
    terminal.press("Tab");            // the disabled button is skipped: first checkbox
    terminal.expectStyled("snapshots/button_look_checkbox_focus.txt");
    terminal.press("Space");          // toggles it off
    terminal.press("Tab");
    terminal.press("Space");          // second checkbox on
    terminal.press("Tab");
    terminal.press("Tab");            // second radio
    terminal.press("Space");          // checks it, unchecking the first
    terminal.expectStyled("snapshots/button_look_toggled.txt");
    terminal.press("Tab");            // third radio
    terminal.press("Tab");            // first switch
    terminal.expectStyled("snapshots/button_look_switch_focus.txt");
}
