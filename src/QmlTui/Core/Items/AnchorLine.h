#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <cstdint>
#include <QObject>
#include <QQmlEngine>
#include <QmlTui/Core/Items/Item.h>

namespace qmltui {

// Edge or center line handle of an Item (left, right, top, bottom,
// horizontalCenter, verticalCenter) referenced by Anchors bindings.
class QMLTUI_CORE_EXPORT AnchorLine : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("AnchorLine is only available as a property of Item")

public:
    enum Edge : uint8_t {
        Left,
        Right,
        Top,
        Bottom,
        HorizontalCenter,
        VerticalCenter,
    };
    Q_ENUM(Edge)

    AnchorLine(Edge edge, Item* owner) : QObject(owner), m_edge(edge), m_owner(owner) {}
    ~AnchorLine() override = default;

    Edge edge() const { return m_edge; }
    Item* owner() const { return m_owner; }

private:
    Edge m_edge;
    Item* m_owner;
};

} // namespace qmltui
