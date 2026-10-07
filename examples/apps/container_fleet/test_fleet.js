function main(terminal) {
    terminal.waitForText("DOCKER FLEET MONITOR");
    terminal.captureText("fleet/all.txt");

    terminal.press("2");
    terminal.waitForText("RUNNING");
    terminal.captureText("fleet/running.txt");

    terminal.press("3");
    terminal.waitForText("CRITICAL");
    terminal.captureText("fleet/critical.txt");

    terminal.press("L");
    terminal.waitForFrame();
    terminal.captureText("fleet/logs.txt");
    terminal.finish();
}
