// In-tree JavaScript test scenario for Container Fleet Demo
TestDriver.snapshot("/tmp/fleet_1_all.png");

// 1. Switch to Filter 2 (RUNNING)
TestDriver.sendKey("2");
TestDriver.wait(300);
TestDriver.snapshot("/tmp/fleet_2_running.png");

// 2. Switch to Filter 3 (CRITICAL)
TestDriver.sendKey("3");
TestDriver.wait(300);
TestDriver.snapshot("/tmp/fleet_3_critical.png");

// 3. Toggle Live Logs Drawer via 'L'
TestDriver.sendKey("L");
TestDriver.wait(300);
TestDriver.snapshot("/tmp/fleet_4_logs.png");

// 4. Cycle Sort Mode via 'S'
TestDriver.sendKey("S");
TestDriver.wait(300);
TestDriver.snapshot("/tmp/fleet_5_sorted.png");

TestDriver.quit(0);
