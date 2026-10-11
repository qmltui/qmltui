// Ambient TypeScript declarations for the Qtui Runner scenario API.
//
// Scenarios are plain JavaScript executed by QJSEngine, so TypeScript syntax
// is not valid inside them. Editor completion comes from one JSDoc line:
//
//     /** @param {Terminal} terminal */
//     function main(terminal) {
//         terminal.press("Enter");
//     }
//
// The repository's jsconfig.json makes these declarations ambient for every
// scenario under tests/ and examples/; installed consumers can reference this
// file from their own jsconfig.json. Runner.md stays the authoritative
// behavioural documentation; test_runner_declarations fails when a facade
// method is missing here.

/** Mouse button name accepted by the mouse operations. */
type QtuiMouseButton = "left" | "middle" | "right";

/** Modifier names are case-insensitive; "Control" is accepted for "Ctrl". */
type QtuiModifier = "Shift" | "Ctrl" | "Control" | "Alt" | "Meta";

interface QtuiTimeoutOptions {
    /** Milliseconds; expect* default to the configured operation timeout. */
    timeout?: number;
}

interface QtuiModifierOptions {
    modifiers?: QtuiModifier[];
}

interface QtuiClickOptions extends QtuiModifierOptions {
    button?: QtuiMouseButton;
}

interface QtuiClickTextOptions extends QtuiClickOptions {
    /** Zero-based index into the row-major matches. */
    occurrence?: number;
}

/** One rendered cell, as reported by cell(). */
interface QtuiCell {
    x: number;
    y: number;
    character: string;
    /** True for the trailing cell of a wide character. */
    continuation: boolean;
    /** "#RRGGBBAA", or "default" for the terminal's own color. */
    foreground: string;
    background: string;
    attributes: {
        bold: boolean;
        dim: boolean;
        italic: boolean;
        underline: boolean;
        reversed: boolean;
        blink: boolean;
        conceal: boolean;
        strikethrough: boolean;
    };
}

/** Expected cell properties for expectStyle(). */
interface QtuiStyleExpectation {
    character?: string;
    bold?: boolean;
    dim?: boolean;
    italic?: boolean;
    underline?: boolean;
    reverse?: boolean;
    reversed?: boolean;
    /** A color string, or "default" for the terminal's own color. */
    foreground?: string;
    background?: string;
}

/** One visible occurrence reported by findText(). */
interface QtuiTextMatch {
    x: number;
    y: number;
    width: number;
}

interface QtuiRect {
    x: number;
    y: number;
    width: number;
    height: number;
}

interface QtuiSize {
    columns: number;
    rows: number;
}

/** Cost of the most recent frame and totals for the session. */
interface QtuiFrameStats {
    frame: number;
    paintMicros: number;
    diffMicros: number;
    writeMicros: number;
    diffBytes: number;
    bytesWritten: number;
    dirtyDuringPaint: number;
    queuedBytes: number;
    frames: number;
    paintMicrosAvg: number;
    paintMicrosMax: number;
    diffMicrosAvg: number;
    bytesTotal: number;
    dirtyDuringPaintTotal: number;
    skippedForBackpressure: number;
}

/**
 * A locator scoped to one rectangle of the frame, from region() or item().
 * Every assertion and click operates only inside the rectangle.
 */
interface Region extends QtuiRect {
    /** Set by item(): the objectName the rectangle was resolved from. */
    objectName?: string;
    /** Normalized text of the rectangle, trailing spaces trimmed per row. */
    text(): string;
    /** Visible matches fully inside the rectangle. */
    findText(text: string): QtuiTextMatch[];
    /** Fails unless the next committed frame's rectangle contains text. */
    expectText(text: string, options?: QtuiTimeoutOptions): void;
    /** Fails if the next committed frame's rectangle contains text. */
    expectNoText(text: string, options?: QtuiTimeoutOptions): void;
    /** Waits until the rectangle contains text. */
    waitForText(text: string, options?: QtuiTimeoutOptions): void;
    /** expectSnapshot() for the rectangle's text, with the same update mode. */
    expectSnapshot(snapshotRelativePath: string, options?: QtuiTimeoutOptions): void;
    /** Clicks the center of a text match inside the rectangle. */
    clickText(text: string, options?: QtuiClickTextOptions): void;
    /** Clicks the center of the rectangle. */
    click(options?: QtuiClickOptions): void;
}

/**
 * The driver facade passed to function main(terminal): scripted input,
 * committed-frame queries and assertions, artifacts, and time control.
 * See src/Qtui/Runner/doc/Runner.md for the full behavioural contract.
 */
interface Terminal {
    /** Sends one key (named key, ASCII, or any single grapheme) and waits for the committed frame. */
    press(key: string, modifiers?: QtuiModifier[]): void;
    press(key: string, options?: QtuiModifierOptions): void;
    /** Feeds text through the input decoder, one key event per character, and waits once. */
    typeText(text: string): void;
    /** Sends text as one bracketed paste and waits; newlines and tabs arrive as data. */
    paste(text: string): void;
    /** press() without the implicit frame wait, for expectRepaint(). */
    injectKey(key: string, modifiers?: QtuiModifier[]): void;
    injectKey(key: string, options?: QtuiModifierOptions): void;
    /** typeText() without the implicit frame wait. */
    injectText(text: string): void;
    /** paste() without the implicit frame wait. */
    injectPaste(text: string): void;

    /** Sends a mouse press and release at (x, y) and waits for the frame. */
    click(x: number, y: number, button?: QtuiMouseButton, modifiers?: QtuiModifier[]): void;
    click(x: number, y: number, options?: QtuiClickOptions): void;
    /** Two consecutive press/release cycles at (x, y). */
    doubleClick(x: number, y: number, button?: QtuiMouseButton, modifiers?: QtuiModifier[]): void;
    doubleClick(x: number, y: number, options?: QtuiClickOptions): void;
    /** Presses a button at (x, y) without releasing it. */
    mouseDown(x: number, y: number, button?: QtuiMouseButton, modifiers?: QtuiModifier[]): void;
    mouseDown(x: number, y: number, options?: QtuiClickOptions): void;
    /** Releases a button at (x, y). */
    mouseUp(x: number, y: number, button?: QtuiMouseButton, modifiers?: QtuiModifier[]): void;
    mouseUp(x: number, y: number, options?: QtuiClickOptions): void;
    /** Motion with no button; delivered only while hover reporting is enabled. */
    hover(x: number, y: number, modifiers?: QtuiModifier[]): void;
    hover(x: number, y: number, options?: QtuiModifierOptions): void;
    /** Press, held motion cell by cell along the path, release. */
    drag(startX: number, startY: number, endX: number, endY: number,
         button?: QtuiMouseButton, modifiers?: QtuiModifier[]): void;
    drag(startX: number, startY: number, endX: number, endY: number,
         options?: QtuiClickOptions): void;
    /** Mouse wheel at (x, y). */
    scroll(x: number, y: number, deltaX: number, deltaY: number,
           modifiers?: QtuiModifier[]): void;
    scroll(x: number, y: number, deltaX: number, deltaY: number,
           options?: QtuiModifierOptions): void;
    /** Clicks the center of a rendered text match and waits for the frame. */
    clickText(text: string, occurrence?: number, button?: QtuiMouseButton,
              modifiers?: QtuiModifier[]): void;
    clickText(text: string, options?: QtuiClickTextOptions): void;

    /** Waits for the requested duration; under the virtual clock this is advanceTime(). */
    sleep(durationMs: number): void;
    /** Virtual clock only: advances Qtui time, firing timers and animations exactly. */
    advanceTime(durationMs: number): void;
    /** Milliseconds on the Qtui clock (virtual time under the virtual clock). */
    now(): number;
    /** Requests rendering and waits for a newer committed frame. */
    waitForFrame(timeoutMs?: number): void;
    waitForFrame(options?: QtuiTimeoutOptions): void;
    /** Waits for a frame the application committed on its own, never requesting one. */
    expectRepaint(timeoutMs?: number): void;
    expectRepaint(options?: QtuiTimeoutOptions): void;
    /** Sequence number of the most recent committed frame. */
    frameCount(): number;
    /** Render cost of the most recent frame plus session totals. */
    frameStats(): QtuiFrameStats;

    /** Waits until the committed frame contains text. */
    waitForText(text: string, timeoutMs?: number): void;
    waitForText(text: string, options?: QtuiTimeoutOptions): void;
    /** Waits until the committed frame matches the pattern. */
    waitForRegex(pattern: string, timeoutMs?: number): void;
    waitForRegex(pattern: string, options?: QtuiTimeoutOptions): void;
    /** Polls a synchronous callback with (frameText, elapsedMs) until truthy. */
    waitForCondition(condition: (frameText: string, elapsedMs: number) => unknown,
                     timeoutMs?: number): void;
    waitForCondition(condition: (frameText: string, elapsedMs: number) => unknown,
                     options?: QtuiTimeoutOptions): void;
    /** Waits until an item with the objectName is visible in the viewport. */
    waitForItem(objectName: string, timeoutMs?: number): void;
    waitForItem(objectName: string, options?: QtuiTimeoutOptions): void;

    /** Fails unless the next committed frame contains text. */
    expectText(text: string, timeoutMs?: number): void;
    expectText(text: string, options?: QtuiTimeoutOptions): void;
    /** Fails if the next committed frame contains text. */
    expectNoText(text: string, timeoutMs?: number): void;
    expectNoText(text: string, options?: QtuiTimeoutOptions): void;
    /** Fails unless the next committed frame matches the pattern. */
    expectRegex(pattern: string, timeoutMs?: number): void;
    expectRegex(pattern: string, options?: QtuiTimeoutOptions): void;
    /** Fails unless the bounding box contains text. */
    expectTextInRegion(x: number, y: number, width: number, height: number,
                       text: string, timeoutMs?: number): void;
    expectTextInRegion(x: number, y: number, width: number, height: number,
                       text: string, options?: QtuiTimeoutOptions): void;
    /** Fails unless the cell at (x, y) matches the expected properties. */
    expectStyle(x: number, y: number, expectedStyle: QtuiStyleExpectation,
                timeoutMs?: number): void;
    expectStyle(x: number, y: number, expectedStyle: QtuiStyleExpectation,
                options?: QtuiTimeoutOptions): void;
    /** Compares the frame text against a golden baseline (QTUI_RUNNER_UPDATE_SNAPSHOTS rewrites it). */
    expectSnapshot(snapshotRelativePath: string, timeoutMs?: number): void;
    expectSnapshot(snapshotRelativePath: string, options?: QtuiTimeoutOptions): void;
    /** Like expectSnapshot(), but compares styledText(): colors and attributes too. */
    expectStyled(snapshotRelativePath: string, timeoutMs?: number): void;
    expectStyled(snapshotRelativePath: string, options?: QtuiTimeoutOptions): void;
    /** expectSnapshot() for one region's normalized text. */
    expectRegionSnapshot(x: number, y: number, width: number, height: number,
                         snapshotRelativePath: string, timeoutMs?: number): void;
    expectRegionSnapshot(x: number, y: number, width: number, height: number,
                         snapshotRelativePath: string, options?: QtuiTimeoutOptions): void;

    /** Normalized text of the current committed frame, without waiting. */
    frameText(): string;
    /** Frame text plus a "--- styles" section of colors and attributes. */
    styledText(): string;
    /** Normalized text of a sub-grid, trailing spaces trimmed per row. */
    regionText(x: number, y: number, width: number, height: number): string;
    /** The rendered cell at (x, y). */
    cell(x: number, y: number): QtuiCell;
    /** Every visible single-row occurrence in row-major order. */
    findText(text: string): QtuiTextMatch[];
    /** Rectangle in cells of the first visible item with the objectName. */
    itemRect(objectName: string): QtuiRect;
    /** {columns, rows} of the current committed frame. */
    size(): QtuiSize;
    /** A locator scoped to the rectangle. */
    region(x: number, y: number, width: number, height: number): Region;
    /** A locator scoped to itemRect(objectName), with click() at its center. */
    item(objectName: string): Region;

    /** Writes the next committed frame as text beneath the artifact directory. */
    captureText(relativePath: string): void;
    /** Writes the next committed frame as styled text. */
    captureStyled(relativePath: string): void;
    /** Resizes the test viewport and waits for a frame at the new size. */
    resize(columns: number, rows: number): void;

    /** Fails the scenario immediately. */
    fail(message: string): void;
    /** Marks the scenario as finishing; optional when main returns normally. */
    finish(): void;
    /** The root Terminal QML object — for computing expectations, not asserting. */
    rootObject(): any;
}

/** Alias for consumers who prefer an explicit name. */
type QtuiTerminal = Terminal;
