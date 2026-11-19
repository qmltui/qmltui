#include <QmlTui/Core/Engine/RenderPhase_p.h>
#include <QmlTui/Core/Items/Item.h>

#include <QByteArray>
#include <QSet>
#include <QThreadStorage>
#include <QtGlobal>

namespace qmltui {

namespace {

struct RenderPhaseThreadState {
    RenderPhase::Phase phase = RenderPhase::Idle;
    int dirtyDuringPaint = 0;
    QSet<QByteArray> reported;
};

RenderPhaseThreadState& threadState() {
    static QThreadStorage<RenderPhaseThreadState> state;
    return state.localData();
}

bool debugPaintDirtyEnabled() {
    static const bool enabled = [] {
        const QByteArray value = qgetenv("QMLTUI_DEBUG_PAINT_DIRTY").trimmed().toLower();
        return !value.isEmpty() && value != "0" && value != "false" && value != "off";
    }();
    return enabled;
}

} // namespace

RenderPhase::Phase RenderPhase::current() {
    return threadState().phase;
}

int RenderPhase::dirtyDuringPaintCount() {
    return threadState().dirtyDuringPaint;
}

void RenderPhase::recordDirtyDuringPaint(const Item* item) {
    RenderPhaseThreadState& state = threadState();
    ++state.dirtyDuringPaint;
    if (!debugPaintDirtyEnabled() || !item) return;
    QByteArray key = item->metaObject()->className();
    if (!item->objectName().isEmpty()) key += " \"" + item->objectName().toUtf8() + '"';
    if (state.reported.contains(key)) return;
    state.reported.insert(key);
    qWarning("QmlTui: %s invalidated itself while painting; move the change to "
             "updateLayout() or a property setter",
             key.constData());
}

RenderPhase::Scope::Scope(Phase phase) : m_previous(threadState().phase) {
    RenderPhaseThreadState& state = threadState();
    state.phase = phase;
    if (phase == Paint) state.dirtyDuringPaint = 0;
}

RenderPhase::Scope::~Scope() {
    threadState().phase = m_previous;
}

} // namespace qmltui
