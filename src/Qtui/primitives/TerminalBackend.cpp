#include "TerminalBackend.h"

#ifdef _WIN32
#include "Win32ConsoleBackend.h"
#else
#include "PosixTerminalBackend.h"
#endif

namespace qtui {

TerminalBackend::TerminalBackend(QObject* parent) : QObject(parent) {}

TerminalBackend::~TerminalBackend() = default;

std::unique_ptr<TerminalBackend> createTerminalBackend(int outputFd)
{
#ifdef _WIN32
    return std::make_unique<Win32ConsoleBackend>(outputFd);
#else
    return std::make_unique<PosixTerminalBackend>(outputFd);
#endif
}

} // namespace qtui
