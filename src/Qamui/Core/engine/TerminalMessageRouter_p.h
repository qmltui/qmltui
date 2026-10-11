#pragma once

namespace qamui::TerminalMessageRouter {

void registerEngine();
void unregisterEngine();
void beginAlternateScreenSession();
void endAlternateScreenSession();

} // namespace qamui::TerminalMessageRouter
