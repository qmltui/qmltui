#include "Playlist.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

#include <algorithm>

namespace cyber {

QStringList audioFileExtensions()
{
    return { QStringLiteral("mp3"), QStringLiteral("flac"), QStringLiteral("wav"),
             QStringLiteral("ogg"), QStringLiteral("oga"), QStringLiteral("opus"),
             QStringLiteral("m4a"), QStringLiteral("aac"), QStringLiteral("wma"),
             QStringLiteral("aiff"), QStringLiteral("aif") };
}

static bool looksLikeRemoteUrl(const QString& input)
{
    const QUrl url(input);
    // A one-letter scheme is a Windows drive letter, not a protocol.
    return url.isValid() && url.scheme().size() > 1 && !url.host().isEmpty();
}

PlaylistScan collectPlaylist(const QStringList& inputs)
{
    PlaylistScan scan;
    const QStringList extensions = audioFileExtensions();

    for (const QString& input : inputs) {
        const QString trimmed = input.trimmed();
        if (trimmed.isEmpty())
            continue;

        const QFileInfo info(trimmed);
        if (info.isDir()) {
            QStringList files;
            QDirIterator it(info.absoluteFilePath(), QDir::Files | QDir::Readable,
                            QDirIterator::Subdirectories);
            while (it.hasNext()) {
                const QString path = it.next();
                if (extensions.contains(QFileInfo(path).suffix().toLower()))
                    files << path;
            }
            std::sort(files.begin(), files.end());
            if (files.isEmpty())
                scan.warnings << QStringLiteral("No audio files in %1").arg(trimmed);
            for (const QString& path : files)
                scan.entries << QUrl::fromLocalFile(path);
        } else if (info.isFile()) {
            scan.entries << QUrl::fromLocalFile(info.absoluteFilePath());
        } else if (looksLikeRemoteUrl(trimmed)) {
            scan.entries << QUrl(trimmed);
        } else {
            scan.warnings << QStringLiteral("Not found: %1").arg(trimmed);
        }
    }
    return scan;
}

QString displayNameFor(const QUrl& url)
{
    if (url.isLocalFile())
        return QFileInfo(url.toLocalFile()).completeBaseName();
    const QString name = url.fileName();
    return name.isEmpty() ? url.toDisplayString() : name;
}

} // namespace cyber
