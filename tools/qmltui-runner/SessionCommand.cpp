#include "SessionCommand.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QRegularExpression>
#include <QSaveFile>

namespace qmltui::runner_cli {

namespace {

bool isSignedIntLiteral(const QString& token) {
    static const QRegularExpression re(QStringLiteral("^-?\\d+$"));
    return re.match(token).hasMatch();
}

QJsonArray modifiersArray(const QString& spec) {
    QJsonArray arr;
    for (const QString& part : spec.split(QLatin1Char(','), Qt::SkipEmptyParts)) {
        const QString trimmed = part.trimmed();
        if (!trimmed.isEmpty()) arr.append(trimmed);
    }
    return arr;
}

} // namespace

bool parseIntValue(const QString& raw, const QString& label, int minVal, int maxVal, int* out,
                   QString* error) {
    bool ok = false;
    const int value = raw.toInt(&ok);
    if (!ok || value < minVal || value > maxVal) {
        *error = QStringLiteral("Invalid %1 '%2': expected integer in %3..%4.")
                     .arg(label, raw)
                     .arg(minVal)
                     .arg(maxVal);
        return false;
    }
    *out = value;
    return true;
}

bool writeTextFileAtomic(const QString& path, const QString& text, QString* error) {
    const QString abs = QFileInfo(path).absoluteFilePath();
    if (!QDir().mkpath(QFileInfo(abs).absolutePath())) {
        *error = QStringLiteral("Cannot create parent directory for %1").arg(abs);
        return false;
    }
    QSaveFile file(abs);
    const QByteArray bytes = text.toUtf8();
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text) || file.write(bytes) != bytes.size() ||
        !file.commit()) {
        *error = file.errorString();
        return false;
    }
    return true;
}

bool splitCommandLine(const QString& line, QStringList* tokens, QString* error) {
    tokens->clear();
    QString current;
    bool inToken = false;
    QChar quote;
    for (qsizetype i = 0; i < line.size(); ++i) {
        const QChar ch = line.at(i);
        if (!quote.isNull()) {
            if (quote == QLatin1Char('"') && ch == QLatin1Char('\\') && i + 1 < line.size()) {
                const QChar next = line.at(++i);
                if (next == QLatin1Char('n'))
                    current.append(QLatin1Char('\n'));
                else if (next == QLatin1Char('t'))
                    current.append(QLatin1Char('\t'));
                else
                    current.append(next);
            } else if (ch == quote) {
                quote = QChar();
            } else {
                current.append(ch);
            }
            continue;
        }
        if (ch == QLatin1Char('#') && !inToken) break;
        if (ch.isSpace()) {
            if (inToken) {
                tokens->append(current);
                current.clear();
                inToken = false;
            }
            continue;
        }
        if (ch == QLatin1Char('\'') || ch == QLatin1Char('"')) {
            quote = ch;
            inToken = true;
            continue;
        }
        if (ch == QLatin1Char('\\') && i + 1 < line.size()) {
            current.append(line.at(++i));
            inToken = true;
            continue;
        }
        current.append(ch);
        inToken = true;
    }
    if (!quote.isNull()) {
        if (error) *error = QStringLiteral("Unterminated %1 quote.").arg(quote);
        return false;
    }
    if (inToken) tokens->append(current);
    return true;
}

bool hasJsonFlag(const QStringList& rawArgs) {
    for (const QString& arg : rawArgs) {
        if (arg == QLatin1String("--")) break;
        if (arg == QLatin1String("--json")) return true;
    }
    return false;
}

bool validateAllowedFlags(const QString& verb, const QStringList& explicitFlags,
                          const QStringList& allowed, QString* error) {
    for (const QString& flag : explicitFlags) {
        if (!allowed.contains(flag)) {
            if (error) {
                *error = QStringLiteral("Option %1 is not valid for \"%2\".").arg(flag, verb);
            }
            return false;
        }
    }
    return true;
}

bool parseStepArgs(const QStringList& rawArgs, StepOptions* opts, QStringList* positional,
                   QString* error) {
    opts->explicitFlags.clear();
    opts->diffFlagExplicit = false;
    if (hasJsonFlag(rawArgs)) opts->json = true;

    const auto recordFlag = [&](const QString& canonical) {
        if (!opts->explicitFlags.contains(canonical)) opts->explicitFlags.append(canonical);
    };

    bool endOfOptions = false;
    for (int i = 0; i < rawArgs.size(); ++i) {
        const QString& arg = rawArgs.at(i);
        if (endOfOptions || arg == QLatin1String("-") || !arg.startsWith(QLatin1Char('-')) ||
            isSignedIntLiteral(arg)) {
            positional->append(arg);
            continue;
        }
        if (arg == QLatin1String("--")) {
            endOfOptions = true;
            continue;
        }

        QString flag = arg;
        QString inlineValue;
        bool hasInline = false;
        const qsizetype eq = flag.indexOf(QLatin1Char('='));
        if (eq > 0) {
            inlineValue = flag.mid(eq + 1);
            flag = flag.left(eq);
            hasInline = true;
        }

        const auto takeValue = [&](QString* out) -> bool {
            if (hasInline) {
                *out = inlineValue;
                return true;
            }
            if (i + 1 >= rawArgs.size()) {
                *error = QStringLiteral("Option %1 requires a value.").arg(flag);
                return false;
            }
            *out = rawArgs.at(++i);
            return true;
        };

        if (flag == QLatin1String("--session") || flag == QLatin1String("-s")) {
            if (!takeValue(&opts->sessionId)) return false;
            recordFlag(QStringLiteral("--session"));
        } else if (flag == QLatin1String("--ruler")) {
            opts->showRuler = true;
            opts->includeFrame = true;
            opts->frameFlagExplicit = true;
            recordFlag(QStringLiteral("--ruler"));
        } else if (flag == QLatin1String("--no-ruler") || flag == QLatin1String("--plain")) {
            opts->showRuler = false;
            recordFlag(flag);
        } else if (flag == QLatin1String("--no-frame")) {
            opts->includeFrame = false;
            opts->frameFlagExplicit = true;
            recordFlag(QStringLiteral("--no-frame"));
        } else if (flag == QLatin1String("--diff")) {
            opts->showDiff = true;
            opts->diffFlagExplicit = true;
            recordFlag(QStringLiteral("--diff"));
        } else if (flag == QLatin1String("--styled")) {
            opts->showStyled = true;
            recordFlag(QStringLiteral("--styled"));
        } else if (flag == QLatin1String("--scrollback")) {
            opts->showScrollback = true;
            recordFlag(QStringLiteral("--scrollback"));
        } else if (flag == QLatin1String("--tree")) {
            opts->showTree = true;
            recordFlag(QStringLiteral("--tree"));
        } else if (flag == QLatin1String("--include-hidden")) {
            opts->includeHidden = true;
            recordFlag(QStringLiteral("--include-hidden"));
        } else if (flag == QLatin1String("--no-record")) {
            opts->noRecord = true;
            recordFlag(QStringLiteral("--no-record"));
        } else if (flag == QLatin1String("--json")) {
            opts->json = true;
            recordFlag(QStringLiteral("--json"));
        } else if (flag == QLatin1String("--batch")) {
            opts->batch = true;
            recordFlag(QStringLiteral("--batch"));
        } else if (flag == QLatin1String("--continue-on-error")) {
            opts->continueOnError = true;
            recordFlag(QStringLiteral("--continue-on-error"));
        } else if (flag == QLatin1String("--all")) {
            opts->allLogs = true;
            opts->stopAll = true;
            recordFlag(QStringLiteral("--all"));
        } else if (flag == QLatin1String("--button")) {
            if (!takeValue(&opts->button)) return false;
            recordFlag(QStringLiteral("--button"));
        } else if (flag == QLatin1String("--modifiers")) {
            if (!takeValue(&opts->modifiers)) return false;
            recordFlag(QStringLiteral("--modifiers"));
        } else if (flag == QLatin1String("--occurrence")) {
            QString raw;
            if (!takeValue(&raw) || !parseIntValue(raw, QStringLiteral("--occurrence"), 0, 100000,
                                                   &opts->occurrence, error))
                return false;
            recordFlag(QStringLiteral("--occurrence"));
        } else if (flag == QLatin1String("--from-row")) {
            QString raw;
            if (!takeValue(&raw) || !parseIntValue(raw, QStringLiteral("--from-row"), 0, 10000000,
                                                   &opts->fromRow, error))
                return false;
            recordFlag(QStringLiteral("--from-row"));
        } else if (flag == QLatin1String("--timeout")) {
            QString raw;
            if (!takeValue(&raw) || !parseIntValue(raw, QStringLiteral("--timeout"), 1, 300000,
                                                   &opts->timeoutMs, error))
                return false;
            recordFlag(QStringLiteral("--timeout"));
        } else if (flag == QLatin1String("-o") || flag == QLatin1String("--output")) {
            if (!takeValue(&opts->outputPath)) return false;
            recordFlag(QStringLiteral("-o"));
        } else if (flag == QLatin1String("--export")) {
            if (!takeValue(&opts->exportPath)) return false;
            recordFlag(QStringLiteral("--export"));
        } else {
            *error = QStringLiteral("Unknown option: %1").arg(flag);
            return false;
        }
    }
    return true;
}

bool buildSessionStepRequest(const QString& verb, const QStringList& rawArgs,
                             const StepOptions& defaults, ParsedSessionStep* out, QString* error) {
    StepOptions opts = defaults;
    opts.frameFlagExplicit = false;
    opts.diffFlagExplicit = false;
    opts.noRecord = false;
    opts.explicitFlags.clear();
    QStringList pos;
    const bool parsedOk = parseStepArgs(rawArgs, &opts, &pos, error);
    // Always propagate parsed options (notably `--json`) to `out` so callers
    // can format CLI parse/usage errors in the requested output format.
    out->options = opts;
    if (!parsedOk) return false;

    QString cmd;
    QJsonObject cmdArgs;
    bool defaultIncludeFrame = true;
    bool showTree = opts.showTree;
    bool showStyled = opts.showStyled;
    bool showScrollback = opts.showScrollback;
    QStringList allowedFlags = {QStringLiteral("--session"),       QStringLiteral("--json"),
                                QStringLiteral("--ruler"),         QStringLiteral("--no-ruler"),
                                QStringLiteral("--plain"),         QStringLiteral("--no-frame"),
                                QStringLiteral("--diff"),          QStringLiteral("--styled"),
                                QStringLiteral("--scrollback"),    QStringLiteral("--tree"),
                                QStringLiteral("--include-hidden")};
    if (opts.showScrollback) allowedFlags << QStringLiteral("--from-row");

    const auto requireCount = [&](int n, const QString& usage) -> bool {
        if (pos.size() != n) {
            *error = QStringLiteral("Usage: %1 %2").arg(verb, usage);
            return false;
        }
        return true;
    };

    if (verb == QLatin1String("press") || verb == QLatin1String("inject-key")) {
        allowedFlags << QStringLiteral("--modifiers") << QStringLiteral("--no-record");
        if (!validateAllowedFlags(verb, opts.explicitFlags, allowedFlags, error)) return false;
        if (!requireCount(1, QStringLiteral("<key> [--modifiers Shift,Ctrl]"))) return false;
        cmd = (verb == QLatin1String("press")) ? QStringLiteral("press")
                                               : QStringLiteral("injectKey");
        cmdArgs.insert(QStringLiteral("key"), pos.at(0));
        if (!opts.modifiers.isEmpty())
            cmdArgs.insert(QStringLiteral("modifiers"), modifiersArray(opts.modifiers));
    } else if (verb == QLatin1String("type") || verb == QLatin1String("inject-text") ||
               verb == QLatin1String("paste") || verb == QLatin1String("inject-paste")) {
        allowedFlags << QStringLiteral("--no-record");
        if (!validateAllowedFlags(verb, opts.explicitFlags, allowedFlags, error)) return false;
        if (!requireCount(1, QStringLiteral("<text>"))) return false;
        if (verb == QLatin1String("type"))
            cmd = QStringLiteral("typeText");
        else if (verb == QLatin1String("inject-text"))
            cmd = QStringLiteral("injectText");
        else if (verb == QLatin1String("paste"))
            cmd = QStringLiteral("paste");
        else
            cmd = QStringLiteral("injectPaste");
        cmdArgs.insert(QStringLiteral("text"), pos.at(0));
    } else if (verb == QLatin1String("click") || verb == QLatin1String("double-click") ||
               verb == QLatin1String("mouse-down") || verb == QLatin1String("mouse-up")) {
        allowedFlags << QStringLiteral("--button") << QStringLiteral("--modifiers")
                     << QStringLiteral("--no-record");
        if (!validateAllowedFlags(verb, opts.explicitFlags, allowedFlags, error)) return false;
        if (!requireCount(2,
                          QStringLiteral("<x> <y> [--button left|middle|right] [--modifiers ...]")))
            return false;
        int x = 0;
        int y = 0;
        if (!parseIntValue(pos.at(0), QStringLiteral("x"), 0, 10000, &x, error) ||
            !parseIntValue(pos.at(1), QStringLiteral("y"), 0, 10000, &y, error))
            return false;
        if (verb == QLatin1String("click"))
            cmd = QStringLiteral("click");
        else if (verb == QLatin1String("double-click"))
            cmd = QStringLiteral("doubleClick");
        else if (verb == QLatin1String("mouse-down"))
            cmd = QStringLiteral("mouseDown");
        else
            cmd = QStringLiteral("mouseUp");
        cmdArgs.insert(QStringLiteral("x"), x);
        cmdArgs.insert(QStringLiteral("y"), y);
        cmdArgs.insert(QStringLiteral("button"), opts.button);
        if (!opts.modifiers.isEmpty())
            cmdArgs.insert(QStringLiteral("modifiers"), modifiersArray(opts.modifiers));
    } else if (verb == QLatin1String("hover")) {
        allowedFlags << QStringLiteral("--modifiers") << QStringLiteral("--no-record");
        if (!validateAllowedFlags(verb, opts.explicitFlags, allowedFlags, error)) return false;
        if (!requireCount(2, QStringLiteral("<x> <y> [--modifiers ...]"))) return false;
        int x = 0;
        int y = 0;
        if (!parseIntValue(pos.at(0), QStringLiteral("x"), 0, 10000, &x, error) ||
            !parseIntValue(pos.at(1), QStringLiteral("y"), 0, 10000, &y, error))
            return false;
        cmd = QStringLiteral("hover");
        cmdArgs.insert(QStringLiteral("x"), x);
        cmdArgs.insert(QStringLiteral("y"), y);
        if (!opts.modifiers.isEmpty())
            cmdArgs.insert(QStringLiteral("modifiers"), modifiersArray(opts.modifiers));
    } else if (verb == QLatin1String("drag")) {
        allowedFlags << QStringLiteral("--button") << QStringLiteral("--modifiers")
                     << QStringLiteral("--no-record");
        if (!validateAllowedFlags(verb, opts.explicitFlags, allowedFlags, error)) return false;
        if (!requireCount(4,
                          QStringLiteral("<x1> <y1> <x2> <y2> [--button ...] [--modifiers ...]")))
            return false;
        int x1 = 0;
        int y1 = 0;
        int x2 = 0;
        int y2 = 0;
        if (!parseIntValue(pos.at(0), QStringLiteral("x1"), 0, 10000, &x1, error) ||
            !parseIntValue(pos.at(1), QStringLiteral("y1"), 0, 10000, &y1, error) ||
            !parseIntValue(pos.at(2), QStringLiteral("x2"), 0, 10000, &x2, error) ||
            !parseIntValue(pos.at(3), QStringLiteral("y2"), 0, 10000, &y2, error))
            return false;
        cmd = QStringLiteral("drag");
        cmdArgs.insert(QStringLiteral("startX"), x1);
        cmdArgs.insert(QStringLiteral("startY"), y1);
        cmdArgs.insert(QStringLiteral("endX"), x2);
        cmdArgs.insert(QStringLiteral("endY"), y2);
        cmdArgs.insert(QStringLiteral("button"), opts.button);
        if (!opts.modifiers.isEmpty())
            cmdArgs.insert(QStringLiteral("modifiers"), modifiersArray(opts.modifiers));
    } else if (verb == QLatin1String("scroll")) {
        allowedFlags << QStringLiteral("--modifiers") << QStringLiteral("--no-record");
        if (!validateAllowedFlags(verb, opts.explicitFlags, allowedFlags, error)) return false;
        if (!requireCount(4, QStringLiteral("<x> <y> <deltaX> <deltaY> [--modifiers ...]")))
            return false;
        int x = 0;
        int y = 0;
        int dx = 0;
        int dy = 0;
        if (!parseIntValue(pos.at(0), QStringLiteral("x"), 0, 10000, &x, error) ||
            !parseIntValue(pos.at(1), QStringLiteral("y"), 0, 10000, &y, error) ||
            !parseIntValue(pos.at(2), QStringLiteral("deltaX"), -1000, 1000, &dx, error) ||
            !parseIntValue(pos.at(3), QStringLiteral("deltaY"), -1000, 1000, &dy, error))
            return false;
        cmd = QStringLiteral("scroll");
        cmdArgs.insert(QStringLiteral("x"), x);
        cmdArgs.insert(QStringLiteral("y"), y);
        cmdArgs.insert(QStringLiteral("deltaX"), dx);
        cmdArgs.insert(QStringLiteral("deltaY"), dy);
        if (!opts.modifiers.isEmpty())
            cmdArgs.insert(QStringLiteral("modifiers"), modifiersArray(opts.modifiers));
    } else if (verb == QLatin1String("click-text") || verb == QLatin1String("hover-text")) {
        allowedFlags << QStringLiteral("--occurrence") << QStringLiteral("--modifiers")
                     << QStringLiteral("--no-record");
        if (verb == QLatin1String("click-text")) allowedFlags << QStringLiteral("--button");
        if (!validateAllowedFlags(verb, opts.explicitFlags, allowedFlags, error)) return false;
        if (!requireCount(
                1, QStringLiteral("<text> [--occurrence N] [--button ...] [--modifiers ...]")))
            return false;
        cmd = (verb == QLatin1String("click-text")) ? QStringLiteral("clickText")
                                                    : QStringLiteral("hoverText");
        cmdArgs.insert(QStringLiteral("text"), pos.at(0));
        cmdArgs.insert(QStringLiteral("occurrence"), opts.occurrence);
        if (verb == QLatin1String("click-text"))
            cmdArgs.insert(QStringLiteral("button"), opts.button);
        if (!opts.modifiers.isEmpty())
            cmdArgs.insert(QStringLiteral("modifiers"), modifiersArray(opts.modifiers));
    } else if (verb == QLatin1String("click-item")) {
        allowedFlags << QStringLiteral("--button") << QStringLiteral("--modifiers")
                     << QStringLiteral("--no-record");
        if (!validateAllowedFlags(verb, opts.explicitFlags, allowedFlags, error)) return false;
        if (!requireCount(1, QStringLiteral("<objectName> [--button ...] [--modifiers ...]")))
            return false;
        cmd = QStringLiteral("clickItem");
        cmdArgs.insert(QStringLiteral("objectName"), pos.at(0));
        cmdArgs.insert(QStringLiteral("button"), opts.button);
        if (!opts.modifiers.isEmpty())
            cmdArgs.insert(QStringLiteral("modifiers"), modifiersArray(opts.modifiers));
    } else if (verb == QLatin1String("resize")) {
        allowedFlags << QStringLiteral("--no-record");
        if (!validateAllowedFlags(verb, opts.explicitFlags, allowedFlags, error)) return false;
        if (!requireCount(2, QStringLiteral("<columns> <rows>"))) return false;
        int cols = 0;
        int rows = 0;
        if (!parseIntValue(pos.at(0), QStringLiteral("columns"), 1, 1000, &cols, error) ||
            !parseIntValue(pos.at(1), QStringLiteral("rows"), 1, 1000, &rows, error))
            return false;
        cmd = QStringLiteral("resize");
        cmdArgs.insert(QStringLiteral("columns"), cols);
        cmdArgs.insert(QStringLiteral("rows"), rows);
    } else if (verb == QLatin1String("advance") || verb == QLatin1String("sleep")) {
        allowedFlags << QStringLiteral("--no-record");
        if (!validateAllowedFlags(verb, opts.explicitFlags, allowedFlags, error)) return false;
        if (!requireCount(1, QStringLiteral("<ms>"))) return false;
        int ms = 0;
        if (!parseIntValue(pos.at(0), QStringLiteral("ms"), 1, 30000, &ms, error)) return false;
        cmd = (verb == QLatin1String("advance")) ? QStringLiteral("advanceTime")
                                                 : QStringLiteral("sleep");
        cmdArgs.insert(QStringLiteral("durationMs"), ms);
    } else if (verb == QLatin1String("wait-frame") || verb == QLatin1String("expect-repaint")) {
        allowedFlags << QStringLiteral("--timeout") << QStringLiteral("--no-record");
        if (!validateAllowedFlags(verb, opts.explicitFlags, allowedFlags, error)) return false;
        if (!requireCount(0, QStringLiteral("[--timeout <ms>]"))) return false;
        cmd = (verb == QLatin1String("wait-frame")) ? QStringLiteral("waitForFrame")
                                                    : QStringLiteral("expectRepaint");
        if (opts.timeoutMs > 0) cmdArgs.insert(QStringLiteral("timeoutMs"), opts.timeoutMs);
    } else if (verb == QLatin1String("wait-text") || verb == QLatin1String("expect-text") ||
               verb == QLatin1String("expect-no-text") ||
               verb == QLatin1String("wait-scrollback") ||
               verb == QLatin1String("expect-scrollback") ||
               verb == QLatin1String("expect-no-scrollback")) {
        allowedFlags << QStringLiteral("--timeout") << QStringLiteral("--no-record");
        if (!validateAllowedFlags(verb, opts.explicitFlags, allowedFlags, error)) return false;
        if (!requireCount(1, QStringLiteral("<text> [--timeout <ms>]"))) return false;
        if (verb == QLatin1String("wait-text"))
            cmd = QStringLiteral("waitForText");
        else if (verb == QLatin1String("expect-text"))
            cmd = QStringLiteral("expectText");
        else if (verb == QLatin1String("expect-no-text"))
            cmd = QStringLiteral("expectNoText");
        else if (verb == QLatin1String("wait-scrollback"))
            cmd = QStringLiteral("waitForScrollbackText");
        else if (verb == QLatin1String("expect-scrollback"))
            cmd = QStringLiteral("expectScrollbackText");
        else
            cmd = QStringLiteral("expectNoScrollbackText");
        cmdArgs.insert(QStringLiteral("text"), pos.at(0));
        if (opts.timeoutMs > 0) cmdArgs.insert(QStringLiteral("timeoutMs"), opts.timeoutMs);
    } else if (verb == QLatin1String("wait-regex") || verb == QLatin1String("expect-regex")) {
        allowedFlags << QStringLiteral("--timeout") << QStringLiteral("--no-record");
        if (!validateAllowedFlags(verb, opts.explicitFlags, allowedFlags, error)) return false;
        if (!requireCount(1, QStringLiteral("<pattern> [--timeout <ms>]"))) return false;
        cmd = (verb == QLatin1String("wait-regex")) ? QStringLiteral("waitForRegex")
                                                    : QStringLiteral("expectRegex");
        cmdArgs.insert(QStringLiteral("pattern"), pos.at(0));
        if (opts.timeoutMs > 0) cmdArgs.insert(QStringLiteral("timeoutMs"), opts.timeoutMs);
    } else if (verb == QLatin1String("wait-item")) {
        allowedFlags << QStringLiteral("--timeout") << QStringLiteral("--no-record");
        if (!validateAllowedFlags(verb, opts.explicitFlags, allowedFlags, error)) return false;
        if (!requireCount(1, QStringLiteral("<objectName> [--timeout <ms>]"))) return false;
        cmd = QStringLiteral("waitForItem");
        cmdArgs.insert(QStringLiteral("objectName"), pos.at(0));
        if (opts.timeoutMs > 0) cmdArgs.insert(QStringLiteral("timeoutMs"), opts.timeoutMs);
    } else if (verb == QLatin1String("frame")) {
        allowedFlags << QStringLiteral("--from-row");
        if (!validateAllowedFlags(verb, opts.explicitFlags, allowedFlags, error)) return false;
        if (!requireCount(
                0, QStringLiteral("[--ruler|--no-ruler|--plain] [--diff] [--styled] [--scrollback] "
                                  "[--from-row N] [--tree]")))
            return false;
        cmd = QStringLiteral("frame");
        if (opts.diffFlagExplicit) {
            opts.showDiff = true;
        } else {
            // An explicit `frame` command without `--diff` always shows the full
            // frame even if the console default is `mode off` or `mode diff`.
            opts.includeFrame = true;
            opts.showDiff = false;
        }
    } else if (verb == QLatin1String("scrollback")) {
        allowedFlags << QStringLiteral("--from-row");
        if (!validateAllowedFlags(verb, opts.explicitFlags, allowedFlags, error)) return false;
        if (!requireCount(0, QStringLiteral("[--from-row N] [--styled]"))) return false;
        cmd = QStringLiteral("scrollback");
        cmdArgs.insert(QStringLiteral("fromRow"), opts.fromRow);
        defaultIncludeFrame = false;
        showScrollback = true;
    } else if (verb == QLatin1String("tree")) {
        if (!validateAllowedFlags(verb, opts.explicitFlags, allowedFlags, error)) return false;
        if (!requireCount(0, QStringLiteral("[--include-hidden]"))) return false;
        cmd = QStringLiteral("itemTree");
        cmdArgs.insert(QStringLiteral("includeHidden"), opts.includeHidden);
        defaultIncludeFrame = false;
        showTree = true;
    } else if (verb == QLatin1String("cell")) {
        if (!validateAllowedFlags(verb, opts.explicitFlags, allowedFlags, error)) return false;
        if (!requireCount(2, QStringLiteral("<x> <y>"))) return false;
        int x = 0;
        int y = 0;
        if (!parseIntValue(pos.at(0), QStringLiteral("x"), 0, 10000, &x, error) ||
            !parseIntValue(pos.at(1), QStringLiteral("y"), 0, 10000, &y, error))
            return false;
        cmd = QStringLiteral("cell");
        cmdArgs.insert(QStringLiteral("x"), x);
        cmdArgs.insert(QStringLiteral("y"), y);
        defaultIncludeFrame = false;
    } else if (verb == QLatin1String("find-text")) {
        if (!validateAllowedFlags(verb, opts.explicitFlags, allowedFlags, error)) return false;
        if (!requireCount(1, QStringLiteral("<text>"))) return false;
        cmd = QStringLiteral("findText");
        cmdArgs.insert(QStringLiteral("text"), pos.at(0));
        defaultIncludeFrame = false;
    } else if (verb == QLatin1String("item-rect")) {
        if (!validateAllowedFlags(verb, opts.explicitFlags, allowedFlags, error)) return false;
        if (!requireCount(1, QStringLiteral("<objectName>"))) return false;
        cmd = QStringLiteral("itemRect");
        cmdArgs.insert(QStringLiteral("objectName"), pos.at(0));
        defaultIncludeFrame = false;
    } else if (verb == QLatin1String("stats")) {
        if (!validateAllowedFlags(verb, opts.explicitFlags, allowedFlags, error)) return false;
        if (!requireCount(0, QString())) return false;
        cmd = QStringLiteral("frameStats");
        defaultIncludeFrame = false;
    } else if (verb == QLatin1String("logs")) {
        allowedFlags << QStringLiteral("--all");
        if (!validateAllowedFlags(verb, opts.explicitFlags, allowedFlags, error)) return false;
        if (!requireCount(0, QStringLiteral("[--all]"))) return false;
        cmd = QStringLiteral("logs");
        cmdArgs.insert(QStringLiteral("all"), opts.allLogs);
        defaultIncludeFrame = false;
    } else if (verb == QLatin1String("eval")) {
        allowedFlags << QStringLiteral("--timeout") << QStringLiteral("--no-record");
        if (!validateAllowedFlags(verb, opts.explicitFlags, allowedFlags, error)) return false;
        if (pos.isEmpty()) {
            *error = QStringLiteral("Usage: eval <js-code> [--timeout <ms>] [--no-record]");
            return false;
        }
        cmd = QStringLiteral("eval");
        cmdArgs.insert(QStringLiteral("code"), pos.join(QLatin1Char(' ')));
    } else if (verb == QLatin1String("marker")) {
        allowedFlags << QStringLiteral("--no-record");
        if (!validateAllowedFlags(verb, opts.explicitFlags, allowedFlags, error)) return false;
        if (!requireCount(1, QStringLiteral("<label>"))) return false;
        cmd = QStringLiteral("marker");
        cmdArgs.insert(QStringLiteral("label"), pos.at(0));
        defaultIncludeFrame = false;
    } else if (verb == QLatin1String("record")) {
        allowedFlags << QStringLiteral("--no-record");
        if (!validateAllowedFlags(verb, opts.explicitFlags, allowedFlags, error)) return false;
        if (pos.size() == 2 && pos.at(0) == QLatin1String("start")) {
            cmd = QStringLiteral("startRecording");
            cmdArgs.insert(QStringLiteral("path"), pos.at(1));
            defaultIncludeFrame = false;
        } else if (pos.size() == 1 && pos.at(0) == QLatin1String("stop")) {
            cmd = QStringLiteral("stopRecording");
            defaultIncludeFrame = false;
        } else {
            *error = QStringLiteral("Usage: record start <rel.cast> | stop");
            return false;
        }
    } else {
        *error =
            QStringLiteral("Unknown session command \"%1\". Run `qmltui-runner session --help`.")
                .arg(verb);
        return false;
    }

    if (opts.noRecord) cmdArgs.insert(QStringLiteral("record"), false);

    const bool wantFullFrame =
        defaultIncludeFrame && opts.includeFrame && (!opts.showDiff || opts.frameFlagExplicit);

    QJsonObject include;
    include.insert(QStringLiteral("frame"), wantFullFrame);
    if (opts.showDiff && defaultIncludeFrame) include.insert(QStringLiteral("diff"), true);
    if (showStyled) include.insert(QStringLiteral("styled"), true);
    if (showScrollback) {
        include.insert(QStringLiteral("scrollback"), true);
        if (opts.fromRow > 0) include.insert(QStringLiteral("fromRow"), opts.fromRow);
    }
    if (showTree) {
        include.insert(QStringLiteral("tree"), true);
        include.insert(QStringLiteral("includeHidden"), opts.includeHidden);
    }

    QJsonObject request{{QStringLiteral("cmd"), cmd},
                        {QStringLiteral("args"), cmdArgs},
                        {QStringLiteral("include"), include}};
    if (opts.timeoutMs > 0) request.insert(QStringLiteral("timeoutMs"), opts.timeoutMs);

    out->verb = verb;
    out->cmd = cmd;
    out->request = request;
    out->options = opts;
    out->showStyled = showStyled;
    out->showScrollback = showScrollback;
    out->showTree = showTree;
    return true;
}

} // namespace qmltui::runner_cli
