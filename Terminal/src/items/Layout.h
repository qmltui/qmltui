#pragma once

#include <QObject>
#include <QQmlEngine>
#include <limits>

namespace qtui {

class LayoutAttached : public QObject {
    Q_OBJECT
    Q_PROPERTY(int minimumWidth MEMBER minimumWidth NOTIFY changed)
    Q_PROPERTY(int minimumHeight MEMBER minimumHeight NOTIFY changed)
    Q_PROPERTY(int preferredWidth MEMBER preferredWidth NOTIFY changed)
    Q_PROPERTY(int preferredHeight MEMBER preferredHeight NOTIFY changed)
    Q_PROPERTY(int maximumWidth MEMBER maximumWidth NOTIFY changed)
    Q_PROPERTY(int maximumHeight MEMBER maximumHeight NOTIFY changed)
    Q_PROPERTY(bool fillWidth MEMBER fillWidth NOTIFY changed)
    Q_PROPERTY(bool fillHeight MEMBER fillHeight NOTIFY changed)
    Q_PROPERTY(int horizontalStretch MEMBER horizontalStretch NOTIFY changed)
    Q_PROPERTY(int verticalStretch MEMBER verticalStretch NOTIFY changed)

public:
    explicit LayoutAttached(QObject *parent = nullptr) : QObject(parent) {}

    int minimumWidth = 0;
    int minimumHeight = 0;
    int preferredWidth = -1;
    int preferredHeight = -1;
    int maximumWidth = std::numeric_limits<int>::max();
    int maximumHeight = std::numeric_limits<int>::max();
    bool fillWidth = false;
    bool fillHeight = false;
    int horizontalStretch = 1;
    int verticalStretch = 1;

signals:
    void changed();
};

class Layout : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Layout is an attached property")
    QML_ATTACHED(LayoutAttached)

public:
    static LayoutAttached *qmlAttachedProperties(QObject *object);
};

} // namespace qtui

QML_DECLARE_TYPEINFO(qtui::Layout, QML_HAS_ATTACHED_PROPERTIES)
