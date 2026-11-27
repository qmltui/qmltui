#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace qmltui::runner_cli {

struct StepOptions {
    QString sessionId;
    bool showRuler = true;
    bool includeFrame = true;
    bool frameFlagExplicit = false;
    bool diffFlagExplicit = false;
    bool showDiff = false;
    bool showStyled = false;
    bool showScrollback = false;
    bool showTree = false;
    bool includeHidden = false;
    bool noRecord = false;
    bool json = false;
    bool allLogs = false;
    bool batch = false;
    bool continueOnError = false;
    QString button = QStringLiteral("left");
    QString modifiers;
    int occurrence = 0;
    int fromRow = 0;
    int timeoutMs = -1;
    QString outputPath;
    QString exportPath;
    bool stopAll = false;
    QStringList explicitFlags;
};

struct ParsedSessionStep {
    QString verb;
    QString cmd;
    QJsonObject request;
    StepOptions options;
    bool showStyled = false;
    bool showScrollback = false;
    bool showTree = false;
};

bool parseIntValue(const QString& raw, const QString& label, int minVal, int maxVal, int* out,
                   QString* error);

// Returns true when `--json` appears before `--` in rawArgs.
bool hasJsonFlag(const QStringList& rawArgs);

// Validates that every canonical flag recorded in explicitFlags is permitted
// for verb.
bool validateAllowedFlags(const QString& verb, const QStringList& explicitFlags,
                          const QStringList& allowed, QString* error);

// Parses common step options while treating signed integer literals (e.g. -1)
// and arguments after `--` as positional values.
bool parseStepArgs(const QStringList& rawArgs, StepOptions* opts, QStringList* positional,
                   QString* error);

// Splits a shell-style console line into tokens, supporting single quotes,
// double quotes, backslash escapes, and `#` comments outside quotes.
bool splitCommandLine(const QString& line, QStringList* tokens, QString* error);

// Builds a Runner session JSON request for one action/inspection verb.
bool buildSessionStepRequest(const QString& verb, const QStringList& rawArgs,
                             const StepOptions& defaults, ParsedSessionStep* out, QString* error);

// Atomically writes UTF-8 text to disk, creating parent directories if needed.
bool writeTextFileAtomic(const QString& path, const QString& text, QString* error);

} // namespace qmltui::runner_cli
