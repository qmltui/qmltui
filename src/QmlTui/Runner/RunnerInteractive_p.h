#pragma once

// Internal: cross-platform stdin line reader thread for interactive Runner sessions.

#include <QByteArray>
#include <QPointer>
#include <atomic>
#include <memory>
#include <thread>

namespace qmltui {

class Runner;

// Reads newline-delimited commands from standard input on a background thread
// and forwards each complete line (and EOF) onto Runner on the Qt main thread.
class RunnerInteractiveReader final {
public:
    explicit RunnerInteractiveReader(Runner* runner);
    ~RunnerInteractiveReader();

    // Stops delivery of further lines. When detachIfBlocked is true (for
    // example during QCoreApplication::aboutToQuit while stdin may still be
    // open), detaches the blocking reader thread so process exit never stalls.
    void stop(bool detachIfBlocked);

private:
    std::shared_ptr<std::atomic<bool>> m_stopped;
    std::thread m_thread;
};

} // namespace qmltui
