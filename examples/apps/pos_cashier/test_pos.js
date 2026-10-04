Test.waitForText("RapidPOS");
Test.captureText("pos/catalog.txt");

Test.press("F4");
Test.waitForText("ITEM NAME");
Test.expectText("ITEM NAME");

Test.press("F12");
Test.waitForText("PAYMENT METHOD");
Test.expectText("PAYMENT METHOD");
Test.expectText("CASH TENDERED");

Test.clickText("[$20]");
Test.waitForText("CHANGE DUE: $0.47");
Test.captureText("pos/payment.txt");

Test.clickText("CONFIRM PAYMENT");
Test.waitForText("TRANSACTION COMPLETED");
Test.captureText("pos/receipt.txt");
Test.finish();
