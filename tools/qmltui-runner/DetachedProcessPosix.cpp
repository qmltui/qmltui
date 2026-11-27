#include "DetachedProcess.h"

#include <QProcess>

namespace qmltui::runner_cli {

bool startDetachedDaemon(const QString& program, const QStringList& arguments,
                         const QString& workingDirectory, qint64* pid) {
    if (program.isEmpty()) return false;
    QProcess daemonProc;
    daemonProc.setProgram(program);
    daemonProc.setArguments(arguments);
    if (!workingDirectory.isEmpty()) daemonProc.setWorkingDirectory(workingDirectory);
    daemonProc.setStandardInputFile(QProcess::nullDevice());
    daemonProc.setStandardOutputFile(QProcess::nullDevice());
    daemonProc.setStandardErrorFile(QProcess::nullDevice());
    return daemonProc.startDetached(pid);
}

} // namespace qmltui::runner_cli
