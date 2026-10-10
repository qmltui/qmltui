#include "frameprovider.h"

#include <QSignalSpy>
#include <QTest>

class TestFrameProvider : public QObject {
    Q_OBJECT

private slots:
    void paintRadarSizesAndAnimates() {
        const QSize size(120, 60);
        const QImage frame0 = RadarProvider::paintRadar(0, size);
        const QImage frame1 = RadarProvider::paintRadar(5, size);

        QCOMPARE(frame0.size(), size);
        QCOMPARE(frame1.size(), size);
        QVERIFY(!frame0.hasAlphaChannel());
        QVERIFY(frame0 != frame1);
    }

    void requestImageResponseProducesRadar() {
        RadarProvider provider;
        QVERIFY(!provider.requestImageResponse(QStringLiteral("unknown"), QSize(64, 64)));

        auto* response = provider.requestImageResponse(QStringLiteral("radar?tick=2"), QSize(80, 40));
        QVERIFY(response);
        QSignalSpy finished(response, &qtui::ImageResponse::finished);
        QVERIFY(finished.wait(1000));
        QCOMPARE(response->image().size(), QSize(80, 40));
        QVERIFY(!response->image().isNull());
        delete response;
    }
};

QTEST_MAIN(TestFrameProvider)
#include "test_frameprovider.moc"
