#pragma once

#include <QmlTui/Core/Items/Item.h>
#include <QmlTui/Core/Primitives/QmlTuiExport.h>
#include <QtQml/qqmlregistration.h>
#include <memory>

namespace qmltui {

struct FocusScopePrivate;

// FocusScope provides an isolated focus namespace for subtrees, acting as a
// single focusable proxy externally while remembering internal active child focus.
class QMLTUI_CORE_EXPORT FocusScope : public Item {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(FocusScope)
    QML_ELEMENT

public:
    explicit FocusScope(QObject* parent = nullptr);
    ~FocusScope() override;

    bool isFocusScope() const override { return true; }

    Item* subFocusItem() const;
    void setSubFocusItem(Item* item);

    void setFocus(bool focus) override;

private:
    std::unique_ptr<FocusScopePrivate> m_d;
};

} // namespace qmltui
