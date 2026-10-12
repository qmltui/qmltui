#include <QmlTui/Core/Engine/Keys.h>

namespace qmltui {

Keys *Keys::qmlAttachedProperties(QObject *object) {
    return new Keys(object);
}

} // namespace qmltui
