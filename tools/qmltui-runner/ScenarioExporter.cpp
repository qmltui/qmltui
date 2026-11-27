#include "ScenarioExporter.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QStringList>

namespace qmltui::runner_cli {

namespace {

QString jsString(const QString& value) {
    const QByteArray json = QJsonDocument(QJsonArray{value}).toJson(QJsonDocument::Compact);
    return QString::fromUtf8(json.mid(1, json.size() - 2));
}

QString jsStringArray(const QJsonValue& value) {
    QJsonArray array;
    if (value.isArray()) {
        array = value.toArray();
    } else if (value.isString() && !value.toString().trimmed().isEmpty()) {
        for (const QString& item : value.toString().split(QLatin1Char(','), Qt::SkipEmptyParts))
            array.append(item.trimmed());
    }
    return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
}

QString formatStep(const RecordedStep& step) {
    const QString& cmd = step.cmd;
    const QJsonObject& a = step.args;

    if (cmd == QLatin1String("eval")) {
        const QString code = a.value(QStringLiteral("code")).toString().trimmed();
        QStringList lines = code.split(QLatin1Char('\n'));
        for (qsizetype i = lines.size() - 1; i >= 0; --i) {
            const QString trimmed = lines.at(i).trimmed();
            if (trimmed.isEmpty()) continue;
            if (!trimmed.startsWith(QLatin1String("//")) && !trimmed.endsWith(QLatin1Char(';')) &&
                !trimmed.endsWith(QLatin1Char('}')) && !trimmed.endsWith(QLatin1String("*/"))) {
                lines[i] += QLatin1Char(';');
            }
            break;
        }
        QStringList indented;
        for (const QString& line : std::as_const(lines))
            indented << (QStringLiteral("    ") + line);
        return indented.join(QLatin1Char('\n'));
    }

    const auto hasModifiers = [&] {
        const QJsonValue v = a.value(QStringLiteral("modifiers"));
        return (v.isArray() && !v.toArray().isEmpty()) ||
               (v.isString() && !v.toString().trimmed().isEmpty());
    };
    const auto button = [&] {
        return a.value(QStringLiteral("button")).toString(QStringLiteral("left"));
    };

    if (cmd == QLatin1String("press") || cmd == QLatin1String("injectKey")) {
        if (hasModifiers()) {
            return QStringLiteral("    app.%1(%2, %3);")
                .arg(cmd, jsString(a.value(QStringLiteral("key")).toString()),
                     jsStringArray(a.value(QStringLiteral("modifiers"))));
        }
        return QStringLiteral("    app.%1(%2);")
            .arg(cmd, jsString(a.value(QStringLiteral("key")).toString()));
    }
    if (cmd == QLatin1String("typeText") || cmd == QLatin1String("injectText") ||
        cmd == QLatin1String("paste") || cmd == QLatin1String("injectPaste")) {
        return QStringLiteral("    app.%1(%2);")
            .arg(cmd, jsString(a.value(QStringLiteral("text")).toString()));
    }
    if (cmd == QLatin1String("click") || cmd == QLatin1String("doubleClick") ||
        cmd == QLatin1String("mouseDown") || cmd == QLatin1String("mouseUp")) {
        QStringList params{QString::number(a.value(QStringLiteral("x")).toInt()),
                           QString::number(a.value(QStringLiteral("y")).toInt())};
        const QString btn = button();
        if (btn != QLatin1String("left") || hasModifiers()) params << jsString(btn);
        if (hasModifiers()) params << jsStringArray(a.value(QStringLiteral("modifiers")));
        return QStringLiteral("    app.%1(%2);").arg(cmd, params.join(QStringLiteral(", ")));
    }
    if (cmd == QLatin1String("hover")) {
        QStringList params{QString::number(a.value(QStringLiteral("x")).toInt()),
                           QString::number(a.value(QStringLiteral("y")).toInt())};
        if (hasModifiers()) params << jsStringArray(a.value(QStringLiteral("modifiers")));
        return QStringLiteral("    app.hover(%1);").arg(params.join(QStringLiteral(", ")));
    }
    if (cmd == QLatin1String("drag")) {
        QStringList params{QString::number(a.value(QStringLiteral("startX")).toInt()),
                           QString::number(a.value(QStringLiteral("startY")).toInt()),
                           QString::number(a.value(QStringLiteral("endX")).toInt()),
                           QString::number(a.value(QStringLiteral("endY")).toInt())};
        const QString btn = button();
        if (btn != QLatin1String("left") || hasModifiers()) params << jsString(btn);
        if (hasModifiers()) params << jsStringArray(a.value(QStringLiteral("modifiers")));
        return QStringLiteral("    app.drag(%1);").arg(params.join(QStringLiteral(", ")));
    }
    if (cmd == QLatin1String("scroll")) {
        QStringList params{QString::number(a.value(QStringLiteral("x")).toInt()),
                           QString::number(a.value(QStringLiteral("y")).toInt()),
                           QString::number(a.value(QStringLiteral("deltaX")).toInt()),
                           QString::number(a.value(QStringLiteral("deltaY")).toInt())};
        if (hasModifiers()) params << jsStringArray(a.value(QStringLiteral("modifiers")));
        return QStringLiteral("    app.scroll(%1);").arg(params.join(QStringLiteral(", ")));
    }
    if (cmd == QLatin1String("clickText")) {
        QStringList params{jsString(a.value(QStringLiteral("text")).toString())};
        const int occ = a.value(QStringLiteral("occurrence")).toInt(0);
        const QString btn = button();
        if (occ != 0 || btn != QLatin1String("left") || hasModifiers())
            params << QString::number(occ);
        if (btn != QLatin1String("left") || hasModifiers()) params << jsString(btn);
        if (hasModifiers()) params << jsStringArray(a.value(QStringLiteral("modifiers")));
        return QStringLiteral("    app.clickText(%1);").arg(params.join(QStringLiteral(", ")));
    }
    if (cmd == QLatin1String("hoverText")) {
        QStringList params{jsString(a.value(QStringLiteral("text")).toString())};
        const int occ = a.value(QStringLiteral("occurrence")).toInt(0);
        if (occ != 0 || hasModifiers()) params << QString::number(occ);
        if (hasModifiers()) params << jsStringArray(a.value(QStringLiteral("modifiers")));
        return QStringLiteral("    app.hoverText(%1);").arg(params.join(QStringLiteral(", ")));
    }
    if (cmd == QLatin1String("clickItem")) {
        const QString name = jsString(a.value(QStringLiteral("objectName")).toString());
        const QString btn = button();
        QStringList optEntries;
        if (btn != QLatin1String("left"))
            optEntries << QStringLiteral("button: %1").arg(jsString(btn));
        if (hasModifiers())
            optEntries << QStringLiteral("modifiers: %1")
                              .arg(jsStringArray(a.value(QStringLiteral("modifiers"))));
        if (optEntries.isEmpty()) return QStringLiteral("    app.item(%1).click();").arg(name);
        return QStringLiteral("    app.item(%1).click({ %2 });")
            .arg(name, optEntries.join(QStringLiteral(", ")));
    }
    if (cmd == QLatin1String("resize")) {
        return QStringLiteral("    app.resize(%1, %2);")
            .arg(a.value(QStringLiteral("columns")).toInt())
            .arg(a.value(QStringLiteral("rows")).toInt());
    }
    if (cmd == QLatin1String("advanceTime") || cmd == QLatin1String("sleep")) {
        return QStringLiteral("    app.%1(%2);")
            .arg(cmd)
            .arg(a.value(QStringLiteral("durationMs")).toInt());
    }
    if (cmd == QLatin1String("waitForFrame") || cmd == QLatin1String("expectRepaint")) {
        if (a.contains(QStringLiteral("timeoutMs"))) {
            return QStringLiteral("    app.%1(%2);")
                .arg(cmd)
                .arg(a.value(QStringLiteral("timeoutMs")).toInt());
        }
        return QStringLiteral("    app.%1();").arg(cmd);
    }
    if (cmd == QLatin1String("waitForText") || cmd == QLatin1String("expectText") ||
        cmd == QLatin1String("expectNoText") || cmd == QLatin1String("waitForScrollbackText") ||
        cmd == QLatin1String("expectScrollbackText") ||
        cmd == QLatin1String("expectNoScrollbackText")) {
        QStringList params{jsString(a.value(QStringLiteral("text")).toString())};
        if (a.contains(QStringLiteral("timeoutMs")))
            params << QString::number(a.value(QStringLiteral("timeoutMs")).toInt());
        return QStringLiteral("    app.%1(%2);").arg(cmd, params.join(QStringLiteral(", ")));
    }
    if (cmd == QLatin1String("waitForRegex") || cmd == QLatin1String("expectRegex")) {
        QStringList params{jsString(a.value(QStringLiteral("pattern")).toString())};
        if (a.contains(QStringLiteral("timeoutMs")))
            params << QString::number(a.value(QStringLiteral("timeoutMs")).toInt());
        return QStringLiteral("    app.%1(%2);").arg(cmd, params.join(QStringLiteral(", ")));
    }
    if (cmd == QLatin1String("waitForItem")) {
        QStringList params{jsString(a.value(QStringLiteral("objectName")).toString())};
        if (a.contains(QStringLiteral("timeoutMs")))
            params << QString::number(a.value(QStringLiteral("timeoutMs")).toInt());
        return QStringLiteral("    app.waitForItem(%1);").arg(params.join(QStringLiteral(", ")));
    }
    if (cmd == QLatin1String("startRecording")) {
        return QStringLiteral("    app.startRecording(%1);")
            .arg(jsString(a.value(QStringLiteral("path")).toString()));
    }
    if (cmd == QLatin1String("stopRecording")) return QStringLiteral("    app.stopRecording();");
    if (cmd == QLatin1String("marker")) {
        return QStringLiteral("    app.marker(%1);")
            .arg(jsString(a.value(QStringLiteral("label")).toString()));
    }
    return {};
}

} // namespace

bool isExportableCommand(const QString& cmd) {
    static const QStringList exportable = {
        QStringLiteral("press"),
        QStringLiteral("typeText"),
        QStringLiteral("paste"),
        QStringLiteral("injectKey"),
        QStringLiteral("injectText"),
        QStringLiteral("injectPaste"),
        QStringLiteral("click"),
        QStringLiteral("doubleClick"),
        QStringLiteral("mouseDown"),
        QStringLiteral("mouseUp"),
        QStringLiteral("hover"),
        QStringLiteral("drag"),
        QStringLiteral("scroll"),
        QStringLiteral("clickText"),
        QStringLiteral("hoverText"),
        QStringLiteral("clickItem"),
        QStringLiteral("resize"),
        QStringLiteral("advanceTime"),
        QStringLiteral("sleep"),
        QStringLiteral("waitForFrame"),
        QStringLiteral("expectRepaint"),
        QStringLiteral("waitForText"),
        QStringLiteral("waitForRegex"),
        QStringLiteral("waitForItem"),
        QStringLiteral("waitForScrollbackText"),
        QStringLiteral("expectText"),
        QStringLiteral("expectNoText"),
        QStringLiteral("expectRegex"),
        QStringLiteral("expectScrollbackText"),
        QStringLiteral("expectNoScrollbackText"),
        QStringLiteral("startRecording"),
        QStringLiteral("stopRecording"),
        QStringLiteral("marker"),
        QStringLiteral("eval"),
    };
    return exportable.contains(cmd);
}

QString exportScenarioScript(const QVector<RecordedStep>& history, const ExportMetadata& metadata) {
    QStringList lines;
    bool hasRecording = false;
    for (const RecordedStep& step : history) {
        if (step.cmd == QLatin1String("startRecording")) {
            hasRecording = true;
            break;
        }
    }
    if (metadata.columns > 0 && metadata.rows > 0) {
        QString header = QStringLiteral("// Recorded from qmltui-runner session (%1x%2")
                             .arg(metadata.columns)
                             .arg(metadata.rows);
        if (!metadata.clock.isEmpty()) header += QStringLiteral(", clock=%1").arg(metadata.clock);
        header += QLatin1Char(')');
        lines << header;
    }
    if (hasRecording) {
        lines << QStringLiteral(
            "// Note: includes app.startRecording(); replay with --record <file.cast> "
            "(or RECORD in qmltui_add_scenario).");
    }
    lines << QStringLiteral("/** @param {Runner} app */");
    lines << QStringLiteral("function main(app) {");
    for (const RecordedStep& step : history) {
        const QString formatted = formatStep(step);
        if (!formatted.isEmpty()) lines << formatted;
    }
    lines << QStringLiteral("}");
    return lines.join(QLatin1Char('\n')) + QLatin1Char('\n');
}

} // namespace qmltui::runner_cli
