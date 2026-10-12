  ## Qt Code Review Report

  Scope: src/Qamui/Core/ (universal Qt6 C++ rules)
  Files reviewed: 126
  Issues found: 285 (239 from lint, 46 from deep analysis) + 10 investigation targets
  ──────
  ### Lint findings

  #### [L-001] Get-prefix on mere getters in TerminalDevice

  • File: TerminalDevice.h:47-48
  • Rule: API-5 (get-prefix means user interaction or decomposition/out-params, not mere getters)
  • Finding: Methods getBaudRate() and getDataBits() use the get prefix on const property query accessors.
  • Mitigation: Rename getBaudRate() to baudRate() and getDataBits() to dataBits().

  #### [L-002] Deprecated QPair usage

  • Files:
      • src/Qamui/Core/items/Item.h:10, 334
      • Item.cpp:944
  • Rule: DEP-5 (QPair — use std::pair)
  • Finding: QPair<QString, QVariant> is used for property bindings and signal emissions instead of std::pair.
  • Mitigation: Replace QPair with std::pair and include <utility>.

  #### [L-003] Deprecated Qt math algorithms (qMin, qMax, qBound)

  • File: src/Qamui/Core/engine/Clock.cpp:115, 160, 187
  • Rule: DEP-7 (qMin/qMax/qBound — use (std::min)()/(std::max)()/std::clamp())
  • Finding: Clock.cpp uses qMin, qMax, and qBound which are deprecated in modern Qt 6 in favor of standard algorithms.
  • Mitigation: Replace with macro-safe (std::min)(), (std::max)(), or std::clamp().

  #### [L-004] Direct QChar object instantiation

  • Files:
      • src/Qamui/Core/engine/TerminalEngine.cpp:878, 880
      • TextEditorState.cpp:255
  • Rule: DEP-13 (QChar as object type — use char16_t; QChar:: namespace is OK)
  • Finding: Raw QChar is used for character variables rather than char16_t.
  • Mitigation: Use char16_t for scalar character storage, reserving QChar for static utility methods.

  #### [L-005] Two or more consecutive boolean parameters (Boolean trap)

  • Files:
      • InputDecoder.h:16
      • InputRouter_p.h:23
      • InputRouter.cpp:234
      • TerminalEngine.h:160
      • TerminalEngine.cpp:561
  • Rule: ENM-9 (Two or more bool parameters create boolean parameter traps; use named enums)
  • Finding: Signatures take multiple boolean parameters (e.g. sendMouseEvent(..., bool pressed, bool released)), obscuring call-site intent.
  • Mitigation: Replace boolean parameters with descriptive scoped enums or strongly typed flags.

  #### [L-006] Unscoped enums without explicit underlying type

  • Files:
      • AnchorLine.h:17 (AnchorLineType)
      • Gradient.h:24 (Orientation)
      • GridLayout.h:20 (Flow)
      • LinearLayout.h:14 (Direction)
      • Loader.h:34 (Status)
      • Paragraph.h:37 (TextFormat)
      • Popup.h:24 (ClosePolicy)
      • TableView.cpp:17 (ColumnRole)
      • Text.h:36 (TextFormat)
      • src/Qamui/Core/items/TextInput.h:49, 56 (EchoMode, InputMethodHints)
      • InputDecoder.h:30 (Modifier)
      • Animation.h:20 (LoopMode)
      • Keys.h:20 (Priority)
      • RenderPhase_p.h:24 (Phase)
  • Rule: ENM-2 (Unscoped enum without explicit underlying type — add type to prevent ABI/BiC breaks)
  • Finding: 15 public and internal enums omit underlying type declarations (: int, : quint8, etc.).
  • Mitigation: Add explicit underlying types to all unscoped enums (e.g. enum Orientation : uint8_t { ... }).

  #### [L-007] Non-const range-for reference over COW containers

  • Files:
      • src/Qamui/Core/engine/TerminalEngine.cpp:850, 935
      • src/Qamui/Core/items/TableView.cpp:85, 102, 753
  • Rule: PAT-12 (Non-const range-for reference may trigger COW detach; use const auto&)
  • Finding: Iterating over QList and QHash with for (auto &x : ...) invokes non-const begin()/end(), triggering unnecessary container detach and deep copy.
  • Mitigation: Use for (const auto &x : ...) or wrap with std::as_const(...).

  #### [L-008] Integer timeout and interval parameters

  • Files: 27 occurrences, including:
      • src/Qamui/Core/engine/Clipboard.h:17, 25, 55
      • src/Qamui/Core/engine/Clock_p.h:80, 81, 87, 98
      • src/Qamui/Core/engine/TerminalEngine.h:141, 256
      • TerminalBackend.h:71
      • src/Qamui/Core/primitives/TerminalDevice.h:56, 62, 68
      • PosixTerminalBackend.h:31
      • Win32ConsoleBackend.h:37
  • Rule: TMO-1 (No ints/qint64 for timeouts or intervals; use QDeadlineTimer or std::chrono)
  • Finding: Millisecond intervals and timeout durations are passed as raw int or qint64 parameters without unit safety.
  • Mitigation: Migrate timeout signatures to std::chrono::milliseconds or QDeadlineTimer.

  #### [L-009] Unprotected std::min and std::max calls

  • Files: 128 occurrences across 25 source files (e.g. TextGridBuffer.cpp:96, TextArea.cpp:284, TextInput.cpp:133, InputDecoder.cpp:87)
  • Rule: HDR-3 (Unprotected min/max calls — use (std::min)(a,b) for Windows macro safety)
  • Finding: Bare calls to std::min(...), std::max(...), and std::numeric_limits<T>::min() will fail compilation on MSVC when <windows.h> defines min and max macros.
  • Mitigation: Wrap invocation targets in parentheses: (std::min)(a, b) and (std::numeric_limits<T>::min)().

  #### [L-010] Deprecated .count() and .length() calls

  • Files: 22 occurrences across Item.cpp:69, Paragraph.cpp:99, Text.cpp:77, Scrollback.cpp:379, InputRouter.cpp:304
  • Rule: DEP-10 (.count()/.length() — prefer .size() for standard library consistency)
  • Finding: Calling .count() or .length() on QString, QList, or QByteArray instead of .size().
  • Mitigation: Replace calls with .size().

  #### [L-011] C++ standard header included before Qt headers

  • Files:
      • Animation.cpp:5
      • GradientStop.h:7
  • Rule: INC-2 (C++ standard header before Qt header — reorder by descending specificity)
  • Finding: <algorithm> and <utility> are included before module headers and Qt headers.
  • Mitigation: Reorder #include statements from specific (Qamui headers) to general (Qt headers), placing standard library headers last.
  ──────
  ### Deep analysis findings

  #### [D-001] TableView drops dataChanged notifications for columns > 0 with rowDelegate

  • File: TableView.cpp:758
  • Category: Model Contracts
  • Confidence: 92/100
  • Finding: When rowDelegate is active, TableView discards all dataChanged signals for columns greater than 0. TableView::layoutDelegates() instantiates one row delegate
  per visible row anchored at column 0 (record.source is sourceIndex(row, 0), so index.column() is always 0). In TableView::handleDataChanged(), the filter condition
  requires index.column() >= topLeft.column() && index.column() <= bottomRight.column(). When a model emits dataChanged for any column where topLeft.column() > 0, 0 >=
  topLeft.column() evaluates to false, leaving affected = false and omitting the row delegate from dirty role marking and repainting.
  • Trace: Traced TableView.cpp:969 (wanted inserts {row, 0}), TableView.cpp:983 (record.source = sourceIndex(row, 0)), and TableView.cpp:754-758 (0 >= topLeft.column()
  test).
  • Mitigation: When m_rowDelegate is active, check row intersection without restricting index.column() against topLeft.column(), or verify column intersection across the
  full row range [0, columnCount() - 1].

  #### [D-002] ListView and GridView removeModelRows missing bounds checks on first > last

  • File: ListView.cpp:644, GridView.cpp:535
  • Category: Model Contracts
  • Confidence: 88/100
  • Finding: ListView::removeModelRows and GridView::removeModelRows compute count = last - first + 1 and shift item collections without validating first < 0 || first >
  last. If a model calls beginRemoveRows with first > last (a known QAIM edge case when clearing 0 rows), count becomes <= 0. In ListView, m_currentIndex increases, and
  delegates with idx > last have their keys remapped to idx - count, shifting indices forward to higher numbers instead of removing them.
  • Trace: Verified in ListView.cpp:644-680 and GridView.cpp:535-553. In contrast, Repeater::onRowsRemoved (Repeater.cpp:453) correctly guards if (first < 0 || first >
  last) return;.
  • Mitigation: Add an early return guard if (first < 0 || first > last) return; at the entry of both ListView::removeModelRows and GridView::removeModelRows.

  #### [D-003] ListView fails to update delegate context property "index" on structural row shifts

  • File: ListView.cpp:189
  • Category: Model Contracts
  • Confidence: 85/100
  • Finding: In ListView::insertModelRows and ListView::removeModelRows, existing delegates in m_createdItems shifted to new positions are updated via
  refreshDelegateStates() -> updateDelegateState(item, index). updateDelegateState only calls setPropertyIfExists(item, "index", index), which writes exclusively to
  declared root item properties. It never updates the delegate context property index (DelegateUtils::setContextProperty(component, context, "index", index)). Consequently,
  delegates that access index via the standard QML context property retain stale pre-shift index values.
  • Trace: Traced ListView.cpp:189 and compared with GridView::reindexItem (GridView.cpp:187) and Repeater (Repeater.cpp:443, 472), which update both the context property
  and the item property.
  • Mitigation: In ListView::updateDelegateState, call DelegateUtils::setContextProperty(m_delegate, delegateContext(item), QStringLiteral("index"), index) alongside
  setPropertyIfExists(item, "index", index).

  #### [D-004] NumericItemRoles gates modelIndex publication on !checkable condition

  • File: NumericItemRoles_p.h:191
  • Category: Model Contracts
  • Confidence: 84/100
  • Finding: In NumericItemRoles::apply(), modelIndex is passed to the insertion helper with unset set to !checkable: put(QStringLiteral("modelIndex"), ClaimsModelIndex,
  QVariant::fromValue(QModelIndex(index)), !checkable);. When delegates are created initially (includeUnset == false in ListView, GridView, and Repeater), any non-checkable
  row evaluates unset == true, omitting modelIndex from the delegate's initial model map. When a subsequent dataChanged signal occurs, includeUnset == true is passed,
  causing modelIndex to appear unexpectedly.
  • Trace: Confirmed in NumericItemRoles_p.h:170, 191 and delegate creation in ListView.cpp:363, GridView.cpp:258, and Repeater.cpp:313.
  • Mitigation: Decouple modelIndex publication from checkable by passing !index.isValid() as the unset condition.

  #### [D-005] Repeater rebuilds roleMap and queries roleNames() on every dataChanged emission

  • File: src/Qamui/Core/items/Repeater.cpp:233, 379
  • Category: Model Contracts
  • Confidence: 82/100
  • Finding: In Repeater.cpp, the dataChanged connection invokes buildRoleMap(guardedModel) on line 233, which calls guardedModel->roleNames() and constructs a new
  QHash<int, QString> from scratch on every single dataChanged emission. Similarly, Repeater::onRowsInserted calls model->roleNames() on line 379, violating MDL-10
  (roleNames() should cache the QHash).
  • Trace: Traced Repeater.cpp:218-233, 379. Compared with ListView and GridView, which maintain a member m_roleMap populated once on generateItems().
  • Mitigation: Cache m_roleMap as a member of Repeater populated once upon model attachment, and reuse it in dataChanged and onRowsInserted.

  #### [D-006] TreeView duplicates virtual roleNames() calls and hash copies per node load

  • File: TreeView.cpp:548, NumericItemRoles_p.h:143
  • Category: Model Contracts
  • Confidence: 81/100
  • Finding: In TreeView::fillNodeFromModel, line 548 calls model->roleNames(). Immediately after, line 553 calls NumericItemRoles::apply(...) without the mask, which calls
  claimedNames(model) (NumericItemRoles_p.h:198), calling model->roleNames() a second time. For every node loaded, two virtual roleNames() calls and two complete QHash
  copies are performed.
  • Trace: Traced TreeView.cpp:548, 553 and NumericItemRoles_p.h:143, 198.
  • Mitigation: Resolve and cache m_claimedNames and m_roleNames in TreeView when attaching a model, pass m_claimedNames into NumericItemRoles::apply, and reuse the cached
  hash.

  #### [D-007] AnimationManager::handleTick shallow copy use-after-free during iteration

  • File: Animation.cpp:391
  • Category: Ownership & Lifecycle
  • Confidence: 92/100
  • Finding: AnimationManager::handleTick creates a local snapshot of its registered animations (QList<Animation*> activeList = m_animations;) and iterates through it
  calling anim->tick(delta). Synchronous handlers (such as Item::setState() deleting running transition animations or QML onFinished deleting items) unregister the object
  from m_animations. However, activeList retains the dangling raw pointer. Subsequent iterations execute if (anim && anim->running()), dereferencing the freed pointer.
  • Trace: Verified in Animation.cpp:386-397, Item::setState() (Item.cpp:827), and AnimationManager::unregisterAnimation().
  • Mitigation: Store activeList as QList<QPointer<Animation>> so destroyed instances automatically clear to null, or check whether m_animations.contains(anim) before
  invoking anim->running().

  #### [D-008] Keys::m_forwardToItems stores raw Item pointers without destruction tracking

  • File: Keys.h:78
  • Category: Ownership & Lifecycle
  • Confidence: 90/100
  • Finding: The attached Keys type maintains QList<qamui::Item*> m_forwardToItems; holding raw pointers to forwarded target items. When a forwarded item is deleted,
  m_forwardToItems retains a dangling pointer. In InputRouter.cpp:74-80, attached->forwardToItems() is copied into QList<QPointer<Item>> forwardedItems, but initializing a
  QPointer with an already-deleted address cannot detect prior destruction, leading to a crash when calling isEffectivelyEnabled() or keyPressEvent().
  • Trace: Traced Keys.h:61-78 and InputRouter.cpp:73-82.
  • Mitigation: Store m_forwardToItems as QList<QPointer<Item>> in Keys, or connect to each added item's QObject::destroyed signal to prune it upon deletion.

  #### [D-009] Rectangle and Text maintain unmonitored raw Gradient pointer causing dangling dereference during paint

  • File: Rectangle.h:49, Text.h:150
  • Category: Ownership & Lifecycle
  • Confidence: 90/100
  • Finding: Both Rectangle and Text store their assigned gradient as a raw pointer Gradient* m_gradient = nullptr;. They connect to Gradient::updated, but do not observe
  QObject::destroyed and do not wrap the reference in QPointer<Gradient>. If the Gradient object is destroyed while still assigned, m_gradient is left dangling and
  dereferenced during subsequent render passes (Rectangle::paint line 70, Text::paint line 425).
  • Trace: Confirmed in Rectangle.h:49, Rectangle.cpp:20-29, 70, Text.h:150, and Text.cpp:218-226, 425-448.
  • Mitigation: Change m_gradient to QPointer<Gradient> in both Rectangle and Text, and connect to destroyed to reset the pointer.

  #### [D-010] Gradient::m_stops stores raw GradientStop pointers without lifecycle tracking

  • File: Gradient.h:57
  • Category: Ownership & Lifecycle
  • Confidence: 88/100
  • Finding: Gradient stores stops as QList<GradientStop*> m_stops;. When addStop(stop) is invoked via the QML list property, it connects to stop->updated and appends to
  m_stops, but does not establish parent-child ownership or observe stop destruction. If an assigned GradientStop is deleted, m_stops retains a dangling pointer, crashing
  colorAt() during sorting and clearStops() during teardown.
  • Trace: Verified in Gradient.h:57 and Gradient.cpp:23-28, 45-51, 67-119.
  • Mitigation: Store stops as QList<QPointer<GradientStop>> or connect to each stop's QObject::destroyed signal to prune it from m_stops.

  #### [D-011] TerminalMessageRouter unbounded message accumulation during alternate screen session

  • File: TerminalMessageRouter.cpp:85
  • Category: Ownership & Lifecycle
  • Confidence: 85/100
  • Finding: In TerminalMessageRouter::routeMessage, when an alternate screen session is active, all log messages (QtDebug, QtWarning, QtInfo, QtCritical) are appended to
  router.messages (QList<BufferedMessage>) without any upper bound, size limit, or eviction policy. A long-running terminal application emitting repeated logs causes
  indefinite heap growth in a static process-global structure.
  • Trace: Verified in TerminalMessageRouter.cpp:32, 84-95.
  • Mitigation: Introduce a maximum capacity (e.g. 500-1000 messages) with FIFO eviction, outputting a truncation notification when entries are dropped.

  #### [D-012] Abstract interface InputScope has only a single implementation (YAGNI violation)

  • File: InputScope.h:7
  • Category: Ownership & Lifecycle
  • Confidence: 88/100
  • Finding: InputScope defines an abstract interface with 8 pure virtual methods. Across the entire Qamui codebase, Popup (Popup.h:11) is the sole class that implements
  InputScope. Terminal::orderedInputScopes() and topModalScope() simply cast and delegate directly to Popup collections, introducing unnecessary polymorphic indirection.
  • Trace: Confirmed across all headers and source files in src/.
  • Mitigation: Remove the abstract InputScope interface and allow InputRouter and Terminal to interact directly with Popup.

  #### [D-013] Clock virtual mode allocates unparented QObjects to raw pointer members without cleanup

  • File: Clock.cpp:69
  • Category: Ownership & Lifecycle
  • Confidence: 82/100
  • Finding: In Clock::setVirtual(), m_driver and m_driverKeepalive are allocated via new VirtualAnimationDriver() and new QVariantAnimation() without parents. They are
  stored as raw pointers in Clock (Clock_p.h:66), and Clock::~Clock() is defaulted without deleting them, leaking resources when virtual clock mode is toggled or reset in
  test fixtures.
  • Trace: Verified in Clock.cpp:69-77 and Clock_p.h:58, 66-67.
  • Mitigation: Parent the animation objects to the Clock instance or manage them with std::unique_ptr.

  #### [D-014] State and Transition child lists hold unmonitored raw pointers

  • File: State.h:78, Transition.h:47
  • Category: Ownership & Lifecycle
  • Confidence: 82/100
  • Finding: State stores QList<QObject*> m_changesList; and Transition stores QList<Animation*> m_animationsList;. Items appended via QML list properties are stored as raw
  pointers without parent ownership, without QPointer, and without listening to QObject::destroyed, allowing dangling pointer dereferences if items are deleted externally.
  • Trace: Confirmed in State.h:78, State.cpp:60-65, Transition.h:47, and Transition.cpp:37-42.
  • Mitigation: Use QList<QPointer<QObject>> in State and QList<QPointer<Animation>> in Transition, or listen to &QObject::destroyed to prune dead entries.

  #### [D-015] Data race on non-atomic m_stopped in TerminalEngine::requestRender()

  • File: TerminalEngine.cpp:254
  • Category: Thread Safety
  • Confidence: 90/100
  • Finding: TerminalEngine::requestRender() is designed to be invokable across threads (Q_INVOKABLE, atomic m_renderPending.exchange(true), and Qt::QueuedConnection
  dispatch). However, line 254 checks if (m_stopped) return;, where m_stopped is declared as plain non-atomic bool m_stopped = false; in TerminalEngine.h:269.
  TerminalEngine::stop() writes m_stopped = true; without synchronization on the main thread, causing an unsynchronized data race and undefined behavior under C++20
  ([intro.races]).
  • Trace: Verified in TerminalEngine.h:269, TerminalEngine.cpp:200 (stop()), and TerminalEngine.cpp:254 (requestRender()).
  • Mitigation: Change m_stopped to std::atomic<bool> m_stopped{false};.

  #### [D-016] Main-thread-only Clock::isVirtual() queried from worker threads before thread affinity check

  • File: TerminalEngine.cpp:259
  • Category: Thread Safety
  • Confidence: 85/100
  • Finding: In TerminalEngine::requestRender(), the frame-pacing condition is written as if (!Clock::isVirtual() && QThread::currentThread() == thread() && ...). Because
  && evaluates left-to-right, calling requestRender() from a worker thread executes !Clock::isVirtual() before verifying thread affinity. Clock is explicitly documented in
  Clock_p.h:29 as Main-thread only, violating the subsystem's single-thread affinity contract.
  • Trace: Traced Clock_p.h:29, Clock.cpp:42-45, and TerminalEngine.cpp:259.
  • Mitigation: Reorder the condition so thread affinity is evaluated first: if (QThread::currentThread() == thread() && !Clock::isVirtual() && ...).

  #### [D-017] Non-atomic s_pipe accessed inside asynchronous signal handlers (SIGWINCH/SIGCONT)

  • File: PosixTerminalBackend.cpp:48
  • Category: Thread Safety
  • Confidence: 88/100
  • Finding: In SignalBridge, the self-pipe descriptors are declared as static int s_pipe[2];. In SignalBridge::notify() (line 48), s_pipe[1] is checked: if (s_pipe[1] != -
  1). notify() is invoked directly inside handler(int) (SIGWINCH) and contHandler(int) (SIGCONT). Meanwhile, the main thread modifies s_pipe in install() and closePipe().
  Reading plain int s_pipe[1] from an asynchronous signal handler while the main thread may mutate it violates POSIX.1-2017 §2.4.3 and C++20 [support.signal].
  • Trace: Traced PosixTerminalBackend.cpp:48, 83, 138-146, 160, 257.
  • Mitigation: Change s_pipe to std::atomic<int> s_pipe[2] or volatile sig_atomic_t s_pipe[2].

  #### [D-018] Non-atomic fields of EmergencyState accessed in fatal signal handlers without memory barrier

  • File: PosixTerminalBackend.cpp:177
  • Category: Thread Safety
  • Confidence: 82/100
  • Finding: EmergencyState g_emergency stores file descriptors and termios structs for restoring terminal state upon crash. While active is volatile sig_atomic_t, members
  outputFd, inputFd, original, and raw are plain non-atomic structures. When a synchronous fatal signal (SIGSEGV, SIGBUS, SIGFPE, SIGILL) occurs on a worker thread,
  fatalHandler(signo) reads these fields without memory synchronization, risking reading torn or unpropagated values.
  • Trace: Confirmed in PosixTerminalBackend.cpp:172-181, 218-232, 271-278.
  • Mitigation: Insert std::atomic_thread_fence(std::memory_order_release) in installEmergencyHandlers() prior to setting g_emergency.active = 1, and
  std::atomic_thread_fence(std::memory_order_acquire) in restoreTerminalFromSignal().

  #### [D-019] Non-const Q_PROPERTY READ accessor on Rectangle::border

  • File: Rectangle.h:32
  • Category: API & C++ Correctness
  • Confidence: 95/100
  • Finding: The border property declared via Q_PROPERTY(qamui::Border* border READ border CONSTANT) has a non-const getter Border* border() { return &m_border; }. In the
  Qt meta-object system, reading properties on const QObject* instances expects const member function accessors; invoking a non-const READ accessor through meta-property
  inspection on a const pointer invokes undefined behavior.
  • Trace: Verified Rectangle.h:17 and Rectangle.h:32.
  • Mitigation: Mark border() const as a const member function returning const Border*, or provide overloaded accessors (const Border* border() const and Border* border()).

  #### [D-020] Non-const Q_PROPERTY READ accessors for QQmlListProperty

  • File: Item.h:109
  • Category: API & C++ Correctness
  • Confidence: 92/100
  • Finding: Multiple classes declare QQmlListProperty getters as non-const member functions despite being referenced in Q_PROPERTY(QQmlListProperty<...> ... READ ...)
  declarations:
      • Item::childrenProperty() (Item.h:109)
      • Keys::forwardTo() (Keys.h:38)
      • State::changes() (State.h:63)
      • Transition::animations() (Transition.h:30)
      • Gradient::stops() (Gradient.h:32)
  • Trace: Verified in Item.h, Keys.h, State.h, Transition.h, and Gradient.h.
  • Mitigation: Add const qualification to these QQmlListProperty accessors across all five classes.

  #### [D-021] Conflicting TextFormat enum duplicated across Paragraph and Text

  • File: Paragraph.h:37, Text.h:36
  • Category: API & C++ Correctness
  • Confidence: 95/100
  • Finding: Both Paragraph and Text define an unscoped enum named TextFormat with conflicting integer mappings:
      • Paragraph: AutoText = 0, PlainText = 1, RichText = 2
      • Text: PlainText = 0, RichText = 1, AnsiText = 2, AutoText = 3
      Having identical enum names in core visual items representing the same domain concept with inverted integer values creates severe inconsistency and prevents uniform
      formatting handling.
  • Trace: Compared Paragraph.h:37-41 with Text.h:36-41.
  • Mitigation: Centralize TextFormat in the qamui namespace (e.g. QamuiGlobal.h) or adopt standard Qt::TextFormat.

  #### [D-022] Fragmented orientation vocabulary across Gradient, LinearLayout, and ListView

  • File: Gradient.h:24
  • Category: API & C++ Correctness
  • Confidence: 90/100
  • Finding: Orientation is represented using three distinct, conflicting types across Core classes:
      • Gradient: enum Orientation { Vertical, Horizontal } (Vertical = 0)
      • LinearLayout: enum Direction { Horizontal, Vertical } (Horizontal = 0)
      • ListView: Qt::Orientation (Qt::Horizontal = 1, Qt::Vertical = 2)
  • Trace: Inspected enum declarations in Gradient.h:24, LinearLayout.h:14, and ListView.h:53.
  • Mitigation: Standardize on Qt::Orientation (Qt::Horizontal, Qt::Vertical) across all three classes.

  #### [D-023] Fixed project-owned vocabulary held in QString rather than enums (ENM-8)

  • File: Paragraph.h:32, TableView.h:51, Border.h:20
  • Category: API & C++ Correctness
  • Confidence: 92/100
  • Finding: Closed vocabularies are declared as QString rather than strongly typed enums:
      • Paragraph::alignment: compared against "right" and "center"
      • TableView::gridStyle: compared against "double" and "ascii"
      • Border::style: compared against "double", "ascii", "rounded"
      • TableView column alignment: stored as QString and branched on string equality.
  • Trace: Verified property declarations and string comparison call sites in Paragraph.cpp, TableView.cpp, and Rectangle.cpp.
  • Mitigation: Define strongly typed scoped enums (or standard Qt types like Qt::Alignment) exposed via Q_ENUM.

  #### [D-024] Getter with get-prefix on Paragraph::getRenderedLines

  • File: Paragraph.h:85
  • Category: API & C++ Correctness
  • Confidence: 90/100
  • Finding: Paragraph::getRenderedLines(int availW) const uses the get prefix. In Qt API guidelines, get is reserved exclusively for interactive user dialogs or functions
  with out-parameters.
  • Trace: Inspected Paragraph.h:85 and Paragraph.cpp:240.
  • Mitigation: Rename getRenderedLines(int availW) const to renderedLines(int availW) const.

  #### [D-025] Const query method Item::findFocusRequest returns mutable pointer to child item

  • File: Item.h:269
  • Category: API & C++ Correctness
  • Confidence: 88/100
  • Finding: Item::findFocusRequest() const is marked const, but traverses m_visualChildren and returns a mutable Item*. Callers holding a const Item* can obtain a non-
  const pointer to descendant items, circumventing const-correctness.
  • Trace: Inspected Item.h:269 and Item.cpp:335-343.
  • Mitigation: Remove const qualification from findFocusRequest() if it is an internal mutating helper for applyPendingFocus(), or provide a const overload returning const
  Item*.

  #### [D-026] Non-mutating lookup method takeNextDue misnamed with take-prefix and returns mutable pointer from const method

  • File: Clock_p.h:60
  • Category: API & C++ Correctness
  • Confidence: 88/100
  • Finding: Clock::takeNextDue(qint64 timeMs) const violates two naming and correctness rules:
      1. In Qt conventions, methods prefixed with take remove and return the element. takeNextDue does not remove the timer from m_timers.
      2. The method is marked const, but returns a mutable raw pointer ClockTimer* which callers mutate (due->fire()).
  • Trace: Inspected Clock_p.h:60 and Clock.cpp:94-104.
  • Mitigation: Rename takeNextDue to nextDueTimer or findNextDue, and remove const qualification.

  #### [D-027] const local variables returned by value inhibiting move semantics

  • File: TextInput.cpp:315, NumericItemRoles_p.h:56, Clipboard.cpp:111
  • Category: API & C++ Correctness
  • Confidence: 90/100
  • Finding: Local variables declared const (const QString sanitized, const QString streamed, const auto promise) are returned by value, preventing implicit move
  construction and forcing ref-count increment or deep copy.
  • Trace: Verified in TextInput.cpp:315-317, NumericItemRoles_p.h:56-57, and Clipboard.cpp:111, 117-130.
  • Mitigation: Remove the const qualifier from these local return variables.

  #### [D-028] Single-argument default constructors missing explicit keyword

  • File: GridView.h:31, ListView.h:64, Repeater.h:29, src/Qamui/Core/items/Canvas.h:50, 155, 183, 196, 231, 257, 280
  • Category: API & C++ Correctness
  • Confidence: 88/100
  • Finding: Multiple QObject-derived classes declare single-argument constructors with a default parameter (QObject* parent = nullptr) without marking them explicit,
  enabling accidental implicit conversion from any QObject*.
  • Trace: Inspected constructors across GridView.h, ListView.h, Repeater.h, and Canvas.h.
  • Mitigation: Add the explicit keyword to all single-argument constructors taking QObject* parent = nullptr.

  #### [D-029] Missing trailing comma on last enumerator across Core enum definitions

  • File: src/Qamui/Core/items/Terminal.h:35, 40, Cell.h:12, src/Qamui/Core/items/Canvas.h:29, 36, TerminalBackend.h:38
  • Category: API & C++ Correctness
  • Confidence: 85/100
  • Finding: Enum definitions omit trailing commas on their final enumerator, causing unnecessary multi-line diff churn when new enumerators are added.
  • Trace: Verified across enum definitions in Core headers.
  • Mitigation: Add trailing commas after the last enumerator across all enum definitions.

  #### [D-030] Out-of-bounds heap access on 0-width TextGridBuffer in setGrapheme

  • File: TextGridBuffer.cpp:100-105
  • Category: Error Handling
  • Confidence: 95/100
  • Finding: When placing a zero-width grapheme cluster into a buffer with m_width == 0 (e.g. collapsed or initialized empty buffer via resize(0, h)), targetX evaluates to
  -1 (std::min(x, 0) - 1). cellAt(-1, y) asserts in debug builds and performs an out-of-bounds vector access with index SIZE_MAX in release builds.
  • Trace: Traced TextGridBuffer.cpp:100-105 and cellAt() index computation.
  • Mitigation: Add a guard returning 0 if m_width <= 0 || m_height <= 0, or verify m_width > 0 before computing targetX.

  #### [D-031] Missing bounds checks in QQmlListProperty at callbacks

  • File: src/Qamui/Core/items/Item.cpp:72-75, 735-738, 777-780, State.cpp:72, Transition.cpp:49, Gradient.cpp:35
  • Category: Error Handling
  • Confidence: 90/100
  • Finding: QQmlListProperty indexing callbacks (dataAt, stateAt, transitionAt, changeAt, animationAt, stopAt) execute return self->m_container.at(index); without
  validating index >= 0 && index < container.size(), causing crashes on negative or out-of-range indexing from QML.
  • Trace: Traced and contrasted against Item::childrenProperty() (Item.cpp:97), which validates bounds.
  • Mitigation: Add defensive bounds checks if (!self || index < 0 || index >= list.size()) return nullptr; consistently to all QQmlListProperty at callback functions.

  #### [D-032] Undefined bitshift on excess length hex strings in TerminalEngine::parseOsc11Color

  • File: TerminalEngine.cpp:462-472
  • Category: Error Handling
  • Confidence: 85/100
  • Finding: scaleTo8Bit(uint val, qsizetype len) shifts val right by ((len - 2) * 4) bits. If len >= 10, the shift amount is ≥ 32 bits on a 32-bit unsigned integer,
  causing undefined behavior in C++ ([expr.shift]).
  • Trace: Traced TerminalEngine.cpp:462-472.
  • Mitigation: Validate that RGB component string length is between 1 and 4 characters, or clamp len to 4 prior to shifting.

  #### [D-033] Case-sensitive index mismatch corrupting attribute parsing in Paragraph

  • File: src/Qamui/Core/items/Paragraph.cpp:110-115, 128-133
  • Category: Error Handling
  • Confidence: 90/100
  • Finding: Paragraph::parseFormattedText checks tag attributes using lowercase lowerTag.contains(...) but calculates start offsets using case-sensitive tag.indexOf(...).
  Uppercase attributes (e.g. <a HREF="..."> or <font COLOR="...">) produce invalid slice indices (-1 + 5 = 4) and corrupt extracted attribute values.
  • Trace: Traced Paragraph.cpp:110-115, 128-133.
  • Mitigation: Use tag.indexOf(QLatin1String("href="), 0, Qt::CaseInsensitive) when calculating the attribute start offsets.

  #### [D-034] Undefined left-shift on negative values in SGR truecolor parsing in Text

  • File: Text.cpp:107-118
  • Category: Error Handling
  • Confidence: 85/100
  • Finding: SGR 24-bit direct color sequence parser converts color channel tokens via toInt() and left-shifts signed integers without verifying non-negativity or channel
  bounds (r << 16 | g << 8 | b). If a sequence supplies negative values (e.g. 38;2;-1;0;0), shifting a negative signed integer triggers undefined behavior.
  • Trace: Traced Text.cpp:107-118.
  • Mitigation: Clamp channel values r, g, b to [0, 255] using std::clamp or unsigned byte casting prior to bit-shifting.

  #### [D-035] Inconsistent spacing validation accepting negative values in layout containers

  • Files: Column.cpp:11, Row.cpp:11, GridLayout.cpp:32, ListView.cpp:126
  • Category: Error Handling
  • Confidence: 85/100
  • Finding: While LinearLayout and Scrollback sanitize negative spacing using std::max(0, spacing), Column, Row, GridLayout, and ListView accept negative spacing values
  unchecked, distorting positioning and implicit size calculations.
  • Trace: Confirmed setters in Column.cpp, Row.cpp, GridLayout.cpp, and ListView.cpp.
  • Mitigation: Apply std::max(0, spacing) consistently in the setter methods across all layout containers.

  #### [D-036] Regex compilation inside loop during HTML tag parsing in Paragraph

  • File: src/Qamui/Core/items/Paragraph.cpp:113, 131
  • Category: Performance & Quality
  • Confidence: 95/100
  • Finding: Inside Paragraph::parseRichLine(), character scanning repeatedly executes tag.indexOf(QRegularExpression("[\"'>\\s]"), startQuote) whenever an <a> or <font>
  tag is parsed. This compiles the regex automaton on every tag occurrence within the text instead of reusing a compiled regex or using character scanning.
  • Trace: Confirmed in Paragraph.cpp:113, 131. In contrast, Text.cpp:50 correctly uses static const QRegularExpression.
  • Mitigation: Define a static const QRegularExpression endQuoteRegex(QStringLiteral("[\"'>\\s]")) or replace regex matching with a direct character scan.

  #### [D-037] Virtual roleNames() queried and deep-copied on every delegate update in ListView, GridView, Repeater, and TreeView

  • File: NumericItemRoles_p.h:198
  • Category: Performance & Quality
  • Confidence: 95/100
  • Finding: NumericItemRoles::apply() provides an optimized overload accepting a pre-resolved bitmask quint16 claimed. When omitted, the default overload calls
  claimedNames(model), invoking virtual model->roleNames() and deep-copying a QHash<int, QByteArray> on every single call. Header comments in NumericItemRoles_p.h:120-123
  specifically document this cost and instruct views to cache the mask. While TableView caches m_claimedNames, ListView, GridView, Repeater, and TreeView omit the parameter
  across delegate creation, updates, and dataChanged handling.
  • Trace: Confirmed in ListView.cpp:292, 363, 561, GridView.cpp:214, 258, 454, Repeater.cpp:251, 313, 395, and TreeView.cpp:553.
  • Mitigation: Cache quint16 m_claimedNames = NumericItemRoles::claimedNames(model) in ListView, GridView, Repeater, and TreeView, and pass m_claimedNames to all
  NumericItemRoles::apply() calls.

  #### [D-038] Redundant roleNames() queries and un-cached string conversions on every row in Scrollback

  • File: Scrollback.cpp:469
  • Category: Performance & Quality
  • Confidence: 92/100
  • Finding: In Scrollback::appendRows(), delegateForRow(row) is invoked in a loop for every row appended. Inside delegateForRow(), it calls const QHash<int, QByteArray>
  roles = m_model->roleNames(). Additionally, QString::fromUtf8(role.value()) is repeatedly called for every role of every row. Appending 500 rows with 8 roles results in
  500 roleNames() hash copies and 4,000 UTF-8 string allocations.
  • Trace: Confirmed in Scrollback.cpp:312-327 and Scrollback.cpp:460-475.
  • Mitigation: Cache the model's role map (QHash<int, QString>) and pre-resolve the role ID once before entering the append loop.

  #### [D-039] TreeView::fillNodeFromModel queries roleNames() and copies QHash on every node load

  • File: TreeView.cpp:548
  • Category: Performance & Quality
  • Confidence: 90/100
  • Finding: TreeView::fillNodeFromModel() is invoked for every visible node loaded or updated. On each call, it queries const QHash<int, QByteArray> roles = model-
  >roleNames() and converts each role name via QString::fromUtf8(it.value()), allocating a temporary hash copy and strings per node.
  • Trace: Confirmed in TreeView.cpp:548-550. Unlike TableView, which caches m_roleNames and m_roleNameStrings, TreeView has no cached role map.
  • Mitigation: Cache m_roleNames and m_roleNameStrings (QHash<int, QString>) in TreeView when attaching a model, and reuse them in fillNodeFromModel().

  #### [D-040] Repeated full-document string splitting in TextArea during rendering and keystrokes

  • File: TextArea.cpp:305
  • Category: Performance & Quality
  • Confidence: 93/100
  • Finding: In TextArea::paint(), displayText.split('\n') is invoked on every single frame render. Similarly, TextArea::applyEditorChange() (invoked on every
  keystroke/insertion/deletion) and TextArea::positionForLineColumn() call m_editor.text().split('\n'). For a multiline text area, this repeatedly allocates a QStringList
  containing individual heap-allocated QString copies for every line.
  • Trace: Confirmed in TextArea.cpp:25, 147, 305.
  • Mitigation: Replace split('\n') with QStringTokenizer (QStringTokenizer{displayText, u'\n'}) or string view slicing (QStringView).

  #### [D-041] Repeated popup list sorting and container deep-copies on every paint frame, mouse move, and keypress

  • File: Terminal.cpp:141
  • Category: Performance & Quality
  • Confidence: 92/100
  • Finding: Terminal::orderedPopups() returns QList<Popup*> by value, executing std::stable_sort, partitioning into temporary below and group lists, and returning a
  freshly allocated list. This method is called by paintPopups() on every frame, and by InputRouter on every key press and twice on every mouse event. Even when popup state
  is static, input processing churns multiple QList<Popup*> allocations and sorting passes per event.
  • Trace: Confirmed in Terminal.cpp:141-165, 371 and InputRouter.cpp:94, 241, 242.
  • Mitigation: Maintain a cached QList<Popup*> m_orderedPopups in Terminal that is marked dirty and rebuilt only when a popup is added, removed, or has its z-
  order/modality changed.

  #### [D-042] Paragraph::getRenderedLines returns container by value causing per-frame copy in paint

  • File: Paragraph.h:85
  • Category: Performance & Quality
  • Confidence: 90/100
  • Finding: Paragraph::getRenderedLines(int availW) const caches formatted lines in mutable QList<FormattedLine> m_cachedLines, but returns the container by value
  (QList<FormattedLine>). Because FormattedLine is QList<FormattedChar>, calling getRenderedLines() in paint() (line 272) copies the nested container structure on every
  frame of every Paragraph item.
  • Trace: Confirmed in Paragraph.h:85 and Paragraph.cpp:240, 272.
  • Mitigation: Change getRenderedLines() to return const QList<FormattedLine>& and capture it as const QList<FormattedLine>& in paint().

  #### [D-043] Dead member variables maintained in ListView and TreeView

  • File: ListView.h:150, TreeView.h:221
  • Category: Performance & Quality
  • Confidence: 95/100
  • Finding: ListView maintains m_resolvedHighlightColor and m_resolvedHighlightBackgroundColor, resolving them via ColorMapper::fromString() in its constructor and
  property setters, but neither variable is ever read anywhere in ListView or delegate styling. Similarly, TreeView declares and maintains m_resolvedBranchColor with
  ColorMapper::fromString() updates, but never reads it during tree rendering or branch drawing.
  • Trace: Confirmed in ListView.h:150, 152, ListView.cpp:205, 213, TreeView.h:221, and TreeView.cpp:255.
  • Mitigation: Either connect these resolved color members to their corresponding delegate/painting routines, or remove the dead member variables and resolution logic.

  #### [D-044] Missing re-entrancy guard and premature signal emission in Item::setState

  • File: Item.cpp:819
  • Category: Performance & Quality
  • Confidence: 88/100
  • Finding: Item::setState() updates m_stateName and immediately emits emit stateChanged(); at line 824 before reverting old state properties, applying new state
  properties, or starting transition animations. If a slot or QML binding connected to stateChanged triggers a re-entrant setState(), the inner call mutates state, then the
  outer call resumes with stale local variables, clobbering the inner state with superseded property changes and animations.
  • Trace: Confirmed in Item.cpp:819-860.
  • Mitigation: Add a re-entrancy guard to Item::setState() and defer emitting stateChanged() until after property overrides have been applied and transitions have been
  configured.

  #### [D-045] TableView::paint iterates over non-const COW container without std::as_const

  • File: src/Qamui/Core/items/TableView.cpp:1044, 1069
  • Category: Performance & Quality
  • Confidence: 85/100
  • Finding: In TableView::paint(), range-for loops iterate over m_geometry using for (const auto &geometry : m_geometry) at lines 1044 and 1069. Because TableView::paint()
  is non-const, range-for over a non-const Qt container invokes non-const begin(), which forces a COW detach and deep copy if m_geometry is shared.
  • Trace: Confirmed in TableView.cpp:1044, 1069. Lines 967 and 1256 in the same file correctly wrap the container with std::as_const(m_geometry).
  • Mitigation: Wrap m_geometry in std::as_const(m_geometry) in both loops.

  #### [D-046] Duplicated model parsing and magic number clamping between ListView and GridView

  • File: ListView.cpp:515
  • Category: Performance & Quality
  • Confidence: 90/100
  • Finding: ListView::resetModel() and GridView::resetModel() share an identical 20-line copy-pasted block for extracting QJSValue arrays and QVariantList models,
  including an arbitrary hardcoded clamp constant 10000, reserving length, populating m_listValues, and extracting model roles into m_roleMap.
  • Trace: Confirmed by comparing ListView.cpp:515-534 with GridView.cpp:396-417.
  • Mitigation: Extract model unpacking and role map extraction into shared helper functions in DelegateUtils.h, and define a named constant for the item limit (e.g.
  DelegateUtils::MaxMaterializedModelItems).
  ──────
  ### Investigation targets (human verification needed)

  #### [I-001] SignalBridge::instance() thread check prevents listener cleanup when PosixTerminalBackend is destroyed on worker thread

  • File: PosixTerminalBackend.cpp:485
  • Category: Thread Safety
  • Confidence: 75/100
  • Finding: In PosixTerminalBackend::stopInput() (called from ~PosixTerminalBackend()), cleanup checks if (SignalBridge* bridge = SignalBridge::instance()). However,
  SignalBridge::instance() returns nullptr whenever QThread::currentThread() != qApp->thread(). If a backend is destroyed on a worker thread, bridge->removeListener(this)
  is bypassed, leaving a dangling pointer in SignalBridge::m_listeners and leaking the self-pipe descriptors.
  • Unverified because: In standard application flows, backends are owned by TerminalEngine on the main GUI thread; cross-thread destruction requires explicit backend
  transfers in user code or tests.
  • How to verify: Verify whether backend instances can be instantiated and torn down across threads in background worker pipelines or test runners.

  #### [I-002] Missing Q_DISABLE_COPY_MOVE on major polymorphic QObject subclasses

  • File: Terminal.h:54, TerminalEngine.h:60
  • Category: Ownership & Lifecycle
  • Confidence: 75/100
  • Finding: Prominent polymorphic QObject subclasses (Terminal, TerminalEngine, TextInput, TableView, TreeView, Canvas) do not declare Q_DISABLE_COPY_MOVE, whereas Item,
  Loader, Repeater, and ScrollView explicitly do.
  • Unverified because: QObject's deleted copy constructor already suppresses copy and move construction on derived classes by default; however, omitting the macro is
  inconsistent with Qt 6 conventions.
  • How to verify: Check whether Qamui architectural conventions mandate explicit Q_DISABLE_COPY_MOVE across all public exported QObject types.

  #### [I-003] Missing early return check on Polygon::setPoints

  • File: Canvas.cpp:388
  • Category: Performance & Quality
  • Confidence: 75/100
  • Finding: Polygon::setPoints(const QVariantList& points) immediately clears m_parsedPoints and parses all points without first checking if (m_points == points) return;.
  Re-evaluating bindings that re-assign identical point lists reparses every point and triggers repaint.
  • Unverified because: Value equality comparisons on QVariantList containing QJSValue instances may have edge cases under Qt's variant equality rules.
  • How to verify: Benchmark binding re-evaluation on Polygon::setPoints and confirm value equality behavior with JS array variants.

  #### [I-004] TreeView::handleModelDataChanged lacks validity guard on topLeft and bottomRight

  • File: TreeView.cpp:635
  • Category: Model Contracts
  • Confidence: 72/100
  • Finding: In TreeView::handleModelDataChanged, there is no topLeft.isValid() && bottomRight.isValid() guard before calling topLeft.parent() and looping from topLeft.
  row() to bottomRight.row(). If topLeft is invalid (row() == -1), m_cppModel->index(-1, 0, parent) is called.
  • Unverified because: Well-behaved models do not emit dataChanged with invalid topLeft, and m_visibleLookup prevents downstream null dereferences.
  • How to verify: Test TreeView with a test model emitting dataChanged(QModelIndex(), QModelIndex()) or under QAbstractItemModelTester.

  #### [I-005] LayoutAttached signals only connected at instantiation time

  • File: Layout.cpp:9
  • Category: Ownership & Lifecycle
  • Confidence: 72/100
  • Finding: Layout::qmlAttachedProperties() queries item->parentItem() once at creation time and connects attached->changed to parent->markDirty(). During QML construction,
  attached properties are created before items are attached to parent layouts, leaving item->parentItem() null. If an item is reparented or attached later,
  LayoutAttached::changed is never observed by the layout.
  • Unverified because: Layout containers might re-evaluate item sizes during explicit layout passes regardless of the changed signal.
  • How to verify: Test dynamic modification of Layout.preferredWidth on a child item after component completion to observe whether the parent layout automatically
  relayouts.

  #### [I-006] const QMetaObject::Connection preventing move into tracking map

  • File: Terminal.cpp:218
  • Category: API & C++ Correctness
  • Confidence: 70/100
  • Finding: In Terminal::setMouseMotionRequested, the connection handle is declared as const QMetaObject::Connection connection = connect(...) before being inserted into
  m_mouseMotionRequesters.insert(requester, connection);. Declaring the connection const prevents move insertion, forcing a copy.
  • Unverified because: QMetaObject::Connection is an inexpensive copyable handle, so copying does not cause functional errors.
  • How to verify: Verify if moving the connection into m_mouseMotionRequesters improves clarity and avoids handle duplication.

  #### [I-007] TreeView setRootNode vs setModel state inconsistency

  • File: TreeView.cpp:120
  • Category: Performance & Quality
  • Confidence: 70/100
  • Finding: TreeView::setRootNode(const QVariant &node) updates m_rootNodeVariant and calls setModel(node). However, calling setModel() directly does not clear or
  synchronize m_rootNodeVariant. If a user assigns rootNode and subsequently assigns model, m_rootNodeVariant retains stale state.
  • Unverified because: rootNode may be a legacy convenience property with intentionally separate retention.
  • How to verify: Check TreeView test suite and QML property interaction expectations between rootNode and model.

  #### [I-008] Unconditional NumericItemRoles::apply on narrow dataChanged in views

  • File: ListView.cpp:560, GridView.cpp:453, Repeater.cpp:250
  • Category: Model Contracts
  • Confidence: 68/100
  • Finding: When dataChanged is emitted with specific changedRoles (e.g. only {Qt::DisplayRole}), views filter m_roleMap lookups, but then unconditionally invoke
  NumericItemRoles::apply(..., true). This re-reads all standard roles (ForegroundRole, BackgroundRole, TextAlignmentRole, CheckStateRole, flags()) for every row even when
  none of those standard roles changed.
  • Unverified because: This is an over-query performance inefficiency rather than a visual corruption bug.
  • How to verify: Profile model data() invocation counts when models emit dataChanged for custom roles.

  #### [I-009] Non-atomic global session flags in TerminalEngine

  • File: TerminalEngine.cpp:53
  • Category: Thread Safety
  • Confidence: 68/100
  • Finding: TerminalEngine::requestTestSession(bool liveOutput) modifies static process-global flags s_requestedTestSession and s_requestedLiveOutput, which are read in
  TerminalEngine::TerminalEngine(). Neither variable is atomic or guarded by a mutex.
  • Unverified because: Test frameworks typically invoke requestTestSession() sequentially on the main thread prior to terminal instantiation.
  • How to verify: Check whether multi-threaded test runners invoke requestTestSession() concurrently with engine instantiation.

  #### [I-010] TerminalBackend::setFactory() lacks thread synchronization for custom backend registration

  • File: TerminalBackend.cpp:27
  • Category: Thread Safety
  • Confidence: 65/100
  • Finding: TerminalBackend::setFactory(Factory factory) updates static TerminalBackend::Factory factory; by value move without synchronization, while
  createTerminalBackend() reads it.
  • Unverified because: Setting a custom backend factory is typically performed once during application initialization before worker threads are spawned.
  • How to verify: Check whether test fixtures dynamically swap backend factories while background engine threads are active.
  ──────
  ### Summary

   Category                          |              Lint               |              Deep               |           Investigate           |             Total
  -----------------------------------|---------------------------------|---------------------------------|---------------------------------|--------------------------------
   Model Contracts                   |                0                |                6                |                2                |               8
   Ownership & Lifecycle             |                7                |                8                |                2                |               17
   Thread Safety                     |                0                |                4                |                3                |               7
   API, Naming & C++ Correctness     |               19                |               11                |                1                |               31
   Error Handling & Validation       |                0                |                6                |                0                |               6
   Performance & Code Quality        |               12                |               11                |                2                |               25
   Headers & Windows Macro Hygiene   |               128               |                0                |                0                |              128
   Timeout & Interval Types          |               27                |                0                |                0                |               27
   Deprecated Methods & Types        |               28                |                0                |                0                |               28
   Syntax & Initialization (VAR/INC) |               18                |                0                |                0                |               18
   Total                             |               239               |               46                |               10                |              295

  Findings below confidence 60 were suppressed entirely.