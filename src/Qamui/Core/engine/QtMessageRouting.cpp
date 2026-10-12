#include <Qamui/Core/engine/QtMessageRouting.h>
#include <Qamui/Core/engine/TerminalMessageRouter_p.h>

#include <atomic>

namespace qamui {
namespace {

std::atomic<uint64_t> s_nextRegistrationId{1};

} // namespace

QtMessageRouting::QtMessageRouting(Token, uint64_t registrationId)
    : m_active(true)
    , m_registrationId(registrationId)
{
}

QtMessageRouting::~QtMessageRouting()
{
    reset();
}

std::unique_ptr<QtMessageRouting> QtMessageRouting::install(
    QtMessageSink diagnosticSink,
    QtMessageSink presentationSink)
{
    if (!diagnosticSink)
        return nullptr;

    const bool hasPresentationSink = static_cast<bool>(presentationSink);
    const uint64_t regId = s_nextRegistrationId.fetch_add(1, std::memory_order_relaxed);
    if (!TerminalMessageRouter::installRegistration(regId,
                                                    std::move(diagnosticSink),
                                                    std::move(presentationSink),
                                                    hasPresentationSink)) {
        return nullptr;
    }

    return std::make_unique<QtMessageRouting>(Token{}, regId);
}

QtMessageSink QtMessageRouting::stderrPresentationSink()
{
    return TerminalMessageRouter::stderrPresentationSink();
}

void QtMessageRouting::reset()
{
    if (!m_active)
        return;

    m_active = false;
    TerminalMessageRouter::resetRegistration(m_registrationId);
}

} // namespace qamui
