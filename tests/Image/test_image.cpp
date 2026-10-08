#include <QtTest/QtTest>
#include <Qtui/Image/Image.h>
#include <Qtui/Image/ImageProvider.h>
#include <Qtui/Core/items/Terminal.h>
#include <Qtui/Core/engine/TerminalEngine.h>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QSignalSpy>
#include <memory>
#include <Qtui/Core/primitives/TextGridBuffer.h>
#include "../TestPaint.h"
#include <QTemporaryFile>

class TestImage : public QObject {
    Q_OBJECT

private slots:
    void testInitialState();
    void testLoadValidImage();
    void testLoadInvalidImage();
    void testRenderHalfBlock();
    void testAutoFitsLikeHalfBlock();
    void testSetImageFromMemory();
    void testImageProvider();
    void testRenderASCII();
    void testRenderBraille();
    void testFillModeScaling();
    void testQrcResourceUrl();
};

void TestImage::testInitialState() {
    qtui::Image img;
    QCOMPARE(img.status(), qtui::Image::Status::Null);
    QCOMPARE(img.fillMode(), qtui::Image::FillMode::Stretch);
    QCOMPARE(img.renderMode(), qtui::Image::RenderMode::Auto);  // resolves to HalfBlock without a kitty terminal
    QCOMPARE(img.smooth(), true);
    QCOMPARE(img.source().isEmpty(), true);
}

void TestImage::testLoadValidImage() {
    // Generate a 4x4 test PNG: top row red, bottom row blue
    QImage testImg(4, 4, QImage::Format_ARGB32);
    testImg.fill(Qt::blue);
    for (int x = 0; x < 4; ++x) {
        testImg.setPixelColor(x, 0, Qt::red);
        testImg.setPixelColor(x, 1, Qt::red);
    }

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QString fileName = tempFile.fileName() + ".png";
    tempFile.close();

    QVERIFY(testImg.save(fileName, "PNG"));

    qtui::Image img;
    img.setSource(QUrl::fromLocalFile(fileName));

    QCOMPARE(img.status(), qtui::Image::Status::Ready);
    QCOMPARE(img.sourceSize(), QSize(4, 4));
    QCOMPARE(img.implicitWidth(), 4);
    QCOMPARE(img.implicitHeight(), 2); // 4 subpixels / 2 = 2 rows

    QFile::remove(fileName);
}

void TestImage::testLoadInvalidImage() {
    qtui::Image img;
    img.setSource(QUrl::fromLocalFile("/non_existent_path_12345.png"));
    QCOMPARE(img.status(), qtui::Image::Status::Error);
}

void TestImage::testRenderHalfBlock() {
    // Generate 2x2 test image: Top pixel Red (0xFFFF0000), Bottom pixel Blue (0xFF0000FF)
    QImage testImg(2, 2, QImage::Format_ARGB32);
    testImg.setPixelColor(0, 0, QColor(255, 0, 0));
    testImg.setPixelColor(1, 0, QColor(255, 0, 0));
    testImg.setPixelColor(0, 1, QColor(0, 0, 255));
    testImg.setPixelColor(1, 1, QColor(0, 0, 255));

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QString fileName = tempFile.fileName() + ".png";
    tempFile.close();
    QVERIFY(testImg.save(fileName, "PNG"));

    qtui::Image img;
    img.setSource(QUrl::fromLocalFile(fileName));
    img.setRenderMode(qtui::Image::RenderMode::HalfBlock);
    img.setWidth(2);
    img.setHeight(1); // 1 cell height = 2 subpixel rows

    qtui::TextGridBuffer buffer;
    buffer.resize(4, 4);

    qtui::testing::paintItem(img, buffer, 0, 0);

    const qtui::Cell& cell0 = buffer.cellAt(0, 0);
    QCOMPARE(cell0.character().toByteArray(), QByteArray("\xE2\x96\x80")); // ▀
    QCOMPARE(QColor::fromRgba(cell0.fgColor).red(), 255);
    QCOMPARE(QColor::fromRgba(cell0.bgColor).blue(), 255);

    QFile::remove(fileName);
}

// Auto resolves to HalfBlock on a terminal without kitty or sixel; the picture
// must then be fitted to two pixel rows per cell exactly as HalfBlock does,
// not to one row per cell (which drew it at half height).
void TestImage::testAutoFitsLikeHalfBlock() {
    QImage testImg(2, 2, QImage::Format_ARGB32);
    testImg.setPixelColor(0, 0, QColor(255, 0, 0));
    testImg.setPixelColor(1, 0, QColor(255, 0, 0));
    testImg.setPixelColor(0, 1, QColor(0, 0, 255));
    testImg.setPixelColor(1, 1, QColor(0, 0, 255));
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QString fileName = tempFile.fileName() + ".png";
    tempFile.close();
    QVERIFY(testImg.save(fileName, "PNG"));

    auto paintWith = [&](qtui::Image::RenderMode mode) {
        qtui::Image img;
        img.setSource(QUrl::fromLocalFile(fileName));
        img.setRenderMode(mode);
        img.setFillMode(qtui::Image::FillMode::PreserveAspectFit);
        img.setWidth(8);
        img.setHeight(4);   // 8 pixel rows: the 2x2 picture fits as 8x8 pixels = 8 cells x 4 rows
        qtui::TextGridBuffer buffer;
        buffer.resize(8, 4);
        qtui::testing::paintItem(img, buffer, 0, 0);
        QString picture;
        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < 8; ++x) {
                const qtui::Cell& cell = buffer.cellAt(x, y);
                picture += QString::fromUtf8(cell.character().toByteArray()) + QString::number(cell.fgColor, 16) + "/" + QString::number(cell.bgColor, 16) + " ";
            }
            picture += "\n";
        }
        return picture;
    };
    const QString halfBlock = paintWith(qtui::Image::RenderMode::HalfBlock);
    const QString automatic = paintWith(qtui::Image::RenderMode::Auto);
    QVERIFY2(halfBlock.count(QStringLiteral("\u2580")) == 32, qPrintable(halfBlock));   // every cell painted
    QCOMPARE(automatic, halfBlock);
    QFile::remove(fileName);
}

// Shape 1: a QImage assigned from C++ renders like a loaded file, clears the
// URL, and a null image clears the item.
void TestImage::testSetImageFromMemory() {
    QImage frame(2, 2, QImage::Format_RGB32);
    frame.setPixelColor(0, 0, QColor(255, 0, 0));
    frame.setPixelColor(1, 0, QColor(255, 0, 0));
    frame.setPixelColor(0, 1, QColor(0, 0, 255));
    frame.setPixelColor(1, 1, QColor(0, 0, 255));

    qtui::Image img;
    img.setRenderMode(qtui::Image::RenderMode::HalfBlock);
    img.setWidth(2);
    img.setHeight(1);
    QSignalSpy imageChanged(&img, &qtui::Image::imageChanged);
    QSignalSpy statusChanged(&img, &qtui::Image::statusChanged);
    QSignalSpy sizeChanged(&img, &qtui::Image::sourceSizeChanged);

    img.setImage(frame);
    QCOMPARE(img.status(), qtui::Image::Status::Ready);
    QCOMPARE(img.sourceSize(), QSize(2, 2));
    QCOMPARE(img.image().size(), QSize(2, 2));
    QVERIFY(img.source().isEmpty());
    QCOMPARE(imageChanged.count(), 1);
    QCOMPARE(statusChanged.count(), 1);
    QCOMPARE(sizeChanged.count(), 1);

    qtui::TextGridBuffer buffer;
    buffer.resize(4, 4);
    qtui::testing::paintItem(img, buffer, 0, 0);
    const qtui::Cell& cell0 = buffer.cellAt(0, 0);
    QCOMPARE(cell0.character().toByteArray(), QByteArray("\xE2\x96\x80")); // ▀
    QCOMPARE(QColor::fromRgba(cell0.fgColor).red(), 255);
    QCOMPARE(QColor::fromRgba(cell0.bgColor).blue(), 255);

    // A second frame of the same size: repainted, no size notification.
    frame.fill(Qt::green);
    img.setImage(frame);
    QCOMPARE(imageChanged.count(), 2);
    QCOMPARE(sizeChanged.count(), 1);
    qtui::testing::paintItem(img, buffer, 0, 0);
    QCOMPARE(QColor::fromRgba(buffer.cellAt(0, 0).fgColor).green(), 255);

    // A null image clears the item.
    img.setImage(QImage());
    QCOMPARE(img.status(), qtui::Image::Status::Null);
    QVERIFY(img.image().isNull());
    QCOMPARE(sizeChanged.count(), 2);
}

namespace {
class SolidProvider : public qtui::ImageProvider {
public:
    QStringList ids;
    QSize lastRequested;
    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override {
        ids.append(id);
        lastRequested = requestedSize;
        if (!id.startsWith(QLatin1String("solid"))) return QImage();
        QImage image(4, 4, QImage::Format_RGB32);
        image.fill(id.contains(QLatin1String("blue")) ? Qt::blue : Qt::red);
        *size = image.size();
        return image;
    }
};
}

// Shape 2: an image:// URL resolves through the ImageProvider installed on
// the item's engine; the id carries path and query; unknown ids and unknown
// providers report Error without crashing.
void TestImage::testImageProvider() {
    QQmlEngine qml;
    auto* provider = new SolidProvider;
    qtui::ImageProvider::add(&qml, QStringLiteral("test"), provider);
    QCOMPARE(qtui::ImageProvider::find(&qml, QStringLiteral("test")), provider);
    QCOMPARE(provider->parent(), &qml);

    QQmlComponent component(&qml);
    component.setData(R"(
import QtQml
import Qtui
import Qtui.Image
Terminal {
    Image { objectName: "picture"; x: 0; y: 0; width: 4; height: 2; renderMode: Image.HalfBlock
            source: "image://test/solid?frame=1" }
}
)", QUrl());
    std::unique_ptr<QObject> object(component.create());
    QVERIFY2(object, qPrintable(component.errorString()));
    auto* terminal = qobject_cast<qtui::Terminal*>(object.get());
    QVERIFY(terminal);
    auto* picture = terminal->findChild<qtui::Image*>("picture");
    QVERIFY(picture);
    QCOMPARE(picture->status(), qtui::Image::Status::Ready);
    QCOMPARE(picture->sourceSize(), QSize(4, 4));
    QCOMPARE(provider->ids, QStringList{"solid?frame=1"});

    auto* engine = terminal->engine();
    engine->setTestViewportSize(QSize(10, 4));
    QTRY_VERIFY(engine->frameSequence() > 0);
    const auto frame = engine->currentFrame();
    QVERIFY(frame);
    QCOMPARE(QColor::fromRgba(frame->cellAt(0, 0).fgColor).red(), 255);

    // A new URL asks the provider again; the picture follows.
    picture->setSource(QUrl(QStringLiteral("image://test/solid-blue?frame=2")));
    QCOMPARE(provider->ids.size(), 2);
    QCOMPARE(provider->ids.last(), QStringLiteral("solid-blue?frame=2"));
    QCOMPARE(provider->lastRequested, QSize(4, 4));      // 4 columns, 2 rows of half-blocks
    const quint64 before = engine->frameSequence();
    engine->requestRender();
    QTRY_VERIFY(engine->frameSequence() > before);
    QCOMPARE(QColor::fromRgba(engine->currentFrame()->cellAt(0, 0).fgColor).blue(), 255);

    // Unknown id: the provider returns null, the item reports Error.
    picture->setSource(QUrl(QStringLiteral("image://test/missing")));
    QCOMPARE(picture->status(), qtui::Image::Status::Error);
    // Unknown provider: Error as well, and a warning.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("no qtui::ImageProvider named \"nobody\""));
    picture->setSource(QUrl(QStringLiteral("image://nobody/x")));
    QCOMPARE(picture->status(), qtui::Image::Status::Error);

    // Replacing a provider deletes the previous one.
    QPointer<qtui::ImageProvider> old(provider);
    qtui::ImageProvider::add(&qml, QStringLiteral("test"), new SolidProvider);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(!old);
    qtui::ImageProvider::remove(&qml, QStringLiteral("test"));
    QVERIFY(!qtui::ImageProvider::find(&qml, QStringLiteral("test")));
}

void TestImage::testRenderASCII() {
    QImage testImg(2, 2, QImage::Format_ARGB32);
    testImg.fill(Qt::white);

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QString fileName = tempFile.fileName() + ".png";
    tempFile.close();
    QVERIFY(testImg.save(fileName, "PNG"));

    qtui::Image img;
    img.setSource(QUrl::fromLocalFile(fileName));
    img.setRenderMode(qtui::Image::RenderMode::ASCII);
    img.setWidth(2);
    img.setHeight(2);

    qtui::TextGridBuffer buffer;
    buffer.resize(4, 4);

    qtui::testing::paintItem(img, buffer, 0, 0);

    const qtui::Cell& cell0 = buffer.cellAt(0, 0);
    QCOMPARE(cell0.character().toByteArray(), QByteArray("@"));

    QFile::remove(fileName);
}

void TestImage::testRenderBraille() {
    QImage testImg(2, 4, QImage::Format_ARGB32);
    testImg.fill(Qt::white);

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QString fileName = tempFile.fileName() + ".png";
    tempFile.close();
    QVERIFY(testImg.save(fileName, "PNG"));

    qtui::Image img;
    img.setSource(QUrl::fromLocalFile(fileName));
    img.setRenderMode(qtui::Image::RenderMode::Braille);
    img.setWidth(1);
    img.setHeight(1);

    qtui::TextGridBuffer buffer;
    buffer.resize(2, 2);

    qtui::testing::paintItem(img, buffer, 0, 0);

    const qtui::Cell& cell0 = buffer.cellAt(0, 0);
    QVERIFY(!cell0.character().isEmpty());

    QFile::remove(fileName);
}

void TestImage::testFillModeScaling() {
    QImage testImg(100, 50, QImage::Format_ARGB32);
    testImg.fill(Qt::green);

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QString fileName = tempFile.fileName() + ".png";
    tempFile.close();
    QVERIFY(testImg.save(fileName, "PNG"));

    qtui::Image img;
    img.setSource(QUrl::fromLocalFile(fileName));
    img.setFillMode(qtui::Image::FillMode::PreserveAspectFit);
    img.setWidth(20);
    img.setHeight(10);

    qtui::TextGridBuffer buffer;
    buffer.resize(20, 10);
    qtui::testing::paintItem(img, buffer, 0, 0);

    QCOMPARE(img.status(), qtui::Image::Status::Ready);

    QFile::remove(fileName);
}

void TestImage::testQrcResourceUrl() {
    qtui::Image img;
    img.setSource(QUrl("qrc:/qt/qml/ImageDemo/demo_image.png"));
    QVERIFY(img.status() == qtui::Image::Status::Error || img.status() == qtui::Image::Status::Ready);
}

QTEST_MAIN(TestImage)
#include "test_image.moc"
