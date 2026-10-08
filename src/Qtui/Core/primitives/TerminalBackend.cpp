#include <Qtui/Core/primitives/TerminalBackend.h>

#ifdef _WIN32
#include <Qtui/Core/primitives/Win32ConsoleBackend.h>
#else
#include <Qtui/Core/primitives/PosixTerminalBackend.h>
#endif

namespace qtui {

TerminalBackend::TerminalBackend(QObject* parent) : QObject(parent) {}

TerminalBackend::~TerminalBackend() = default;

namespace {
TerminalBackend::Factory& installedFactory()
{
    static TerminalBackend::Factory factory;
    return factory;
}
}

void TerminalBackend::setFactory(Factory factory)
{
    installedFactory() = std::move(factory);
}

const TerminalBackend::Factory& TerminalBackend::factory()
{
    return installedFactory();
}

std::unique_ptr<TerminalBackend> createTerminalBackend(int outputFd)
{
    if (const TerminalBackend::Factory& factory = TerminalBackend::factory()) {
        if (std::unique_ptr<TerminalBackend> backend = factory(outputFd))
            return backend;
    }
#ifdef _WIN32
    return std::make_unique<Win32ConsoleBackend>(outputFd);
#else
    return std::make_unique<PosixTerminalBackend>(outputFd);
#endif
}

} // namespace qtui
