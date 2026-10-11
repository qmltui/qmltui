#include <QCoreApplication>
#include <QDebug>
#include <QMetaProperty>
#include <QUrl>
#include <QVariant>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlError>

#include <Qamui/Core/primitives/Color.h>

#include <memory>

namespace {

QString errorText(const QQmlComponent &component)
{
    QStringList messages;
    for (const QQmlError &error : component.errors())
        messages.append(error.toString());
    return messages.join(QLatin1Char('\n'));
}

bool verifyQamuiRgba(QQmlEngine &engine)
{
    QQmlComponent component(&engine);
    component.setData(R"qml(
        import QtQml
        import Qamui

        QtObject {
            property rgba accent: Qamui.rgba(0.2, 0.4, 0.6, 1.0)
        }
    )qml", QUrl());

    if (!component.isReady()) {
        qCritical().noquote() << errorText(component);
        return false;
    }

    std::unique_ptr<QObject> object(component.create());
    if (!object)
        return false;

    const int index = object->metaObject()->indexOfProperty("accent");
    if (index < 0)
        return false;

    const QMetaProperty property = object->metaObject()->property(index);
    const QVariant value = property.read(object.get());
    const qamui::Color color = value.value<qamui::Color>();
    qInfo().noquote()
        << "property rgba accent"
        << "declared type:" << property.typeName()
        << "stored type:" << value.metaType().name()
        << "value:" << color.toString();

    return QByteArray(property.typeName()) == "qamui::Color"
        && QByteArray(value.metaType().name()) == "qamui::Color"
        && color == qamui::Color("#336699");
}

bool verifyBuiltinColorIsUnavailable(QQmlEngine &engine)
{
    QQmlComponent component(&engine);
    component.setData(R"qml(
        import QtQml
        import Qamui

        QtObject {
            property color accent: "#336699"
        }
    )qml", QUrl());

    const QString errors = errorText(component);
    qInfo().noquote()
        << "property color accent result:"
        << (errors.isEmpty() ? QStringLiteral("unexpectedly accepted") : errors);

    return component.isError()
        && errors.contains(QStringLiteral("color is not a type"));
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QQmlEngine engine;
    engine.addImportPath(QStringLiteral(QAMUI_QML_IMPORT_PATH));

    const bool qamuiRgbaWorks = verifyQamuiRgba(engine);
    const bool builtinColorIsUnavailable = verifyBuiltinColorIsUnavailable(engine);
    return qamuiRgbaWorks && builtinColorIsUnavailable ? 0 : 1;
}
