#include <QmlTui/Runner/Runner.h>
#include <QmlTui/Runner/RunnerInput_p.h>

#include <QmlTui/Core/Engine/Renderer.h>
#include <QmlTui/Core/Engine/TerminalEngine.h>
#include <QmlTui/Core/Primitives/TextGridBuffer.h>

#include <QHash>
#include <QTextBoundaryFinder>
#include <algorithm>

namespace qmltui {

namespace runner_detail {

bool parseKey(const QString& input, ParsedKey* parsed) {
    static const QHash<QString, int> namedKeys = {
        {QStringLiteral("ENTER"), Qt::Key_Return},
        {QStringLiteral("RETURN"), Qt::Key_Return},
        {QStringLiteral("ESC"), Qt::Key_Escape},
        {QStringLiteral("ESCAPE"), Qt::Key_Escape},
        {QStringLiteral("TAB"), Qt::Key_Tab},
        {QStringLiteral("BACKTAB"), Qt::Key_Backtab},
        {QStringLiteral("BACKSPACE"), Qt::Key_Backspace},
        {QStringLiteral("SPACE"), Qt::Key_Space},
        {QStringLiteral("UP"), Qt::Key_Up},
        {QStringLiteral("DOWN"), Qt::Key_Down},
        {QStringLiteral("LEFT"), Qt::Key_Left},
        {QStringLiteral("RIGHT"), Qt::Key_Right},
        {QStringLiteral("HOME"), Qt::Key_Home},
        {QStringLiteral("END"), Qt::Key_End},
        {QStringLiteral("INSERT"), Qt::Key_Insert},
        {QStringLiteral("DELETE"), Qt::Key_Delete},
        {QStringLiteral("PAGEUP"), Qt::Key_PageUp},
        {QStringLiteral("PAGEDOWN"), Qt::Key_PageDown},
    };

    const QString trimmed = input.trimmed();
    const QString upper = trimmed.toUpper();
    if (const auto it = namedKeys.constFind(upper); it != namedKeys.cend()) {
        parsed->key = it.value();
        if (parsed->key == Qt::Key_Space)
            parsed->text = QStringLiteral(" ");
        else if (parsed->key == Qt::Key_Return)
            parsed->text = QStringLiteral("\n");
        return true;
    }

    if (upper.startsWith(QLatin1Char('F')) && upper.size() > 1) {
        bool ok = false;
        const int number = upper.sliced(1).toInt(&ok);
        if (ok && number >= 1 && number <= 35) {
            parsed->key = Qt::Key_F1 + number - 1;
            return true;
        }
        return false;
    }

    if (trimmed.size() != 1 || trimmed.front().unicode() > 0x7f) return false;

    const QChar character = trimmed.front();
    parsed->text = trimmed;
    if (character.isLetter())
        parsed->key = Qt::Key_A + character.toUpper().unicode() - 'A';
    else if (character.isDigit())
        parsed->key = Qt::Key_0 + character.unicode() - '0';
    else
        parsed->key = character.unicode();
    return true;
}

bool parseModifiers(const QStringList& names, Qt::KeyboardModifiers* modifiers,
                    QString* invalidModifier) {
    for (const QString& name : names) {
        const QString upper = name.trimmed().toUpper();
        if (upper == QLatin1String("SHIFT"))
            *modifiers |= Qt::ShiftModifier;
        else if (upper == QLatin1String("CTRL") || upper == QLatin1String("CONTROL"))
            *modifiers |= Qt::ControlModifier;
        else if (upper == QLatin1String("ALT"))
            *modifiers |= Qt::AltModifier;
        else if (upper == QLatin1String("META"))
            *modifiers |= Qt::MetaModifier;
        else {
            *invalidModifier = name;
            return false;
        }
    }
    return true;
}

bool parseMouseButton(const QString& name, int* button, QString* error) {
    const QString upper = name.trimmed().toUpper();
    if (upper == QLatin1String("LEFT") || upper.isEmpty()) {
        *button = 0;
        return true;
    }
    if (upper == QLatin1String("MIDDLE")) {
        *button = 1;
        return true;
    }
    if (upper == QLatin1String("RIGHT")) {
        *button = 2;
        return true;
    }
    if (error) *error = QStringLiteral("Unknown mouse button: %1").arg(name);
    return false;
}

bool isSingleGrapheme(const QString& text) {
    if (text.isEmpty()) return false;
    QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme, text);
    finder.setPosition(0);
    return finder.toNextBoundary() == text.size();
}

} // namespace runner_detail

void Runner::press(const QString& key, const QStringList& modifiers) {
    sendKey("press", key, modifiers, /*awaitFrame=*/true);
}

void Runner::injectKey(const QString& key, const QStringList& modifiers) {
    sendKey("injectKey", key, modifiers, /*awaitFrame=*/false);
}

void Runner::sendKey(const char* operation, const QString& key, const QStringList& modifiers,
                     bool awaitFrame) {
    if (!ensureRunning(operation)) return;

    runner_detail::ParsedKey parsed;
    if (!runner_detail::parseKey(key, &parsed)) {
        // Any single grapheme a terminal sends as text is a valid key: route
        // it through the input decoder like typed text. Modifier combinations
        // have no byte representation for arbitrary text, so they are invalid.
        const QString trimmed = key.trimmed();
        if (runner_detail::isSingleGrapheme(trimmed)) {
            if (!modifiers.isEmpty()) {
                failScenario(QStringLiteral("Modifiers are not supported with the text key \"%1\"")
                                 .arg(trimmed));
                return;
            }
            m_terminalEngine->feedInput(trimmed.toUtf8());
            if (awaitFrame) (void)waitForNextFrame(m_operationTimeoutMs);
            return;
        }
        failScenario(QStringLiteral("Unknown or unsupported key: %1").arg(key));
        return;
    }
    Qt::KeyboardModifiers parsedModifiers = Qt::NoModifier;
    QString invalidModifier;
    if (!runner_detail::parseModifiers(modifiers, &parsedModifiers, &invalidModifier)) {
        failScenario(QStringLiteral("Unknown modifier: %1").arg(invalidModifier));
        return;
    }

    if (parsed.key == Qt::Key_Backtab) parsedModifiers |= Qt::ShiftModifier;
    if (parsedModifiers & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)) {
        parsed.text.clear();
    } else if ((parsedModifiers & Qt::ShiftModifier) && parsed.text.size() == 1 &&
               parsed.text.front().isLetter()) {
        parsed.text = parsed.text.toUpper();
    }

    m_terminalEngine->handleKeyInput(parsed.key, parsed.text, parsedModifiers);
    if (awaitFrame) (void)waitForNextFrame(m_operationTimeoutMs);
}

void Runner::typeText(const QString& text) {
    sendText("typeText", text, /*awaitFrame=*/true);
}

void Runner::injectText(const QString& text) {
    // Deliberately does not wait for a frame: typeText() would request one and
    // consume the repaint that expectRepaint() needs to observe.
    sendText("injectText", text, /*awaitFrame=*/false);
}

void Runner::sendText(const char* operation, const QString& text, bool awaitFrame) {
    if (!ensureRunning(operation)) return;
    if (text.isEmpty()) {
        failScenario(
            QStringLiteral("app.%1() requires non-empty text").arg(QLatin1String(operation)));
        return;
    }

    // The bytes take the same path as a real keyboard: the input decoder turns
    // them into one key event per character with the key code a terminal
    // keystroke carries, so a Keys handler matching Qt.Key_W or Qt.Key_1 sees
    // scripted typing exactly as it sees a user typing, and every future
    // decoder change applies to scripts without a parallel table here.
    m_terminalEngine->feedInput(text.toUtf8());
    if (awaitFrame) (void)waitForNextFrame(m_operationTimeoutMs);
}

void Runner::paste(const QString& text) {
    sendPaste("paste", text, /*awaitFrame=*/true);
}

void Runner::injectPaste(const QString& text) {
    sendPaste("injectPaste", text, /*awaitFrame=*/false);
}

void Runner::sendPaste(const char* operation, const QString& text, bool awaitFrame) {
    if (!ensureRunning(operation)) return;
    if (text.isEmpty()) {
        failScenario(
            QStringLiteral("app.%1() requires non-empty text").arg(QLatin1String(operation)));
        return;
    }
    // The decoder frames the bytes exactly as a terminal in bracketed-paste
    // mode delivers them, so newlines and tabs reach TextInput::paste()
    // untranslated instead of acting as Return and Tab keys.
    m_terminalEngine->feedInput(QByteArrayLiteral("\x1b[200~") + text.toUtf8() +
                                QByteArrayLiteral("\x1b[201~"));
    if (awaitFrame) (void)waitForNextFrame(m_operationTimeoutMs);
}

bool Runner::parseMouseArguments(const QString& button, const QStringList& modifiers,
                                 int* parsedButton, Qt::KeyboardModifiers* parsedModifiers) {
    QString error;
    if (!runner_detail::parseMouseButton(button, parsedButton, &error)) {
        failScenario(error);
        return false;
    }
    if (!runner_detail::parseModifiers(modifiers, parsedModifiers, &error)) {
        failScenario(QStringLiteral("Unknown modifier: %1").arg(error));
        return false;
    }
    return true;
}

void Runner::click(int x, int y, const QString& button, const QStringList& modifiers) {
    if (!ensureRunning("click")) return;
    int b = 0;
    Qt::KeyboardModifiers parsedModifiers = Qt::NoModifier;
    if (!parseMouseArguments(button, modifiers, &b, &parsedModifiers)) return;
    m_terminalEngine->handleMouseInput(b, x, y, false, false, parsedModifiers);
    m_terminalEngine->handleMouseInput(b, x, y, true, false, parsedModifiers);
    (void)waitForNextFrame(m_operationTimeoutMs);
}

void Runner::doubleClick(int x, int y, const QString& button, const QStringList& modifiers) {
    if (!ensureRunning("doubleClick")) return;
    int b = 0;
    Qt::KeyboardModifiers parsedModifiers = Qt::NoModifier;
    if (!parseMouseArguments(button, modifiers, &b, &parsedModifiers)) return;
    m_terminalEngine->handleMouseInput(b, x, y, false, false, parsedModifiers);
    m_terminalEngine->handleMouseInput(b, x, y, true, false, parsedModifiers);
    m_terminalEngine->handleMouseInput(b, x, y, false, false, parsedModifiers);
    m_terminalEngine->handleMouseInput(b, x, y, true, false, parsedModifiers);
    (void)waitForNextFrame(m_operationTimeoutMs);
}

void Runner::hover(int x, int y, const QStringList& modifiers) {
    if (!ensureRunning("hover")) return;
    Qt::KeyboardModifiers parsedModifiers = Qt::NoModifier;
    QString invalidModifier;
    if (!runner_detail::parseModifiers(modifiers, &parsedModifiers, &invalidModifier)) {
        failScenario(QStringLiteral("Unknown modifier: %1").arg(invalidModifier));
        return;
    }
    // A terminal sends motion without a button only under any-motion
    // reporting. When the application never enabled it, the event does not
    // exist; the frame wait still runs so timing matches the delivered case.
    if (m_terminalEngine->mouseMotionReporting()) {
        // SGR button code 3 is "no button" in a motion report.
        m_terminalEngine->handleMouseInput(3, x, y, false, true, parsedModifiers);
    }
    (void)waitForNextFrame(m_operationTimeoutMs);
}

void Runner::mouseDown(int x, int y, const QString& button, const QStringList& modifiers) {
    if (!ensureRunning("mouseDown")) return;
    int b = 0;
    Qt::KeyboardModifiers parsedModifiers = Qt::NoModifier;
    if (!parseMouseArguments(button, modifiers, &b, &parsedModifiers)) return;
    m_terminalEngine->handleMouseInput(b, x, y, false, false, parsedModifiers);
    (void)waitForNextFrame(m_operationTimeoutMs);
}

void Runner::mouseUp(int x, int y, const QString& button, const QStringList& modifiers) {
    if (!ensureRunning("mouseUp")) return;
    int b = 0;
    Qt::KeyboardModifiers parsedModifiers = Qt::NoModifier;
    if (!parseMouseArguments(button, modifiers, &b, &parsedModifiers)) return;
    m_terminalEngine->handleMouseInput(b, x, y, true, false, parsedModifiers);
    (void)waitForNextFrame(m_operationTimeoutMs);
}

void Runner::drag(int startX, int startY, int endX, int endY, const QString& button,
                  const QStringList& modifiers) {
    if (!ensureRunning("drag")) return;
    int b = 0;
    Qt::KeyboardModifiers parsedModifiers = Qt::NoModifier;
    if (!parseMouseArguments(button, modifiers, &b, &parsedModifiers)) return;
    m_terminalEngine->handleMouseInput(b, startX, startY, false, false, parsedModifiers);
    // Button-held motion is reported cell by cell under mode 1002; walk the
    // path so drag handlers see the same intermediate positions a user
    // produces instead of one jump to the end.
    const int steps = (std::max)(qAbs(endX - startX), qAbs(endY - startY));
    for (int step = 1; step <= steps; ++step) {
        const int x = startX + ((endX - startX) * step) / (std::max)(1, steps);
        const int y = startY + ((endY - startY) * step) / (std::max)(1, steps);
        m_terminalEngine->handleMouseInput(b, x, y, false, true, parsedModifiers);
    }
    m_terminalEngine->handleMouseInput(b, endX, endY, true, false, parsedModifiers);
    (void)waitForNextFrame(m_operationTimeoutMs);
}

void Runner::scroll(int x, int y, int deltaX, int deltaY, const QStringList& modifiers) {
    if (!ensureRunning("scroll")) return;
    QString error;
    Qt::KeyboardModifiers parsedModifiers = Qt::NoModifier;
    if (!runner_detail::parseModifiers(modifiers, &parsedModifiers, &error)) {
        failScenario(error);
        return;
    }
    m_terminalEngine->handleMouseInput(0, x, y, false, false, parsedModifiers, deltaX, deltaY);
    (void)waitForNextFrame(m_operationTimeoutMs);
}

void Runner::clickText(const QString& text, int occurrence, const QString& button,
                       const QStringList& modifiers) {
    if (!ensureRunning("clickText") || !m_terminalEngine) return;
    if (text.isEmpty() || text.contains(QLatin1Char('\n'))) {
        failScenario(QStringLiteral("app.clickText() requires non-empty single-row text"));
        return;
    }
    const std::optional<TextGridBuffer> frame = m_terminalEngine->currentFrame();
    if (!frame) {
        failScenario(QStringLiteral("No committed frame buffer is available"));
        return;
    }
    const QList<TextMatch> matches = textMatches(*frame, text);
    if (occurrence < 0 || occurrence >= matches.size()) {
        failScenario(QStringLiteral("Text \"%1\" occurrence %2 was not found (%3 matches)")
                         .arg(text)
                         .arg(occurrence)
                         .arg(matches.size()));
        return;
    }
    const TextMatch& match = matches.at(occurrence);
    const auto offset = m_terminalEngine->renderer()->mouseRowOffset();
    if (!offset) {
        failScenario(QStringLiteral("The inline frame's screen position is not yet known"));
        return;
    }
    const int screenY = match.y + *offset;
    click(match.x + (std::max)(0, match.width - 1) / 2, screenY, button, modifiers);
}

void Runner::hoverText(const QString& text, int occurrence, const QStringList& modifiers) {
    if (!ensureRunning("hoverText") || !m_terminalEngine) return;
    if (text.isEmpty() || text.contains(QLatin1Char('\n'))) {
        failScenario(QStringLiteral("app.hoverText() requires non-empty single-row text"));
        return;
    }
    const std::optional<TextGridBuffer> frame = m_terminalEngine->currentFrame();
    if (!frame) {
        failScenario(QStringLiteral("No committed frame buffer is available"));
        return;
    }
    const QList<TextMatch> matches = textMatches(*frame, text);
    if (occurrence < 0 || occurrence >= matches.size()) {
        failScenario(QStringLiteral("Text \"%1\" occurrence %2 was not found (%3 matches)")
                         .arg(text)
                         .arg(occurrence)
                         .arg(matches.size()));
        return;
    }
    const TextMatch& match = matches.at(occurrence);
    const auto offset = m_terminalEngine->renderer()->mouseRowOffset();
    if (!offset) {
        failScenario(QStringLiteral("The inline frame's screen position is not yet known"));
        return;
    }
    const int screenY = match.y + *offset;
    hover(match.x + (std::max)(0, match.width - 1) / 2, screenY, modifiers);
}

} // namespace qmltui
