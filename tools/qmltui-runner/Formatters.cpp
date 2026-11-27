#include "Formatters.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QStringList>
#include <algorithm>

namespace qmltui::runner_cli {

QByteArray renderLiveAnsiFrame(const QByteArray& ansiFrame, const QByteArray& scrollbackAnsi) {
    QByteArray out;
    out.reserve(ansiFrame.size() + scrollbackAnsi.size() + 256);
    // Synchronized update (?2026h) + cursor home (H) keeps frames tear-free.
    out.append("\033[?2026h\033[H");
    const auto appendClearedLines = [&](const QByteArray& block) {
        const QList<QByteArray> lines = block.split('\n');
        for (qsizetype i = 0; i < lines.size(); ++i) {
            out.append(lines.at(i));
            out.append("\033[0m\033[K");
            if (i + 1 < lines.size()) out.append("\r\n");
        }
    };
    if (!scrollbackAnsi.isEmpty()) {
        appendClearedLines(scrollbackAnsi);
        out.append("\r\n");
    }
    appendClearedLines(ansiFrame);
    out.append("\033[0m\033[J\033[?2026l");
    return out;
}

QString formatRulerFrame(const QString& text, int cols, int rows, bool showRuler) {
    QString trimmed = text;
    if (trimmed.endsWith(QLatin1Char('\n'))) trimmed.chop(1);
    QStringList rawLines = trimmed.isEmpty() ? QStringList{} : trimmed.split(QLatin1Char('\n'));
    if (!showRuler) return rawLines.join(QLatin1Char('\n'));

    const int displayRows = (std::max)(rows, static_cast<int>(rawLines.size()));
    while (rawLines.size() < displayRows)
        rawLines.append(QString());

    const int rowWidth =
        (std::max)(2, static_cast<int>(QString::number((std::max)(0, displayRows - 1)).size()));
    const QString gutterPad(rowWidth + 2, QLatin1Char(' '));
    QString tens;
    QString ones;
    tens.reserve(cols);
    ones.reserve(cols);
    for (int x = 0; x < cols; ++x) {
        tens.append(x % 10 == 0 ? QChar('0' + ((x / 10) % 10)) : QChar(' '));
        ones.append(QChar('0' + (x % 10)));
    }

    QStringList out;
    out << (gutterPad + tens);
    out << (gutterPad + ones);
    out << (QString(rowWidth, QLatin1Char(' ')) + QStringLiteral(" ┌") +
            QString(cols, QChar(0x2500)));
    for (int y = 0; y < displayRows; ++y) {
        out << QStringLiteral("%1 │%2").arg(y, rowWidth, 10, QChar('0')).arg(rawLines.at(y));
    }
    return out.join(QLatin1Char('\n'));
}

QString extractStylesSection(const QString& styledText) {
    const QString marker = QStringLiteral("\n--- styles\n");
    const int idx = styledText.indexOf(marker);
    if (idx >= 0) return styledText.mid(idx + marker.size()).trimmed();
    const QString head = QStringLiteral("--- styles\n");
    if (styledText.startsWith(head)) return styledText.mid(head.size()).trimmed();
    return {};
}

QString formatFrameStatsLine(const QJsonObject& stats) {
    if (stats.isEmpty()) return {};
    return QStringLiteral(
               "FrameStats: frames=%1 paintAvg=%2us paintMax=%3us diffBytes=%4 bytesTotal=%5 "
               "dirtyDuringPaint=%6 skippedBackpressure=%7")
        .arg(stats.value(QStringLiteral("frames")).toInt())
        .arg(stats.value(QStringLiteral("paintMicrosAvg")).toInt())
        .arg(stats.value(QStringLiteral("paintMicrosMax")).toInt())
        .arg(stats.value(QStringLiteral("diffBytes")).toInt())
        .arg(stats.value(QStringLiteral("bytesTotal")).toInteger())
        .arg(stats.value(QStringLiteral("dirtyDuringPaintTotal")).toInt())
        .arg(stats.value(QStringLiteral("skippedForBackpressure")).toInt());
}

QString formatItemTree(const QJsonObject& node, int indent) {
    if (node.isEmpty()) return {};
    QStringList lines;
    QString line(indent * 2, QLatin1Char(' '));
    line += node.value(QStringLiteral("type")).toString(QStringLiteral("Item"));
    const QString objectName = node.value(QStringLiteral("objectName")).toString();
    if (!objectName.isEmpty()) line += QStringLiteral(" #%1").arg(objectName);
    line += QStringLiteral(" (%1,%2 %3x%4)")
                .arg(node.value(QStringLiteral("x")).toInt())
                .arg(node.value(QStringLiteral("y")).toInt())
                .arg(node.value(QStringLiteral("width")).toInt())
                .arg(node.value(QStringLiteral("height")).toInt());

    if (node.value(QStringLiteral("activeFocus")).toBool())
        line += QStringLiteral(" [ACTIVE FOCUS]");
    else if (node.value(QStringLiteral("focus")).toBool())
        line += QStringLiteral(" [focus]");
    if (!node.value(QStringLiteral("visible")).toBool(true)) line += QStringLiteral(" [hidden]");
    if (!node.value(QStringLiteral("enabled")).toBool(true)) line += QStringLiteral(" [disabled]");
    if (!node.value(QStringLiteral("inViewport")).toBool(true))
        line += QStringLiteral(" [offscreen]");
    if (node.value(QStringLiteral("clip")).toBool()) line += QStringLiteral(" [clip]");

    const auto escapeQuoted = [](QString s) {
        s.replace(QLatin1Char('"'), QStringLiteral("\\\""));
        return s;
    };

    const QString focusPolicy = node.value(QStringLiteral("focusPolicy")).toString();
    if (!focusPolicy.isEmpty()) line += QStringLiteral(" focusPolicy=%1").arg(focusPolicy);
    const QString state = node.value(QStringLiteral("state")).toString();
    if (!state.isEmpty()) line += QStringLiteral(" state=\"%1\"").arg(escapeQuoted(state));
    const QString text = node.value(QStringLiteral("text")).toString();
    if (!text.isEmpty()) line += QStringLiteral(" text=\"%1\"").arg(escapeQuoted(text));
    const QString title = node.value(QStringLiteral("title")).toString();
    if (!title.isEmpty()) line += QStringLiteral(" title=\"%1\"").arg(escapeQuoted(title));
    const QString placeholder = node.value(QStringLiteral("placeholderText")).toString();
    if (!placeholder.isEmpty())
        line += QStringLiteral(" placeholder=\"%1\"").arg(escapeQuoted(placeholder));
    if (node.contains(QStringLiteral("checked")))
        line += QStringLiteral(" checked=%1")
                    .arg(node.value(QStringLiteral("checked")).toBool() ? QStringLiteral("true")
                                                                        : QStringLiteral("false"));
    if (node.contains(QStringLiteral("currentIndex")))
        line += QStringLiteral(" currentIndex=%1")
                    .arg(node.value(QStringLiteral("currentIndex")).toInt());
    if (node.contains(QStringLiteral("value"))) {
        const QJsonValue v = node.value(QStringLiteral("value"));
        if (v.isString())
            line += QStringLiteral(" value=\"%1\"").arg(escapeQuoted(v.toString()));
        else if (v.isBool())
            line += QStringLiteral(" value=%1")
                        .arg(v.toBool() ? QStringLiteral("true") : QStringLiteral("false"));
        else if (v.isDouble())
            line += QStringLiteral(" value=%1").arg(v.toDouble());
    }

    lines << line;
    const QJsonArray children = node.value(QStringLiteral("children")).toArray();
    for (const QJsonValue& child : children) {
        if (child.isObject()) lines << formatItemTree(child.toObject(), indent + 1);
    }
    return lines.join(QLatin1Char('\n'));
}

QString formatInteractiveResponse(const QJsonObject& response, bool showRuler, bool showStyled,
                                  bool showScrollback, bool showTree) {
    QStringList out;
    const QString sessionId = response.value(QStringLiteral("session")).toString();
    if (!sessionId.isEmpty() && response.value(QStringLiteral("started")).toBool()) {
        out << QStringLiteral("Session: %1 (pid=%2, clock=%3, mode=%4, artifacts=%5)")
                   .arg(sessionId)
                   .arg(response.value(QStringLiteral("pid")).toInteger())
                   .arg(response.value(QStringLiteral("clock")).toString(),
                        response.value(QStringLiteral("mode")).toString(),
                        response.value(QStringLiteral("artifacts")).toString());
    }
    if (!response.value(QStringLiteral("ok")).toBool(true)) {
        const QString err = response.value(QStringLiteral("error")).toString();
        if (!err.isEmpty()) out << QStringLiteral("Error: %1").arg(err);
    }
    if (response.value(QStringLiteral("exited")).toBool()) {
        out << QStringLiteral("Application exited (status=%1, exitCode=%2)")
                   .arg(response.value(QStringLiteral("exitStatus"))
                            .toString(QStringLiteral("normal")))
                   .arg(response.value(QStringLiteral("exitCode")).toInt());
    }
    if (response.contains(QStringLiteral("result")) &&
        !response.value(QStringLiteral("result")).isNull() &&
        !response.value(QStringLiteral("result")).isUndefined()) {
        const QJsonValue res = response.value(QStringLiteral("result"));
        if (res.isString()) {
            out << QStringLiteral("Result: %1").arg(res.toString());
        } else if (res.isBool()) {
            out << QStringLiteral("Result: %1")
                       .arg(res.toBool() ? QStringLiteral("true") : QStringLiteral("false"));
        } else if (res.isDouble()) {
            out << QStringLiteral("Result: %1").arg(res.toDouble());
        } else if (res.isObject()) {
            out << QStringLiteral("Result: %1")
                       .arg(QString::fromUtf8(
                           QJsonDocument(res.toObject()).toJson(QJsonDocument::Compact)));
        } else if (res.isArray()) {
            out << QStringLiteral("Result: %1")
                       .arg(QString::fromUtf8(
                           QJsonDocument(res.toArray()).toJson(QJsonDocument::Compact)));
        }
    }

    const QJsonObject stats = response.value(QStringLiteral("frameStats")).toObject();
    const QString statsLine = formatFrameStatsLine(stats);
    if (!statsLine.isEmpty()) {
        QString line = statsLine;
        if (response.contains(QStringLiteral("virtualTimeMs"))) {
            line += QStringLiteral(" virtualTime=%1ms")
                        .arg(response.value(QStringLiteral("virtualTimeMs")).toInteger());
        }
        out << line;
        const int dirty = stats.value(QStringLiteral("dirtyDuringPaint")).toInt();
        const int dirtyTotal = stats.value(QStringLiteral("dirtyDuringPaintTotal")).toInt();
        if (dirty > 0 || dirtyTotal > 0) {
            out << QStringLiteral("WARNING: dirtyDuringPaint detected (frame=%1, total=%2)")
                       .arg(dirty)
                       .arg(dirtyTotal);
        }
    }

    const QJsonArray logs = response.value(QStringLiteral("logs")).toArray();
    if (!logs.isEmpty()) {
        out << QStringLiteral("--- Logs ---");
        for (const QJsonValue& entry : logs)
            out << entry.toString();
    }

    if (response.contains(QStringLiteral("frameDiff"))) {
        const QJsonArray diff = response.value(QStringLiteral("frameDiff")).toArray();
        if (diff.isEmpty()) {
            out << QStringLiteral("\n=== Frame Diff (no visual changes) ===");
        } else {
            int maxRow = 0;
            for (const QJsonValue& entry : diff) {
                maxRow = (std::max)(maxRow, entry.toObject().value(QStringLiteral("row")).toInt());
            }
            const int rowWidth = (std::max)(2, static_cast<int>(QString::number(maxRow).size()));
            out << QStringLiteral("\n=== Frame Diff (%1 changed row(s)) ===").arg(diff.size());
            for (const QJsonValue& entry : diff) {
                const QJsonObject rowObj = entry.toObject();
                const int row = rowObj.value(QStringLiteral("row")).toInt();
                out << QStringLiteral("- %1 │%2")
                           .arg(row, rowWidth, 10, QChar('0'))
                           .arg(rowObj.value(QStringLiteral("before")).toString());
                out << QStringLiteral("+ %1 │%2")
                           .arg(row, rowWidth, 10, QChar('0'))
                           .arg(rowObj.value(QStringLiteral("after")).toString());
            }
        }
    }

    if (response.contains(QStringLiteral("frame"))) {
        const QJsonObject vp = response.value(QStringLiteral("viewport")).toObject();
        const int cols = vp.value(QStringLiteral("columns")).toInt(80);
        const int rows = vp.value(QStringLiteral("rows")).toInt(24);
        const int frameCount = response.value(QStringLiteral("frameCount")).toInt(-1);
        QString header = QStringLiteral("\n=== Viewport %1x%2").arg(cols).arg(rows);
        if (frameCount >= 0) {
            header += QStringLiteral(" (frame %1").arg(frameCount);
            if (response.contains(QStringLiteral("frameDelta"))) {
                const int delta = response.value(QStringLiteral("frameDelta")).toInt();
                header += QStringLiteral(", +%1").arg(delta);
            }
            header += QLatin1Char(')');
        }
        header += QStringLiteral(" ===");
        out << header;
        out << formatRulerFrame(response.value(QStringLiteral("frame")).toString(), cols, rows,
                                showRuler);
    }

    if (showStyled && response.contains(QStringLiteral("styled"))) {
        const QString styles =
            extractStylesSection(response.value(QStringLiteral("styled")).toString());
        out << QStringLiteral("--- Styles ---");
        out << (styles.isEmpty() ? QStringLiteral("(all default styles)") : styles);
    }

    if (showScrollback && response.contains(QStringLiteral("scrollback"))) {
        const QString sb = response.value(QStringLiteral("scrollback")).toString();
        const int rowCount = response.value(QStringLiteral("scrollbackRowCount")).toInt(0);
        out << QStringLiteral("\n=== Inline Scrollback (%1 rows) ===").arg(rowCount);
        out << (sb.trimmed().isEmpty() ? QStringLiteral("(empty scrollback)")
                                       : (sb.endsWith(QLatin1Char('\n')) ? sb.chopped(1) : sb));
        if (showStyled && response.contains(QStringLiteral("scrollbackStyled"))) {
            const QString styles =
                extractStylesSection(response.value(QStringLiteral("scrollbackStyled")).toString());
            out << QStringLiteral("--- Scrollback Styles ---");
            out << (styles.isEmpty() ? QStringLiteral("(all default styles)") : styles);
        }
    }

    if (showTree && response.contains(QStringLiteral("tree")) &&
        response.value(QStringLiteral("tree")).isObject()) {
        out << QStringLiteral("\n=== Item Tree ===");
        out << formatItemTree(response.value(QStringLiteral("tree")).toObject());
    }

    return out.join(QLatin1Char('\n')) + QLatin1Char('\n');
}

} // namespace qmltui::runner_cli
