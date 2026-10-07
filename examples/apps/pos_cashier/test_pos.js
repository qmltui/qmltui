function main(terminal) {
    terminal.waitForText("RapidPOS");
    terminal.captureText("pos/catalog.txt");

    terminal.press("F4");
    terminal.waitForText("ITEM NAME");
    terminal.expectText("ITEM NAME");

    terminal.press("F12");
    terminal.waitForText("PAYMENT METHOD");
    terminal.expectText("PAYMENT METHOD");
    terminal.expectText("CASH TENDERED");

    terminal.clickText("[$20]");
    terminal.waitForText("CHANGE DUE: $0.47");
    terminal.captureText("pos/payment.txt");

    terminal.clickText("CONFIRM PAYMENT");
    terminal.waitForText("TRANSACTION COMPLETED");
    terminal.captureText("pos/receipt.txt");
    terminal.finish();
}
