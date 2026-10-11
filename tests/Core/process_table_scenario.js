function main(terminal) {
    terminal.expectText("Select a row or click a header");
    terminal.expectText("PID");
    terminal.expectText("NAME");
    terminal.expectText("CPU%");
    terminal.expectText("STATE");

    terminal.expectText("systemd");
    terminal.expectText("qamui_gallery");
    terminal.expectText("sshd");
    terminal.expectText("postgres");
    terminal.expectText("node");
    terminal.expectText("zsh");

    terminal.expectNoText("│                                      │");
    terminal.captureText("initial_table_grid.txt");

    terminal.clickText("PID");
    terminal.expectText("Header 0 clicked (sorting is the model's job)");

    terminal.press("DOWN");
    terminal.press("DOWN");
    terminal.press("ENTER");
    terminal.expectText("Activated row 1: qamui_gallery");

    terminal.captureText("activated_table_grid.txt");
}
