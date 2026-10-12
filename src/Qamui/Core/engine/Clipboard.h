#pragma once

#include <Qamui/Core/primitives/QamuiExport.h>
#include <Qamui/Core/items/Terminal.h>
#include <QJSValue>
#include <QPointer>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>

namespace qamui {

class QAMUI_CORE_EXPORT Clipboard : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(qamui::Terminal* terminal READ terminal WRITE setTerminal NOTIFY terminalChanged)
    Q_PROPERTY(int readTimeout READ readTimeout WRITE setReadTimeout NOTIFY readTimeoutChanged)

public:
    static Clipboard* create(QQmlEngine* engine, QJSEngine*);
    ~Clipboard() override;

    Terminal* terminal() const;
    void setTerminal(Terminal* terminal);
    int readTimeout() const { return m_readTimeout; }
    void setReadTimeout(int milliseconds);
    Q_INVOKABLE void copy(const QString& text);
    Q_INVOKABLE QJSValue requestText();

signals:
    void copyRequested(const QString& text);
    void terminalChanged();
    void readTimeoutChanged();

private:
    friend class Terminal;
    explicit Clipboard(QQmlEngine* engine);
    void addTerminal(Terminal* terminal);
    void removeTerminal(Terminal* terminal);
    void updateTerminal();
    void finishRead(const QString& text, const QString& code);
    QJSValue error(const QString& code) const;

    QPointer<QQmlEngine> m_qmlEngine;
    QList<QPointer<Terminal>> m_terminals;
    QPointer<Terminal> m_override;
    QPointer<Terminal> m_effective;
    QPointer<TerminalEngine> m_pendingEngine;
    QMetaObject::Connection m_readConnection;
    QMetaObject::Connection m_copyConnection;
    QMetaObject::Connection m_overrideDestroyed;
    struct Request { QJSValue resolve; QJSValue reject; };
    QList<Request> m_requests;
    QJSValue m_promiseFactory;
    int m_readTimeout = 2000;
};

} // namespace qamui
