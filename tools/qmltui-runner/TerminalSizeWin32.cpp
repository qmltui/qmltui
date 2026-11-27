#include "TerminalSize.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <algorithm>

namespace qmltui::runner_cli {

QSize queryHostTerminalSize() {
    for (DWORD stdId : {STD_OUTPUT_HANDLE, STD_ERROR_HANDLE}) {
        const HANDLE handle = GetStdHandle(stdId);
        if (handle == INVALID_HANDLE_VALUE || handle == nullptr) continue;
        CONSOLE_SCREEN_BUFFER_INFO info{};
        if (!GetConsoleScreenBufferInfo(handle, &info)) continue;
        const int cols = info.srWindow.Right - info.srWindow.Left + 1;
        const int rows = info.srWindow.Bottom - info.srWindow.Top + 1;
        if (cols > 0 && rows > 0)
            return QSize((std::clamp)(cols, 1, 1000), (std::clamp)(rows, 1, 1000));
    }
    return {};
}

} // namespace qmltui::runner_cli
