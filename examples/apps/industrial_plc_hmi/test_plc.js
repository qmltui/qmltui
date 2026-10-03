// In-tree JavaScript test scenario for Industrial PLC HMI
TestDriver.snapshot("/tmp/in_tree_plc_1_standby.png");

// 1. Press 'S' to start PLC machine
TestDriver.sendKey("S");
TestDriver.wait(1500);
TestDriver.assertTextContains("[● RUNNING]");
TestDriver.snapshot("/tmp/in_tree_plc_2_running.png");

// 2. Press '2' to switch to Alarms View
TestDriver.sendKey("2");
TestDriver.assertTextContains("ALARM HISTORY");

// 3. Press 'M' to trigger Maintenance dialog
TestDriver.sendKey("M");
TestDriver.assertTextContains("MAINTENANCE MODE");
TestDriver.snapshot("/tmp/in_tree_plc_3_dialog.png");

// 4. Press 'Y' to confirm Maintenance mode
TestDriver.sendKey("Y");
TestDriver.assertTextContains("[⚙ MAINT]");
TestDriver.snapshot("/tmp/in_tree_plc_4_maint.png");

TestDriver.quit(0);
