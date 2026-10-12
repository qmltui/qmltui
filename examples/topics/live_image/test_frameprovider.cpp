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

    void cancelledResponsesMayBeDeletedBeforeRenderingFinishes() {
        RadarProvider provider;
        for (int tick = 0; tick < 16; ++tick) {
            auto* response = provider.requestImageResponse(
                QStringLiteral("radar?tick=%1").arg(tick), QSize(256, 128));
            QVERIFY(response);
            response->cancel();
            delete response;
        }
        // Requests render in order. Completing this one proves the worker has
        // also processed all requests whose response objects were deleted.
        auto* response = provider.requestImageResponse(QStringLiteral("radar?tick=16"), QSize(80, 40));
        QVERIFY(response);
        QSignalSpy finished(response, &qmltui::ImageResponse::finished);
        QVERIFY(finished.wait(1000));
        QVERIFY(!response->image().isNull());
        delete response;
    }

    void requestImageResponseProducesRadar() {
        RadarProvider provider;
        QVERIFY(!provider.requestImageResponse(QStringLiteral("unknown"), QSize(64, 64)));

        auto* response = provider.requestImageResponse(QStringLiteral("radar?tick=2"), QSize(80, 40));
        QVERIFY(response);
        QSignalSpy finished(response, &qmltui::ImageResponse::finished);
        QVERIFY(finished.wait(1000));
        QCOMPARE(response->image().size(), QSize(80, 40));
        QVERIFY(!response->image().isNull());
        delete response;
    }
};

QTEST_MAIN(TestFrameProvider)
#include "test_frameprovider.moc"
