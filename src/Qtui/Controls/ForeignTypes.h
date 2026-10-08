#pragma once

#include <QtQml/qqml.h>
#include <Qtui/Core/items/Popup.h>

namespace qtui {

// Core types that belong to the Controls module from QML's point of view.
struct PopupForeign {
    Q_GADGET
    QML_FOREIGN(qtui::Popup)
    QML_NAMED_ELEMENT(Popup)
};

} // namespace qtui
