#include "TerminalView.h"

#include "../engine/KeyEvent.h"
#include "../engine/MouseEvent.h"
#include "../items/Terminal.h"
#include "UnixPtySession.h"
#include "VTermAdapter.h"

#include <QProcessEnvironment>
#include <QTimer>

#include <algorithm>

namespace qtui {

TerminalView::TerminalView(QObject *parent)
    : Item(parent), m_adapter(std::make_unique<VTermAdapter>(1, 1)),
      m_session(std::make_unique<UnixPtySession>()) {
  setFocusable(true);
  setClip(true);
  m_adapter->setDefaultColors(m_foregroundColor, m_backgroundColor);
  m_adapter->setScrollbackLimit(m_scrollbackLimit);
  m_adapter->setOutputCallback(
      [this](const QByteArray &bytes) { m_session->write(bytes); });
  m_adapter->setDamageCallback([this] { markDirty(); });
  m_adapter->setBellCallback([this] { emit bell(); });
  m_adapter->setTitleCallback([this](const QString &value) {
    if (!m_allowTitleChanges || m_title == value)
      return;
    m_title = value;
    emit titleChanged();
  });
  m_adapter->setClipboardCallback([this](const QString &value) {
    if (m_allowClipboardWrite)
      emit clipboardWriteRequested(value);
  });
  m_adapter->setNotificationCallback([this](const QString &value) {
    if (!m_allowNotifications)
      return;
    const qsizetype separator = value.indexOf(';');
    if (separator < 0)
      emit notificationRequested(QString(), value);
    else
      emit notificationRequested(value.left(separator),
                                 value.mid(separator + 1));
  });
  m_adapter->setMouseModeCallback([this](int mode) {
    if (Terminal *root = terminal())
      root->setMouseMotionRequested(this, mode == VTERM_PROP_MOUSE_MOVE);
  });

  connect(m_session.get(), &UnixPtySession::started, this, [this] {
    setProcessState(Running);
    emit processIdChanged();
    emit started();
  });
  connect(m_session.get(), &UnixPtySession::dataReceived, this,
          [this](const QByteArray &bytes) {
            const int before = m_adapter->scrollbackSize();
            m_adapter->feed(bytes);
            if (m_scrollOffset > 0) {
              const int added =
                  std::max(0, m_adapter->scrollbackSize() - before);
              if (added)
                setScrollOffset(m_scrollOffset + added);
            }
          });
  connect(m_session.get(), &UnixPtySession::finished, this, [this](int code) {
    m_exitCode = code;
    emit exitCodeChanged();
    setProcessState(NotRunning);
    emit processIdChanged();
    emit finished(code);
  });
  connect(m_session.get(), &UnixPtySession::errorOccurred, this,
          &TerminalView::setError);
  connect(this, &Item::activeFocusChanged, this,
          [this] { m_adapter->setFocused(activeFocus()); });
}

TerminalView::~TerminalView() {
  if (Terminal *root = terminal())
    root->setMouseMotionRequested(this, false);
}

#define QTUI_SIMPLE_SETTER(Type, Name, Member, Signal)                         \
  void TerminalView::Name(Type value) {                                        \
    if (Member == value)                                                       \
      return;                                                                  \
    Member = value;                                                            \
    emit Signal();                                                             \
  }

QTUI_SIMPLE_SETTER(const QString &, setProgram, m_program, programChanged)
QTUI_SIMPLE_SETTER(const QStringList &, setArguments, m_arguments,
                   argumentsChanged)
QTUI_SIMPLE_SETTER(const QString &, setWorkingDirectory, m_workingDirectory,
                   workingDirectoryChanged)
QTUI_SIMPLE_SETTER(const QVariantMap &, setEnvironment, m_environment,
                   environmentChanged)
QTUI_SIMPLE_SETTER(const QString &, setTerminalType, m_terminalType,
                   terminalTypeChanged)

#undef QTUI_SIMPLE_SETTER

void TerminalView::setInheritEnvironment(bool value) {
  if (m_inheritEnvironment == value)
    return;
  m_inheritEnvironment = value;
  emit inheritEnvironmentChanged();
}

void TerminalView::setAutoStart(bool value) {
  if (m_autoStart == value)
    return;
  m_autoStart = value;
  emit autoStartChanged();
  if (m_componentComplete && value && !m_program.isEmpty() &&
      !m_autoStartAttempted) {
    m_autoStartAttempted = true;
    QTimer::singleShot(0, this, &TerminalView::start);
  }
}

qint64 TerminalView::processId() const {
  return m_session ? m_session->processId() : 0;
}

void TerminalView::setForegroundColor(const QColor &value) {
  if (!value.isValid() || m_foregroundColor == value)
    return;
  m_foregroundColor = value;
  m_adapter->setDefaultColors(m_foregroundColor, m_backgroundColor);
  emit foregroundColorChanged();
  markDirty();
}

void TerminalView::setBackgroundColor(const QColor &value) {
  if (!value.isValid() || m_backgroundColor == value)
    return;
  m_backgroundColor = value;
  m_adapter->setDefaultColors(m_foregroundColor, m_backgroundColor);
  emit backgroundColorChanged();
  markDirty();
}

void TerminalView::setScrollbackLimit(int value) {
  value = std::max(0, value);
  if (m_scrollbackLimit == value)
    return;
  m_scrollbackLimit = value;
  m_adapter->setScrollbackLimit(value);
  emit scrollbackLimitChanged();
  setScrollOffset(m_scrollOffset);
}

void TerminalView::setScrollOffset(int value) {
  value = std::clamp(value, 0, m_adapter->scrollbackSize());
  if (m_scrollOffset == value)
    return;
  m_scrollOffset = value;
  emit scrollOffsetChanged();
  markDirty();
}

void TerminalView::setAllowTitleChanges(bool value) {
  if (m_allowTitleChanges == value)
    return;
  m_allowTitleChanges = value;
  emit allowTitleChangesChanged();
}

void TerminalView::setAllowClipboardWrite(bool value) {
  if (m_allowClipboardWrite == value)
    return;
  m_allowClipboardWrite = value;
  emit allowClipboardWriteChanged();
}

void TerminalView::setAllowNotifications(bool value) {
  if (m_allowNotifications == value)
    return;
  m_allowNotifications = value;
  emit allowNotificationsChanged();
}

void TerminalView::start() {
  if (m_processState != NotRunning) {
    setError(QStringLiteral("Terminal process is already active"));
    return;
  }
  if (m_program.trimmed().isEmpty()) {
    setError(QStringLiteral("program must not be empty"));
    return;
  }

  QProcessEnvironment processEnvironment =
      m_inheritEnvironment ? QProcessEnvironment::systemEnvironment()
                           : QProcessEnvironment();
  for (auto iterator = m_environment.cbegin(); iterator != m_environment.cend();
       ++iterator) {
    if (iterator.key().isEmpty() || iterator.key().contains('=')) {
      setError(QStringLiteral("Invalid environment variable name: %1")
                   .arg(iterator.key()));
      return;
    }
    processEnvironment.insert(iterator.key(), iterator.value().toString());
  }
  processEnvironment.insert(QStringLiteral("TERM"),
                            m_terminalType.isEmpty()
                                ? QStringLiteral("xterm-256color")
                                : m_terminalType);
  processEnvironment.insert(QStringLiteral("COLORTERM"),
                            QStringLiteral("truecolor"));
  processEnvironment.insert(QStringLiteral("TERM_PROGRAM"),
                            QStringLiteral("qtui"));

  if (m_exitCode != -1) {
    m_exitCode = -1;
    emit exitCodeChanged();
  }
  if (!m_errorString.isEmpty()) {
    m_errorString.clear();
    emit errorStringChanged();
  }
  setProcessState(Starting);
  if (!m_session->start(m_program, m_arguments, m_workingDirectory,
                        processEnvironment, std::max(1, height()),
                        std::max(1, width())))
    setProcessState(NotRunning);
}

void TerminalView::terminate() {
  if (m_processState == NotRunning)
    return;
  setProcessState(Stopping);
  m_session->terminate();
}

void TerminalView::kill() {
  if (m_processState == NotRunning)
    return;
  setProcessState(Stopping);
  m_session->kill();
}

void TerminalView::sendText(const QString &text) {
  if (m_processState != Running) {
    setError(
        QStringLiteral("Cannot send text while the terminal is not running"));
    return;
  }
  m_session->write(text.toUtf8());
}

void TerminalView::clearScrollback() {
  m_adapter->clearScrollback();
  setScrollOffset(0);
}

void TerminalView::scrollToBottom() { setScrollOffset(0); }

void TerminalView::classBegin() {}

void TerminalView::componentComplete() {
  m_componentComplete = true;
  syncSize();
  if (m_autoStart && !m_program.isEmpty() && !m_autoStartAttempted) {
    m_autoStartAttempted = true;
    QTimer::singleShot(0, this, &TerminalView::start);
  }
}

void TerminalView::setWidth(int value) {
  Item::setWidth(value);
  syncSize();
}

void TerminalView::setHeight(int value) {
  Item::setHeight(value);
  syncSize();
}

void TerminalView::render(TextGridBuffer &buffer, int absX, int absY,
                          const ClipRect &clipRect) {
  m_adapter->render(buffer, absX, absY, clipRect, std::max(0, width()),
                    std::max(0, height()), m_scrollOffset);
  Item::render(buffer, absX, absY, clipRect);
}

void TerminalView::keyPressEvent(KeyEvent *event) {
  if (!event || m_processState != Running)
    return;
  if (event->isPaste())
    m_adapter->sendPaste(event->text());
  else
    m_adapter->sendKey(event->key(), event->text(), event->modifiers());
  event->setAccepted(true);
  if (m_scrollOffset != 0)
    setScrollOffset(0);
}

void TerminalView::mousePressEvent(MouseEvent *event) {
  if (!event)
    return;
  if (!focus())
    setFocus(true);
  if (m_processState == Running && m_adapter->mouseReporting()) {
    m_adapter->sendMouseMove(event->y(), event->x(), event->modifiers());
    m_adapter->sendMouseButton(event->button(), true, event->modifiers());
    event->setAccepted(true);
  }
}

void TerminalView::mouseReleaseEvent(MouseEvent *event) {
  if (event && m_processState == Running && m_adapter->mouseReporting()) {
    m_adapter->sendMouseMove(event->y(), event->x(), event->modifiers());
    m_adapter->sendMouseButton(event->button(), false, event->modifiers());
    event->setAccepted(true);
  }
}

void TerminalView::mouseMoveEvent(MouseEvent *event) {
  if (event && m_processState == Running && m_adapter->mouseReporting()) {
    m_adapter->sendMouseMove(event->y(), event->x(), event->modifiers());
    event->setAccepted(true);
  }
}

void TerminalView::wheelEvent(MouseEvent *event) {
  if (!event || event->wheelDeltaY() == 0)
    return;
  const bool localScroll = (event->modifiers() & Qt::ShiftModifier) ||
                           !m_adapter->mouseReporting() ||
                           m_processState != Running;
  if (localScroll)
    setScrollOffset(m_scrollOffset + event->wheelDeltaY());
  else
    m_adapter->sendWheel(event->wheelDeltaY(), event->modifiers());
  event->setAccepted(true);
}

void TerminalView::setProcessState(ProcessState value) {
  if (m_processState == value)
    return;
  const bool wasRunning = running();
  m_processState = value;
  emit processStateChanged();
  if (wasRunning != running())
    emit runningChanged();
}

void TerminalView::setError(const QString &error) {
  if (m_errorString != error) {
    m_errorString = error;
    emit errorStringChanged();
  }
  emit errorOccurred(error);
  if (m_processState == Starting)
    setProcessState(NotRunning);
}

void TerminalView::syncSize() {
  const int rows = std::max(1, height());
  const int columns = std::max(1, width());
  m_adapter->resize(rows, columns);
  m_session->resize(rows, columns);
}

} // namespace qtui
