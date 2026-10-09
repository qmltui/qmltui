function main(terminal) {
    terminal.expectText("Status: Active");
    terminal.expectText("Workers: 4");
    terminal.expectText("S: change base style  T: toggle spans  Q / Esc: quit");
    const inherited = terminal.findText("Inherited")[0];
    const green = terminal.findText("explicit green")[0];
    const defaults = terminal.findText("terminal default")[0];
    const badge = terminal.findText("READY")[0];
    terminal.expectStyle(badge.x, badge.y, {bold: true, foreground: "#001B10FF", background: "#65E6A0FF"});
    terminal.expectStyle(inherited.x, inherited.y, {bold: false, foreground: "#8BE9FDFF"});
    terminal.captureStyled("initial.txt");
    terminal.press("s");
    terminal.expectStyle(inherited.x, inherited.y, {bold: true, foreground: "#FFB86CFF"});
    terminal.expectStyle(green.x, green.y, {bold: false, foreground: "green"});
    terminal.expectStyle(defaults.x, defaults.y, {bold: false, foreground: "default"});
    terminal.captureStyled("changed-style.txt");
    terminal.press("t");
    terminal.expectText("Fallback text: press T to restore spans");
    terminal.expectNoText("explicit green");
    terminal.captureStyled("fallback.txt");
    terminal.press("t");
    terminal.expectText("Inherited | explicit green | terminal default");
    terminal.expectNoText("Fallback text");
}
