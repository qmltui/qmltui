#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QmlTui/Core/Primitives/Color.h>

#include <QmlTui/Core/Primitives/Span.h>
#include <QVariantMap>
#include <QObject>

#include <QtQml/qqmlregistration.h>

namespace qmltui {

// QmlTui QML singleton providing value-type factory helpers (QmlTui.span,
// QmlTui.color, QmlTui.rgba, QmlTui.ansi, QmlTui.defaultColor, and QmlTui.transparent).
class QMLTUI_CORE_EXPORT QmlTuiGlobal : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(QmlTui)
    QML_SINGLETON

    Q_PROPERTY(qmltui::Color defaultColor READ defaultColor CONSTANT FINAL)
    Q_PROPERTY(qmltui::Color transparent READ transparent CONSTANT FINAL)

public:
    explicit QmlTuiGlobal(QObject* parent = nullptr) : QObject(parent) {}

    constexpr Color defaultColor() const { return Color::Default; }
    constexpr Color transparent() const { return Color::Transparent; }

    Q_INVOKABLE qmltui::Span span(const QString& text) const;
    Q_INVOKABLE qmltui::Span span(const QString& text, const QVariantMap& options) const;

    Q_INVOKABLE qmltui::Color color(const QString& value) const;
    Q_INVOKABLE qmltui::Color rgba(qreal red, qreal green, qreal blue, qreal alpha = 1.0) const;
    Q_INVOKABLE qmltui::Color ansi(int index) const;
};

} // namespace qmltui
