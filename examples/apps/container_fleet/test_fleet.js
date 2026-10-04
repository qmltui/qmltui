Test.waitForText("CONTAINER FLEET");
Test.captureText("fleet/all.txt");

Test.press("2");
Test.waitForText("RUNNING");
Test.captureText("fleet/running.txt");

Test.press("3");
Test.waitForText("CRITICAL");
Test.captureText("fleet/critical.txt");

Test.press("L");
Test.waitForFrame();
Test.captureImage("fleet/logs.png");
Test.finish();
