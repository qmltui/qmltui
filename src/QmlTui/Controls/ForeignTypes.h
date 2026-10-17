#pragma once

// Internal: QML foreign-type registrations exposing Core items under QmlTui.Controls.

#include <QtQml/qqml.h>
#include <QmlTui/Core/Items/Popup.h>

namespace qmltui {

struct PopupForeign {
    Q_GADGET
    QML_FOREIGN(qmltui::Popup)
    QML_NAMED_ELEMENT(Popup)
};

} // namespace qmltui
