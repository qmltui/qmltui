#pragma once

#include <QSize>

namespace qmltui::runner_cli {

// Queries the host terminal emulator's character-cell dimensions when attached
// to an interactive TTY, returning an invalid QSize otherwise.
QSize queryHostTerminalSize();

} // namespace qmltui::runner_cli
