#include <QmlTui/Core/Primitives/TerminalBackend.h>

#if defined(_WIN32)
#include <QmlTui/Core/Primitives/Win32ConsoleBackend.h>
#elif defined(__EMSCRIPTEN__)
#include <QmlTui/Core/Primitives/WasmTerminalBackend.h>
#else
#include <QmlTui/Core/Primitives/PosixTerminalBackend.h>
#endif

namespace qmltui {

TerminalBackend::TerminalBackend(QObject* parent) : QObject(parent) {}

TerminalBackend::~TerminalBackend() = default;

namespace {
TerminalBackend::Factory& installedFactory() {
    static TerminalBackend::Factory factory;
    return factory;
}
} // namespace

void TerminalBackend::setFactory(Factory factory) {
    installedFactory() = std::move(factory);
}

const TerminalBackend::Factory& TerminalBackend::factory() {
    return installedFactory();
}

std::unique_ptr<TerminalBackend> createTerminalBackend(int outputFd) {
    if (const TerminalBackend::Factory& factory = TerminalBackend::factory()) {
        if (std::unique_ptr<TerminalBackend> backend = factory(outputFd)) return backend;
    }
#if defined(_WIN32)
    return std::make_unique<Win32ConsoleBackend>(outputFd);
#elif defined(__EMSCRIPTEN__)
    Q_UNUSED(outputFd); // the browser has no descriptors; output goes to the page
    return std::make_unique<WasmTerminalBackend>();
#else
    return std::make_unique<PosixTerminalBackend>(outputFd);
#endif
}

} // namespace qmltui
