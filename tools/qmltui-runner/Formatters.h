#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QString>

namespace qmltui::runner_cli {

// Wraps ANSI frame rows (and optional Inline scrollback rows) with synchronized
// output bracketing, cursor homing, and per-line/end-of-screen clearing so live
// viewers paint cleanly even across viewport shrinks.
QByteArray renderLiveAnsiFrame(const QByteArray& ansiFrame, const QByteArray& scrollbackAnsi = {});

// Renders a committed text grid with optional 0-based (x, y) column and row rulers.
QString formatRulerFrame(const QString& text, int cols, int rows, bool showRuler);

// Extracts the lines following "--- styles" from styledText() output.
QString extractStylesSection(const QString& styledText);

// Formats a single-line summary of Runner frameStats().
QString formatFrameStatsLine(const QJsonObject& stats);

// Formats a recursive itemTree() object as an indented, agent-readable tree.
QString formatItemTree(const QJsonObject& node, int indent = 0);

// Formats an interactive step or inspection response for terminal/agent stdout.
QString formatInteractiveResponse(const QJsonObject& response, bool showRuler, bool showStyled,
                                  bool showScrollback, bool showTree);

} // namespace qmltui::runner_cli
