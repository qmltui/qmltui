#pragma once

#include <Item.h>

#include <QColor>
#include <QQmlParserStatus>
#include <QStringList>
#include <QVariantMap>

#include <memory>

namespace qtui {

class UnixPtySession;
class VTermAdapter;

class TerminalView : public Item, public QQmlParserStatus {
    Q_OBJECT
    QML_NAMED_ELEMENT(TerminalView)
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(QString program READ program WRITE setProgram NOTIFY programChanged)
    Q_PROPERTY(QStringList arguments READ arguments WRITE setArguments NOTIFY argumentsChanged)
    Q_PROPERTY(QString workingDirectory READ workingDirectory WRITE setWorkingDirectory NOTIFY workingDirectoryChanged)
    Q_PROPERTY(QVariantMap environment READ environment WRITE setEnvironment NOTIFY environmentChanged)
    Q_PROPERTY(bool inheritEnvironment READ inheritEnvironment WRITE setInheritEnvironment NOTIFY inheritEnvironmentChanged)
    Q_PROPERTY(QString terminalType READ terminalType WRITE setTerminalType NOTIFY terminalTypeChanged)
    Q_PROPERTY(bool autoStart READ autoStart WRITE setAutoStart NOTIFY autoStartChanged)
    Q_PROPERTY(ProcessState processState READ processState NOTIFY processStateChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(qint64 processId READ processId NOTIFY processIdChanged)
    Q_PROPERTY(int exitCode READ exitCode NOTIFY exitCodeChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(QColor foregroundColor READ foregroundColor WRITE setForegroundColor NOTIFY foregroundColorChanged)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)
    Q_PROPERTY(int scrollbackLimit READ scrollbackLimit WRITE setScrollbackLimit NOTIFY scrollbackLimitChanged)
    Q_PROPERTY(int scrollOffset READ scrollOffset WRITE setScrollOffset NOTIFY scrollOffsetChanged)
    Q_PROPERTY(bool allowTitleChanges READ allowTitleChanges WRITE setAllowTitleChanges NOTIFY allowTitleChangesChanged)
    Q_PROPERTY(bool allowClipboardWrite READ allowClipboardWrite WRITE setAllowClipboardWrite NOTIFY allowClipboardWriteChanged)
    Q_PROPERTY(bool allowNotifications READ allowNotifications WRITE setAllowNotifications NOTIFY allowNotificationsChanged)

public:
    enum ProcessState { NotRunning, Starting, Running, Stopping };
    Q_ENUM(ProcessState)

    explicit TerminalView(QObject* parent = nullptr);
    ~TerminalView() override;

    QString program() const { return m_program; }
    void setProgram(const QString& value);
    QStringList arguments() const { return m_arguments; }
    void setArguments(const QStringList& value);
    QString workingDirectory() const { return m_workingDirectory; }
    void setWorkingDirectory(const QString& value);
    QVariantMap environment() const { return m_environment; }
    void setEnvironment(const QVariantMap& value);
    bool inheritEnvironment() const { return m_inheritEnvironment; }
    void setInheritEnvironment(bool value);
    QString terminalType() const { return m_terminalType; }
    void setTerminalType(const QString& value);
    bool autoStart() const { return m_autoStart; }
    void setAutoStart(bool value);

    ProcessState processState() const { return m_processState; }
    bool running() const { return m_processState == Running; }
    qint64 processId() const;
    int exitCode() const { return m_exitCode; }
    QString errorString() const { return m_errorString; }
    QString title() const { return m_title; }

    QColor foregroundColor() const { return m_foregroundColor; }
    void setForegroundColor(const QColor& value);
    QColor backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(const QColor& value);
    int scrollbackLimit() const { return m_scrollbackLimit; }
    void setScrollbackLimit(int value);
    int scrollOffset() const { return m_scrollOffset; }
    void setScrollOffset(int value);

    bool allowTitleChanges() const { return m_allowTitleChanges; }
    void setAllowTitleChanges(bool value);
    bool allowClipboardWrite() const { return m_allowClipboardWrite; }
    void setAllowClipboardWrite(bool value);
    bool allowNotifications() const { return m_allowNotifications; }
    void setAllowNotifications(bool value);

    Q_INVOKABLE void start();
    Q_INVOKABLE void terminate();
    Q_INVOKABLE void kill();
    Q_INVOKABLE void sendText(const QString& text);
    Q_INVOKABLE void clearScrollback();
    Q_INVOKABLE void scrollToBottom();

    void classBegin() override;
    void componentComplete() override;
    void setWidth(int width) override;
    void setHeight(int height) override;
    void render(TextGridBuffer& buffer, int absX, int absY,
                const ClipRect& clipRect = ClipRect()) override;
    void keyPressEvent(KeyEvent* event) override;
    void mousePressEvent(MouseEvent* event) override;
    void mouseReleaseEvent(MouseEvent* event) override;
    void mouseMoveEvent(MouseEvent* event) override;
    void wheelEvent(MouseEvent* event) override;

signals:
    void programChanged();
    void argumentsChanged();
    void workingDirectoryChanged();
    void environmentChanged();
    void inheritEnvironmentChanged();
    void terminalTypeChanged();
    void autoStartChanged();
    void processStateChanged();
    void runningChanged();
    void processIdChanged();
    void exitCodeChanged();
    void errorStringChanged();
    void titleChanged();
    void foregroundColorChanged();
    void backgroundColorChanged();
    void scrollbackLimitChanged();
    void scrollOffsetChanged();
    void allowTitleChangesChanged();
    void allowClipboardWriteChanged();
    void allowNotificationsChanged();

    void started();
    void finished(int exitCode);
    void errorOccurred(const QString& error);
    void bell();
    void clipboardWriteRequested(const QString& text);
    void notificationRequested(const QString& title, const QString& message);

private:
    void setProcessState(ProcessState state);
    void setError(const QString& error);
    void syncSize();

    QString m_program;
    QStringList m_arguments;
    QString m_workingDirectory;
    QVariantMap m_environment;
    bool m_inheritEnvironment = true;
    QString m_terminalType = QStringLiteral("xterm-256color");
    bool m_autoStart = false;
    bool m_componentComplete = false;
    bool m_autoStartAttempted = false;
    ProcessState m_processState = NotRunning;
    int m_exitCode = -1;
    QString m_errorString;
    QString m_title;
    QColor m_foregroundColor = Qt::white;
    QColor m_backgroundColor = Qt::black;
    int m_scrollbackLimit = 10000;
    int m_scrollOffset = 0;
    bool m_allowTitleChanges = true;
    bool m_allowClipboardWrite = false;
    bool m_allowNotifications = false;
    std::unique_ptr<VTermAdapter> m_adapter;
    std::unique_ptr<UnixPtySession> m_session;
};

} // namespace qtui
