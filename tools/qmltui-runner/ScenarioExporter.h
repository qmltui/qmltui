#pragma once

#include <QJsonObject>
#include <QString>
#include <QVector>

namespace qmltui::runner_cli {

struct RecordedStep {
    QString cmd;
    QJsonObject args;
};

struct ExportMetadata {
    QString clock;
    int columns = 0;
    int rows = 0;
};

// Returns true when cmd is a mutating, wait, assertion, or recording step that
// should be retained in exported scenarios (excluding pure read-only queries).
bool isExportableCommand(const QString& cmd);

// Formats recorded interactive steps into a runnable Runner JavaScript scenario
// with JSDoc typing (/** @param {Runner} app */ function main(app) { ... }).
QString exportScenarioScript(const QVector<RecordedStep>& history,
                             const ExportMetadata& metadata = {});

} // namespace qmltui::runner_cli
