#include <Qtui/Image/ImageProvider.h>

#include <QQmlEngine>

namespace qtui {

namespace {
QString registryName(const QString& id) { return QStringLiteral("qtui.imageProvider/") + id; }
}

ImageProvider::ImageProvider(QObject* parent) : QObject(parent) {}
ImageProvider::~ImageProvider() = default;

void ImageProvider::add(QQmlEngine* engine, const QString& id, ImageProvider* provider) {
    if (!engine || !provider) return;
    remove(engine, id);
    // The registry is the engine's child list: no static state, and the
    // provider dies with the engine, as with QQmlEngine::addImageProvider().
    provider->setObjectName(registryName(id));
    provider->setParent(engine);
}

ImageProvider* ImageProvider::find(const QQmlEngine* engine, const QString& id) {
    if (!engine) return nullptr;
    return engine->findChild<ImageProvider*>(registryName(id), Qt::FindDirectChildrenOnly);
}

void ImageProvider::remove(QQmlEngine* engine, const QString& id) {
    if (ImageProvider* previous = find(engine, id))
        delete previous;
}

} // namespace qtui
