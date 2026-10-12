#pragma once

#include <QmlTui/Core/Engine/QtMessageRouting.h>

#include <cstdint>

namespace qmltui::TerminalMessageRouter {

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

} // namespace qmltui::TerminalMessageRouter
