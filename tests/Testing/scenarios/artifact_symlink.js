// A symlinked subdirectory must not let a capture escape the artifact root.
// The harness creates "escape-link" pointing outside the root before the run.
Test.captureText("escape-link/escaped.txt");
Test.finish();
