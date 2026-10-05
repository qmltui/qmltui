Test.waitForText("DOCKER FLEET MONITOR");
Test.captureText("fleet/all.txt");

Test.press("2");
Test.waitForText("RUNNING");
Test.captureText("fleet/running.txt");

Test.press("3");
Test.waitForText("CRITICAL");
Test.captureText("fleet/critical.txt");

Test.press("L");
Test.waitForFrame();
Test.captureText("fleet/logs.txt");
Test.finish();
