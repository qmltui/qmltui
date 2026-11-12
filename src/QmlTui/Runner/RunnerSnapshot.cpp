#include <QmlTui/Runner/Runner.h>
#include <QmlTui/Runner/RunnerSnapshot_p.h>

#include <QmlTui/Core/Engine/Clock_p.h>
#include <QmlTui/Core/Engine/TerminalEngine.h>
#include <QmlTui/Core/Items/Item.h>
#include <QmlTui/Core/Items/Terminal.h>
#include <QmlTui/Core/Primitives/Color.h>
#include <QmlTui/Core/Primitives/TextGridBuffer.h>

#include <QDeadlineTimer>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <algorithm>
#include <utility>

namespace qmltui {

namespace runner_detail {

// Appended to wait-timeout diagnostics: under the virtual clock, application
// time cannot pass on its own, which is the usual reason such a wait expires.
QString pausedClockHint() {
    return Clock::isVirtual()
               ? QStringLiteral(" (the virtual clock is paused; advance it with advanceTime())")
               : QString();
}

// Depth-first search for the first visible item with the given objectName.
// A hidden subtree cannot contribute cells to the frame, so it is skipped.
Item* findVisibleItem(Item* item, const QString& objectName) {
    if (!item || !item->visible()) return nullptr;
    if (item->objectName() == objectName) return item;
    const QList<Item*>& children = item->childItems();
    for (Item* child : children) {
        if (Item* match = findVisibleItem(child, objectName)) return match;
    }
    return nullptr;
}

QString rgbaString(uint32_t rgba) {
    const uint32_t argb = Color::toArgb(rgba);
    return QStringLiteral("#%1%2%3%4")
        .arg((argb >> 16) & 0xffU, 2, 16, QLatin1Char('0'))
        .arg((argb >> 8) & 0xffU, 2, 16, QLatin1Char('0'))
        .arg(argb & 0xffU, 2, 16, QLatin1Char('0'))
        .arg((argb >> 24) & 0xffU, 2, 16, QLatin1Char('0'))
        .toUpper();
}

QString attributeNames(TextAttribute attributes) {
    static const std::pair<TextAttribute, const char*> names[] = {
        {TextAttribute::Bold, "bold"},        {TextAttribute::Dim, "dim"},
        {TextAttribute::Italic, "italic"},    {TextAttribute::Underline, "underline"},
        {TextAttribute::Reversed, "reverse"}, {TextAttribute::Blink, "blink"},
        {TextAttribute::Conceal, "conceal"},  {TextAttribute::Strikethrough, "strikethrough"},
    };
    QStringList result;
    for (const auto& [flag, name] : names) {
        if (static_cast<uint8_t>(attributes) & static_cast<uint8_t>(flag))
            result.append(QLatin1String(name));
    }
    return result.join(QLatin1Char(','));
}

bool parseRgbaString(const QString& text, uint32_t* rgba) {
    static const QRegularExpression rgbaPattern(QStringLiteral("^#([0-9A-Fa-f]{8})$"));
    const QRegularExpressionMatch match = rgbaPattern.match(text.trimmed());
    if (match.hasMatch()) {
        bool ok = false;
        const uint32_t encoded = match.captured(1).toUInt(&ok, 16);
        if (ok) {
            const int red = (encoded >> 24) & 0xff;
            const int green = (encoded >> 16) & 0xff;
            const int blue = (encoded >> 8) & 0xff;
            const int alpha = encoded & 0xff;
            *rgba = (static_cast<uint32_t>(alpha) << 24) | (static_cast<uint32_t>(red) << 16) |
                    (static_cast<uint32_t>(green) << 8) | static_cast<uint32_t>(blue);
            return true;
        }
    }
    // Named colors and "ansi:" strings resolve the same way QML "color" strings do.
    const Color color = Color::fromString(text.trimmed());
    if (!color.isValid()) return false;
    *rgba = color.argb32();
    return true;
}

} // namespace runner_detail

void Runner::waitForRegex(const QString& pattern, int timeoutMs) {
    if (!ensureRunning("waitForRegex")) return;
    const int validated = validateTimeout(timeoutMs);
    if (!validated) return;

    const QRegularExpression re(pattern);
    if (!re.isValid()) {
        failScenario(
            QStringLiteral("Invalid regex pattern: %1 (%2)").arg(pattern, re.errorString()));
        return;
    }

    QString error;
    const auto matches = [&] {
        const QString frame = currentFrameText(&error);
        return error.isEmpty() && re.match(frame).hasMatch();
    };

    if (matches()) return;

    QDeadlineTimer deadline(validated);
    if (m_activeLoop) {
        failScenario(QStringLiteral("Nested Test wait operations are not supported"));
        return;
    }

    bool matched = false;
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    const QMetaObject::Connection frameConnection =
        connect(m_terminalEngine, &TerminalEngine::frameRendered, &loop, [&](quint64) {
            if (matches()) {
                matched = true;
                loop.quit();
            }
        });
    connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

    m_activeLoop = &loop;
    timeout.start(static_cast<int>((std::max<qint64>)(1, deadline.remainingTime())));
    m_terminalEngine->requestRender();
    loop.exec();
    m_activeLoop = nullptr;
    disconnect(frameConnection);

    if (m_state == State::Failed) return;
    if (!matched) {
        failScenario(QStringLiteral("Timed out after %1 ms waiting for regex pattern \"%2\"")
                         .arg(validated)
                         .arg(pattern) +
                     runner_detail::pausedClockHint());
    }
}

void Runner::expectRegex(const QString& pattern, int timeoutMs) {
    if (!ensureRunning("expectRegex") || !waitForNextFrame(resolveExpectTimeout(timeoutMs))) return;
    const QRegularExpression re(pattern);
    if (!re.isValid()) {
        failScenario(
            QStringLiteral("Invalid regex pattern: %1 (%2)").arg(pattern, re.errorString()));
        return;
    }
    QString error;
    const QString frame = currentFrameText(&error);
    if (!error.isEmpty())
        failScenario(error);
    else if (!re.match(frame).hasMatch())
        failScenario(QStringLiteral("Expected frame to match regex \"%1\"").arg(pattern));
}

void Runner::expectTextInRegion(int x, int y, int width, int height, const QString& text,
                                int timeoutMs) {
    if (!ensureRunning("expectTextInRegion") || !waitForNextFrame(resolveExpectTimeout(timeoutMs)))
        return;
    if (!m_terminalEngine) {
        failScenario(QStringLiteral("TerminalEngine is no longer available"));
        return;
    }
    const std::optional<TextGridBuffer> frame = m_terminalEngine->currentFrame();
    if (!frame) {
        failScenario(QStringLiteral("No committed frame buffer is available"));
        return;
    }
    if (x < 0 || y < 0 || width <= 0 || height <= 0 || x + width > frame->width() ||
        y + height > frame->height()) {
        failScenario(QStringLiteral("Region bounds [%1,%2,%3,%4] out of frame size %5x%6")
                         .arg(x)
                         .arg(y)
                         .arg(width)
                         .arg(height)
                         .arg(frame->width())
                         .arg(frame->height()));
        return;
    }

    QString regionText;
    for (int r = y; r < y + height; ++r) {
        QString line;
        for (int c = x; c < x + width; ++c) {
            const Cell& cell = frame->cellAt(c, r);
            if (cell.continuation) continue;
            line.append(cell.character().isEmpty() ? QStringLiteral(" ")
                                                   : QString::fromUtf8(cell.character()));
        }
        regionText.append(line);
        regionText.append(QLatin1Char('\n'));
    }

    if (!regionText.contains(text)) {
        failScenario(QStringLiteral("Expected region [%1,%2,%3,%4] to contain \"%5\"")
                         .arg(x)
                         .arg(y)
                         .arg(width)
                         .arg(height)
                         .arg(text));
    }
}

void Runner::expectStyle(int x, int y, const QJSValue& expectedStyle, int timeoutMs) {
    if (!ensureRunning("expectStyle") || !waitForNextFrame(resolveExpectTimeout(timeoutMs))) return;
    if (!m_terminalEngine) {
        failScenario(QStringLiteral("TerminalEngine is no longer available"));
        return;
    }
    const std::optional<TextGridBuffer> frame = m_terminalEngine->currentFrame();
    if (!frame) {
        failScenario(QStringLiteral("No committed frame buffer is available"));
        return;
    }
    if (x < 0 || y < 0 || x >= frame->width() || y >= frame->height()) {
        failScenario(QStringLiteral("Coordinates (%1,%2) out of frame bounds %3x%4")
                         .arg(x)
                         .arg(y)
                         .arg(frame->width())
                         .arg(frame->height()));
        return;
    }
    if (!expectedStyle.isObject()) {
        failScenario(
            QStringLiteral("expectStyle requires an object with expected cell attributes"));
        return;
    }

    const Cell& cell = frame->cellAt(x, y);

    if (expectedStyle.hasProperty(QStringLiteral("bold"))) {
        const bool expectedBold = expectedStyle.property(QStringLiteral("bold")).toBool();
        const bool actualBold = (static_cast<uint8_t>(cell.attributes) &
                                 static_cast<uint8_t>(TextAttribute::Bold)) != 0;
        if (expectedBold != actualBold) {
            failScenario(QStringLiteral("Cell at (%1,%2) bold expected %3 but got %4")
                             .arg(x)
                             .arg(y)
                             .arg(expectedBold)
                             .arg(actualBold));
            return;
        }
    }
    if (expectedStyle.hasProperty(QStringLiteral("dim"))) {
        const bool expectedDim = expectedStyle.property(QStringLiteral("dim")).toBool();
        const bool actualDim =
            (static_cast<uint8_t>(cell.attributes) & static_cast<uint8_t>(TextAttribute::Dim)) != 0;
        if (expectedDim != actualDim) {
            failScenario(QStringLiteral("Cell at (%1,%2) dim expected %3 but got %4")
                             .arg(x)
                             .arg(y)
                             .arg(expectedDim)
                             .arg(actualDim));
            return;
        }
    }
    if (expectedStyle.hasProperty(QStringLiteral("italic"))) {
        const bool expectedItalic = expectedStyle.property(QStringLiteral("italic")).toBool();
        const bool actualItalic = (static_cast<uint8_t>(cell.attributes) &
                                   static_cast<uint8_t>(TextAttribute::Italic)) != 0;
        if (expectedItalic != actualItalic) {
            failScenario(QStringLiteral("Cell at (%1,%2) italic expected %3 but got %4")
                             .arg(x)
                             .arg(y)
                             .arg(expectedItalic)
                             .arg(actualItalic));
            return;
        }
    }
    if (expectedStyle.hasProperty(QStringLiteral("underline"))) {
        const bool expectedUnderline = expectedStyle.property(QStringLiteral("underline")).toBool();
        const bool actualUnderline = (static_cast<uint8_t>(cell.attributes) &
                                      static_cast<uint8_t>(TextAttribute::Underline)) != 0;
        if (expectedUnderline != actualUnderline) {
            failScenario(QStringLiteral("Cell at (%1,%2) underline expected %3 but got %4")
                             .arg(x)
                             .arg(y)
                             .arg(expectedUnderline)
                             .arg(actualUnderline));
            return;
        }
    }
    if (expectedStyle.hasProperty(QStringLiteral("reversed"))) {
        const bool expectedRev = expectedStyle.property(QStringLiteral("reversed")).toBool();
        const bool actualRev = (static_cast<uint8_t>(cell.attributes) &
                                static_cast<uint8_t>(TextAttribute::Reversed)) != 0;
        if (expectedRev != actualRev) {
            failScenario(QStringLiteral("Cell at (%1,%2) reversed expected %3 but got %4")
                             .arg(x)
                             .arg(y)
                             .arg(expectedRev)
                             .arg(actualRev));
            return;
        }
    }
    if (expectedStyle.hasProperty(QStringLiteral("character"))) {
        const QString expectedChar = expectedStyle.property(QStringLiteral("character")).toString();
        const QString actualChar = QString::fromUtf8(cell.character());
        if (expectedChar != actualChar) {
            failScenario(QStringLiteral("Cell at (%1,%2) character expected \"%3\" but got \"%4\"")
                             .arg(x)
                             .arg(y)
                             .arg(expectedChar)
                             .arg(actualChar));
            return;
        }
    }
    const auto compareColor = [&](const QString& property, uint32_t actual) {
        if (!expectedStyle.hasProperty(property)) return true;
        const QString expectedText = expectedStyle.property(property).toString();
        if (expectedText.trimmed() == QLatin1String("default")) {
            if (actual != 0) {
                failScenario(QStringLiteral("Cell at (%1,%2) %3 expected default but got %4")
                                 .arg(x)
                                 .arg(y)
                                 .arg(property, runner_detail::rgbaString(actual)));
                return false;
            }
            return true;
        }
        uint32_t expectedRgba = 0;
        if (!runner_detail::parseRgbaString(expectedText, &expectedRgba)) {
            failScenario(QStringLiteral("Cell style %1 is not a valid color: %2")
                             .arg(property, expectedText));
            return false;
        }
        if (Color::toArgb(expectedRgba) != Color::toArgb(actual)) {
            failScenario(QStringLiteral("Cell at (%1,%2) %3 expected %4 but got %5")
                             .arg(x)
                             .arg(y)
                             .arg(property, expectedText, runner_detail::rgbaString(actual)));
            return false;
        }
        return true;
    };
    if (!compareColor(QStringLiteral("foreground"), cell.fgColor) ||
        !compareColor(QStringLiteral("background"), cell.bgColor)) {
        return;
    }
}

void Runner::expectSnapshot(const QString& snapshotRelativePath, int timeoutMs) {
    if (!ensureRunning("expectSnapshot") || !waitForNextFrame(resolveExpectTimeout(timeoutMs)))
        return;
    QString error;
    const QString actualText = currentFrameText(&error);
    if (!error.isEmpty()) {
        failScenario(error);
        return;
    }
    compareSnapshot(snapshotRelativePath, actualText);
}

void Runner::expectStyled(const QString& snapshotRelativePath, int timeoutMs) {
    if (!ensureRunning("expectStyled") || !waitForNextFrame(resolveExpectTimeout(timeoutMs)))
        return;
    QString error;
    const QString actualText = currentStyledFrameText(&error);
    if (!error.isEmpty()) {
        failScenario(error);
        return;
    }
    compareSnapshot(snapshotRelativePath, actualText);
}

void Runner::compareSnapshot(const QString& snapshotRelativePath, const QString& actualText) {
    const QString snapshotPath = resolveSnapshotPath(snapshotRelativePath);
    QFile snapshotFile(snapshotPath);
    const bool baselineExists = snapshotFile.exists();
    QString expectedText;
    if (baselineExists) {
        if (!snapshotFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            failScenario(QStringLiteral("Cannot open snapshot file %1: %2")
                             .arg(snapshotPath, snapshotFile.errorString()));
            return;
        }
        expectedText = QString::fromUtf8(snapshotFile.readAll());
        snapshotFile.close();
        if (actualText == expectedText) return;
    }

    // Derive the name from the whole relative path, not just the file name,
    // so two snapshots called a/frame.txt and b/frame.txt do not overwrite
    // each other's .actual.txt / .diff artifacts. The conventional
    // "snapshots/" prefix is dropped because the artifacts already live
    // under snapshots/.
    QString baseName = snapshotRelativePath;
    if (baseName.startsWith(QLatin1String("snapshots/")))
        baseName = baseName.mid(QStringLiteral("snapshots/").size());
    if (const int dot = baseName.lastIndexOf(QLatin1Char('.')); dot > 0) baseName.truncate(dot);
    static const QRegularExpression sanitizer(QStringLiteral("[^A-Za-z0-9._-]+"));
    baseName.replace(sanitizer, QStringLiteral("_"));
    baseName = baseName.mid(0, 120);
    if (baseName.isEmpty()) baseName = QStringLiteral("snapshot");
    const QString actualArtifactPath = QStringLiteral("snapshots/%1.actual.txt").arg(baseName);
    const QString diffArtifactPath = QStringLiteral("snapshots/%1.diff").arg(baseName);

    if (m_updateSnapshots) {
        // A Qt resource cannot be written back; record the frame where the
        // developer can copy it from and name the file it belongs in.
        if (snapshotPath.startsWith(QLatin1Char(':'))) {
            QString writeError;
            if (writeTextArtifact(actualArtifactPath, actualText, &writeError))
                m_capturePaths.append(actualArtifactPath);
            failScenario(QStringLiteral("Snapshot update mode cannot update resource snapshot %1; "
                                        "the actual frame was written to %2")
                             .arg(snapshotPath, actualArtifactPath));
            return;
        }
        if (!QDir().mkpath(QFileInfo(snapshotPath).absolutePath())) {
            failScenario(
                QStringLiteral("Cannot create snapshot directory for %1").arg(snapshotPath));
            return;
        }
        QSaveFile baseline(snapshotPath);
        const QByteArray bytes = actualText.toUtf8();
        if (!baseline.open(QIODevice::WriteOnly | QIODevice::Text) ||
            baseline.write(bytes) != bytes.size() || !baseline.commit()) {
            failScenario(QStringLiteral("Cannot update snapshot %1: %2")
                             .arg(snapshotPath, baseline.errorString()));
            return;
        }
        m_updatedSnapshots.append(snapshotPath);
        return;
    }

    if (!baselineExists) {
        failScenario(QStringLiteral("Missing snapshot baseline %1; run with "
                                    "QMLTUI_RUNNER_UPDATE_SNAPSHOTS=1 to record it")
                         .arg(snapshotPath));
        return;
    }

    QString writeError;
    if (!writeTextArtifact(actualArtifactPath, actualText, &writeError) ||
        !writeTextArtifact(diffArtifactPath,
                           unifiedTextDiff(snapshotPath, expectedText, actualText), &writeError)) {
        failScenario(writeError);
        return;
    }
    m_capturePaths.append(actualArtifactPath);
    m_capturePaths.append(diffArtifactPath);
    failScenario(QStringLiteral("Snapshot mismatch for %1 (actual: %2, diff: %3)")
                     .arg(snapshotPath, actualArtifactPath, diffArtifactPath));
}

QString Runner::scrollbackTextFrom(int fromRow, bool styled, QString* error) const {
    if (!m_terminalEngine) {
        if (error) *error = QStringLiteral("TerminalEngine is no longer available");
        return {};
    }
    if (fromRow < 0) {
        if (error) *error = QStringLiteral("fromRow must not be negative");
        return {};
    }
    const TextGridBuffer rows =
        m_terminalEngine->committedScrollback(static_cast<quint64>(fromRow));
    if (rows.height() <= 0) return {};
    return styled ? formatStyledText(rows) : normalizedText(rows);
}

int Runner::scrollbackRowCount() {
    if (!ensureRunning("scrollbackRowCount") || !ensureScrollbackSession("scrollbackRowCount"))
        return -1;
    return static_cast<int>(m_terminalEngine->committedScrollbackTotal());
}

QString Runner::scrollbackText(int fromRow) {
    if (!ensureRunning("scrollbackText") || !ensureScrollbackSession("scrollbackText")) return {};
    QString error;
    const QString text = scrollbackTextFrom(fromRow, false, &error);
    if (!error.isEmpty()) failScenario(error);
    return text;
}

void Runner::waitForScrollbackText(const QString& text, int timeoutMs) {
    if (!ensureRunning("waitForScrollbackText") ||
        !ensureScrollbackSession("waitForScrollbackText"))
        return;
    if (text.isEmpty()) {
        failScenario(QStringLiteral("Text conditions require a non-empty string"));
        return;
    }
    const int validated = validateTimeout(timeoutMs);
    if (!validated) return;
    const auto contains = [&] {
        QString error;
        const QString rows = scrollbackTextFrom(0, false, &error);
        if (!error.isEmpty()) {
            failScenario(error);
            return false;
        }
        return rows.contains(text);
    };
    (void)waitForEngineCondition(contains, validated,
                                 QStringLiteral("scrollback text \"%1\"").arg(text));
}

void Runner::expectScrollbackText(const QString& text, int timeoutMs) {
    if (!ensureRunning("expectScrollbackText") ||
        !ensureScrollbackSession("expectScrollbackText") ||
        !waitForNextFrame(resolveExpectTimeout(timeoutMs)))
        return;
    QString error;
    const QString rows = scrollbackTextFrom(0, false, &error);
    if (!error.isEmpty())
        failScenario(error);
    else if (text.isEmpty() || !rows.contains(text))
        failScenario(QStringLiteral("Expected scrollback to contain \"%1\"").arg(text));
}

void Runner::expectNoScrollbackText(const QString& text, int timeoutMs) {
    if (!ensureRunning("expectNoScrollbackText") ||
        !ensureScrollbackSession("expectNoScrollbackText") ||
        !waitForNextFrame(resolveExpectTimeout(timeoutMs)))
        return;
    QString error;
    const QString rows = scrollbackTextFrom(0, false, &error);
    if (!error.isEmpty())
        failScenario(error);
    else if (text.isEmpty() || rows.contains(text))
        failScenario(QStringLiteral("Expected scrollback not to contain \"%1\"").arg(text));
}

void Runner::captureScrollback(const QString& relativePath, int fromRow) {
    if (!ensureRunning("captureScrollback") || !ensureScrollbackSession("captureScrollback") ||
        !waitForNextFrame(m_operationTimeoutMs))
        return;
    QString error;
    const QString rows = scrollbackTextFrom(fromRow, false, &error);
    if (error.isEmpty() && writeTextArtifact(relativePath, rows, &error))
        m_capturePaths.append(QDir::cleanPath(relativePath));
    if (!error.isEmpty()) failScenario(error);
}

void Runner::captureScrollbackStyled(const QString& relativePath, int fromRow) {
    if (!ensureRunning("captureScrollbackStyled") ||
        !ensureScrollbackSession("captureScrollbackStyled") ||
        !waitForNextFrame(m_operationTimeoutMs))
        return;
    QString error;
    const QString rows = scrollbackTextFrom(fromRow, true, &error);
    if (error.isEmpty() && writeTextArtifact(relativePath, rows, &error))
        m_capturePaths.append(QDir::cleanPath(relativePath));
    if (!error.isEmpty()) failScenario(error);
}

void Runner::expectScrollbackSnapshot(const QString& snapshotRelativePath, int fromRow,
                                      int timeoutMs) {
    if (!ensureRunning("expectScrollbackSnapshot") ||
        !ensureScrollbackSession("expectScrollbackSnapshot") ||
        !waitForNextFrame(resolveExpectTimeout(timeoutMs)))
        return;
    QString error;
    const QString rows = scrollbackTextFrom(fromRow, false, &error);
    if (!error.isEmpty()) {
        failScenario(error);
        return;
    }
    compareSnapshot(snapshotRelativePath, rows);
}

QString Runner::normalizedText(const TextGridBuffer& frame) {
    QString result;
    for (int row = 0; row < frame.height(); ++row) {
        QString line;
        for (int column = 0; column < frame.width(); ++column) {
            const Cell& cell = frame.cellAt(column, row);
            if (cell.continuation) continue;
            line.append(cell.character().isEmpty() ? QStringLiteral(" ")
                                                   : QString::fromUtf8(cell.character()));
        }
        while (line.endsWith(QLatin1Char(' ')))
            line.chop(1);
        result.append(line);
        result.append(QLatin1Char('\n'));
    }
    return result;
}

QString Runner::formatStyledText(const TextGridBuffer& frame) {
    QString result = normalizedText(frame);
    result.append(QStringLiteral("--- styles\n"));
    const auto sameStyle = [](const Cell& lhs, const Cell& rhs) {
        return lhs.fgColor == rhs.fgColor && lhs.bgColor == rhs.bgColor &&
               lhs.attributes == rhs.attributes;
    };
    for (int row = 0; row < frame.height(); ++row) {
        int column = 0;
        while (column < frame.width()) {
            const Cell& first = frame.cellAt(column, row);
            int end = column + 1;
            while (end < frame.width() && sameStyle(first, frame.cellAt(end, row)))
                ++end;
            // Runs that use the terminal defaults everywhere carry no information
            // and would bury the interesting lines on themes without a background.
            const bool isDefault =
                first.fgColor == 0 && first.bgColor == 0 && first.attributes == TextAttribute::None;
            if (!isDefault) {
                QString line = QStringLiteral("y=%1 x=%2-%3 fg=%4 bg=%5")
                                   .arg(row)
                                   .arg(column)
                                   .arg(end - 1)
                                   .arg(first.fgColor ? runner_detail::rgbaString(first.fgColor)
                                                      : QStringLiteral("default"),
                                        first.bgColor ? runner_detail::rgbaString(first.bgColor)
                                                      : QStringLiteral("default"));
                const QString attributes = runner_detail::attributeNames(first.attributes);
                if (!attributes.isEmpty()) line += QLatin1Char(' ') + attributes;
                result.append(line);
                result.append(QLatin1Char('\n'));
            }
            column = end;
        }
    }
    return result;
}

QList<Runner::TextMatch> Runner::textMatches(const TextGridBuffer& frame,
                                             const QString& text) const {
    QList<TextMatch> matches;
    if (text.isEmpty() || text.contains(QLatin1Char('\n'))) return matches;

    for (int row = 0; row < frame.height(); ++row) {
        QString renderedRow;
        QVector<int> sourceColumns;
        QVector<int> sourceEnds;
        for (int column = 0; column < frame.width(); ++column) {
            const Cell& renderedCell = frame.cellAt(column, row);
            if (renderedCell.continuation) continue;
            const QString character = renderedCell.character().isEmpty()
                                          ? QStringLiteral(" ")
                                          : QString::fromUtf8(renderedCell.character());
            renderedRow.append(character);
            for (qsizetype index = 0; index < character.size(); ++index) {
                sourceColumns.append(column);
                sourceEnds.append(column +
                                  (std::max)(1, static_cast<int>(renderedCell.displayWidth)));
            }
        }

        qsizetype from = 0;
        while (from <= renderedRow.size() - text.size()) {
            const qsizetype matchIndex = renderedRow.indexOf(text, from);
            if (matchIndex < 0) break;
            const qsizetype lastIndex = matchIndex + text.size() - 1;
            if (matchIndex < sourceColumns.size() && lastIndex < sourceEnds.size()) {
                const int x = sourceColumns.at(matchIndex);
                matches.append(TextMatch{x, row, (std::max)(1, sourceEnds.at(lastIndex) - x)});
            }
            from = matchIndex + (std::max<qsizetype>)(1, text.size());
        }
    }
    return matches;
}

QString Runner::resolveSnapshotPath(const QString& path) const {
    if (path.startsWith(QLatin1Char(':')) || QFileInfo(path).isAbsolute())
        return QDir::cleanPath(path);
    return QDir(QFileInfo(m_scriptPath).path()).filePath(path);
}

QString Runner::unifiedTextDiff(const QString& expectedPath, const QString& expected,
                                const QString& actual) {
    const QStringList expectedLines = expected.split(QLatin1Char('\n'), Qt::KeepEmptyParts);
    const QStringList actualLines = actual.split(QLatin1Char('\n'), Qt::KeepEmptyParts);
    QString diff = QStringLiteral("--- %1\n+++ actual\n@@ -1,%2 +1,%3 @@\n")
                       .arg(expectedPath)
                       .arg(expectedLines.size())
                       .arg(actualLines.size());
    const qsizetype lineCount = (std::max)(expectedLines.size(), actualLines.size());
    for (qsizetype index = 0; index < lineCount; ++index) {
        const bool hasExpected = index < expectedLines.size();
        const bool hasActual = index < actualLines.size();
        if (hasExpected && hasActual && expectedLines.at(index) == actualLines.at(index)) {
            diff += QLatin1Char(' ') + expectedLines.at(index) + QLatin1Char('\n');
            continue;
        }
        if (hasExpected) diff += QLatin1Char('-') + expectedLines.at(index) + QLatin1Char('\n');
        if (hasActual) diff += QLatin1Char('+') + actualLines.at(index) + QLatin1Char('\n');
    }
    return diff;
}

QString Runner::currentFrameText(QString* error) const {
    if (!m_terminalEngine) {
        if (error) *error = QStringLiteral("TerminalEngine is no longer available");
        return {};
    }
    const std::optional<TextGridBuffer> frame = m_terminalEngine->currentFrame();
    if (!frame || frame->width() <= 0 || frame->height() <= 0) {
        if (error) *error = QStringLiteral("No committed frame buffer is available");
        return {};
    }
    return normalizedText(*frame);
}

QString Runner::currentStyledFrameText(QString* error) const {
    if (!m_terminalEngine) {
        if (error) *error = QStringLiteral("TerminalEngine is no longer available");
        return {};
    }
    const std::optional<TextGridBuffer> frame = m_terminalEngine->currentFrame();
    if (!frame || frame->width() <= 0 || frame->height() <= 0) {
        if (error) *error = QStringLiteral("No committed frame buffer is available");
        return {};
    }
    return formatStyledText(*frame);
}

QString Runner::resolveArtifactPath(const QString& relativePath, QString* error) const {
    const QString cleaned = QDir::cleanPath(relativePath);
    if (relativePath.trimmed().isEmpty() || cleaned == QLatin1String(".") ||
        QFileInfo(cleaned).isAbsolute() || cleaned == QLatin1String("..") ||
        cleaned.startsWith(QLatin1String("../")) || relativePath.contains(QLatin1Char('\n')) ||
        relativePath.contains(QLatin1Char('\r'))) {
        if (error)
            *error = QStringLiteral("Artifact path must be a relative path without newlines: %1")
                         .arg(relativePath);
        return {};
    }

    QString path = QDir(m_artifactDirectory).absoluteFilePath(cleaned);

    // The lexical checks above cannot see through symlinks, so verify the real
    // location: create the parent, then require that its canonical path is the
    // artifact root or below it. Without this a symlinked subdirectory lets a
    // scenario write anywhere on the filesystem while still reporting success.
    const QString parent = QFileInfo(path).absolutePath();
    if (!QDir().mkpath(parent)) {
        if (error)
            *error = QStringLiteral("Cannot create artifact parent directory for %1").arg(path);
        return {};
    }
    const QString canonicalRoot = QFileInfo(m_artifactDirectory).canonicalFilePath();
    const QString canonicalParent = QFileInfo(parent).canonicalFilePath();
    if (canonicalRoot.isEmpty() || canonicalParent.isEmpty() ||
        (canonicalParent != canonicalRoot &&
         !canonicalParent.startsWith(canonicalRoot + QLatin1Char('/')))) {
        if (error)
            *error = QStringLiteral("Artifact path escapes the artifact directory: %1")
                         .arg(relativePath);
        return {};
    }
    if (QFileInfo(path).isSymLink()) {
        if (error)
            *error = QStringLiteral("Artifact path is a symbolic link: %1").arg(relativePath);
        return {};
    }
    return path;
}

bool Runner::writeTextArtifact(const QString& relativePath, const QString& contents,
                               QString* error) const {
    QString localError;
    QString* destinationError = error ? error : &localError;
    const QString path = resolveArtifactPath(relativePath, destinationError);
    if (path.isEmpty()) return false;
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
        *destinationError =
            QStringLiteral("Cannot create artifact parent directory for %1").arg(path);
        return false;
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        *destinationError =
            QStringLiteral("Cannot open artifact %1: %2").arg(path, file.errorString());
        return false;
    }
    const QByteArray bytes = contents.toUtf8();
    if (file.write(bytes) != bytes.size()) {
        *destinationError =
            QStringLiteral("Cannot write artifact %1: %2").arg(path, file.errorString());
        return false;
    }
    if (!file.commit()) {
        *destinationError =
            QStringLiteral("Cannot commit artifact %1: %2").arg(path, file.errorString());
        return false;
    }
    return true;
}

void Runner::captureText(const QString& relativePath) {
    if (!ensureRunning("captureText") || !waitForNextFrame(m_operationTimeoutMs)) return;
    QString error;
    const QString frame = currentFrameText(&error);
    if (error.isEmpty() && writeTextArtifact(relativePath, frame, &error))
        m_capturePaths.append(QDir::cleanPath(relativePath));
    if (!error.isEmpty()) failScenario(error);
}

void Runner::captureStyled(const QString& relativePath) {
    if (!ensureRunning("captureStyled") || !waitForNextFrame(m_operationTimeoutMs)) return;
    QString error;
    const QString frame = currentStyledFrameText(&error);
    if (error.isEmpty() && writeTextArtifact(relativePath, frame, &error))
        m_capturePaths.append(QDir::cleanPath(relativePath));
    if (!error.isEmpty()) failScenario(error);
}

QString Runner::styledText() {
    if (!ensureRunning("styledText")) return {};
    QString error;
    const QString text = currentStyledFrameText(&error);
    if (!error.isEmpty()) failScenario(error);
    return text;
}

QString Runner::frameText() {
    if (!ensureRunning("frameText")) return {};
    QString error;
    const QString text = currentFrameText(&error);
    if (!error.isEmpty()) failScenario(error);
    return text;
}

QVariantMap Runner::cell(int x, int y) {
    if (!ensureRunning("cell") || !m_terminalEngine) return {};
    const std::optional<TextGridBuffer> frame = m_terminalEngine->currentFrame();
    if (!frame || x < 0 || y < 0 || x >= frame->width() || y >= frame->height()) {
        failScenario(
            QStringLiteral("Coordinates (%1,%2) are outside the committed frame").arg(x).arg(y));
        return {};
    }

    const Cell& renderedCell = frame->cellAt(x, y);
    const auto hasAttribute = [&](TextAttribute attribute) {
        return static_cast<uint8_t>(renderedCell.attributes & attribute) != 0;
    };
    QVariantMap attributes;
    attributes.insert(QStringLiteral("bold"), hasAttribute(TextAttribute::Bold));
    attributes.insert(QStringLiteral("dim"), hasAttribute(TextAttribute::Dim));
    attributes.insert(QStringLiteral("italic"), hasAttribute(TextAttribute::Italic));
    attributes.insert(QStringLiteral("underline"), hasAttribute(TextAttribute::Underline));
    attributes.insert(QStringLiteral("reversed"), hasAttribute(TextAttribute::Reversed));
    attributes.insert(QStringLiteral("blink"), hasAttribute(TextAttribute::Blink));
    attributes.insert(QStringLiteral("conceal"), hasAttribute(TextAttribute::Conceal));
    attributes.insert(QStringLiteral("strikethrough"), hasAttribute(TextAttribute::Strikethrough));

    QVariantMap result;
    result.insert(QStringLiteral("x"), x);
    result.insert(QStringLiteral("y"), y);
    result.insert(QStringLiteral("character"), QString::fromUtf8(renderedCell.character()));
    result.insert(QStringLiteral("continuation"), renderedCell.continuation);
    // "default" mirrors styledText(): the cell uses the terminal's own color.
    result.insert(QStringLiteral("foreground"),
                  renderedCell.fgColor ? runner_detail::rgbaString(renderedCell.fgColor)
                                       : QStringLiteral("default"));
    result.insert(QStringLiteral("background"),
                  renderedCell.bgColor ? runner_detail::rgbaString(renderedCell.bgColor)
                                       : QStringLiteral("default"));
    result.insert(QStringLiteral("attributes"), attributes);
    return result;
}

QVariantList Runner::findText(const QString& text) {
    if (!ensureRunning("findText") || !m_terminalEngine) return {};
    if (text.isEmpty() || text.contains(QLatin1Char('\n'))) {
        failScenario(QStringLiteral("app.findText() requires non-empty single-row text"));
        return {};
    }
    const std::optional<TextGridBuffer> frame = m_terminalEngine->currentFrame();
    if (!frame) {
        failScenario(QStringLiteral("No committed frame buffer is available"));
        return {};
    }

    QVariantList result;
    for (const TextMatch& match : textMatches(*frame, text)) {
        result.append(QVariantMap{{QStringLiteral("x"), match.x},
                                  {QStringLiteral("y"), match.y},
                                  {QStringLiteral("width"), match.width}});
    }
    return result;
}

QString Runner::regionText(int x, int y, int width, int height) {
    if (!ensureRunning("regionText") || !m_terminalEngine) return {};
    const std::optional<TextGridBuffer> frame = m_terminalEngine->currentFrame();
    if (!frame) {
        failScenario(QStringLiteral("No committed frame buffer is available"));
        return {};
    }
    if (x < 0 || y < 0 || width <= 0 || height <= 0 || x + width > frame->width() ||
        y + height > frame->height()) {
        failScenario(QStringLiteral("Region bounds [%1,%2,%3,%4] out of frame size %5x%6")
                         .arg(x)
                         .arg(y)
                         .arg(width)
                         .arg(height)
                         .arg(frame->width())
                         .arg(frame->height()));
        return {};
    }

    QString result;
    for (int row = y; row < y + height; ++row) {
        QString line;
        for (int column = x; column < x + width; ++column) {
            const Cell& cell = frame->cellAt(column, row);
            if (cell.continuation) continue;
            line.append(cell.character().isEmpty() ? QStringLiteral(" ")
                                                   : QString::fromUtf8(cell.character()));
        }
        while (line.endsWith(QLatin1Char(' ')))
            line.chop(1);
        result.append(line);
        result.append(QLatin1Char('\n'));
    }
    return result;
}

bool Runner::locateItem(const QString& objectName, QRect* outRect, QString* error) const {
    if (objectName.trimmed().isEmpty()) {
        *error = QStringLiteral("itemRect() requires a non-empty objectName");
        return false;
    }
    if (!m_terminal || !m_terminalEngine) {
        *error = QStringLiteral("Target Terminal is no longer available");
        return false;
    }
    Item* item = runner_detail::findVisibleItem(m_terminal, objectName);
    if (!item) {
        *error = QStringLiteral("No visible item with objectName \"%1\"").arg(objectName);
        return false;
    }
    if (m_terminalEngine->frameSequence() == 0) {
        *error = QStringLiteral("No committed frame buffer is available");
        return false;
    }
    const int frameWidth = m_terminalEngine->terminalWidth();
    const int frameHeight = m_terminalEngine->terminalHeight();
    if (frameWidth <= 0 || frameHeight <= 0) {
        *error = QStringLiteral("No committed frame buffer is available");
        return false;
    }
    const QPoint origin = item->absolutePosition();
    const QRect clipped =
        QRect(origin, QSize(item->width(), item->height())) & QRect(0, 0, frameWidth, frameHeight);
    if (clipped.isEmpty()) {
        *error = QStringLiteral("Item \"%1\" lies outside the %2x%3 viewport")
                     .arg(objectName)
                     .arg(frameWidth)
                     .arg(frameHeight);
        return false;
    }
    *outRect = clipped;
    return true;
}

QVariantMap Runner::itemRect(const QString& objectName) {
    if (!ensureRunning("itemRect")) return {};
    QRect rect;
    QString error;
    if (!locateItem(objectName, &rect, &error)) {
        failScenario(error);
        return {};
    }
    return {{QStringLiteral("x"), rect.x()},
            {QStringLiteral("y"), rect.y()},
            {QStringLiteral("width"), rect.width()},
            {QStringLiteral("height"), rect.height()}};
}

void Runner::waitForItem(const QString& objectName, int timeoutMs) {
    if (!ensureRunning("waitForItem")) return;
    const int validated = validateTimeout(timeoutMs);
    if (!validated) return;

    QRect rect;
    QString error;
    if (locateItem(objectName, &rect, &error)) return;

    if (m_activeLoop) {
        failScenario(QStringLiteral("Nested wait operations are not supported"));
        return;
    }

    bool matched = false;
    QEventLoop loop;
    QTimer pollTimer;
    pollTimer.setTimerType(Qt::PreciseTimer);
    pollTimer.setInterval(runner_detail::conditionPollIntervalMs);
    QTimer timeout;
    timeout.setSingleShot(true);
    // Visibility is QML state that normally arrives with a frame, but poll as
    // well so a change that failed to invalidate anything is still seen.
    const auto evaluate = [&] {
        QString pollError;
        if (locateItem(objectName, &rect, &pollError)) {
            matched = true;
            loop.quit();
        }
    };
    const QMetaObject::Connection frameConnection = connect(
        m_terminalEngine, &TerminalEngine::frameRendered, &loop, [&](quint64) { evaluate(); });
    connect(&pollTimer, &QTimer::timeout, &loop, evaluate);
    connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

    m_activeLoop = &loop;
    pollTimer.start();
    timeout.start(validated);
    m_terminalEngine->requestRender();
    loop.exec();
    m_activeLoop = nullptr;
    disconnect(frameConnection);

    if (m_state == State::Failed) return;
    if (!matched) {
        failScenario(QStringLiteral("Timed out after %1 ms waiting for item \"%2\" (%3)")
                         .arg(validated)
                         .arg(objectName, error) +
                     runner_detail::pausedClockHint());
    }
}

void Runner::expectRegionSnapshot(int x, int y, int width, int height,
                                  const QString& snapshotRelativePath, int timeoutMs) {
    if (!ensureRunning("expectRegionSnapshot") ||
        !waitForNextFrame(resolveExpectTimeout(timeoutMs)))
        return;
    const QString actualText = regionText(x, y, width, height);
    if (m_state == State::Failed) return;
    compareSnapshot(snapshotRelativePath, actualText);
}

bool Runner::writeFinalFrameArtifact(QString* error) {
    QString frameError;
    const QString frame = currentFrameText(&frameError);
    if (!frameError.isEmpty()) {
        m_finalFrameRelativePath.clear();
        if (error) *error = frameError;
        return false;
    }

    if (m_state == State::Failed)
        m_finalFrameRelativePath = QStringLiteral("failure.txt");
    else if (m_mode == QLatin1String("capture"))
        m_finalFrameRelativePath = QStringLiteral("capture.txt");
    else
        m_finalFrameRelativePath = QStringLiteral("final.txt");
    return writeTextArtifact(m_finalFrameRelativePath, frame, error);
}

bool Runner::writeResultArtifact(int exitCode, QString* error) {
    const auto operationObject = [](const OperationRecord& operation) {
        QJsonObject location;
        if (!operation.file.isEmpty()) {
            location.insert(QStringLiteral("file"), operation.file);
            location.insert(QStringLiteral("line"), operation.line);
            location.insert(QStringLiteral("column"), operation.column);
        }
        QJsonObject result{{QStringLiteral("index"), operation.index},
                           {QStringLiteral("name"), operation.name},
                           {QStringLiteral("status"), operation.status},
                           {QStringLiteral("elapsedMs"), operation.elapsedMs}};
        result.insert(QStringLiteral("location"),
                      location.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(location));
        return result;
    };

    QJsonArray operations;
    for (const OperationRecord& operation : std::as_const(m_operations))
        operations.append(operationObject(operation));

    QJsonObject viewport{{QStringLiteral("columns"), m_viewport.width()},
                         {QStringLiteral("rows"), m_viewport.height()}};
    // "viewport" keeps the initial size the scenario was configured with;
    // "finalViewport" records where resize() calls left it.
    QJsonValue finalViewport = QJsonValue(QJsonValue::Null);
    if (m_terminalEngine && m_terminalEngine->frameSequence() > 0) {
        finalViewport = QJsonObject{{QStringLiteral("columns"), m_terminalEngine->terminalWidth()},
                                    {QStringLiteral("rows"), m_terminalEngine->terminalHeight()}};
    }
    QJsonArray captures;
    m_capturePaths.removeDuplicates();
    for (const QString& capture : std::as_const(m_capturePaths))
        captures.append(capture);
    QJsonArray updatedSnapshots;
    m_updatedSnapshots.removeDuplicates();
    for (const QString& snapshot : std::as_const(m_updatedSnapshots))
        updatedSnapshots.append(snapshot);
    QJsonObject artifacts{{QStringLiteral("directory"), m_artifactDirectory},
                          {QStringLiteral("result"), QStringLiteral("result.json")},
                          {QStringLiteral("captures"), captures},
                          {QStringLiteral("updatedSnapshots"), updatedSnapshots}};
    QJsonArray casts;
    for (const QString& cast : std::as_const(m_castPaths))
        casts.append(cast);
    artifacts.insert(QStringLiteral("casts"), casts);
    artifacts.insert(QStringLiteral("finalFrame"), m_finalFrameRelativePath.isEmpty()
                                                       ? QJsonValue(QJsonValue::Null)
                                                       : QJsonValue(m_finalFrameRelativePath));

    QJsonObject root{{QStringLiteral("clock"),
                      Clock::isVirtual() ? QStringLiteral("virtual") : QStringLiteral("real")},
                     {QStringLiteral("schemaVersion"), 1},
                     {QStringLiteral("status"),
                      exitCode == 0 ? QStringLiteral("passed") : QStringLiteral("failed")},
                     {QStringLiteral("exitCode"), exitCode},
                     {QStringLiteral("mode"), m_mode},
                     {QStringLiteral("scenario"), m_scriptPath},
                     {QStringLiteral("message"), m_message},
                     {QStringLiteral("elapsedMs"), m_elapsed.isValid() ? m_elapsed.elapsed() : 0},
                     {QStringLiteral("viewport"), viewport},
                     {QStringLiteral("finalViewport"), finalViewport},
                     {QStringLiteral("operations"), operations},
                     {QStringLiteral("artifacts"), artifacts},
                     {QStringLiteral("frameStats"), QJsonObject::fromVariantMap(frameStats())}};
    if (Clock::isVirtual())
        root.insert(QStringLiteral("virtualTimeMs"), Clock::instance()->elapsed());
    root.insert(QStringLiteral("operation"),
                m_currentOperation >= 0 && m_currentOperation < m_operations.size()
                    ? QJsonValue(operationObject(m_operations.at(m_currentOperation)))
                    : QJsonValue(QJsonValue::Null));

    const QString json = QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return writeTextArtifact(QStringLiteral("result.json"), json, error);
}

} // namespace qmltui
