#pragma once

#include <QString>
#include <QStringList>
#include <QtGlobal>

namespace qmltui::runner_cli {

// Spawns a detached background daemon whose standard streams are redirected to
// the null device and that does not inherit caller pipe or file handles.
bool startDetachedDaemon(const QString& program, const QStringList& arguments,
                         const QString& workingDirectory, qint64* pid = nullptr);

} // namespace qmltui::runner_cli
