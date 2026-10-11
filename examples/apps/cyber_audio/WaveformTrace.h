#pragma once

#include <Qamui/Core/items/Canvas.h>

#include <QList>
#include <QQmlEngine>

// A Canvas shape that plots a sample array as a polyline across the full canvas
// width. Drawing happens in C++ through the same CanvasPainter the built-in shapes
// use, so QML only binds the data and never runs a per-dot loop.
//
//   Canvas {
//       renderMode: Canvas.RenderMode.Braille
//       WaveformTrace { samples: engine.waveformLeft; color: "#ffb700" }
//   }
class WaveformTrace : public qamui::CanvasShape
{
    Q_OBJECT
    QML_ELEMENT
    // Samples in -1..1, stretched across the canvas with linear interpolation.
    Q_PROPERTY(QList<double> samples READ samples WRITE setSamples NOTIFY samplesChanged)
    // Fraction of the half height used by a full-scale sample (default 0.9).
    Q_PROPERTY(qreal amplitude READ amplitude WRITE setAmplitude NOTIFY amplitudeChanged)
    // Traces whose every sample is below this magnitude are not drawn at all, so a
    // paused and settled scope shows an empty canvas rather than a flat line.
    Q_PROPERTY(qreal silenceThreshold READ silenceThreshold WRITE setSilenceThreshold NOTIFY silenceThresholdChanged)

public:
    explicit WaveformTrace(QObject* parent = nullptr);

    QList<double> samples() const { return m_samples; }
    void setSamples(const QList<double>& samples);
    qreal amplitude() const { return m_amplitude; }
    void setAmplitude(qreal amplitude);
    qreal silenceThreshold() const { return m_silenceThreshold; }
    void setSilenceThreshold(qreal threshold);

    void draw(qamui::CanvasPainter& painter) const override;

signals:
    void samplesChanged();
    void amplitudeChanged();
    void silenceThresholdChanged();

private:
    void requestRepaint();

    QList<double> m_samples;
    qreal m_amplitude = 0.9;
    qreal m_silenceThreshold = 0.004;
};
