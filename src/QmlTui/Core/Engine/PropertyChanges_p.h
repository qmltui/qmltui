#pragma once

// Internal: private storage and QQmlCustomParser implementation for
// PropertyChanges.

#include <QmlTui/Core/Engine/State.h>
#include <QString>
#include <QVariant>
#include <QList>
#include <QUrl>
#include <memory>

#if defined(QMLTUI_ENABLE_PRIVATE_QT)
#include <private/qv4compileddata_p.h>
#include <private/qqmlcustomparser_p.h>
#endif

namespace qmltui {

// One property assignment or binding expression decoded from a PropertyChanges
// QML block by PropertyChangesParser.
struct DecodedBinding {
    QString propertyName;
    QVariant literalValue;
    bool isLiteral = false;
    bool isExpression = false;
    quint32 compiledScriptIndex = ~0u;
    QString expressionCode;
    QUrl url;
    int line = -1;
    int column = -1;
#if defined(QMLTUI_ENABLE_PRIVATE_QT)
    const QV4::CompiledData::Binding *rawBinding = nullptr;
#endif
};

class PropertyChangesPrivate {
public:
    PropertyChanges* q = nullptr;
    bool isExplicit = false;
    bool hasCustomBindings = false;
    bool hasDecoded = false;
    QList<DecodedBinding> decodedBindings;

#if defined(QMLTUI_ENABLE_PRIVATE_QT)
    QQmlRefPointer<QV4::ExecutableCompilationUnit> compilationUnit;
    QList<const QV4::CompiledData::Binding *> rawBindings;

    void setBindings(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit,
                     const QList<const QV4::CompiledData::Binding *> &bindings);
    void decode();
    void decodeBinding(const QString &propertyPrefix,
                       const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit,
                       const QV4::CompiledData::Binding *binding);
#endif
};

#if defined(QMLTUI_ENABLE_PRIVATE_QT)
// Intercepts direct property assignments on PropertyChanges in QML so they are
// deferred until the owning State becomes active rather than evaluated on the
// PropertyChanges object itself.
class PropertyChangesParser : public QT_PREPEND_NAMESPACE(QQmlCustomParser) {
public:
    PropertyChangesParser() : QT_PREPEND_NAMESPACE(QQmlCustomParser)(AcceptsAttachedProperties) {}

    void verifyList(const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &compilationUnit,
                    const QV4::CompiledData::Binding *binding,
                    bool &hasPropertiesMap,
                    bool &hasDirectProps);

    void verifyBindings(const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &compilationUnit,
                        const QList<const QV4::CompiledData::Binding *> &props) override;

    void applyBindings(QObject *obj,
                       const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit,
                       const QList<const QV4::CompiledData::Binding *> &bindings) override;
};
#endif

} // namespace qmltui
