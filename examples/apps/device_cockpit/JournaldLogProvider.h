#pragma once

#include "LogProvider.h"

#include <QProcess>
#include <QTimer>

// Tails the systemd journal by polling one-shot `journalctl --after-cursor`
// runs. Polling avoids `journalctl -f` pipe-buffering quirks (it produces no
// output at all on some hosts, e.g. WSL) and inherits journalctl's own
// permission handling.
class JournaldLogProvider : public LogProvider {
    Q_OBJECT

public:
    explicit JournaldLogProvider(QObject *parent = nullptr);

private:
    void poll();
    void handleFinished(int exitCode, QProcess::ExitStatus status);

    QProcess m_journalctl;
    QTimer m_timer;
    QString m_cursor;
};
