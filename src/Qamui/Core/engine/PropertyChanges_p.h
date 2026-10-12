#pragma once

#include <Qamui/Core/engine/State.h>
#include <QString>
#include <QVariant>
#include <QList>
#include <QUrl>
#include <memory>

#if defined(QAMUI_ENABLE_PRIVATE_QT)
#include <private/qv4compileddata_p.h>
#include <private/qqmlcustomparser_p.h>
#endif

namespace qamui {

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
#if defined(QAMUI_ENABLE_PRIVATE_QT)
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

#if defined(QAMUI_ENABLE_PRIVATE_QT)
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

#if defined(QAMUI_ENABLE_PRIVATE_QT)
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

} // namespace qamui
