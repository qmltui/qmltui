#pragma once

#include <QList>
#include <QStringList>
#include <QUrl>

namespace cyber {

struct PlaylistScan {
    QList<QUrl> entries;
    QStringList warnings;
};

// Extensions (lower case, without dot) accepted when scanning directories.
QStringList audioFileExtensions();

// Expands files, directories (recursively, sorted by path) and URLs given on the
// command line into a flat, ordered playlist. Files passed explicitly are kept
// regardless of extension; unreadable inputs produce a warning instead.
PlaylistScan collectPlaylist(const QStringList& inputs);

// Display name for a playlist entry: file base name, or the URL's last path segment.
QString displayNameFor(const QUrl& url);

} // namespace cyber
