#include "QtuiGlobal.h"

namespace qtui {

Color QtuiGlobal::rgba(qreal red, qreal green, qreal blue, qreal alpha) const
{
    return Color::fromRgba(red, green, blue, alpha);
}

} // namespace qtui
