#include <QmlTui/Core/Engine/QmlPrivateBridge_p.h>
#include <QmlTui/Core/Engine/PropertyChanges_p.h>
#include <QmlTui/Core/Engine/State.h>
#include <QmlTui/Core/Primitives/Color.h>

#include <QCoreApplication>
#include <QMetaProperty>
#include <QMetaType>
#include <QQmlEngine>

#if defined(QMLTUI_ENABLE_PRIVATE_QT)
#include <QtQml/private/qqmlanybinding_p.h>
#include <QtQml/private/qqmlbinding_p.h>
#include <QtQml/private/qqmlcontextdata_p.h>
#include <QtQml/private/qqmldata_p.h>
#include <QtQml/private/qqmlengine_p.h>
#include <QtQml/private/qqmlproperty_p.h>
#include <QtQml/private/qv4qmlcontext_p.h>
#endif

namespace qmltui {

SavedBinding SavedBinding::takeFrom(QObject* target, const QString& propertyName) {
#if defined(QMLTUI_ENABLE_PRIVATE_QT)
    if (target && !propertyName.isEmpty()) {
        QQmlProperty prop(target, propertyName);
        if (prop.isValid()) {
            return SavedBinding(QQmlAnyBinding::takeFrom(prop));
        }
    }
#else
    Q_UNUSED(target);
    Q_UNUSED(propertyName);
#endif
    return SavedBinding();
}

void SavedBinding::installOn(const QQmlProperty& property) {
#if defined(QMLTUI_ENABLE_PRIVATE_QT)
    if (property.isValid() && property.object() && m_binding) {
        m_binding.installOn(property);
    }
#else
    Q_UNUSED(property);
#endif
}

void writePropertyBypassingInterceptor(const QQmlProperty& property, const QVariant& value) {
    if (!property.isValid() || !property.object()) return;
#if defined(QMLTUI_ENABLE_PRIVATE_QT)
    QQmlPropertyPrivate::write(
        property, value, QQmlPropertyData::BypassInterceptor | QQmlPropertyData::DontRemoveBinding);
#else
    writePropertyBypassingInterceptor(property.object(), property.name(), value);
#endif
}

void removeBindingFromProperty(const QQmlProperty& property) {
#if defined(QMLTUI_ENABLE_PRIVATE_QT)
    if (property.isValid() && property.object()) {
        QQmlProperty mutableProperty = property;
        QQmlAnyBinding::removeBindingFrom(mutableProperty);
    }
#else
    Q_UNUSED(property);
#endif
}

void writePropertyBypassingInterceptor(QObject* target, const QString& name,
                                       const QVariant& value) {
    if (!target || name.isEmpty()) return;
#if defined(QMLTUI_ENABLE_PRIVATE_QT)
    // Dynamic QML properties have no C++ static_metacall. A regular property
    // write would re-enter Behavior and remove the property's source binding.
    const QQmlProperty property(target, name);
    const QVariant converted = property.propertyType() == QMetaType::QString &&
                                       value.metaType() == QMetaType::fromType<Color>()
                                   ? QVariant(value.value<Color>().toString())
                                   : value;
    writePropertyBypassingInterceptor(property, converted);
#else
    const QMetaObject* meta = target->metaObject();
    int idx = meta->indexOfProperty(name.toUtf8().constData());
    if (idx >= 0) {
        QMetaProperty prop = meta->property(idx);

        // Find the C++ class QMetaObject defining the property to get its C++ static_metacall
        const QMetaObject* definingMeta = target->metaObject();
        while (definingMeta) {
            const QMetaObject* super = definingMeta->superClass();
            if (!super || idx >= super->propertyCount()) {
                break;
            }
            definingMeta = super;
        }

        if (definingMeta && definingMeta->d.static_metacall) {
            int relativeId = idx;
            if (definingMeta->superClass()) {
                relativeId -= definingMeta->superClass()->propertyCount();
            }

            QMetaType mType = prop.metaType();
            int typeId = mType.id();
            int status = -1;
            int flags = 0;

            if (typeId == QMetaType::Int) {
                int val = value.toInt();
                void* argv[] = {&val, nullptr, &status, &flags};
                definingMeta->d.static_metacall(target, QMetaObject::WriteProperty, relativeId,
                                                argv);
            } else if (typeId == QMetaType::Double) {
                double val = value.toDouble();
                void* argv[] = {&val, nullptr, &status, &flags};
                definingMeta->d.static_metacall(target, QMetaObject::WriteProperty, relativeId,
                                                argv);
            } else if (typeId == QMetaType::QString) {
                QString val = (value.userType() == QMetaType::fromType<Color>().id())
                                  ? value.value<Color>().toString()
                                  : value.toString();
                void* argv[] = {&val, nullptr, &status, &flags};
                definingMeta->d.static_metacall(target, QMetaObject::WriteProperty, relativeId,
                                                argv);
            } else if (typeId == QMetaType::fromType<Color>().id()) {
                Color col;
                if (value.userType() == typeId) {
                    col = value.value<Color>();
                } else if (value.userType() == QMetaType::QString) {
                    col = Color::fromString(value.toString());
                } else {
                    col = Color::fromArgb32(Color::resolve(value.toString()));
                }
                void* argv[] = {&col, nullptr, &status, &flags};
                definingMeta->d.static_metacall(target, QMetaObject::WriteProperty, relativeId,
                                                argv);
            } else if (typeId == QMetaType::Bool) {
                bool val = value.toBool();
                void* argv[] = {&val, nullptr, &status, &flags};
                definingMeta->d.static_metacall(target, QMetaObject::WriteProperty, relativeId,
                                                argv);
            } else {
                QVariant converted = value;
                if (!mType.isValid() ||
                    (converted.metaType() != mType && !converted.convert(mType)) ||
                    !converted.isValid() || !converted.data()) {
                    prop.write(target, value);
                    return;
                }
                void* argv[] = {converted.data(), nullptr, &status, &flags};
                definingMeta->d.static_metacall(target, QMetaObject::WriteProperty, relativeId,
                                                argv);
            }
        } else {
            const QQmlProperty qmlProp(target, name);
            if (qmlProp.isValid()) {
                const QVariant converted = qmlProp.propertyType() == QMetaType::QString &&
                                                   value.metaType() == QMetaType::fromType<Color>()
                                               ? QVariant(value.value<Color>().toString())
                                               : value;
                qmlProp.write(converted);
            } else {
                prop.write(target, value);
            }
        }
    }
#endif
}

bool resolvePropertyAlias(const QQmlProperty& property, QPointer<QObject>& outTarget,
                          QString& outName) {
#if defined(QMLTUI_ENABLE_PRIVATE_QT)
    if (property.isValid() && QQmlPropertyPrivate::get(property)->core.isAlias()) {
        auto resolved = QQmlPropertyPrivate::findAliasTarget(
            property.object(), QQmlPropertyPrivate::propertyIndex(property));
        if (resolved.targetObject) {
            outTarget = resolved.targetObject;
            const QMetaObject* mo = resolved.targetObject->metaObject();
            int coreIdx = resolved.targetIndex.coreIndex();
            if (coreIdx >= 0 && coreIdx < mo->propertyCount()) {
                outName = QString::fromUtf8(mo->property(coreIdx).name());
            }
        }
        return true;
    }
#else
    Q_UNUSED(property);
    Q_UNUSED(outTarget);
    Q_UNUSED(outName);
#endif
    return false;
}

QQmlProperty createQmlProperty(QObject* target, const QString& name, QQmlContext* rawContext) {
#if defined(QMLTUI_ENABLE_PRIVATE_QT)
    QQmlRefPointer<QQmlContextData> context =
        rawContext ? QQmlContextData::get(rawContext) : QQmlRefPointer<QQmlContextData>();
    if (!context.isNull()) {
        return QQmlPropertyPrivate::create(target, name, context,
                                           QQmlPropertyPrivate::InitFlag::AllowId);
    }
    if (target) {
        return QQmlProperty(target, name);
    }
    return QQmlProperty();
#else
    if (rawContext) {
        return QQmlProperty(target, name, rawContext);
    }
    if (target) {
        return QQmlProperty(target, name);
    }
    return QQmlProperty();
#endif
}

void evaluateDecodedBinding(PropertyChanges* pc, const DecodedBinding& b, const QQmlProperty& prop,
                            QQmlContext* rawContext, QVariant& outValue, SavedBinding& outBinding,
                            bool& outIsBinding) {
    outValue = QVariant();
    outBinding = SavedBinding();
    outIsBinding = false;
#if defined(QMLTUI_ENABLE_PRIVATE_QT)
    if (!pc || !pc->d_func() || !pc->d_func()->compilationUnit || !prop.isValid() ||
        !prop.object()) {
        return;
    }

    QQmlEngine* engine = qmlEngine(pc);
    if (!engine && rawContext) engine = rawContext->engine();
    if (!engine || !engine->handle()) return;

    QQmlRefPointer<QQmlContextData> context =
        rawContext ? QQmlContextData::get(rawContext) : QQmlRefPointer<QQmlContextData>();

    QV4::Scope scope(engine->handle());
    QV4::Scoped<QV4::QmlContext> qmlCtxt(
        scope, QV4::QmlContext::create(scope.engine->rootContext(), context, prop.object()));

    std::unique_ptr<QQmlBinding> evalBinding;
    if (b.rawBinding && b.rawBinding->isTranslationBinding()) {
        evalBinding.reset(QQmlBinding::createTranslationBinding(
            pc->d_func()->compilationUnit, b.rawBinding, prop.object(), context));
    } else if (b.compiledScriptIndex != ~0u &&
               b.compiledScriptIndex < pc->d_func()->compilationUnit->runtimeFunctions.size()) {
        evalBinding.reset(QQmlBinding::create(
            &QQmlPropertyPrivate::get(prop)->core,
            pc->d_func()->compilationUnit->runtimeFunctions.at(b.compiledScriptIndex),
            prop.object(), context, qmlCtxt));
    } else if (!b.expressionCode.isEmpty()) {
        evalBinding.reset(QQmlBinding::create(&QQmlPropertyPrivate::get(prop)->core,
                                              b.expressionCode, prop.object(), context,
                                              b.url.toString(), b.line));
    }
    outValue = evalBinding ? evalBinding->evaluate() : QVariant();

    if (pc->isExplicit()) {
        outIsBinding = false;
    } else {
        QQmlAnyBinding newBinding;
        if (b.rawBinding && b.rawBinding->isTranslationBinding()) {
            newBinding = QQmlAnyBinding::createTranslationBinding(
                prop, pc->d_func()->compilationUnit, b.rawBinding, prop.object(), context);
        } else if (b.compiledScriptIndex != ~0u &&
                   b.compiledScriptIndex < pc->d_func()->compilationUnit->runtimeFunctions.size()) {
            newBinding = QQmlAnyBinding::createFromFunction(
                prop, pc->d_func()->compilationUnit->runtimeFunctions.at(b.compiledScriptIndex),
                prop.object(), context, qmlCtxt);
        } else if (!b.expressionCode.isEmpty()) {
            newBinding = QQmlAnyBinding::createFromCodeString(prop, b.expressionCode, prop.object(),
                                                              context, b.url.toString(), b.line);
        }
        outBinding = SavedBinding(std::move(newBinding));
        outIsBinding = true;
    }
#else
    Q_UNUSED(pc);
    Q_UNUSED(b);
    Q_UNUSED(prop);
    Q_UNUSED(rawContext);
#endif
}

void restoreOrWriteProperty(QObject* target, const QString& name, const QVariant& value,
                            bool hadBinding, const SavedBinding& binding) {
    if (!target) return;
    QQmlProperty prop(target, name);
    if (prop.isValid()) {
#if defined(QMLTUI_ENABLE_PRIVATE_QT)
        QQmlAnyBinding::removeBindingFrom(prop);
        if (hadBinding && binding) {
            SavedBinding copy = binding;
            copy.installOn(prop);
            return;
        }
#else
        Q_UNUSED(hadBinding);
        Q_UNUSED(binding);
#endif
        prop.write(value);
        return;
    }
    target->setProperty(name.toUtf8().constData(), value);
}

void installOrWriteProperty(QObject* target, const QString& name, const QVariant& value,
                            bool isBinding, const SavedBinding& binding) {
    if (!target) return;
    QQmlProperty prop(target, name);
    if (prop.isValid()) {
#if defined(QMLTUI_ENABLE_PRIVATE_QT)
        if (isBinding && binding) {
            SavedBinding copy = binding;
            copy.installOn(prop);
            return;
        }
#else
        Q_UNUSED(isBinding);
        Q_UNUSED(binding);
#endif
        prop.write(value);
        return;
    }
    target->setProperty(name.toUtf8().constData(), value);
}

void PropertyChangesPrivate::decode() {
#if defined(QMLTUI_ENABLE_PRIVATE_QT)
    if (hasDecoded || !compilationUnit) return;
    hasDecoded = true;
    decodedBindings.clear();

    for (const auto* b : rawBindings) {
        decodeBinding(QString(), compilationUnit, b);
    }
#endif
}

#if defined(QMLTUI_ENABLE_PRIVATE_QT)
void PropertyChangesPrivate::setBindings(const QQmlRefPointer<QV4::ExecutableCompilationUnit>& unit,
                                         const QList<const QV4::CompiledData::Binding*>& bindings) {
    compilationUnit = unit;
    rawBindings = bindings;
    hasCustomBindings = !rawBindings.isEmpty();
    hasDecoded = false;
    decode();
}

void PropertyChangesPrivate::decodeBinding(
    const QString& propertyPrefix, const QQmlRefPointer<QV4::ExecutableCompilationUnit>& unit,
    const QV4::CompiledData::Binding* binding) {
    QString propertyName = propertyPrefix + unit->stringAt(binding->propertyNameIndex);

    switch (binding->type()) {
    case QV4::CompiledData::Binding::Type_GroupProperty:
    case QV4::CompiledData::Binding::Type_AttachedProperty: {
        QString pre = propertyName + QLatin1Char('.');
        const QV4::CompiledData::Object* subObj = unit->objectAt(binding->value.objectIndex);
        const QV4::CompiledData::Binding* subBinding = subObj->bindingTable();
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
        propertyName == QLatin1String("explicit") || propertyName == QLatin1String("properties") ||
        propertyName == QLatin1String("objectName")) {
        return;
    }

    DecodedBinding decoded;
    decoded.propertyName = propertyName;
    decoded.rawBinding = binding;

    if (binding->type() == QV4::CompiledData::Binding::Type_Script ||
        binding->isTranslationBinding()) {
        decoded.isExpression = true;
        if (!binding->isTranslationBinding()) {
            decoded.expressionCode = unit->bindingValueAsString(binding);
            decoded.compiledScriptIndex = binding->value.compiledScriptIndex;
        }
        QQmlData* ddata = QQmlData::get(q);
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

void PropertyChangesParser::verifyList(
    const QQmlRefPointer<QV4::CompiledData::CompilationUnit>& compilationUnit,
    const QV4::CompiledData::Binding* binding, bool& hasPropertiesMap, bool& hasDirectProps) {
    QString name = compilationUnit->stringAt(binding->propertyNameIndex);
    if (name == QLatin1String("properties")) {
        hasPropertiesMap = true;
    } else if (name != QLatin1String("target") && name != QLatin1String("restoreEntryValues") &&
               name != QLatin1String("explicit") && name != QLatin1String("objectName")) {
        hasDirectProps = true;
    }

    switch (binding->type()) {
    case QV4::CompiledData::Binding::Type_Object:
        error(compilationUnit->objectAt(binding->value.objectIndex),
              QCoreApplication::translate(
                  "PropertyChanges",
                  "PropertyChanges does not support creating state-specific objects."));
        break;
    case QV4::CompiledData::Binding::Type_GroupProperty:
    case QV4::CompiledData::Binding::Type_AttachedProperty: {
        const QV4::CompiledData::Object* subObj =
            compilationUnit->objectAt(binding->value.objectIndex);
        const QV4::CompiledData::Binding* subBinding = subObj->bindingTable();
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
    const QQmlRefPointer<QV4::CompiledData::CompilationUnit>& compilationUnit,
    const QList<const QV4::CompiledData::Binding*>& props) {
    bool hasPropertiesMap = false;
    bool hasDirectProps = false;
    for (const auto* b : props) {
        verifyList(compilationUnit, b, hasPropertiesMap, hasDirectProps);
    }

    if (!props.isEmpty()) {
        const QV4::CompiledData::Binding* firstBinding = props.first();
        for (quint32 i = 0; i < compilationUnit->objectCount(); ++i) {
            const QV4::CompiledData::Object* obj = compilationUnit->objectAt(i);
            if (firstBinding >= obj->bindingTable() &&
                firstBinding < obj->bindingTable() + obj->nBindings) {
                for (quint32 j = 0; j < obj->nBindings; ++j) {
                    const QV4::CompiledData::Binding* b = obj->bindingTable() + j;
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
              QCoreApplication::translate(
                  "PropertyChanges",
                  "Cannot mix properties map and direct assignments in PropertyChanges."));
    }
}

void PropertyChangesParser::applyBindings(
    QObject* obj, const QQmlRefPointer<QV4::ExecutableCompilationUnit>& compilationUnit,
    const QList<const QV4::CompiledData::Binding*>& bindings) {
    auto* pc = qobject_cast<PropertyChanges*>(obj);
    if (pc && pc->d_func()) {
        pc->d_func()->setBindings(compilationUnit, bindings);
    }

    QQmlData* data = QQmlData::get(obj);
    if (data && !data->wasDeleted(obj)) data->releaseDeferredData();
}
#endif

} // namespace qmltui
