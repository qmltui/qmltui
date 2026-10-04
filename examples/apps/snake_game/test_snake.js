Test.waitForText("SNAKE");
Test.expectText("SCORE");
Test.captureText("snake/start.txt");

// Maneuver the snake around the grid
Test.press("DOWN");
Test.sleep(150);
Test.press("LEFT");
Test.sleep(150);
Test.press("UP");
Test.sleep(150);
Test.press("RIGHT");
Test.sleep(150);
Test.captureText("snake/maneuvers.txt");

// Pause the game
Test.press("SPACE");
Test.waitForText("PAUSED");
Test.captureText("snake/paused.txt");

// Resume and steer down-right
Test.press("SPACE");
Test.press("DOWN");
Test.sleep(150);
Test.press("RIGHT");
Test.sleep(150);

// Restart the game
Test.press("R");
Test.waitForText("SCORE 0");
Test.captureText("snake/restarted.txt");

Test.finish();
