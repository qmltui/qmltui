#include <Qamui/Core/engine/RenderPhase_p.h>
#include <Qamui/Core/items/Item.h>

#include <QByteArray>
#include <QSet>
#include <QtGlobal>

namespace qamui {

namespace {

RenderPhase::Phase s_phase = RenderPhase::Idle;
int s_dirtyDuringPaint = 0;

bool debugPaintDirtyEnabled()
{
    static const bool enabled = [] {
        const QByteArray value = qgetenv("QAMUI_DEBUG_PAINT_DIRTY").trimmed().toLower();
        return !value.isEmpty() && value != "0" && value != "false" && value != "off";
    }();
    return enabled;
}

} // namespace

RenderPhase::Phase RenderPhase::current()
{
    return s_phase;
}

int RenderPhase::dirtyDuringPaintCount()
{
    return s_dirtyDuringPaint;
}

void RenderPhase::recordDirtyDuringPaint(const Item *item)
{
    ++s_dirtyDuringPaint;
    if (!debugPaintDirtyEnabled() || !item)
        return;
    static QSet<QByteArray> reported;
    QByteArray key = item->metaObject()->className();
    if (!item->objectName().isEmpty())
        key += " \"" + item->objectName().toUtf8() + '"';
    if (reported.contains(key))
        return;
    reported.insert(key);
    qWarning("Qamui: %s invalidated itself while painting; move the change to "
             "updateLayout() or a property setter", key.constData());
}

RenderPhase::Scope::Scope(Phase phase)
    : m_previous(s_phase)
{
    s_phase = phase;
    if (phase == Paint)
        s_dirtyDuringPaint = 0;
}

RenderPhase::Scope::~Scope()
{
    s_phase = m_previous;
}

} // namespace qamui
