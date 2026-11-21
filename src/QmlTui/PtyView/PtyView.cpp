#include <QmlTui/PtyView/PtyView.h>

#include <QmlTui/Core/Engine/KeyEvent.h>
#include <QmlTui/Core/Engine/MouseEvent.h>
#include <QmlTui/Core/Items/Terminal.h>
#include <QmlTui/PtyView/UnixPtySession.h>
#include <QmlTui/PtyView/VTermAdapter.h>

#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QTimer>

#if defined(Q_OS_UNIX)
#include <pwd.h>
#include <unistd.h>
#endif

#include <algorithm>

namespace qmltui {

QString PtyView::defaultUserShell() {
    QString envShell = qEnvironmentVariable("SHELL").trimmed();
    if (!envShell.isEmpty() && QFile::exists(envShell)) return envShell;
#if defined(Q_OS_UNIX)
    if (const struct passwd* pw = getpwuid(getuid())) {
        if (pw->pw_shell && pw->pw_shell[0] != '\0') {
            QString pwShell = QString::fromLocal8Bit(pw->pw_shell).trimmed();
            if (!pwShell.isEmpty() && QFile::exists(pwShell)) return pwShell;
        }
    }
    if (QFile::exists(QStringLiteral("/bin/bash"))) return QStringLiteral("/bin/bash");
    if (QFile::exists(QStringLiteral("/bin/zsh"))) return QStringLiteral("/bin/zsh");
    if (QFile::exists(QStringLiteral("/bin/sh"))) return QStringLiteral("/bin/sh");
#endif
    return QStringLiteral("/bin/sh");
}

PtyView::PtyView(QObject* parent)
    : Item(parent), m_program(defaultUserShell()), m_adapter(std::make_unique<VTermAdapter>(1, 1)),
      m_session(std::make_unique<UnixPtySession>()) {
    setItemFlag(ItemFlag::BoundedSelfPaint);
    setFocusPolicy(Qt::StrongFocus);
    setClip(true);
    m_adapter->setDefaultColors(m_textColor, m_backgroundColor);
    m_adapter->setScrollbackLimit(m_scrollbackLimit);
    m_adapter->setOutputCallback([this](const QByteArray& bytes) { m_session->write(bytes); });
    m_adapter->setDamageCallback([this] { markDirty(); });
    m_adapter->setBellCallback([this] { emit bell(); });
    m_adapter->setTitleCallback([this](const QString& value) {
        if (!m_allowTitleChanges || m_title == value) return;
        m_title = value;
        emit titleChanged();
    });
    m_adapter->setClipboardCallback([this](const QString& value) {
        if (m_allowClipboardWrite) emit clipboardWriteRequested(value);
    });
    m_adapter->setNotificationCallback([this](const QString& value) {
        if (!m_allowNotifications) return;
        const qsizetype separator = value.indexOf(';');
        if (separator < 0)
            emit notificationRequested(QString(), value);
        else
            emit notificationRequested(value.left(separator), value.mid(separator + 1));
    });
    m_adapter->setMouseModeCallback([this](int mode) {
        if (Terminal* root = terminal())
            root->setMouseMotionRequested(this, mode == VTERM_PROP_MOUSE_MOVE);
    });

    connect(this, &Item::visibleChanged, this, [this] {
        if (visible()) syncSize();
    });

    connect(m_session.get(), &UnixPtySession::started, this, [this] {
        setProcessState(Running);
        emit processIdChanged();
        emit started();
    });
    connect(m_session.get(), &UnixPtySession::dataReceived, this, [this](const QByteArray& bytes) {
        const int before = m_adapter->scrollbackSize();
        m_adapter->feed(bytes);
        const int after = m_adapter->scrollbackSize();
        if ((before > 0) != (after > 0)) syncSize();
        if (before != after) emit scrollbackSizeChanged();
        if (m_scrollOffset > 0) {
            const int added = (std::max)(0, after - before);
            if (added) setScrollOffset(m_scrollOffset + added);
        }
    });
    connect(m_session.get(), &UnixPtySession::finished, this, [this](int code) {
        m_exitCode = code;
        emit exitCodeChanged();
        setProcessState(NotRunning);
        emit processIdChanged();
        emit finished(code);
    });
    connect(m_session.get(), &UnixPtySession::errorOccurred, this, &PtyView::setError);
    connect(this, &Item::activeFocusChanged, this, [this] {
        if (m_adapter) m_adapter->setFocused(activeFocus());
    });
}

PtyView::~PtyView() {
    disconnect(this, &Item::activeFocusChanged, this, nullptr);
    m_session->disconnect(this);
    if (Terminal* root = terminal()) root->setMouseMotionRequested(this, false);
}

#define QMLTUI_SIMPLE_SETTER(Type, Name, Member, Signal)                                           \
    void PtyView::Name(Type value) {                                                               \
        if (Member == value) return;                                                               \
        Member = value;                                                                            \
        emit Signal();                                                                             \
    }

QMLTUI_SIMPLE_SETTER(const QString&, setProgram, m_program, programChanged)
QMLTUI_SIMPLE_SETTER(const QStringList&, setArguments, m_arguments, argumentsChanged)
QMLTUI_SIMPLE_SETTER(const QString&, setWorkingDirectory, m_workingDirectory,
                     workingDirectoryChanged)
QMLTUI_SIMPLE_SETTER(const QVariantMap&, setEnvironment, m_environment, environmentChanged)
QMLTUI_SIMPLE_SETTER(const QString&, setTerminalType, m_terminalType, terminalTypeChanged)

#undef QMLTUI_SIMPLE_SETTER

void PtyView::setInheritEnvironment(bool value) {
    if (m_inheritEnvironment == value) return;
    m_inheritEnvironment = value;
    emit inheritEnvironmentChanged();
}

void PtyView::setAutoStart(bool value) {
    if (m_autoStart == value) return;
    m_autoStart = value;
    emit autoStartChanged();
    if (m_componentComplete && value && !m_program.isEmpty() && !m_autoStartAttempted) {
        m_autoStartAttempted = true;
        QTimer::singleShot(0, this, &PtyView::start);
    }
}

qint64 PtyView::processId() const {
    return m_session->processId();
}

void PtyView::setTextColor(Color value) {
    if (m_textColor == value) return;
    m_textColor = value;
    m_adapter->setDefaultColors(m_textColor, m_backgroundColor);
    emit textColorChanged();
    markDirty();
}

void PtyView::setBackgroundColor(Color value) {
    if (m_backgroundColor == value) return;
    m_backgroundColor = value;
    m_adapter->setDefaultColors(m_textColor, m_backgroundColor);
    emit backgroundColorChanged();
    markDirty();
}

void PtyView::setPalette(const QList<Color>& value) {
    if (m_palette == value) return;
    m_palette = value;
    m_adapter->setPalette(m_palette);
    emit paletteChanged();
    markDirty();
}

void PtyView::setScrollbackLimit(int value) {
    value = (std::max)(0, value);
    if (m_scrollbackLimit == value) return;
    const int before = m_adapter->scrollbackSize();
    m_scrollbackLimit = value;
    m_adapter->setScrollbackLimit(value);
    emit scrollbackLimitChanged();
    if ((before > 0) != (m_adapter->scrollbackSize() > 0)) syncSize();
    if (before != m_adapter->scrollbackSize()) emit scrollbackSizeChanged();
    setScrollOffset(m_scrollOffset);
}

void PtyView::setScrollOffset(int value) {
    value = (std::clamp)(value, 0, m_adapter->scrollbackSize());
    if (m_scrollOffset == value) return;
    m_scrollOffset = value;
    emit scrollOffsetChanged();
    markDirty();
}

int PtyView::scrollbackSize() const {
    return m_adapter->scrollbackSize();
}

void PtyView::setScrollBarVisible(bool value) {
    if (m_scrollBarVisible == value) return;
    m_scrollBarVisible = value;
    syncSize();
    emit scrollBarVisibleChanged();
    markDirty();
}

void PtyView::setAllowTitleChanges(bool value) {
    if (m_allowTitleChanges == value) return;
    m_allowTitleChanges = value;
    emit allowTitleChangesChanged();
}

void PtyView::setAllowClipboardWrite(bool value) {
    if (m_allowClipboardWrite == value) return;
    m_allowClipboardWrite = value;
    emit allowClipboardWriteChanged();
}

void PtyView::setAllowNotifications(bool value) {
    if (m_allowNotifications == value) return;
    m_allowNotifications = value;
    emit allowNotificationsChanged();
}

void PtyView::start() {
    if (m_processState != NotRunning) {
        setError(QStringLiteral("Terminal process is already active"));
        return;
    }
    QString prog = m_program.trimmed();
    QStringList args = m_arguments;
    if (prog.isEmpty()) prog = defaultUserShell();
    if (prog.isEmpty()) {
        setError(QStringLiteral("program must not be empty"));
        return;
    }
    if (!QFileInfo::exists(prog) && QStandardPaths::findExecutable(prog).isEmpty()) {
        const QStringList split = QProcess::splitCommand(prog);
        if (split.size() > 1) {
            prog = split.first();
            args = split.mid(1) + args;
        }
    }

    QProcessEnvironment processEnvironment =
        m_inheritEnvironment ? QProcessEnvironment::systemEnvironment() : QProcessEnvironment();
    for (auto iterator = m_environment.cbegin(); iterator != m_environment.cend(); ++iterator) {
        if (iterator.key().isEmpty() || iterator.key().contains('=')) {
            setError(QStringLiteral("Invalid environment variable name: %1").arg(iterator.key()));
            return;
        }
        processEnvironment.insert(iterator.key(), iterator.value().toString());
    }
    processEnvironment.insert(QStringLiteral("TERM"), m_terminalType.isEmpty()
                                                          ? QStringLiteral("xterm-256color")
                                                          : m_terminalType);
    processEnvironment.insert(QStringLiteral("COLORTERM"), QStringLiteral("truecolor"));
    processEnvironment.insert(QStringLiteral("TERM_PROGRAM"), QStringLiteral("qmltui"));

    if (m_exitCode != -1) {
        m_exitCode = -1;
        emit exitCodeChanged();
    }
    if (!m_errorString.isEmpty()) {
        m_errorString.clear();
        emit errorStringChanged();
    }
    m_adapter->reset();
    const int initialRows = (std::max)(1, height() > 1 ? height() : 24);
    const int initialCols = height() > 1 && width() > 1 ? effectiveColumns()
                                                        : (std::max)(1, width() > 1 ? width() : 80);
    m_lastSyncedRows = initialRows;
    m_lastSyncedColumns = initialCols;
    m_adapter->resize(initialRows, initialCols);
    setScrollOffset(0);
    setProcessState(Starting);
    if (!m_session->start(prog, args, m_workingDirectory, processEnvironment, initialRows,
                          initialCols))
        setProcessState(NotRunning);
}

void PtyView::terminate() {
    if (m_processState == NotRunning) return;
    setProcessState(Stopping);
    m_session->terminate();
}

void PtyView::kill() {
    if (m_processState == NotRunning) return;
    setProcessState(Stopping);
    m_session->kill();
}

void PtyView::sendText(const QString& text) {
    if (m_processState != Running) {
        setError(QStringLiteral("Cannot send text while the terminal is not running"));
        return;
    }
    m_session->write(text.toUtf8());
}

void PtyView::clearScrollback() {
    const int before = m_adapter->scrollbackSize();
    m_adapter->clearScrollback();
    if ((before > 0) != (m_adapter->scrollbackSize() > 0)) syncSize();
    if (before != m_adapter->scrollbackSize()) emit scrollbackSizeChanged();
    setScrollOffset(0);
}

void PtyView::scrollToBottom() {
    setScrollOffset(0);
}

void PtyView::classBegin() {
    m_componentComplete = false;
}

void PtyView::componentComplete() {
    m_componentComplete = true;
    syncSize();
    if (m_autoStart && !m_program.isEmpty() && !m_autoStartAttempted) {
        m_autoStartAttempted = true;
        QTimer::singleShot(0, this, &PtyView::start);
    }
}

void PtyView::setWidth(int value) {
    Item::setWidth(value);
    syncSize();
}

void PtyView::setHeight(int value) {
    Item::setHeight(value);
    syncSize();
}

void PtyView::paint(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) const {
    const int w = (std::max)(0, width());
    const int h = (std::max)(0, height());
    const int scrollback = m_adapter->scrollbackSize();
    const bool drawScrollbar = m_scrollBarVisible && (scrollback > 0) && (w > 2);
    const int contentWidth = drawScrollbar ? (w - 1) : w;

    m_adapter->render(buffer, absX, absY, clipRect, contentWidth, h, m_scrollOffset);

    if (drawScrollbar && h > 0) {
        const int totalLines = scrollback + h;
        const int sbH = (std::max)(1, (h * h) / totalLines);
        const int maxScroll = scrollback;
        const int sbPos = ((maxScroll - m_scrollOffset) * (h - sbH)) / (std::max)(1, maxScroll);
        const int sbX = absX + w - 1;

        for (int dy = 0; dy < h; ++dy) {
            const int cy = absY + dy;
            if (clipRect.contains(sbX, cy)) {
                if (sbX >= 0 && sbX < buffer.width() && cy >= 0 && cy < buffer.height()) {
                    const bool isThumb = (dy >= sbPos && dy < sbPos + sbH);
                    buffer.splitWideGlyphSpan(sbX, cy, 1);
                    Cell& cell = buffer.cellAt(sbX, cy);
                    cell.fgColor = Color::DefaultTag;
                    cell.bgColor = Color::DefaultTag;
                    cell.setCharacter(isThumb ? "\xE2\x96\x88" : "\xE2\x96\x91");
                    cell.displayWidth = 1;
                    cell.continuation = false;
                    cell.attributes = isThumb ? TextAttribute::None : TextAttribute::Dim;
                    cell.linkId = 0;
                }
            }
        }
    }
}

void PtyView::keyPressEvent(KeyEvent* event) {
    if (m_processState != Running) return;

    // Shift+PageUp / Shift+PageDown / Shift+Up / Shift+Down local scrollback navigation
    if ((event->modifiers() & Qt::ShiftModifier) && event->key() == Qt::Key_PageUp) {
        setScrollOffset(m_scrollOffset + (std::max)(1, height() - 2));
        event->setAccepted(true);
        return;
    }
    if ((event->modifiers() & Qt::ShiftModifier) && event->key() == Qt::Key_PageDown) {
        setScrollOffset(m_scrollOffset - (std::max)(1, height() - 2));
        event->setAccepted(true);
        return;
    }
    if ((event->modifiers() & Qt::ShiftModifier) && event->key() == Qt::Key_Up) {
        setScrollOffset(m_scrollOffset + 1);
        event->setAccepted(true);
        return;
    }
    if ((event->modifiers() & Qt::ShiftModifier) && event->key() == Qt::Key_Down) {
        setScrollOffset(m_scrollOffset - 1);
        event->setAccepted(true);
        return;
    }

    if (event->isPaste())
        m_adapter->sendPaste(event->text());
    else
        m_adapter->sendKey(event->key(), event->text(), event->modifiers());
    event->setAccepted(true);
    if (m_scrollOffset != 0) setScrollOffset(0);
}

void PtyView::mousePressEvent(MouseEvent* event) {
    if (!focus()) setFocus(true);

    if (m_scrollBarVisible && m_adapter->scrollbackSize() > 0 && event->x() >= width() - 1) {
        const int h = (std::max)(1, height());
        const int totalLines = m_adapter->scrollbackSize() + h;
        const int sbH = (std::max)(1, (h * h) / totalLines);
        const int maxScroll = m_adapter->scrollbackSize();
        const int clickY = (std::clamp)(event->y(), 0, h - 1);
        const int targetRatio = (clickY * maxScroll) / (std::max)(1, h - sbH);
        setScrollOffset((std::clamp)(maxScroll - targetRatio, 0, maxScroll));
        event->setAccepted(true);
        return;
    }

    if (m_processState == Running && m_adapter->mouseReporting()) {
        m_adapter->sendMouseMove(event->y(), event->x(), event->modifiers());
        m_adapter->sendMouseButton(event->nativeButton(), true, event->modifiers());
        event->setAccepted(true);
    }
}

void PtyView::mouseReleaseEvent(MouseEvent* event) {
    if (m_processState == Running && m_adapter->mouseReporting()) {
        m_adapter->sendMouseMove(event->y(), event->x(), event->modifiers());
        m_adapter->sendMouseButton(event->nativeButton(), false, event->modifiers());
        event->setAccepted(true);
    }
}

void PtyView::mouseMoveEvent(MouseEvent* event) {
    if (m_processState == Running && m_adapter->mouseReporting()) {
        m_adapter->sendMouseMove(event->y(), event->x(), event->modifiers());
        event->setAccepted(true);
    }
}

void PtyView::wheelEvent(MouseEvent* event) {
    if (event->wheelDeltaY() == 0) return;
    const bool localScroll = (event->modifiers() & Qt::ShiftModifier) ||
                             !m_adapter->mouseReporting() || m_processState != Running;
    if (localScroll)
        setScrollOffset(m_scrollOffset + event->wheelDeltaY());
    else
        m_adapter->sendWheel(event->wheelDeltaY(), event->modifiers());
    event->setAccepted(true);
}

void PtyView::setProcessState(ProcessState value) {
    if (m_processState == value) return;
    const bool wasRunning = running();
    m_processState = value;
    emit processStateChanged();
    if (wasRunning != running()) emit runningChanged();
}

void PtyView::setError(const QString& error) {
    if (m_errorString != error) {
        m_errorString = error;
        emit errorStringChanged();
    }
    if (m_processState == Starting) setProcessState(NotRunning);
    emit errorOccurred(error);
}

int PtyView::effectiveColumns() const {
    const int w = (std::max)(1, width());
    return (m_scrollBarVisible && m_adapter->scrollbackSize() > 0 && w > 2) ? (w - 1) : w;
}

void PtyView::syncSize() {
    if (height() <= 1 || width() <= 1) return;
    const int rows = (std::max)(1, height());
    const int columns = effectiveColumns();
    if (rows == m_lastSyncedRows && columns == m_lastSyncedColumns) return;
    m_lastSyncedRows = rows;
    m_lastSyncedColumns = columns;
    m_adapter->resize(rows, columns);
    m_session->resize(rows, columns);
}

} // namespace qmltui
