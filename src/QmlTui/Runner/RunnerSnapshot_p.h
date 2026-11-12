#pragma once

// Internal: frame formatting, snapshot diffing, and timing helpers for Runner.

#include <QmlTui/Core/Primitives/Cell.h>
#include <QString>
#include <cstdint>

namespace qmltui {

class Item;

namespace runner_detail {

inline constexpr int minimumTimeoutMs = 1;
inline constexpr int maximumTimeoutMs = 30000;
// A single wait longer than 30 s is a scenario bug, but a long walkthrough is
// not: the whole-scenario watchdog accepts up to five minutes.
inline constexpr int maximumScenarioTimeoutMs = 300000;
inline constexpr int conditionPollIntervalMs = 10;

QString pausedClockHint();
Item* findVisibleItem(Item* item, const QString& objectName);
QString rgbaString(uint32_t rgba);
QString attributeNames(TextAttribute attributes);
bool parseRgbaString(const QString& text, uint32_t* rgba);

} // namespace runner_detail
} // namespace qmltui
