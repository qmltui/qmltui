# Qt Code Review Report: Image, Runner, TerminalHost

Scope: `src/Qamui/Image/`, `src/Qamui/Runner/`, `src/Qamui/TerminalHost/`
Files reviewed: 20
Issues found: 190 (153 from lint, 37 from deep analysis) + 10 investigation targets

---

## 1. Deep Analysis Findings

### Model Contracts
- **[D-006]** `src/Qamui/TerminalHost/TerminalView.cpp:96`: `scrollbackSizeChanged` signal is never emitted when the scrollback row container changes.
- **[D-007]** `src/Qamui/TerminalHost/VTermAdapter.cpp:284`: Unbacked/padding cells in `VTermAdapter::render` are incorrectly marked as wide-character continuation cells.

### Ownership & Lifecycle
- **[D-001]** `src/Qamui/TerminalHost/VTermAdapter.h:17`: Rule of Five violation in `VTermAdapter` leading to double-free of `VTerm`.
- **[D-014]** `src/Qamui/TerminalHost/UnixPtySession.cpp:42`: Destructor signal emission and partially-destroyed object mutation during `m_process.waitForFinished(250)`.
- **[D-022]** `src/Qamui/TerminalHost/UnixPtySession.cpp:146`: Missing loop re-entrancy check during `dataReceived` in `UnixPtySession::readAvailable()`.
- **[D-023]** `src/Qamui/TerminalHost/UnixPtySession.cpp:186`: Synchronous deletion of active `QSocketNotifier` during signal dispatch in `closeMaster()`.
- **[D-024]** `src/Qamui/Image/Image.cpp:203`: Persisted Sixel graphics display artifact leak in `Image::~Image()`.
- **[D-026]** `src/Qamui/Image/ImageProvider.h:28`, `src/Qamui/TerminalHost/UnixPtySession.h:13`: Polymorphic QObject classes missing `Q_DISABLE_COPY_MOVE`.

### Thread Safety
- **[D-002]** `src/Qamui/Runner/Runner.cpp:80`: `RunnerHardWatchdog` worker thread races with `QQmlEngine` destruction on scenario watchdog timeout.
- **[D-025]** `src/Qamui/Image/Image.cpp:269`: Premature deletion of `ImageResponse` via `deleteLater()` during background rendering.

### API & C++ Correctness
- **[D-003]** `src/Qamui/Image/TerminalGraphics.h:33`: Public installed header `TerminalGraphics.h` missing `QAMUI_IMAGE_EXPORT` on library functions.
- **[D-017]** `src/Qamui/Image/Image.cpp:196`: `switch` on `m_renderMode` with `default:` label suppresses `-Wswitch`.
- **[D-027]** `src/Qamui/Image/Image.h:64`, `src/Qamui/TerminalHost/TerminalView.h:49`: Missing trailing commas on enum definitions.
- **[D-028]** `src/Qamui/TerminalHost/TerminalView.cpp:23`, `src/Qamui/Runner/Runner.cpp:2492`: Const local variables prevent implicit move on return.
- **[D-029]** `src/Qamui/Image/Image.h:170`: Misleading `const` on image cleanup methods that issue side-effects.
- **[D-030]** `src/Qamui/TerminalHost/TerminalView.h:163`: `TerminalView::m_componentComplete` initialization breaks C++ instantiation with `autoStart`.

### Error Handling & Validation
- **[D-008]** `src/Qamui/TerminalHost/UnixPtySession.cpp:159`: Fatal PTY master read/write errors fail to disable `QSocketNotifier`, causing CPU busy loop.
- **[D-009]** `src/Qamui/Runner/CastRecorder.cpp:26`: File write return values ignored in `CastRecorder::open` and `event`.
- **[D-010]** `src/Qamui/Image/TerminalGraphics.cpp:106`: Out-of-bounds pointer arithmetic and negative size in `kittyTransmit` when `qCompress` fails.
- **[D-015]** `src/Qamui/Runner/Runner.cpp:2841`: `Runner::writeFinalFrameArtifact` returns true on frame acquisition failure, swallowing export errors.
- **[D-018]** `src/Qamui/Image/Image.cpp:932`: Unbounded pixel scaling and integer overflow in `paintSixel`.
- **[D-019]** `src/Qamui/TerminalHost/UnixPtySession.cpp:79`: Unchecked `dup2` return values during PTY stdio redirection.
- **[D-020]** `src/Qamui/TerminalHost/VTermAdapter.cpp:401`: Multi-fragment clipboard payloads dropped in `VTermAdapter::setSelection`.
- **[D-031]** `src/Qamui/Image/TerminalGraphics.cpp:135`: `kittyPlacement` generates invalid control sequences for non-positive dimensions.
- **[D-033]** `src/Qamui/TerminalHost/UnixPtySession.cpp:146`: Unbounded read loop in `UnixPtySession::readAvailable` starves the GUI event loop.
- **[D-034]** `src/Qamui/Image/Image.cpp:947`: Negative ANSI cursor positioning escape sequences emitted for off-screen Sixel graphics.
- **[D-035]** `src/Qamui/Runner/Runner.cpp:2524`: Unconditional null-pointer dereferences in `writeTextArtifact` and `resolveArtifactPath`.
- **[D-036]** `src/Qamui/Runner/MemoryTerminalBackend.cpp:25`: Unchecked offset in `MemoryTerminalBackend::writeSome` allows unsigned underflow.
- **[D-037]** `src/Qamui/Image/AnimatedSprite.cpp:69`: Negative loop counts in `AnimatedSprite::setLoops` cause premature animation exit.

### Performance & Code Quality
- **[D-004]** `src/Qamui/TerminalHost/VTermAdapter.cpp:286-295`: Per-cell `QString` and `QByteArray` heap allocations in `VTermAdapter::render`.
- **[D-005]** `src/Qamui/Image/TerminalGraphics.cpp:244-245`: Per-cell `QString` and `QByteArray` heap allocations in `writeKittyPlaceholders`.
- **[D-011]** `src/Qamui/Image/Image.cpp:847-857`: Redundant Kitty image re-transmission on placement/geometry resize.
- **[D-012]** `src/Qamui/Runner/Runner.cpp:941-943`: Uncached `QRegularExpression` compilation in hot operation trace path.
- **[D-013]** `src/Qamui/Image/AnimatedSprite.h:43`: Dead member variable `m_interpolate` in `AnimatedSprite`.
- **[D-016]** `src/Qamui/Runner/Runner.cpp:1834, 1867, 2713`: Full `TextGridBuffer` deep copy by value for dimension queries in `Runner`.
- **[D-021]** `src/Qamui/TerminalHost/TerminalView.cpp:486-488`: Stale `processState` during `errorOccurred` signal emission in `TerminalView`.
- **[D-032]** `src/Qamui/Image/TerminalGraphics.cpp:202-207`: Redundant `constScanLine` calls in inner loop of `sixelEncode`.

---

## 2. Investigation Targets
- **[I-001]** `src/Qamui/TerminalHost/TerminalView.cpp:74`: Missing protocol prefix parsing for URXVT-style OSC 777 notifications.
- **[I-002]** `src/Qamui/Runner/CastRecorder.cpp:18`: Non-positive dimensions accepted in `CastRecorder::open` header generation.
- **[I-003]** `src/Qamui/Image/ImageProvider.cpp:8`: Case-sensitive lookup in `ImageProvider` registry via QUrl host.
- **[I-004]** `src/Qamui/Image/Image.cpp:469-472`: Missing re-entrancy guard in `Image::loadImage`.
- **[I-005]** `src/Qamui/Image/Image.cpp:128-145`: Deep recursive resource directory traversal in `Image::resolvePath` fallback.
- **[I-006]** `src/Qamui/TerminalHost/TerminalView.cpp:374-376`: Hardcoded UI styling magic numbers in `TerminalView` scrollbar.
- **[I-007]** `src/Qamui/Runner/Runner.cpp:390`: Stale static `s_activeRunner` preventing runner re-initialization.
- **[I-008]** `src/Qamui/TerminalHost/UnixPtySession.cpp:47`: Synchronous `waitForFinished(250)` blocking GUI thread during `UnixPtySession` destruction.
- **[I-009]** `src/Qamui/Image/Image.h:144`: Out-parameter boolean trap in `Image::loadFromProvider`.
- **[I-010]** `src/Qamui/Runner/Runner.h:390`: Closed vocabulary for `Runner` mode held in `QString` instead of scoped enum.
