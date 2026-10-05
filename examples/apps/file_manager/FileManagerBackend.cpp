#include "FileManagerBackend.h"

#include <QDir>
#include <QDirListing>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QMimeDatabase>
#include <QStorageInfo>
#include <QStandardPaths>
#include <algorithm>

namespace qtui::examples {

namespace {
QStringList wrapTextLine(const QString &rawLine, int maxW) {
    if (maxW <= 0) return {rawLine};
    QString line = rawLine;
    line.replace(QLatin1Char('\t'), QStringLiteral("    "));
    if (line.isEmpty()) return {QString()};
    if (line.length() <= maxW) return {line};

    QStringList chunks;
    QString current;
    const QStringList words = line.split(QLatin1Char(' '));

    for (int i = 0; i < words.size(); ++i) {
        const QString &word = words.at(i);
        if (word.length() > maxW) {
            if (!current.isEmpty()) {
                chunks.append(current);
                current.clear();
            }
            for (int k = 0; k < word.length(); k += maxW) {
                chunks.append(word.mid(k, maxW));
            }
            continue;
        }

        if (current.isEmpty()) {
            current = word;
        } else if (current.length() + 1 + word.length() <= maxW) {
            current += QLatin1Char(' ') + word;
        } else {
            chunks.append(current);
            current = word;
        }
    }
    if (!current.isEmpty()) {
        chunks.append(current);
    }
    return chunks.isEmpty() ? QStringList{line} : chunks;
}
} // namespace

FileManagerBackend::FileManagerBackend(QObject *parent)
    : QObject(parent)
{
    const bool scenarioRequested = qEnvironmentVariableIsSet("QTUI_SCRIPT")
        || qEnvironmentVariableIsSet("QTUI_EVAL")
        || qEnvironmentVariableIsSet("QTUI_INSPECT")
        || qEnvironmentVariableIsSet("QTUI_FM_MOCK");

    if (scenarioRequested && !qEnvironmentVariableIsSet("QTUI_FM_LIVE")) {
        m_useMock = true;
    }

    const QString user = qEnvironmentVariableIsSet("USER") ? qgetenv("USER") : QStringLiteral("user");
    m_userHost = QStringLiteral("%1@terminal").arg(user);
    m_editorProgram = detectEditor();

    if (m_useMock) {
        initMockFileSystem();
        m_currentPath = QStringLiteral("/workspace");
    } else {
        m_currentPath = QDir::currentPath();
    }

    refreshAll();
}

void FileManagerBackend::initMockFileSystem()
{
    m_mockFs.clear();

    auto addDir = [this](const QString &path, const QString &name, const QStringList &children, const QString &perms = QStringLiteral("drwxr-xr-x")) {
        MockItem item;
        item.name = name;
        item.isDir = true;
        item.size = children.size();
        item.modified = QDateTime::currentDateTime().addDays(-2);
        item.permissions = perms;
        item.owner = QStringLiteral("yosri:users");
        item.mimeType = QStringLiteral("inode/directory");
        item.childNames = children;
        m_mockFs[path] = item;
    };

    auto addFile = [this](const QString &path, const QString &name, const QString &content, const QString &mime, qint64 size = 0, const QString &perms = QStringLiteral("-rw-r--r--")) {
        MockItem item;
        item.name = name;
        item.isDir = false;
        item.content = content;
        item.size = size > 0 ? size : content.toUtf8().size();
        item.modified = QDateTime::currentDateTime().addSecs(-3600 * (content.length() % 24 + 1));
        item.permissions = perms;
        item.owner = QStringLiteral("yosri:users");
        item.mimeType = mime;
        m_mockFs[path] = item;
    };

    // Root
    addDir(QStringLiteral("/"), QStringLiteral("root"), {QStringLiteral("workspace"), QStringLiteral("home"), QStringLiteral("etc")});
    addDir(QStringLiteral("/workspace"), QStringLiteral("workspace"),
           {QStringLiteral("assets"), QStringLiteral("docs"), QStringLiteral("src"), QStringLiteral("tests"),
            QStringLiteral(".git"), QStringLiteral(".env"),
            QStringLiteral("CMakeLists.txt"), QStringLiteral("LICENSE"), QStringLiteral("README.md"), QStringLiteral("package.json")});

    // Subdirectories
    addDir(QStringLiteral("/workspace/docs"), QStringLiteral("docs"),
           {QStringLiteral("architecture.md"), QStringLiteral("api_reference.md"), QStringLiteral("user_guide.txt")});
    addDir(QStringLiteral("/workspace/src"), QStringLiteral("src"),
           {QStringLiteral("main.cpp"), QStringLiteral("App.cpp"), QStringLiteral("App.h"),
            QStringLiteral("utils.rs"), QStringLiteral("deploy.py"), QStringLiteral("config.json")});
    addDir(QStringLiteral("/workspace/assets"), QStringLiteral("assets"),
           {QStringLiteral("logo.png"), QStringLiteral("banner.jpg"), QStringLiteral("archive.tar.gz"), QStringLiteral("theme.yaml")});
    addDir(QStringLiteral("/workspace/tests"), QStringLiteral("tests"),
           {QStringLiteral("test_main.cpp"), QStringLiteral("test_cases.json")});
    addDir(QStringLiteral("/workspace/.git"), QStringLiteral(".git"),
           {QStringLiteral("HEAD"), QStringLiteral("config")});

    // Files in /workspace
    addFile(QStringLiteral("/workspace/README.md"), QStringLiteral("README.md"),
            QStringLiteral("# Yazi-Style Terminal File Manager\n\n"
                           "A fast, asynchronous, 3-column Miller column file manager\n"
                           "built with pure QML and Qtui.\n\n"
                           "## Features\n"
                           "- 3-Column Miller layout (Parent, Current, Preview)\n"
                           "- Vim-style keybindings (h/j/k/l, Space, /, d, r, y, p)\n"
                           "- Live filtering and fast fuzzy search\n"
                           "- Syntax preview with line numbers\n"
                           "- Directory inspection and file metadata cards\n"
                           "- Multi-file selection and clipboard management\n\n"
                           "## Shortcuts\n"
                           "- `h/l`: Navigate parent / child directory\n"
                           "- `j/k`: Move cursor down / up\n"
                           "- `Space`: Toggle select file\n"
                           "- `/`: Instant live filter\n"
                           "- `?`: Cheatsheet help modal\n"
                           "- `q`: Quit"),
            QStringLiteral("text/markdown"));

    addFile(QStringLiteral("/workspace/CMakeLists.txt"), QStringLiteral("CMakeLists.txt"),
            QStringLiteral("cmake_minimum_required(VERSION 3.22)\n"
                           "project(YaziFileManager VERSION 1.0.0 LANGUAGES CXX)\n\n"
                           "set(CMAKE_CXX_STANDARD 20)\n"
                           "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n\n"
                           "find_package(Qt6 6.8 REQUIRED COMPONENTS Core Qml)\n"
                           "find_package(Qtui REQUIRED)\n\n"
                           "qt_add_executable(qtui_file_manager main.cpp)\n"
                           "target_link_libraries(qtui_file_manager PRIVATE Qt6::Core Qtui::Core)"),
            QStringLiteral("text/x-cmake"));

    addFile(QStringLiteral("/workspace/package.json"), QStringLiteral("package.json"),
            QStringLiteral("{\n"
                           "  \"name\": \"yazi-file-manager\",\n"
                           "  \"version\": \"1.0.0\",\n"
                           "  \"description\": \"Retained-mode terminal file manager\",\n"
                           "  \"main\": \"Main.qml\",\n"
                           "  \"scripts\": {\n"
                           "    \"start\": \"./build/qtui_file_manager\",\n"
                           "    \"test\": \"ctest --test-dir build\"\n"
                           "  },\n"
                           "  \"author\": \"Qtui Team\",\n"
                           "  \"license\": \"MIT\"\n"
                           "}"),
            QStringLiteral("application/json"));

    addFile(QStringLiteral("/workspace/LICENSE"), QStringLiteral("LICENSE"),
            QStringLiteral("MIT License\n\n"
                           "Copyright (c) 2026 Qtui Contributors\n\n"
                           "Permission is hereby granted, free of charge, to any person obtaining a copy\n"
                           "of this software and associated documentation files (the \"Software\"), to deal\n"
                           "in the Software without restriction..."),
            QStringLiteral("text/plain"));

    addFile(QStringLiteral("/workspace/.env"), QStringLiteral(".env"),
            QStringLiteral("APP_ENV=production\n"
                           "LOG_LEVEL=debug\n"
                           "MAX_PREVIEW_LINES=200\n"
                           "ENABLE_NERD_FONTS=true"),
            QStringLiteral("text/plain"));

    // Files in /workspace/docs
    addFile(QStringLiteral("/workspace/docs/architecture.md"), QStringLiteral("architecture.md"),
            QStringLiteral("# Architecture Overview\n\n"
                           "1. **Core Engine**: C++20 backend with reactive properties.\n"
                           "2. **Declarative UI**: Pure QML frontend utilizing Qtui.\n"
                           "3. **Miller Navigation**: 3 synchronized reactive viewports."),
            QStringLiteral("text/markdown"));

    addFile(QStringLiteral("/workspace/docs/api_reference.md"), QStringLiteral("api_reference.md"),
            QStringLiteral("# API Reference\n\n"
                           "- `navigateTo(path)`: Traverse directory\n"
                           "- `toggleMark(index)`: Select item\n"
                           "- `copyMarked()`: Copy selection"),
            QStringLiteral("text/markdown"));

    addFile(QStringLiteral("/workspace/docs/user_guide.txt"), QStringLiteral("user_guide.txt"),
            QStringLiteral("YAZI FILE MANAGER USER MANUAL\n"
                           "============================\n"
                           "Welcome to the Qtui terminal file manager."),
            QStringLiteral("text/plain"));

    // Files in /workspace/src
    addFile(QStringLiteral("/workspace/src/main.cpp"), QStringLiteral("main.cpp"),
            QStringLiteral("#include <QCoreApplication>\n"
                           "#include <QQmlApplicationEngine>\n\n"
                           "int main(int argc, char *argv[]) {\n"
                           "    QCoreApplication app(argc, argv);\n"
                           "    QQmlApplicationEngine engine;\n"
                           "    engine.loadFromModule(\"Qtui.Examples.FileManager\", \"Main\");\n"
                           "    return app.exec();\n"
                           "}"),
            QStringLiteral("text/x-c++src"));

    addFile(QStringLiteral("/workspace/src/App.cpp"), QStringLiteral("App.cpp"),
            QStringLiteral("#include \"App.h\"\n\n"
                           "App::App(QObject *parent) : QObject(parent) {\n"
                           "    // Initialization logic\n"
                           "}"),
            QStringLiteral("text/x-c++src"));

    addFile(QStringLiteral("/workspace/src/App.h"), QStringLiteral("App.h"),
            QStringLiteral("#pragma once\n#include <QObject>\n\n"
                           "class App : public QObject {\n"
                           "    Q_OBJECT\n"
                           "public:\n"
                           "    explicit App(QObject *parent = nullptr);\n"
                           "};"),
            QStringLiteral("text/x-c++hdr"));

    addFile(QStringLiteral("/workspace/src/utils.rs"), QStringLiteral("utils.rs"),
            QStringLiteral("pub fn format_size(bytes: u64) -> String {\n"
                           "    if bytes < 1024 {\n"
                           "        format!(\"{} B\", bytes)\n"
                           "    } else {\n"
                           "        format!(\"{:.1} KB\", bytes as f64 / 1024.0)\n"
                           "    }\n"
                           "}"),
            QStringLiteral("text/x-rust"));

    addFile(QStringLiteral("/workspace/src/deploy.py"), QStringLiteral("deploy.py"),
            QStringLiteral("#!/usr/bin/env python3\n"
                           "import sys, os\n\n"
                           "def deploy():\n"
                           "    print('Deploying Qtui application...')\n\n"
                           "if __name__ == '__main__':\n"
                           "    deploy()"),
            QStringLiteral("text/x-python"));

    addFile(QStringLiteral("/workspace/src/config.json"), QStringLiteral("config.json"),
            QStringLiteral("{\n"
                           "  \"theme\": \"tokyo_night\",\n"
                           "  \"show_hidden\": false,\n"
                           "  \"columns_ratio\": [20, 35, 45],\n"
                           "  \"sort_by\": \"name\"\n"
                           "}"),
            QStringLiteral("application/json"));

    // Files in /workspace/assets
    addFile(QStringLiteral("/workspace/assets/logo.png"), QStringLiteral("logo.png"),
            QStringLiteral(""), QStringLiteral("image/png"), 48291);
    addFile(QStringLiteral("/workspace/assets/banner.jpg"), QStringLiteral("banner.jpg"),
            QStringLiteral(""), QStringLiteral("image/jpeg"), 125890);
    addFile(QStringLiteral("/workspace/assets/archive.tar.gz"), QStringLiteral("archive.tar.gz"),
            QStringLiteral(""), QStringLiteral("application/gzip"), 2048576);
    addFile(QStringLiteral("/workspace/assets/theme.yaml"), QStringLiteral("theme.yaml"),
            QStringLiteral("name: Catppuccin Mocha\n"
                           "primary: '#89b4fa'\n"
                           "secondary: '#cdd6f4'\n"
                           "background: '#1e1e2e'"),
            QStringLiteral("text/yaml"));

    // Files in /workspace/tests
    addFile(QStringLiteral("/workspace/tests/test_main.cpp"), QStringLiteral("test_main.cpp"),
            QStringLiteral("#include <QtTest>\n\n"
                           "class TestFileManager : public QObject {\n"
                           "    Q_OBJECT\n"
                           "private slots:\n"
                           "    void testNavigation();\n"
                           "};\n\n"
                           "QTEST_MAIN(TestFileManager)\n"
                           "#include \"test_main.moc\""),
            QStringLiteral("text/x-c++src"));

    addFile(QStringLiteral("/workspace/tests/test_cases.json"), QStringLiteral("test_cases.json"),
            QStringLiteral("[\n  {\"test\": \"navigation\", \"expected\": true},\n  {\"test\": \"preview\", \"expected\": true}\n]"),
            QStringLiteral("application/json"));

    // Files in /workspace/.git
    addFile(QStringLiteral("/workspace/.git/HEAD"), QStringLiteral("HEAD"),
            QStringLiteral("ref: refs/heads/main\n"), QStringLiteral("text/plain"));
    addFile(QStringLiteral("/workspace/.git/config"), QStringLiteral("config"),
            QStringLiteral("[core]\n\trepositoryformatversion = 0\n\tfilemode = true\n"), QStringLiteral("text/plain"));
}

QString FileManagerBackend::breadcrumbs() const
{
    if (m_currentPath.isEmpty() || m_currentPath == QStringLiteral("/")) {
        return QStringLiteral(" /");
    }

    QString clean = m_currentPath;
    if (clean.startsWith(QStringLiteral("/workspace"))) {
        clean = QStringLiteral("~") + clean.mid(10);
    }

    QStringList parts = clean.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        return QStringLiteral(" ~");
    }

    return QStringLiteral(" ") + parts.join(QStringLiteral(" ❯ "));
}

void FileManagerBackend::setCurrentIndex(int index)
{
    if (m_filteredEntries.isEmpty()) {
        m_currentIndex = -1;
    } else {
        m_currentIndex = std::clamp(index, 0, static_cast<int>(m_filteredEntries.size()) - 1);
    }
    emit currentIndexChanged();
    updatePreview();
}

void FileManagerBackend::setPreviewWidth(int w)
{
    if (m_previewWidth == w || w <= 0)
        return;
    m_previewWidth = w;
    emit previewWidthChanged();
    if (m_previewData.value(QStringLiteral("type")).toString() == QStringLiteral("text")) {
        updatePreview();
    }
}

void FileManagerBackend::setFilterText(const QString &filter)
{
    if (m_filterText == filter) return;
    m_filterText = filter;
    emit filterTextChanged();
    applyFilterAndSort();
}

void FileManagerBackend::setShowHidden(bool show)
{
    if (m_showHidden == show) return;
    m_showHidden = show;
    emit showHiddenChanged();
    applyFilterAndSort();
}

void FileManagerBackend::setSortField(const QString &field)
{
    if (m_sortField == field) return;
    m_sortField = field;
    emit sortFieldChanged();
    applyFilterAndSort();
}

void FileManagerBackend::setSortAscending(bool ascending)
{
    if (m_sortAscending == ascending) return;
    m_sortAscending = ascending;
    emit sortAscendingChanged();
    applyFilterAndSort();
}

void FileManagerBackend::setEditorProgram(const QString &prog)
{
    if (m_editorProgram == prog || prog.trimmed().isEmpty()) return;
    m_editorProgram = prog.trimmed();
    emit editorProgramChanged();
}

QString FileManagerBackend::detectEditor() const
{
    if (qEnvironmentVariableIsSet("VISUAL")) {
        const QString visual = QString::fromLocal8Bit(qgetenv("VISUAL")).trimmed();
        if (!visual.isEmpty()) return visual;
    }
    if (qEnvironmentVariableIsSet("EDITOR")) {
        const QString editor = QString::fromLocal8Bit(qgetenv("EDITOR")).trimmed();
        if (!editor.isEmpty()) return editor;
    }
    const QStringList candidates = {
        QStringLiteral("nvim"),
        QStringLiteral("vim"),
        QStringLiteral("nano"),
        QStringLiteral("micro"),
        QStringLiteral("emacs"),
        QStringLiteral("vi")
    };
    for (const QString &cand : candidates) {
        if (!QStandardPaths::findExecutable(cand).isEmpty()) {
            return cand;
        }
    }
    return QStringLiteral("nano");
}

QString FileManagerBackend::currentItemPath() const
{
    if (m_currentIndex < 0 || m_currentIndex >= m_filteredEntries.size())
        return QString();
    return m_filteredEntries.at(m_currentIndex).toMap().value(QStringLiteral("path")).toString();
}

bool FileManagerBackend::isCurrentItemDir() const
{
    if (m_currentIndex < 0 || m_currentIndex >= m_filteredEntries.size())
        return false;
    return m_filteredEntries.at(m_currentIndex).toMap().value(QStringLiteral("isDir")).toBool();
}

void FileManagerBackend::refreshCurrentAndPreview()
{
    refreshCurrent();
    applyFilterAndSort();
    updatePreview();
}

void FileManagerBackend::refreshAll()
{
    refreshParent();
    refreshCurrent();
    updateDiskUsage();
    emit currentPathChanged();
}

void FileManagerBackend::refreshParent()
{
    m_parentEntries.clear();

    if (m_currentPath == QStringLiteral("/") || m_currentPath.isEmpty()) {
        m_parentPath.clear();
        emit parentPathChanged();
        emit parentEntriesChanged();
        return;
    }

    int lastSlash = m_currentPath.lastIndexOf(QLatin1Char('/'));
    m_parentPath = (lastSlash == 0) ? QStringLiteral("/") : m_currentPath.left(lastSlash);
    emit parentPathChanged();

    if (m_useMock) {
        if (m_mockFs.contains(m_parentPath)) {
            const auto &parentDir = m_mockFs[m_parentPath];
            for (const QString &childName : parentDir.childNames) {
                if (!m_showHidden && childName.startsWith(QLatin1Char('.'))) continue;
                QString childPath = (m_parentPath == QStringLiteral("/")) ? (m_parentPath + childName) : (m_parentPath + QStringLiteral("/") + childName);
                if (m_mockFs.contains(childPath)) {
                    const auto &it = m_mockFs[childPath];
                    m_parentEntries.append(formatFileEntry(it.name, childPath, it.isDir, it.size, it.modified, it.permissions, it.owner, it.mimeType));
                }
            }
        }
    } else {
        using ItFlag = QDirListing::IteratorFlag;
        QDirListing::IteratorFlags flags = ItFlag::Default;
        if (m_showHidden) {
            flags |= ItFlag::IncludeHidden;
        }
        QMimeDatabase db;
        for (const auto &entry : QDirListing(m_parentPath, flags)) {
            QString mime = db.mimeTypeForFile(entry.filePath()).name();
            m_parentEntries.append(formatFileEntry(entry.fileName(), entry.filePath(), entry.isDir(),
                                                   entry.size(), entry.fileInfo().lastModified(),
                                                   QStringLiteral("-rw-r--r--"), QStringLiteral("user"), mime));
        }

        std::sort(m_parentEntries.begin(), m_parentEntries.end(), [](const QVariant &a, const QVariant &b) {
            QVariantMap ma = a.toMap();
            QVariantMap mb = b.toMap();
            bool isDirA = ma.value(QStringLiteral("isDir")).toBool();
            bool isDirB = mb.value(QStringLiteral("isDir")).toBool();
            if (isDirA != isDirB) return isDirA;
            return QString::compare(ma.value(QStringLiteral("name")).toString(), mb.value(QStringLiteral("name")).toString(), Qt::CaseInsensitive) < 0;
        });
    }

    emit parentEntriesChanged();
}

void FileManagerBackend::refreshCurrent()
{
    m_rawCurrentEntries.clear();

    if (m_useMock) {
        if (m_mockFs.contains(m_currentPath)) {
            const auto &curDir = m_mockFs[m_currentPath];
            for (const QString &childName : curDir.childNames) {
                QString childPath = (m_currentPath == QStringLiteral("/")) ? (m_currentPath + childName) : (m_currentPath + QStringLiteral("/") + childName);
                if (m_mockFs.contains(childPath)) {
                    const auto &it = m_mockFs[childPath];
                    m_rawCurrentEntries.append(formatFileEntry(it.name, childPath, it.isDir, it.size, it.modified, it.permissions, it.owner, it.mimeType));
                }
            }
        }
    } else {
        using ItFlag = QDirListing::IteratorFlag;
        auto flags = ItFlag::Default | ItFlag::IncludeHidden;
        QMimeDatabase db;
        for (const auto &entry : QDirListing(m_currentPath, flags)) {
            QString mime = db.mimeTypeForFile(entry.filePath()).name();
            m_rawCurrentEntries.append(formatFileEntry(entry.fileName(), entry.filePath(), entry.isDir(),
                                                       entry.size(), entry.fileInfo().lastModified(),
                                                       QStringLiteral("-rw-r--r--"), QStringLiteral("user"), mime));
        }
    }

    applyFilterAndSort();
}

void FileManagerBackend::applyFilterAndSort()
{
    m_filteredEntries.clear();

    for (const auto &var : m_rawCurrentEntries) {
        QVariantMap map = var.toMap();
        QString name = map.value(QStringLiteral("name")).toString();

        if (!m_showHidden && name.startsWith(QLatin1Char('.'))) {
            continue;
        }

        if (!m_filterText.isEmpty() && !name.contains(m_filterText, Qt::CaseInsensitive)) {
            continue;
        }

        map[QStringLiteral("isMarked")] = m_markedPaths.contains(map.value(QStringLiteral("path")).toString());
        m_filteredEntries.append(map);
    }

    // Sort
    std::sort(m_filteredEntries.begin(), m_filteredEntries.end(), [this](const QVariant &a, const QVariant &b) {
        QVariantMap ma = a.toMap();
        QVariantMap mb = b.toMap();
        bool isDirA = ma.value(QStringLiteral("isDir")).toBool();
        bool isDirB = mb.value(QStringLiteral("isDir")).toBool();

        if (isDirA != isDirB) {
            return isDirA; // Folders always first
        }

        int cmp = 0;
        if (m_sortField == QStringLiteral("size")) {
            qint64 sa = ma.value(QStringLiteral("size")).toLongLong();
            qint64 sb = mb.value(QStringLiteral("size")).toLongLong();
            cmp = (sa < sb) ? -1 : (sa > sb ? 1 : 0);
        } else if (m_sortField == QStringLiteral("mtime")) {
            QString ta = ma.value(QStringLiteral("modifiedRaw")).toString();
            QString tb = mb.value(QStringLiteral("modifiedRaw")).toString();
            cmp = QString::compare(ta, tb, Qt::CaseInsensitive);
        } else if (m_sortField == QStringLiteral("type")) {
            QString ea = ma.value(QStringLiteral("extension")).toString();
            QString eb = mb.value(QStringLiteral("extension")).toString();
            cmp = QString::compare(ea, eb, Qt::CaseInsensitive);
        }

        if (cmp == 0) {
            QString na = ma.value(QStringLiteral("name")).toString();
            QString nb = mb.value(QStringLiteral("name")).toString();
            cmp = QString::compare(na, nb, Qt::CaseInsensitive);
        }

        return m_sortAscending ? (cmp < 0) : (cmp > 0);
    });

    emit currentEntriesChanged();

    if (m_filteredEntries.isEmpty()) {
        m_currentIndex = -1;
    } else {
        m_currentIndex = std::clamp(m_currentIndex, 0, static_cast<int>(m_filteredEntries.size()) - 1);
    }
    emit currentIndexChanged();
    updatePreview();
}

void FileManagerBackend::updatePreview()
{
    m_previewData.clear();

    if (m_currentIndex < 0 || m_currentIndex >= m_filteredEntries.size()) {
        m_previewData[QStringLiteral("type")] = QStringLiteral("empty");
        emit previewDataChanged();
        return;
    }

    QVariantMap currentItem = m_filteredEntries.at(m_currentIndex).toMap();
    QString itemPath = currentItem.value(QStringLiteral("path")).toString();
    QString itemName = currentItem.value(QStringLiteral("name")).toString();
    bool isDir = currentItem.value(QStringLiteral("isDir")).toBool();
    QString mime = currentItem.value(QStringLiteral("mimeType")).toString();

    m_previewData[QStringLiteral("name")] = itemName;
    m_previewData[QStringLiteral("path")] = itemPath;
    m_previewData[QStringLiteral("isDir")] = isDir;
    m_previewData[QStringLiteral("sizeFormatted")] = currentItem.value(QStringLiteral("sizeFormatted"));
    m_previewData[QStringLiteral("modifiedFormatted")] = currentItem.value(QStringLiteral("modified"));
    m_previewData[QStringLiteral("permissions")] = currentItem.value(QStringLiteral("permissions"));
    m_previewData[QStringLiteral("owner")] = currentItem.value(QStringLiteral("owner"));
    m_previewData[QStringLiteral("mimeType")] = mime;
    m_previewData[QStringLiteral("icon")] = currentItem.value(QStringLiteral("icon"));
    m_previewData[QStringLiteral("iconColor")] = currentItem.value(QStringLiteral("iconColor"));

    if (isDir) {
        m_previewData[QStringLiteral("type")] = QStringLiteral("directory");
        QVariantList childPreviews;

        if (m_useMock) {
            if (m_mockFs.contains(itemPath)) {
                const auto &dirItem = m_mockFs[itemPath];
                m_previewData[QStringLiteral("itemCount")] = dirItem.childNames.size();
                for (const QString &childName : dirItem.childNames) {
                    QString childPath = itemPath + QStringLiteral("/") + childName;
                    if (m_mockFs.contains(childPath)) {
                        const auto &sub = m_mockFs[childPath];
                        childPreviews.append(formatFileEntry(sub.name, childPath, sub.isDir, sub.size, sub.modified, sub.permissions, sub.owner, sub.mimeType));
                    }
                }
            }
        } else {
            using ItFlag = QDirListing::IteratorFlag;
            QDirListing::IteratorFlags flags = ItFlag::Default;
            if (m_showHidden) {
                flags |= ItFlag::IncludeHidden;
            }
            QMimeDatabase db;
            int count = 0;
            for (const auto &entry : QDirListing(itemPath, flags)) {
                count++;
                if (childPreviews.size() < 30) {
                    QString m = db.mimeTypeForFile(entry.filePath()).name();
                    childPreviews.append(formatFileEntry(entry.fileName(), entry.filePath(), entry.isDir(),
                                                         entry.size(), entry.fileInfo().lastModified(),
                                                         QStringLiteral("-rw-r--r--"), QStringLiteral("user"), m));
                }
            }
            m_previewData[QStringLiteral("itemCount")] = count;
        }
        m_previewData[QStringLiteral("dirChildren")] = childPreviews;
    } else {
        // File preview
        bool isText = mime.startsWith(QStringLiteral("text/"))
            || mime.contains(QStringLiteral("json"))
            || mime.contains(QStringLiteral("xml"))
            || mime.contains(QStringLiteral("yaml"))
            || mime.contains(QStringLiteral("javascript"))
            || mime.contains(QStringLiteral("x-c"))
            || mime.contains(QStringLiteral("x-python"))
            || mime.contains(QStringLiteral("x-rust"))
            || mime.contains(QStringLiteral("x-cmake"))
            || itemName.endsWith(QStringLiteral(".md"))
            || itemName.endsWith(QStringLiteral(".txt"))
            || itemName.endsWith(QStringLiteral(".qml"))
            || itemName.endsWith(QStringLiteral(".h"))
            || itemName.endsWith(QStringLiteral(".cpp"))
            || itemName.endsWith(QStringLiteral(".rs"))
            || itemName.endsWith(QStringLiteral(".py"))
            || itemName.endsWith(QStringLiteral(".json"))
            || itemName.endsWith(QStringLiteral(".yaml"))
            || itemName.endsWith(QStringLiteral(".yml"))
            || itemName.endsWith(QStringLiteral(".toml"))
            || itemName.endsWith(QStringLiteral(".sh"))
            || itemName.endsWith(QStringLiteral(".env"))
            || itemName == QStringLiteral("CMakeLists.txt")
            || itemName == QStringLiteral("LICENSE");

        if (isText) {
            m_previewData[QStringLiteral("type")] = QStringLiteral("text");
            QString rawText;
            if (m_useMock) {
                if (m_mockFs.contains(itemPath)) {
                    rawText = m_mockFs[itemPath].content;
                }
            } else {
                QFile file(itemPath);
                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QTextStream stream(&file);
                    int lineCount = 0;
                    while (!stream.atEnd() && lineCount < 200) {
                        rawText += stream.readLine() + QLatin1Char('\n');
                        lineCount++;
                    }
                }
            }

            QStringList lines = rawText.split(QLatin1Char('\n'));
            if (lines.endsWith(QString())) lines.removeLast();
            m_previewData[QStringLiteral("rawText")] = rawText;
            m_previewData[QStringLiteral("lineCount")] = lines.size();
            m_previewData[QStringLiteral("textLines")] = lines;

            QString formattedText;
            int gutterDigits = 2;
            if (lines.size() >= 100) gutterDigits = 3;
            if (lines.size() >= 1000) gutterDigits = 4;
            int gutterWidth = gutterDigits + 3; // e.g. " 01 │ " (5 chars)
            int contentWidth = std::max(10, m_previewWidth - 2 - gutterWidth);
            const QString blankGutter = QString(gutterDigits, QLatin1Char(' ')) + QStringLiteral(" │ ");

            for (int i = 0; i < lines.size(); ++i) {
                const QString lineNumStr = QStringLiteral("%1 │ ").arg(i + 1, gutterDigits, 10, QLatin1Char(' '));
                const QStringList wrappedChunks = wrapTextLine(lines.at(i), contentWidth);
                for (int c = 0; c < wrappedChunks.size(); ++c) {
                    if (c == 0) {
                        formattedText += lineNumStr + wrappedChunks.at(c) + QLatin1Char('\n');
                    } else {
                        formattedText += blankGutter + wrappedChunks.at(c) + QLatin1Char('\n');
                    }
                }
            }
            m_previewData[QStringLiteral("formattedText")] = formattedText;
        } else if (mime.startsWith(QStringLiteral("image/"))) {
            m_previewData[QStringLiteral("type")] = QStringLiteral("image");
        } else {
            m_previewData[QStringLiteral("type")] = QStringLiteral("binary");
        }
    }

    emit previewDataChanged();
}

void FileManagerBackend::updateDiskUsage()
{
    if (m_useMock) {
        m_diskUsage = QStringLiteral("Free: 42.8 GB / 256.0 GB");
    } else {
        QStorageInfo storage(m_currentPath);
        if (storage.isValid()) {
            qint64 freeGb = storage.bytesAvailable() / (1024 * 1024 * 1024);
            qint64 totalGb = storage.bytesTotal() / (1024 * 1024 * 1024);
            m_diskUsage = QStringLiteral("Free: %1 GB / %2 GB").arg(freeGb).arg(totalGb);
        } else {
            m_diskUsage = QStringLiteral("Storage: Ready");
        }
    }
    emit diskUsageChanged();
}

QVariantMap FileManagerBackend::formatFileEntry(const QString &name, const QString &fullPath, bool isDir,
                                               qint64 size, const QDateTime &modified,
                                               const QString &permissions, const QString &owner,
                                               const QString &mimeType)
{
    QVariantMap map;
    map[QStringLiteral("name")] = name;
    map[QStringLiteral("path")] = fullPath;
    map[QStringLiteral("isDir")] = isDir;
    map[QStringLiteral("size")] = size;
    map[QStringLiteral("sizeFormatted")] = isDir ? QStringLiteral("%1 items").arg(size) : formatSize(size);
    map[QStringLiteral("modified")] = modified.toString(QStringLiteral("yyyy-MM-dd hh:mm"));
    map[QStringLiteral("modifiedRaw")] = modified.toString(Qt::ISODate);
    map[QStringLiteral("permissions")] = permissions;
    map[QStringLiteral("owner")] = owner;
    map[QStringLiteral("mimeType")] = mimeType;
    map[QStringLiteral("icon")] = getFileIcon(name, isDir);
    map[QStringLiteral("iconColor")] = getIconColor(name, isDir);

    int dotIndex = name.lastIndexOf(QLatin1Char('.'));
    if (dotIndex > 0 && !isDir) {
        map[QStringLiteral("extension")] = name.mid(dotIndex + 1).toLower();
    } else {
        map[QStringLiteral("extension")] = QString();
    }

    map[QStringLiteral("isMarked")] = m_markedPaths.contains(fullPath);
    return map;
}

QString FileManagerBackend::formatSize(qint64 bytes)
{
    if (bytes < 1024) return QStringLiteral("%1 B").arg(bytes);
    if (bytes < 1024 * 1024) return QStringLiteral("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    if (bytes < 1024 * 1024 * 1024) return QStringLiteral("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
    return QStringLiteral("%1 GB").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
}

QString FileManagerBackend::getFileIcon(const QString &name, bool isDir)
{
    if (isDir) return QStringLiteral("📁");
    QString lower = name.toLower();
    if (lower.endsWith(QStringLiteral(".py"))) return QStringLiteral("🐍");
    if (lower.endsWith(QStringLiteral(".rs"))) return QStringLiteral("🦀");
    if (lower.endsWith(QStringLiteral(".cpp")) || lower.endsWith(QStringLiteral(".c")) || lower.endsWith(QStringLiteral(".h")) || lower.endsWith(QStringLiteral(".hpp"))) return QStringLiteral("⚡");
    if (lower.endsWith(QStringLiteral(".md")) || lower.endsWith(QStringLiteral(".txt"))) return QStringLiteral("📄");
    if (lower.endsWith(QStringLiteral(".json")) || lower.endsWith(QStringLiteral(".yaml")) || lower.endsWith(QStringLiteral(".yml")) || lower.endsWith(QStringLiteral(".toml"))) return QStringLiteral("⚙️");
    if (lower.endsWith(QStringLiteral(".tar.gz")) || lower.endsWith(QStringLiteral(".zip")) || lower.endsWith(QStringLiteral(".tar")) || lower.endsWith(QStringLiteral(".gz"))) return QStringLiteral("📦");
    if (lower.endsWith(QStringLiteral(".png")) || lower.endsWith(QStringLiteral(".jpg")) || lower.endsWith(QStringLiteral(".jpeg")) || lower.endsWith(QStringLiteral(".svg"))) return QStringLiteral("🖼️");
    if (lower.endsWith(QStringLiteral(".sh")) || lower.endsWith(QStringLiteral(".bash"))) return QStringLiteral("📜");
    if (lower.endsWith(QStringLiteral(".qml"))) return QStringLiteral("✨");
    if (lower == QStringLiteral("cmakelists.txt")) return QStringLiteral("🛠️");
    if (lower == QStringLiteral("license")) return QStringLiteral("⚖️");
    return QStringLiteral("📄");
}

QString FileManagerBackend::getIconColor(const QString &name, bool isDir)
{
    if (isDir) return QStringLiteral("#89b4fa"); // Blue
    QString lower = name.toLower();
    if (lower.endsWith(QStringLiteral(".py"))) return QStringLiteral("#f9e2af"); // Yellow
    if (lower.endsWith(QStringLiteral(".rs"))) return QStringLiteral("#fab387"); // Peach / Orange
    if (lower.endsWith(QStringLiteral(".cpp")) || lower.endsWith(QStringLiteral(".h"))) return QStringLiteral("#89dceb"); // Cyan
    if (lower.endsWith(QStringLiteral(".md")) || lower.endsWith(QStringLiteral(".txt"))) return QStringLiteral("#a6e3a1"); // Green
    if (lower.endsWith(QStringLiteral(".json")) || lower.endsWith(QStringLiteral(".yaml"))) return QStringLiteral("#cba6f7"); // Mauve
    if (lower.endsWith(QStringLiteral(".tar.gz")) || lower.endsWith(QStringLiteral(".zip"))) return QStringLiteral("#f38ba8"); // Red
    if (lower.endsWith(QStringLiteral(".png")) || lower.endsWith(QStringLiteral(".jpg"))) return QStringLiteral("#f5c2e7"); // Pink
    return QStringLiteral("#cdd6f4"); // Default
}

void FileManagerBackend::navigateTo(const QString &path)
{
    if (path.isEmpty() || m_currentPath == path) return;
    m_currentPath = path;
    m_currentIndex = 0;
    m_filterText.clear();
    emit filterTextChanged();
    refreshAll();
}

void FileManagerBackend::navigateUp()
{
    if (m_parentPath.isEmpty()) return;
    QString prevCurrent = m_currentPath;
    m_currentPath = m_parentPath;
    m_filterText.clear();
    emit filterTextChanged();
    refreshAll();

    // Find the index of the folder we just came out of
    for (int i = 0; i < m_filteredEntries.size(); ++i) {
        if (m_filteredEntries.at(i).toMap().value(QStringLiteral("path")).toString() == prevCurrent) {
            setCurrentIndex(i);
            return;
        }
    }
    setCurrentIndex(0);
}

void FileManagerBackend::navigateIntoCurrent()
{
    if (m_currentIndex < 0 || m_currentIndex >= m_filteredEntries.size()) return;
    QVariantMap item = m_filteredEntries.at(m_currentIndex).toMap();
    if (item.value(QStringLiteral("isDir")).toBool()) {
        navigateTo(item.value(QStringLiteral("path")).toString());
    } else {
        showToast(QStringLiteral("Opened file: %1").arg(item.value(QStringLiteral("name")).toString()));
    }
}

void FileManagerBackend::toggleMarkCurrent()
{
    toggleMark(m_currentIndex);
}

void FileManagerBackend::toggleMark(int index)
{
    if (index < 0 || index >= m_filteredEntries.size()) return;
    QString path = m_filteredEntries.at(index).toMap().value(QStringLiteral("path")).toString();
    if (m_markedPaths.contains(path)) {
        m_markedPaths.removeAll(path);
    } else {
        m_markedPaths.append(path);
    }
    emit markedPathsChanged();
    applyFilterAndSort();
}

void FileManagerBackend::selectAll()
{
    if (m_markedPaths.size() == m_filteredEntries.size()) {
        m_markedPaths.clear();
    } else {
        m_markedPaths.clear();
        for (const auto &item : m_filteredEntries) {
            m_markedPaths.append(item.toMap().value(QStringLiteral("path")).toString());
        }
    }
    emit markedPathsChanged();
    applyFilterAndSort();
}

void FileManagerBackend::clearMarks()
{
    if (m_markedPaths.isEmpty()) return;
    m_markedPaths.clear();
    emit markedPathsChanged();
    applyFilterAndSort();
}

void FileManagerBackend::copyMarked()
{
    m_clipboardPaths.clear();
    if (m_markedPaths.isEmpty() && m_currentIndex >= 0 && m_currentIndex < m_filteredEntries.size()) {
        m_clipboardPaths.append(m_filteredEntries.at(m_currentIndex).toMap().value(QStringLiteral("path")).toString());
    } else {
        m_clipboardPaths = m_markedPaths;
    }
    m_clipboardOp = QStringLiteral("copy");
    emit clipboardChanged();
    showToast(QStringLiteral("Yanked %1 item(s)").arg(m_clipboardPaths.size()));
}

void FileManagerBackend::cutMarked()
{
    m_clipboardPaths.clear();
    if (m_markedPaths.isEmpty() && m_currentIndex >= 0 && m_currentIndex < m_filteredEntries.size()) {
        m_clipboardPaths.append(m_filteredEntries.at(m_currentIndex).toMap().value(QStringLiteral("path")).toString());
    } else {
        m_clipboardPaths = m_markedPaths;
    }
    m_clipboardOp = QStringLiteral("cut");
    emit clipboardChanged();
    showToast(QStringLiteral("Cut %1 item(s)").arg(m_clipboardPaths.size()));
}

void FileManagerBackend::pasteClipboard()
{
    if (m_clipboardPaths.isEmpty()) {
        showToast(QStringLiteral("Clipboard empty"));
        return;
    }

    int count = 0;
    for (const QString &srcPath : m_clipboardPaths) {
        QString name = QFileInfo(srcPath).fileName();
        QString destPath = m_currentPath + QStringLiteral("/") + name;

        if (m_useMock) {
            if (m_mockFs.contains(srcPath)) {
                MockItem item = m_mockFs[srcPath];
                item.name = name;
                m_mockFs[destPath] = item;
                if (m_mockFs.contains(m_currentPath)) {
                    m_mockFs[m_currentPath].childNames.append(name);
                }
                if (m_clipboardOp == QStringLiteral("cut")) {
                    m_mockFs.remove(srcPath);
                }
                count++;
            }
        } else {
            if (m_clipboardOp == QStringLiteral("cut")) {
                if (QFile::rename(srcPath, destPath)) count++;
            } else {
                if (QFile::copy(srcPath, destPath)) count++;
            }
        }
    }

    if (m_clipboardOp == QStringLiteral("cut")) {
        m_clipboardPaths.clear();
        m_clipboardOp.clear();
        emit clipboardChanged();
    }

    showToast(QStringLiteral("Pasted %1 item(s)").arg(count));
    refreshAll();
}

bool FileManagerBackend::createItem(const QString &name, bool isDirectory)
{
    if (name.trimmed().isEmpty()) return false;
    QString targetPath = m_currentPath + QStringLiteral("/") + name.trimmed();

    if (m_useMock) {
        if (m_mockFs.contains(targetPath)) return false;
        MockItem item;
        item.name = name.trimmed();
        item.isDir = isDirectory;
        item.size = 0;
        item.modified = QDateTime::currentDateTime();
        item.permissions = isDirectory ? QStringLiteral("drwxr-xr-x") : QStringLiteral("-rw-r--r--");
        item.owner = QStringLiteral("yosri:users");
        item.mimeType = isDirectory ? QStringLiteral("inode/directory") : QStringLiteral("text/plain");
        m_mockFs[targetPath] = item;
        if (m_mockFs.contains(m_currentPath)) {
            m_mockFs[m_currentPath].childNames.append(name.trimmed());
        }
    } else {
        if (isDirectory) {
            QDir dir(m_currentPath);
            if (!dir.mkdir(name.trimmed())) return false;
        } else {
            QFile file(targetPath);
            if (!file.open(QIODevice::WriteOnly)) return false;
            file.close();
        }
    }

    showToast(QStringLiteral("Created %1: %2").arg(isDirectory ? QStringLiteral("dir") : QStringLiteral("file"), name.trimmed()));
    refreshAll();

    for (int i = 0; i < m_filteredEntries.size(); ++i) {
        if (m_filteredEntries.at(i).toMap().value(QStringLiteral("name")).toString() == name.trimmed()) {
            setCurrentIndex(i);
            break;
        }
    }
    return true;
}

bool FileManagerBackend::deleteCurrentOrMarked()
{
    QStringList toDelete;
    if (!m_markedPaths.isEmpty()) {
        toDelete = m_markedPaths;
    } else if (m_currentIndex >= 0 && m_currentIndex < m_filteredEntries.size()) {
        toDelete.append(m_filteredEntries.at(m_currentIndex).toMap().value(QStringLiteral("path")).toString());
    }

    if (toDelete.isEmpty()) return false;

    int deleted = 0;
    for (const QString &path : toDelete) {
        if (m_useMock) {
            if (m_mockFs.contains(path)) {
                QString name = m_mockFs[path].name;
                m_mockFs.remove(path);
                if (m_mockFs.contains(m_currentPath)) {
                    m_mockFs[m_currentPath].childNames.removeAll(name);
                }
                deleted++;
            }
        } else {
            QFileInfo info(path);
            if (info.isDir()) {
                QDir dir(path);
                if (dir.removeRecursively()) deleted++;
            } else {
                if (QFile::remove(path)) deleted++;
            }
        }
    }

    clearMarks();
    showToast(QStringLiteral("Deleted %1 item(s)").arg(deleted));
    refreshAll();
    return true;
}

bool FileManagerBackend::renameCurrent(const QString &newName)
{
    if (newName.trimmed().isEmpty() || m_currentIndex < 0 || m_currentIndex >= m_filteredEntries.size()) return false;
    QVariantMap currentItem = m_filteredEntries.at(m_currentIndex).toMap();
    QString oldPath = currentItem.value(QStringLiteral("path")).toString();
    QString oldName = currentItem.value(QStringLiteral("name")).toString();
    QString newPath = m_currentPath + QStringLiteral("/") + newName.trimmed();

    if (oldName == newName.trimmed()) return true;

    if (m_useMock) {
        if (m_mockFs.contains(oldPath)) {
            MockItem item = m_mockFs[oldPath];
            item.name = newName.trimmed();
            m_mockFs.remove(oldPath);
            m_mockFs[newPath] = item;
            if (m_mockFs.contains(m_currentPath)) {
                int idx = m_mockFs[m_currentPath].childNames.indexOf(oldName);
                if (idx >= 0) m_mockFs[m_currentPath].childNames[idx] = newName.trimmed();
            }
        }
    } else {
        if (!QFile::rename(oldPath, newPath)) return false;
    }

    showToast(QStringLiteral("Renamed to: %1").arg(newName.trimmed()));
    refreshAll();
    return true;
}

void FileManagerBackend::cycleSort()
{
    if (m_sortField == QStringLiteral("name")) {
        setSortField(QStringLiteral("size"));
        showToast(QStringLiteral("Sort by: Size"));
    } else if (m_sortField == QStringLiteral("size")) {
        setSortField(QStringLiteral("mtime"));
        showToast(QStringLiteral("Sort by: Modified Time"));
    } else if (m_sortField == QStringLiteral("mtime")) {
        setSortField(QStringLiteral("type"));
        showToast(QStringLiteral("Sort by: Type / Extension"));
    } else {
        setSortField(QStringLiteral("name"));
        showToast(QStringLiteral("Sort by: Name"));
    }
}

void FileManagerBackend::toggleShowHidden()
{
    setShowHidden(!m_showHidden);
    showToast(m_showHidden ? QStringLiteral("Showing hidden files") : QStringLiteral("Hiding dotfiles"));
}

void FileManagerBackend::showToast(const QString &msg)
{
    m_statusMessage = msg;
    emit statusMessageChanged();
}

} // namespace qtui::examples
