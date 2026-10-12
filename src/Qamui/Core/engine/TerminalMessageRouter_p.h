#pragma once

#include <Qamui/Core/engine/QtMessageRouting.h>

#include <cstdint>

namespace qamui::TerminalMessageRouter {

void registerEngine();
void unregisterEngine();
void beginAlternateScreenSession();
void endAlternateScreenSession();

bool installRegistration(uint64_t registrationId,
                         QtMessageSink diagnosticSink,
                         QtMessageSink presentationSink,
                         bool hasPresentationSink);
void resetRegistration(uint64_t registrationId);
QtMessageSink stderrPresentationSink();

} // namespace qamui::TerminalMessageRouter
