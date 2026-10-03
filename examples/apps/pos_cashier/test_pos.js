// In-tree JavaScript test scenario for POS Cashier
TestDriver.snapshot("/tmp/in_tree_pos_1_catalog.png");

// 1. Switch to Cart View via Key 'F4'
TestDriver.sendKey("F4");
TestDriver.assertTextContains("ITEM NAME");
TestDriver.snapshot("/tmp/in_tree_pos_2_cart.png");
TestDriver.wait(500);

// 2. Switch to Payment View via Key 'F12'
TestDriver.sendKey("F12");
TestDriver.assertTextContains("PAYMENT METHOD");
TestDriver.snapshot("/tmp/in_tree_pos_3_payment.png");
TestDriver.wait(500);

// 3. Confirm Payment via Enter key
TestDriver.sendKey("Enter");
TestDriver.wait(500);
TestDriver.assertTextContains("TRANSACTION COMPLETED");
TestDriver.snapshot("/tmp/in_tree_pos_4_receipt.png");

TestDriver.quit(0);
