#pragma once

#include <QStringList>

namespace qmltui::runner_cli {

// Handles `qmltui-runner session <verb> [args...]`.
int runSessionCli(const QStringList& args);

} // namespace qmltui::runner_cli
