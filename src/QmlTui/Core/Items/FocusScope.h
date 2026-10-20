#pragma once

#include <QPointer>
#include <QmlTui/Core/Items/Item.h>
#include <QmlTui/Core/Primitives/QmlTuiExport.h>
#include <QtQml/qqmlregistration.h>

namespace qmltui {

// FocusScope provides an isolated focus namespace for subtrees, acting as a
// single focusable proxy externally while remembering internal active child focus.
class QMLTUI_CORE_EXPORT FocusScope : public Item {
    Q_OBJECT
    QML_ELEMENT

public:
    explicit FocusScope(QObject* parent = nullptr);
    ~FocusScope() override = default;

    bool isFocusScope() const override { return true; }

    Item* subFocusItem() const { return m_subFocusItem.data(); }
    void setSubFocusItem(Item* item);

    void setFocus(bool focus) override;

private:
    QPointer<Item> m_subFocusItem;
};

} // namespace qmltui
