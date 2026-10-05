#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QDateTime>
#include <QQmlEngine>
#include <QMap>

namespace qtui::examples {

struct MockItem {
    QString name;
    bool isDir = false;
    qint64 size = 0;
    QDateTime modified;
    QString permissions;
    QString owner;
    QString mimeType;
    QString content; // For text files
    QStringList childNames; // For directories
};

class FileManagerBackend : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString currentPath READ currentPath NOTIFY currentPathChanged)
    Q_PROPERTY(QString parentPath READ parentPath NOTIFY parentPathChanged)
    Q_PROPERTY(QString breadcrumbs READ breadcrumbs NOTIFY currentPathChanged)
    Q_PROPERTY(QVariantList parentEntries READ parentEntries NOTIFY parentEntriesChanged)
    Q_PROPERTY(QVariantList currentEntries READ currentEntries NOTIFY currentEntriesChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(int previewWidth READ previewWidth WRITE setPreviewWidth NOTIFY previewWidthChanged)
    Q_PROPERTY(QVariantMap previewData READ previewData NOTIFY previewDataChanged)
    Q_PROPERTY(QStringList markedPaths READ markedPaths NOTIFY markedPathsChanged)
    Q_PROPERTY(int markedCount READ markedCount NOTIFY markedPathsChanged)
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged)
    Q_PROPERTY(bool showHidden READ showHidden WRITE setShowHidden NOTIFY showHiddenChanged)
    Q_PROPERTY(QString sortField READ sortField WRITE setSortField NOTIFY sortFieldChanged)
    Q_PROPERTY(bool sortAscending READ sortAscending WRITE setSortAscending NOTIFY sortAscendingChanged)
    Q_PROPERTY(QString diskUsage READ diskUsage NOTIFY diskUsageChanged)
    Q_PROPERTY(QString userHost READ userHost CONSTANT)
    Q_PROPERTY(QString clipboardOp READ clipboardOp NOTIFY clipboardChanged)
    Q_PROPERTY(int clipboardCount READ clipboardCount NOTIFY clipboardChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString editorProgram READ editorProgram WRITE setEditorProgram NOTIFY editorProgramChanged)
    Q_PROPERTY(QString currentItemPath READ currentItemPath NOTIFY currentIndexChanged)
    Q_PROPERTY(bool isCurrentItemDir READ isCurrentItemDir NOTIFY currentIndexChanged)
    Q_PROPERTY(bool isMock READ isMock CONSTANT)

public:
    explicit FileManagerBackend(QObject *parent = nullptr);
    ~FileManagerBackend() override = default;

    QString currentPath() const { return m_currentPath; }
    QString parentPath() const { return m_parentPath; }
    QString breadcrumbs() const;
    QVariantList parentEntries() const { return m_parentEntries; }
    QVariantList currentEntries() const { return m_filteredEntries; }
    int currentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int index);

    int previewWidth() const { return m_previewWidth; }
    void setPreviewWidth(int w);

    QVariantMap previewData() const { return m_previewData; }
    QStringList markedPaths() const { return m_markedPaths; }
    int markedCount() const { return m_markedPaths.size(); }
    QString filterText() const { return m_filterText; }
    void setFilterText(const QString &filter);

    bool showHidden() const { return m_showHidden; }
    void setShowHidden(bool show);

    QString sortField() const { return m_sortField; }
    void setSortField(const QString &field);

    bool sortAscending() const { return m_sortAscending; }
    void setSortAscending(bool ascending);

    QString diskUsage() const { return m_diskUsage; }
    QString userHost() const { return m_userHost; }
    QString clipboardOp() const { return m_clipboardOp; }
    int clipboardCount() const { return m_clipboardPaths.size(); }
    QString statusMessage() const { return m_statusMessage; }
    bool isMock() const { return m_useMock; }

    QString editorProgram() const { return m_editorProgram; }
    void setEditorProgram(const QString &prog);

    QString currentItemPath() const;
    bool isCurrentItemDir() const;

    Q_INVOKABLE QString detectEditor() const;
    Q_INVOKABLE void refreshCurrentAndPreview();
    Q_INVOKABLE void navigateTo(const QString &path);
    Q_INVOKABLE void navigateUp();
    Q_INVOKABLE void navigateIntoCurrent();
    Q_INVOKABLE void toggleMarkCurrent();
    Q_INVOKABLE void toggleMark(int index);
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void clearMarks();
    Q_INVOKABLE void copyMarked();
    Q_INVOKABLE void cutMarked();
    Q_INVOKABLE void pasteClipboard();
    Q_INVOKABLE bool createItem(const QString &name, bool isDirectory);
    Q_INVOKABLE bool deleteCurrentOrMarked();
    Q_INVOKABLE bool renameCurrent(const QString &newName);
    Q_INVOKABLE void cycleSort();
    Q_INVOKABLE void toggleShowHidden();
    Q_INVOKABLE void showToast(const QString &msg);

signals:
    void currentPathChanged();
    void parentPathChanged();
    void parentEntriesChanged();
    void currentEntriesChanged();
    void currentIndexChanged();
    void previewWidthChanged();
    void previewDataChanged();
    void markedPathsChanged();
    void filterTextChanged();
    void showHiddenChanged();
    void sortFieldChanged();
    void sortAscendingChanged();
    void diskUsageChanged();
    void clipboardChanged();
    void statusMessageChanged();
    void editorProgramChanged();

private:
    void initMockFileSystem();
    void refreshAll();
    void refreshParent();
    void refreshCurrent();
    void applyFilterAndSort();
    void updatePreview();
    void updateDiskUsage();

    QVariantMap formatFileEntry(const QString &name, const QString &fullPath, bool isDir,
                                qint64 size, const QDateTime &modified,
                                const QString &permissions, const QString &owner,
                                const QString &mimeType);

    static QString formatSize(qint64 bytes);
    static QString getFileIcon(const QString &name, bool isDir);
    static QString getIconColor(const QString &name, bool isDir);

    bool m_useMock = false;
    QString m_currentPath;
    QString m_parentPath;
    QVariantList m_parentEntries;
    QVariantList m_rawCurrentEntries;
    QVariantList m_filteredEntries;
    int m_currentIndex = 0;
    int m_previewWidth = 40;
    QVariantMap m_previewData;
    QStringList m_markedPaths;
    QString m_filterText;
    bool m_showHidden = false;
    QString m_sortField = "name"; // name, size, mtime, type
    bool m_sortAscending = true;
    QString m_diskUsage;
    QString m_userHost;
    QString m_clipboardOp; // "copy" or "cut"
    QStringList m_clipboardPaths;
    QString m_statusMessage;
    QString m_editorProgram;

    // Mock storage
    QMap<QString, MockItem> m_mockFs;
};

} // namespace qtui::examples
