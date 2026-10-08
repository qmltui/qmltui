#pragma once

namespace qtui::TerminalMessageRouter {

void registerEngine();
void unregisterEngine();
void beginAlternateScreenSession();
void endAlternateScreenSession();

} // namespace qtui::TerminalMessageRouter
