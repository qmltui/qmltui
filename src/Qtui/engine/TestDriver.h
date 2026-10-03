#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QQmlEngine>
#include <QEventLoop>
#include <memory>

namespace qtui {

class TerminalEngine;
class Terminal;

class TestDriver : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool active READ isActive CONSTANT)
    Q_PROPERTY(int frameCount READ frameCount CONSTANT)

public:
    explicit TestDriver(QObject* parent = nullptr);
    ~TestDriver() override;

    static TestDriver* instance();

    bool isActive() const { return m_active; }
    int frameCount() const { return m_frameCount; }

    void attachTerminalEngine(TerminalEngine* engine);
    void attachTerminal(Terminal* terminal);

    // QML / JS Invokable API
    Q_INVOKABLE void sendKey(const QString& key, const QStringList& modifiers = {});
    Q_INVOKABLE void typeText(const QString& text);
    Q_INVOKABLE void wait(int milliseconds);
    Q_INVOKABLE bool snapshot(const QString& imagePath = QString(), const QString& textPath = QString());
    Q_INVOKABLE bool assertTextContains(const QString& expectedText);
    Q_INVOKABLE void quit(int exitCode = 0);

    // Execute script file in QML engine context
    void executeScriptFile(QQmlEngine* engine, const QString& scriptPath);

private:
    static TestDriver* s_instance;
    bool m_active = false;
    int m_frameCount = 0;
    TerminalEngine* m_terminalEngine = nullptr;
    Terminal* m_terminal = nullptr;
    QQmlEngine* m_qmlEngine = nullptr;
    bool m_scriptExecuted = false;
    bool m_quitCalled = false;
};

} // namespace qtui
