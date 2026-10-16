/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Loader Workbench");
    terminal.expectText("SystemCard");
    terminal.expectText("loader.active: true");
    terminal.expectText("loader.status: Ready");
    terminal.sleep(300);

    terminal.press("2");
    terminal.expectText("NetworkCard");
    terminal.expectText("Ingress:    1.42 GB/s");
    terminal.sleep(300);

    terminal.press("3");
    terminal.expectText("loader.active: false");
    terminal.expectText("loader.status: Null");
    terminal.sleep(300);

    terminal.press("1");
    terminal.expectText("loader.status: Ready");
    terminal.expectText("CPU Load:   24.8%");
    terminal.sleep(300);
}
