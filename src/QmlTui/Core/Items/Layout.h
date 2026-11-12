#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QObject>
#include <QQmlEngine>
#include <limits>

namespace qmltui {

// Attached layout constraints and grid cell placement hints read by
// RowLayout, ColumnLayout, and GridLayout.
class QMLTUI_CORE_EXPORT LayoutAttached : public QObject {
    Q_OBJECT
    Q_PROPERTY(int minimumWidth MEMBER minimumWidth NOTIFY changed)
    Q_PROPERTY(int minimumHeight MEMBER minimumHeight NOTIFY changed)
    // -1 falls back to the item's implicitWidth / implicitHeight.
    Q_PROPERTY(int preferredWidth MEMBER preferredWidth NOTIFY changed)
    Q_PROPERTY(int preferredHeight MEMBER preferredHeight NOTIFY changed)
    Q_PROPERTY(int maximumWidth MEMBER maximumWidth NOTIFY changed)
    Q_PROPERTY(int maximumHeight MEMBER maximumHeight NOTIFY changed)
    Q_PROPERTY(bool fillWidth MEMBER fillWidth NOTIFY changed)
    Q_PROPERTY(bool fillHeight MEMBER fillHeight NOTIFY changed)
    Q_PROPERTY(int horizontalStretch MEMBER horizontalStretch NOTIFY changed)
    Q_PROPERTY(int verticalStretch MEMBER verticalStretch NOTIFY changed)
    // -1 lets GridLayout auto-place the item according to its flow.
    Q_PROPERTY(int row MEMBER row NOTIFY changed)
    Q_PROPERTY(int column MEMBER column NOTIFY changed)
    Q_PROPERTY(int rowSpan MEMBER rowSpan NOTIFY changed)
    Q_PROPERTY(int columnSpan MEMBER columnSpan NOTIFY changed)
    Q_PROPERTY(Qt::Alignment alignment MEMBER alignment NOTIFY changed)

public:
    explicit LayoutAttached(QObject* parent = nullptr) : QObject(parent) {}

    int minimumWidth = 0;
    int minimumHeight = 0;
    int preferredWidth = -1;
    int preferredHeight = -1;
    int maximumWidth = (std::numeric_limits<int>::max)();
    int maximumHeight = (std::numeric_limits<int>::max)();
    bool fillWidth = false;
    bool fillHeight = false;
    int horizontalStretch = 1;
    int verticalStretch = 1;
    int row = -1;
    int column = -1;
    int rowSpan = 1;
    int columnSpan = 1;
    Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignTop;

signals:
    void changed();
};

// Uncreatable QML attached-property provider for LayoutAttached (`Layout.*`).
class QMLTUI_CORE_EXPORT Layout : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Layout is an attached property")
    QML_ATTACHED(LayoutAttached)

public:
    static LayoutAttached* qmlAttachedProperties(QObject* object);
};

} // namespace qmltui

QML_DECLARE_TYPEINFO(qmltui::Layout, QML_HAS_ATTACHED_PROPERTIES)
