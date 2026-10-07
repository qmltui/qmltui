// A symlinked subdirectory must not let a capture escape the artifact root.
// The harness creates "escape-link" pointing outside the root before the run.
function main(terminal) {
    terminal.captureText("escape-link/escaped.txt");
    terminal.finish();
}
