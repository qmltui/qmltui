#include <Qamui/Core/engine/State.h>
#include <Qamui/Core/engine/PropertyChanges_p.h>
#include <QCoreApplication>
#include <qqmlinfo.h>

#if defined(QAMUI_ENABLE_PRIVATE_QT)
#include <private/qqmldata_p.h>
#include <private/qqmlanybinding_p.h>
#include <private/qqmlcontextdata_p.h>
#include <private/qqmlengine_p.h>
#include <private/qv4qmlcontext_p.h>
#endif

namespace qamui {

// ==========================================
// PropertyChanges Class
// ==========================================

PropertyChanges::PropertyChanges(QObject* parent)
    : QObject(parent), d(std::make_unique<PropertyChangesPrivate>()) {
    d->q = this;
}

PropertyChanges::~PropertyChanges() = default;

void PropertyChanges::setTarget(QObject* t) {
    if (m_target.data() == t) return;
    m_target = t;
    emit targetChanged();
}

void PropertyChanges::setProperties(const QVariantMap& p) {
    if (m_properties == p) return;
    m_properties = p;
    emit propertiesChanged();
}

void PropertyChanges::setRestoreEntryValues(bool r) {
    if (m_restoreEntryValues == r) return;
    m_restoreEntryValues = r;
    emit restoreEntryValuesChanged();
}

#if defined(QAMUI_ENABLE_PRIVATE_QT)
bool PropertyChanges::isExplicit() const {
    return d->isExplicit;
}

void PropertyChanges::setIsExplicit(bool e) {
    if (d->isExplicit == e) return;
    d->isExplicit = e;
    emit explicitChanged();
}
#endif

// ==========================================
// State Class
// ==========================================

State::State(QObject* parent) : QObject(parent) {
}

void State::setName(const QString& n) {
    if (m_name == n) return;
    m_name = n;
    emit nameChanged();
}

void State::setWhen(bool w) {
    if (m_when == w) return;
    m_when = w;
    emit whenChanged();
}

QQmlListProperty<QObject> State::changes() const {
    return QQmlListProperty<QObject>(
        const_cast<State*>(this),
        nullptr,
        &State::appendChange,
        &State::changeCount,
        &State::changeAt,
        &State::clearChanges
    );
}

void State::appendChange(QQmlListProperty<QObject>* list, QObject* obj) {
    auto* self = qobject_cast<State*>(list->object);
    if (self && obj) {
        self->m_changesList.append(obj);
    }
}

qsizetype State::changeCount(QQmlListProperty<QObject>* list) {
    auto* self = qobject_cast<State*>(list->object);
    return self ? self->m_changesList.size() : 0;
}

QObject* State::changeAt(QQmlListProperty<QObject>* list, qsizetype index) {
    auto* self = qobject_cast<State*>(list->object);
    return (self && index >= 0 && index < self->m_changesList.size())
        ? self->m_changesList.at(index).data() : nullptr;
}

void State::clearChanges(QQmlListProperty<QObject>* list) {
    auto* self = qobject_cast<State*>(list->object);
    if (self) {
        self->m_changesList.clear();
    }
}

QList<PropertyChanges*> State::propertyChangesList() const {
    QList<PropertyChanges*> result;
    for (const QPointer<QObject>& changeObj : m_changesList) {
        if (changeObj) {
            if (auto* pc = qobject_cast<PropertyChanges*>(changeObj.data())) {
                result.append(pc);
            }
        }
    }
    return result;
}

// ==========================================
// PropertyChangesPrivate & Custom Parser
// ==========================================

#if defined(QAMUI_ENABLE_PRIVATE_QT)
void PropertyChangesPrivate::setBindings(
    const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit,
    const QList<const QV4::CompiledData::Binding *> &bindings)
{
    compilationUnit = unit;
    rawBindings = bindings;
    hasCustomBindings = !rawBindings.isEmpty();
    hasDecoded = false;
    decode();
}

void PropertyChangesPrivate::decode()
{
    if (hasDecoded || !compilationUnit)
        return;
    hasDecoded = true;
    decodedBindings.clear();

    for (const auto *b : rawBindings) {
        decodeBinding(QString(), compilationUnit, b);
    }
}

void PropertyChangesPrivate::decodeBinding(
    const QString &propertyPrefix,
    const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit,
    const QV4::CompiledData::Binding *binding)
{
    QString propertyName = propertyPrefix + unit->stringAt(binding->propertyNameIndex);

    switch (binding->type()) {
    case QV4::CompiledData::Binding::Type_GroupProperty:
    case QV4::CompiledData::Binding::Type_AttachedProperty: {
        QString pre = propertyName + QLatin1Char('.');
        const QV4::CompiledData::Object *subObj = unit->objectAt(binding->value.objectIndex);
        const QV4::CompiledData::Binding *subBinding = subObj->bindingTable();
        for (quint32 i = 0; i < subObj->nBindings; ++i, ++subBinding) {
            decodeBinding(pre, unit, subBinding);
        }
        return;
    }
    default:
        break;
    }

    if (propertyName == QLatin1String("target") ||
        propertyName == QLatin1String("restoreEntryValues") ||
        propertyName == QLatin1String("explicit") ||
        propertyName == QLatin1String("properties") ||
        propertyName == QLatin1String("objectName")) {
        return;
    }

    DecodedBinding decoded;
    decoded.propertyName = propertyName;
    decoded.rawBinding = binding;

    if (binding->type() == QV4::CompiledData::Binding::Type_Script || binding->isTranslationBinding()) {
        decoded.isExpression = true;
        if (!binding->isTranslationBinding()) {
            decoded.expressionCode = unit->bindingValueAsString(binding);
            decoded.compiledScriptIndex = binding->value.compiledScriptIndex;
        }
        QQmlData *ddata = QQmlData::get(q);
        if (ddata && ddata->outerContext && !ddata->outerContext->url().isEmpty()) {
            decoded.url = ddata->outerContext->url();
            decoded.line = ddata->lineNumber;
            decoded.column = ddata->columnNumber;
        }
    } else {
        decoded.isLiteral = true;
        switch (binding->type()) {
        case QV4::CompiledData::Binding::Type_String:
            decoded.literalValue = unit->bindingValueAsString(binding);
            break;
        case QV4::CompiledData::Binding::Type_Number:
            decoded.literalValue = unit->bindingValueAsNumber(binding);
            break;
        case QV4::CompiledData::Binding::Type_Boolean:
            decoded.literalValue = binding->valueAsBoolean();
            break;
        case QV4::CompiledData::Binding::Type_Null:
            decoded.literalValue = QVariant::fromValue(nullptr);
            break;
        default:
            break;
        }
    }

    decodedBindings.append(decoded);
}
#endif

#if defined(QAMUI_ENABLE_PRIVATE_QT)

void PropertyChangesParser::verifyList(
    const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &compilationUnit,
    const QV4::CompiledData::Binding *binding,
    bool &hasPropertiesMap,
    bool &hasDirectProps)
{
    QString name = compilationUnit->stringAt(binding->propertyNameIndex);
    if (name == QLatin1String("properties")) {
        hasPropertiesMap = true;
    } else if (name != QLatin1String("target") &&
               name != QLatin1String("restoreEntryValues") &&
               name != QLatin1String("explicit") &&
               name != QLatin1String("objectName")) {
        hasDirectProps = true;
    }

    switch (binding->type()) {
    case QV4::CompiledData::Binding::Type_Object:
        error(compilationUnit->objectAt(binding->value.objectIndex),
              QCoreApplication::translate("PropertyChanges", "PropertyChanges does not support creating state-specific objects."));
        break;
    case QV4::CompiledData::Binding::Type_GroupProperty:
    case QV4::CompiledData::Binding::Type_AttachedProperty: {
        const QV4::CompiledData::Object *subObj = compilationUnit->objectAt(binding->value.objectIndex);
        const QV4::CompiledData::Binding *subBinding = subObj->bindingTable();
        for (quint32 i = 0; i < subObj->nBindings; ++i, ++subBinding) {
            verifyList(compilationUnit, subBinding, hasPropertiesMap, hasDirectProps);
        }
        break;
    }
    default:
        break;
    }
}

void PropertyChangesParser::verifyBindings(
    const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &compilationUnit,
    const QList<const QV4::CompiledData::Binding *> &props)
{
    bool hasPropertiesMap = false;
    bool hasDirectProps = false;
    for (const auto *b : props) {
        verifyList(compilationUnit, b, hasPropertiesMap, hasDirectProps);
    }

    if (!props.isEmpty()) {
        const QV4::CompiledData::Binding *firstBinding = props.first();
        for (quint32 i = 0; i < compilationUnit->objectCount(); ++i) {
            const QV4::CompiledData::Object *obj = compilationUnit->objectAt(i);
            if (firstBinding >= obj->bindingTable() && firstBinding < obj->bindingTable() + obj->nBindings) {
                for (quint32 j = 0; j < obj->nBindings; ++j) {
                    const QV4::CompiledData::Binding *b = obj->bindingTable() + j;
                    QString name = compilationUnit->stringAt(b->propertyNameIndex);
                    if (name == QLatin1String("properties")) {
                        hasPropertiesMap = true;
                        break;
                    }
                }
                break;
            }
        }
    }

    if (hasPropertiesMap && hasDirectProps) {
        error(props.first(),
              QCoreApplication::translate("PropertyChanges", "Cannot mix properties map and direct assignments in PropertyChanges."));
    }
}

void PropertyChangesParser::applyBindings(
    QObject *obj,
    const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit,
    const QList<const QV4::CompiledData::Binding *> &bindings)
{
    auto *pc = qobject_cast<PropertyChanges*>(obj);
    if (pc && pc->d_func()) {
        pc->d_func()->setBindings(compilationUnit, bindings);
    }

    QQmlData *data = QQmlData::get(obj);
    if (data && !data->wasDeleted(obj))
        data->releaseDeferredData();
}
#endif

} // namespace qamui

#if defined(QAMUI_ENABLE_PRIVATE_QT)
QT_BEGIN_NAMESPACE
template<>
QQmlCustomParser *qmlCreateCustomParser<qamui::PropertyChanges>()
{
    return new qamui::PropertyChangesParser;
}
QT_END_NAMESPACE
#endif
