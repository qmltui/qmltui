#pragma once

#include <QStringList>

namespace qmltui::runner_cli {

// Runs `qmltui-runner session connect [id]` (interactive console) or
// `qmltui-runner session batch [id]` (non-interactive piped command script).
int runSessionConnect(const QStringList& args, bool defaultBatch = false);

} // namespace qmltui::runner_cli
