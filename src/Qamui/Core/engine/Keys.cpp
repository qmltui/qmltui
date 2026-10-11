#include <Qamui/Core/engine/Keys.h>

namespace qamui {

Keys *Keys::qmlAttachedProperties(QObject *object) {
    return new Keys(object);
}

} // namespace qamui
