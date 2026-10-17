#pragma once

// Internal: process-wide Qt message handler registry bridging TerminalEngine
// alternate-screen sessions and QtMessageRouting. Suppresses default stderr
// output while an alternate-screen session is active so qDebug/qWarning calls
// do not corrupt the rendered frame.

#include <QmlTui/Core/Engine/QtMessageRouting.h>

#include <cstdint>

namespace qmltui::TerminalMessageRouter {

void registerEngine();
void unregisterEngine();
void beginAlternateScreenSession();
void endAlternateScreenSession();

bool installRegistration(uint64_t registrationId, QtMessageSink diagnosticSink,
                         QtMessageSink presentationSink, bool hasPresentationSink);
void resetRegistration(uint64_t registrationId);
QtMessageSink stderrPresentationSink();

} // namespace qmltui::TerminalMessageRouter
