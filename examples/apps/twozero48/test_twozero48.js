Test.waitForText("2048 TUI");
Test.expectText("SCORE");
Test.expectText("BEST");
Test.captureText("twozero48/initial.txt");

// Arrow moves
Test.press("UP");
Test.press("LEFT");
Test.press("DOWN");
Test.press("RIGHT");
Test.captureText("twozero48/arrow_moves.txt");

// WASD moves
Test.typeText("w");
Test.typeText("a");
Test.typeText("s");
Test.typeText("d");
Test.captureText("twozero48/wasd_moves.txt");

// Vim keys moves
Test.typeText("k");
Test.typeText("h");
Test.typeText("j");
Test.typeText("l");
Test.captureText("twozero48/vim_moves.txt");

// Restart
Test.press("R");
Test.waitForText("SCORE");
Test.captureText("twozero48/restarted.txt");

Test.finish();
