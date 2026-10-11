#pragma once

#include <QtQml/qqml.h>
#include <Qamui/Core/items/Popup.h>

namespace qamui {

// Core types that belong to the Controls module from QML's point of view.
struct PopupForeign {
    Q_GADGET
    QML_FOREIGN(qamui::Popup)
    QML_NAMED_ELEMENT(Popup)
};

} // namespace qamui
