// The quickstart renders, the focused button reacts to Enter, and the status
// line updates. Kept deliberately small: this is what a newcomer's first run
// looks like.
Test.waitForText("Welcome to Qtui");
Test.expectText("[ Press Enter, or Q to quit ]");
Test.expectText("Status: ready");
Test.press("Enter");
Test.waitForText("Status: Clicked!");
Test.captureText("quickstart.txt");
