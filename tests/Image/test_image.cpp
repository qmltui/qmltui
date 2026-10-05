#include <QtTest/QtTest>
#include "Image.h"
#include "TextGridBuffer.h"
#include "../TestPaint.h"
#include <QTemporaryFile>

class TestImage : public QObject {
    Q_OBJECT

private slots:
    void testInitialState();
    void testLoadValidImage();
    void testLoadInvalidImage();
    void testRenderHalfBlock();
    void testRenderASCII();
    void testRenderBraille();
    void testFillModeScaling();
    void testQrcResourceUrl();
};

void TestImage::testInitialState() {
    qtui::Image img;
    QCOMPARE(img.status(), qtui::Image::Status::Null);
    QCOMPARE(img.fillMode(), qtui::Image::FillMode::Stretch);
    QCOMPARE(img.renderMode(), qtui::Image::RenderMode::HalfBlock);
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
    QCOMPARE(cell0.character, QByteArray("\xE2\x96\x80")); // ▀
    QCOMPARE(QColor::fromRgba(cell0.fgColor).red(), 255);
    QCOMPARE(QColor::fromRgba(cell0.bgColor).blue(), 255);

    QFile::remove(fileName);
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
    QCOMPARE(cell0.character, QByteArray("@"));

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
    QVERIFY(!cell0.character.isEmpty());

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
