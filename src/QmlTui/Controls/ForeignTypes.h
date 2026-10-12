#pragma once

#include <QtQml/qqml.h>
#include <QmlTui/Core/Items/Popup.h>

namespace qmltui {

// Core types that belong to the Controls module from QML's point of view.
struct PopupForeign {
    Q_GADGET
    QML_FOREIGN(qmltui::Popup)
    QML_NAMED_ELEMENT(Popup)
};

} // namespace qmltui
