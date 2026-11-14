#include <QmlTui/Core/Engine/Shortcut.h>

#include <QHash>
#include <QMetaEnum>
#include <QQmlInfo>

namespace qmltui {

namespace {

int parseKeyToken(const QString& token, bool* ok) {
    *ok = true;
    const QString trimmed = token.trimmed();
    if (trimmed.isEmpty()) {
        *ok = false;
        return 0;
    }

    static const QHash<QString, int> namedKeys = {
        {QStringLiteral("ESC"), Qt::Key_Escape},
        {QStringLiteral("ESCAPE"), Qt::Key_Escape},
        {QStringLiteral("ENTER"), Qt::Key_Return},
        {QStringLiteral("RETURN"), Qt::Key_Return},
        {QStringLiteral("TAB"), Qt::Key_Tab},
        {QStringLiteral("BACKTAB"), Qt::Key_Backtab},
        {QStringLiteral("SPACE"), Qt::Key_Space},
        {QStringLiteral("BACKSPACE"), Qt::Key_Backspace},
        {QStringLiteral("BKSP"), Qt::Key_Backspace},
        {QStringLiteral("DEL"), Qt::Key_Delete},
        {QStringLiteral("DELETE"), Qt::Key_Delete},
        {QStringLiteral("INS"), Qt::Key_Insert},
        {QStringLiteral("INSERT"), Qt::Key_Insert},
        {QStringLiteral("HOME"), Qt::Key_Home},
        {QStringLiteral("END"), Qt::Key_End},
        {QStringLiteral("PGUP"), Qt::Key_PageUp},
        {QStringLiteral("PAGEUP"), Qt::Key_PageUp},
        {QStringLiteral("PGDN"), Qt::Key_PageDown},
        {QStringLiteral("PAGEDOWN"), Qt::Key_PageDown},
        {QStringLiteral("UP"), Qt::Key_Up},
        {QStringLiteral("DOWN"), Qt::Key_Down},
        {QStringLiteral("LEFT"), Qt::Key_Left},
        {QStringLiteral("RIGHT"), Qt::Key_Right},
        {QStringLiteral("PLUS"), Qt::Key_Plus},
        {QStringLiteral("MINUS"), Qt::Key_Minus},
        {QStringLiteral("SLASH"), Qt::Key_Slash},
        {QStringLiteral("COMMA"), Qt::Key_Comma},
        {QStringLiteral("PERIOD"), Qt::Key_Period},
    };

    const QString upper = trimmed.toUpper();
    if (const auto it = namedKeys.constFind(upper); it != namedKeys.cend()) {
        return it.value();
    }

    if (upper.startsWith(QLatin1Char('F')) && upper.size() > 1) {
        bool numOk = false;
        const int number = upper.sliced(1).toInt(&numOk);
        if (numOk && number >= 1 && number <= 35) {
            return Qt::Key_F1 + number - 1;
        }
    }

    if (trimmed.size() == 1) {
        const QChar ch = trimmed.front();
        if (ch.unicode() >= 0x20 && ch.unicode() <= 0x7e) {
            if (ch.isLetter()) {
                return Qt::Key_A + (ch.toUpper().unicode() - u'A');
            }
            if (ch.isDigit()) {
                return Qt::Key_0 + (ch.unicode() - u'0');
            }
            return static_cast<int>(ch.unicode());
        }
    }

    const QMetaEnum metaEnum = QMetaEnum::fromType<Qt::Key>();
    QString enumCandidate = trimmed;
    if (!enumCandidate.startsWith(QLatin1String("Key_"), Qt::CaseInsensitive)) {
        enumCandidate.prepend(QStringLiteral("Key_"));
    }
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        const char* enumName = metaEnum.key(i);
        if (enumName && enumCandidate.compare(QLatin1String(enumName), Qt::CaseInsensitive) == 0) {
            return metaEnum.value(i);
        }
    }

    *ok = false;
    return 0;
}

int parseChordString(const QString& input, bool* ok) {
    *ok = true;
    const QString trimmed = input.trimmed();
    if (trimmed.isEmpty()) {
        return 0;
    }

    if (trimmed.size() > 1 && trimmed.contains(QLatin1Char(',')) &&
        !trimmed.endsWith(QLatin1String("+,"))) {
        *ok = false;
        return 0;
    }

    int modifiers = 0;
    qsizetype pos = 0;
    const qsizetype len = trimmed.size();
    while (pos < len - 1) {
        const qsizetype plusIdx = trimmed.indexOf(QLatin1Char('+'), pos);
        if (plusIdx < 0) break;
        if (plusIdx == pos || plusIdx == len - 1) {
            *ok = false;
            return 0;
        }
        const QString modName = trimmed.sliced(pos, plusIdx - pos).trimmed().toUpper();
        if (modName == QLatin1String("CTRL") || modName == QLatin1String("CONTROL")) {
            modifiers |= Qt::ControlModifier;
        } else if (modName == QLatin1String("SHIFT")) {
            modifiers |= Qt::ShiftModifier;
        } else if (modName == QLatin1String("ALT")) {
            modifiers |= Qt::AltModifier;
        } else if (modName == QLatin1String("META") || modName == QLatin1String("SUPER") ||
                   modName == QLatin1String("WIN")) {
            modifiers |= Qt::MetaModifier;
        } else {
            *ok = false;
            return 0;
        }
        pos = plusIdx + 1;
    }

    bool keyOk = false;
    const int key = parseKeyToken(trimmed.sliced(pos), &keyOk);
    if (!keyOk || key == 0) {
        *ok = false;
        return 0;
    }
    if (key == Qt::Key_Tab && (modifiers & Qt::ShiftModifier) != 0 &&
        (modifiers & ~Qt::ShiftModifier) == 0) {
        return QKeyCombination(Qt::ShiftModifier, Qt::Key_Backtab).toCombined();
    }
    return modifiers | key;
}

} // namespace

static thread_local QList<Shortcut*> s_shortcuts;

const QList<Shortcut*>& activeShortcuts() {
    return s_shortcuts;
}

void registerShortcut(Shortcut* shortcut) {
    if (shortcut && !s_shortcuts.contains(shortcut)) {
        s_shortcuts.append(shortcut);
    }
}

void unregisterShortcut(Shortcut* shortcut) {
    if (shortcut) {
        s_shortcuts.removeOne(shortcut);
    }
}

Shortcut::Shortcut(QObject* parent) : QObject(parent) {
    registerShortcut(this);
}

Shortcut::~Shortcut() {
    unregisterShortcut(this);
}

void Shortcut::setSequence(const QVariant& sequence) {
    int parsed = 0;
    if (!sequence.isValid() || sequence.isNull()) {
        parsed = 0;
    } else if (sequence.typeId() == QMetaType::QString) {
        bool ok = false;
        const QString text = sequence.toString();
        parsed = parseChordString(text, &ok);
        if (!ok) {
            qmlWarning(this) << "Invalid Shortcut sequence \"" << text
                             << "\"; expected a single key chord such as \"Ctrl+S\" or an "
                                "integer Qt.Key combination.";
        }
    } else if (sequence.userType() == QMetaType::fromType<QKeyCombination>().id()) {
        parsed = sequence.value<QKeyCombination>().toCombined();
    } else {
        bool ok = false;
        parsed = sequence.toInt(&ok);
        if (!ok) {
            qmlWarning(this) << "Invalid Shortcut sequence value of type " << sequence.typeName();
            parsed = 0;
        }
    }

    if (m_sequence == sequence && m_keyCombination == parsed) {
        return;
    }
    m_sequence = sequence;
    m_keyCombination = parsed;
    emit sequenceChanged();
}

void Shortcut::setSequence(int sequence) {
    setSequence(QVariant(sequence));
}

void Shortcut::setSequence(QKeyCombination sequence) {
    setSequence(QVariant(sequence.toCombined()));
}

void Shortcut::setSequence(const QString& sequence) {
    setSequence(QVariant(sequence));
}

void Shortcut::setEnabled(bool enabled) {
    if (m_enabled != enabled) {
        m_enabled = enabled;
        emit enabledChanged();
    }
}

} // namespace qmltui
