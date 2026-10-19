#pragma once

#include <QStandardItemModel>
#include <QQmlEngine>

// A release checklist built exactly the way a Qt Widgets application fills a
// QStandardItemModel: colors through setForeground()/setBackground(), check
// boxes through setCheckable(), a disabled row, a decoration glyph, and a
// right-aligned column. QmlTui's views render all of it without a custom delegate.
class ReleaseChecklistModel : public QStandardItemModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int doneCount READ doneCount NOTIFY doneCountChanged)

public:
    explicit ReleaseChecklistModel(QObject *parent = nullptr);

    int doneCount() const;
    Q_INVOKABLE void toggle(int row);

signals:
    void doneCountChanged();
};
