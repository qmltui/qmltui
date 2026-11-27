#include "TerminalSize.h"

#include <algorithm>
#include <sys/ioctl.h>
#include <unistd.h>

namespace qmltui::runner_cli {

QSize queryHostTerminalSize() {
    for (int fd : {STDOUT_FILENO, STDERR_FILENO, STDIN_FILENO}) {
        if (::isatty(fd) != 1) continue;
        winsize ws{};
        if (::ioctl(fd, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
            return QSize((std::clamp)(static_cast<int>(ws.ws_col), 1, 1000),
                         (std::clamp)(static_cast<int>(ws.ws_row), 1, 1000));
        }
    }
    return {};
}

} // namespace qmltui::runner_cli
