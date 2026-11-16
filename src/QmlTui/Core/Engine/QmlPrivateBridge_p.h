#pragma once

// Internal: centralized adapter isolating Qt6::QmlPrivate APIs from Engine classes.

#include <QObject>
#include <QPointer>
#include <QQmlContext>
#include <QQmlProperty>
#include <QString>
#include <QVariant>

#if defined(QMLTUI_ENABLE_PRIVATE_QT)
#include <QtQml/private/qqmlanybinding_p.h>
#endif

namespace qmltui {

class PropertyChanges;
struct DecodedBinding;

// Type-erased handle for a QML or QProperty binding captured from or installed
// onto a QQmlProperty. Compiles to a no-op stub when QMLTUI_ENABLE_PRIVATE_QT is OFF.
class SavedBinding {
public:
    SavedBinding() = default;

#if defined(QMLTUI_ENABLE_PRIVATE_QT)
    explicit SavedBinding(QQmlAnyBinding binding) noexcept : m_binding(std::move(binding)) {}
#endif

    static SavedBinding takeFrom(QObject* target, const QString& propertyName);

    void installOn(const QQmlProperty& property);

    explicit operator bool() const noexcept {
#if defined(QMLTUI_ENABLE_PRIVATE_QT)
        return static_cast<bool>(m_binding);
#else
        return false;
#endif
    }

private:
#if defined(QMLTUI_ENABLE_PRIVATE_QT)
    QQmlAnyBinding m_binding;
#endif
};

// Writes a value to `property` or `(target, name)` while bypassing any attached
// QQmlPropertyValueInterceptor (such as Behavior) and preserving existing bindings.
void writePropertyBypassingInterceptor(const QQmlProperty& property, const QVariant& value);
void writePropertyBypassingInterceptor(QObject* target, const QString& name, const QVariant& value);

// Detaches any active QML or QProperty binding from `property`.
void removeBindingFromProperty(const QQmlProperty& property);

// If `property` is a QML alias property, resolves its underlying target object
// and property name and returns true. Returns false when not an alias or when
// built without QMLTUI_ENABLE_PRIVATE_QT.
bool resolvePropertyAlias(const QQmlProperty& property, QPointer<QObject>& outTarget,
                          QString& outName);

// Creates a QQmlProperty for `(target, name)` within `rawContext`, allowing
// component ID prefixes (e.g. `label.text`) when QMLTUI_ENABLE_PRIVATE_QT is enabled.
QQmlProperty createQmlProperty(QObject* target, const QString& name, QQmlContext* rawContext);

// Evaluates a deferred script or translation binding from PropertyChanges and,
// unless `pc->isExplicit()` is true, constructs the live SavedBinding to install.
void evaluateDecodedBinding(PropertyChanges* pc, const DecodedBinding& binding,
                            const QQmlProperty& property, QQmlContext* rawContext,
                            QVariant& outValue, SavedBinding& outBinding, bool& outIsBinding);

// Reverts `(target, name)` to its pre-state binding or value.
void restoreOrWriteProperty(QObject* target, const QString& name, const QVariant& value,
                            bool hadBinding, const SavedBinding& binding);

// Applies an un-animated state override by either installing `binding` or writing `value`.
void installOrWriteProperty(QObject* target, const QString& name, const QVariant& value,
                            bool isBinding, const SavedBinding& binding);

} // namespace qmltui
