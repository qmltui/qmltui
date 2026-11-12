#pragma once

// Internal: keyboard, mouse, and grapheme parsing helpers for Runner.

#include <QString>
#include <QStringList>
#include <Qt>

namespace qmltui::runner_detail {

struct ParsedKey {
    int key = Qt::Key_unknown;
    QString text;
};

bool parseKey(const QString& input, ParsedKey* parsed);
bool parseModifiers(const QStringList& names, Qt::KeyboardModifiers* modifiers,
                    QString* invalidModifier);
bool parseMouseButton(const QString& name, int* button, QString* error);
bool isSingleGrapheme(const QString& text);

} // namespace qmltui::runner_detail
