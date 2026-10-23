function main(app) {
    app.expectText("Select a row or click a header");
    app.expectText("PID");
    app.expectText("NAME");
    app.expectText("CPU%");
    app.expectText("STATE");

    app.expectText("systemd");
    app.expectText("qmltui_gallery");
    app.expectText("sshd");
    app.expectText("postgres");
    app.expectText("node");
    app.expectText("zsh");

    app.expectNoText("│                                      │");
    app.captureText("initial_table_grid.txt");

    app.clickText("PID");
    app.expectText("Header 0 clicked (sorting is the model's job)");

    app.press("DOWN");
    app.press("DOWN");
    app.press("ENTER");
    app.expectText("Activated row 1: qmltui_gallery");

    app.captureText("activated_table_grid.txt");
}
