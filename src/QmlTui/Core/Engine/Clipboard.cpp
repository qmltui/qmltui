#include <QmlTui/Core/Engine/Clipboard.h>
#include <QmlTui/Core/Engine/TerminalEngine.h>
#include <QtQml/qqmlinfo.h>
#include <utility>

namespace qmltui {

Clipboard* Clipboard::create(QQmlEngine* engine, QJSEngine*)
{
    return new Clipboard(engine);
}

Clipboard::Clipboard(QQmlEngine* engine) : QObject(engine), m_qmlEngine(engine)
{
    m_promiseFactory = engine->evaluate(QStringLiteral(
        "(function() { var resolve, reject;"
        "var promise = new Promise(function(a, b) { resolve = a; reject = b; });"
        "return [promise, resolve, reject]; })"));
}

Clipboard::~Clipboard()
{
    disconnect(m_readConnection);
    // No JavaScript execution during QML engine teardown.
    if (m_pendingEngine)
        m_pendingEngine->cancelClipboardRead();
}

Terminal* Clipboard::terminal() const { return m_effective; }

void Clipboard::setTerminal(Terminal* value)
{
    if (value && qmlEngine(value) != m_qmlEngine) {
        qmlWarning(this) << "Clipboard.terminal must belong to the same QML engine";
        return;
    }
    disconnect(m_overrideDestroyed);
    m_override = value;
    if (value) {
        // Even an explicitly selected Terminal that failed initialization
        // must release the override when it is destroyed.
        m_overrideDestroyed = connect(value, &QObject::destroyed, this, [this] {
            m_override.clear();
            updateTerminal();
            if (!m_effective) emit terminalChanged();
        });
    }
    updateTerminal();
}

void Clipboard::addTerminal(Terminal* value)
{
    if (m_terminals.contains(value)) return;
    m_terminals.append(value);
    updateTerminal();
}

void Clipboard::removeTerminal(Terminal* value)
{
    m_terminals.removeAll(value);
    if (m_override == value) {
        disconnect(m_overrideDestroyed);
        m_override.clear();
    }
    updateTerminal();
}

void Clipboard::updateTerminal()
{
    Terminal* selected = m_override ? m_override.data()
        : m_terminals.size() == 1 ? m_terminals.first().data() : nullptr;
    if (selected == m_effective) return;
    disconnect(m_copyConnection);
    m_effective = selected;
    if (selected)
        m_copyConnection = connect(selected, &Terminal::clipboardCopyRequested,
                                   this, &Clipboard::copyRequested);
    if (m_pendingEngine) m_pendingEngine->cancelClipboardRead();
    emit terminalChanged();
}

void Clipboard::setReadTimeout(int milliseconds)
{
    if (milliseconds <= 0) {
        qmlWarning(this) << "Clipboard.readTimeout must be positive";
        return;
    }
    if (m_readTimeout == milliseconds) return;
    m_readTimeout = milliseconds;
    emit readTimeoutChanged();
}

void Clipboard::copy(const QString& text)
{
    if (auto* selected = terminal()) selected->copyToClipboard(text);
    else qmlWarning(this) << (m_terminals.isEmpty()
        ? "Clipboard.copy: no terminal" : "Clipboard.copy: ambiguous terminal; set Clipboard.terminal");
}

QJSValue Clipboard::error(const QString& code) const
{
    auto result = m_qmlEngine->newErrorObject(QJSValue::GenericError,
        QStringLiteral("Clipboard read failed: %1").arg(code));
    result.setProperty(QStringLiteral("code"), code);
    return result;
}

QJSValue Clipboard::requestText()
{
    const auto deferred = m_promiseFactory.call();
    const auto promise = deferred.property(0);
    Request request{deferred.property(1), deferred.property(2)};
    auto* selected = terminal();
    if (!selected) {
        request.reject.call({error(m_terminals.isEmpty() ? QStringLiteral("NoTerminal")
                                                       : QStringLiteral("AmbiguousTerminal"))});
        return promise;
    }
    m_requests.append(std::move(request));
    if (m_pendingEngine) return promise;
    m_pendingEngine = selected->engine();
    if (!m_pendingEngine) {
        finishRead({}, QStringLiteral("Unavailable"));
        return promise;
    }
    m_readConnection = connect(m_pendingEngine, &TerminalEngine::clipboardReadFinished,
                              this, &Clipboard::finishRead);
    const QString failure = m_pendingEngine->requestClipboardText(m_readTimeout);
    if (!failure.isEmpty()) finishRead({}, failure);
    return promise;
}

void Clipboard::finishRead(const QString& text, const QString& code)
{
    disconnect(m_readConnection);
    m_pendingEngine.clear();
    const auto requests = std::exchange(m_requests, {});
    if (!m_qmlEngine) return;
    // Detach all state before resolving: callbacks may start another read.
    for (const auto& request : requests) {
        if (code.isEmpty()) request.resolve.call({QJSValue(text)});
        else request.reject.call({error(code)});
    }
}

} // namespace qmltui
