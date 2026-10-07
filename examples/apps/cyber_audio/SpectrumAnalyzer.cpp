#include "SpectrumAnalyzer.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace cyber {

SpectrumAnalyzer::SpectrumAnalyzer(int sampleRate)
    : m_sampleRate(sampleRate > 0 ? sampleRate : 48000)
    , m_scratch(kFftSize)
{
    for (int n = 0; n < kFftSize; ++n)
        m_window[n] = float(0.5 * (1.0 - std::cos(2.0 * std::numbers::pi * n / (kFftSize - 1))));
    rebuildBinTable();
}

void SpectrumAnalyzer::setSampleRate(int rate)
{
    if (rate <= 0 || rate == m_sampleRate)
        return;
    m_sampleRate = rate;
    rebuildBinTable();
    designLowPass();
}

void SpectrumAnalyzer::setScopeLowPassHz(double cutoffHz)
{
    const double clamped = cutoffHz <= 0.0 ? 0.0 : std::clamp(cutoffHz, 20.0, m_sampleRate * 0.45);
    if (clamped == m_scopeLowPassHz)
        return;
    m_scopeLowPassHz = clamped;
    designLowPass();
}

void SpectrumAnalyzer::designLowPass()
{
    if (m_scopeLowPassHz <= 0.0)
        return;
    m_lowPassLeft.design(m_scopeLowPassHz, m_sampleRate);
    m_lowPassRight.design(m_scopeLowPassHz, m_sampleRate);
}

void SpectrumAnalyzer::Biquad::design(double cutoffHz, double sampleRate)
{
    // Butterworth (Q = 1/sqrt(2)) low-pass, RBJ cookbook form.
    const double w0 = 2.0 * std::numbers::pi * std::clamp(cutoffHz, 1.0, sampleRate * 0.45) / sampleRate;
    const double cosw0 = std::cos(w0);
    const double alpha = std::sin(w0) / (2.0 * std::numbers::sqrt2);
    const double a0 = 1.0 + alpha;
    b0 = float((1.0 - cosw0) / 2.0 / a0);
    b1 = float((1.0 - cosw0) / a0);
    b2 = b0;
    a1 = float(-2.0 * cosw0 / a0);
    a2 = float((1.0 - alpha) / a0);
    reset();
}

void SpectrumAnalyzer::rebuildBinTable()
{
    const double binHz = double(m_sampleRate) / kFftSize;
    for (int b = 0; b <= kBandCount; ++b) {
        const int bin = int(std::lround(bandLowerEdgeHz(b) / binHz));
        m_binEdges[b] = std::clamp(bin, 1, kFftSize / 2 - 1);
    }
    // Every band owns at least one bin: at 48 kHz the lowest bands are narrower than
    // the 23.4 Hz bin width.
    for (int b = 1; b <= kBandCount; ++b)
        m_binEdges[b] = std::min(kFftSize / 2, std::max(m_binEdges[b], m_binEdges[b - 1] + 1));
}

void SpectrumAnalyzer::pushInterleaved(const float* samples, int frameCount, int channels)
{
    if (!samples || frameCount <= 0 || channels <= 0)
        return;
    for (int f = 0; f < frameCount; ++f) {
        const float* frame = samples + qsizetype(f) * channels;
        const float l = frame[0];
        const float r = channels > 1 ? frame[1] : l;
        m_mono[m_writePos] = 0.5f * (l + r);
        float scopeL = l;
        float scopeR = r;
        if (m_scopeLowPassHz > 0.0) {
            scopeL = m_lowPassLeft.process(l);
            scopeR = channels > 1 ? m_lowPassRight.process(r) : scopeL;
        }
        m_left[m_writePos] = scopeL;
        m_right[m_writePos] = scopeR;
        m_scopeMono[m_writePos] = 0.5f * (scopeL + scopeR);
        m_writePos = (m_writePos + 1) % kFftSize;
        m_sumSqLeft += double(l) * l;
        m_sumSqRight += double(r) * r;
        m_sumLeftRight += double(l) * r;
    }
    m_rmsCount += frameCount;
    m_dirty = true;
}

void SpectrumAnalyzer::clear()
{
    m_mono.fill(0.f);
    m_left.fill(0.f);
    m_right.fill(0.f);
    m_scopeMono.fill(0.f);
    m_lowPassLeft.reset();
    m_lowPassRight.reset();
    m_writePos = 0;
    m_sumSqLeft = m_sumSqRight = m_sumLeftRight = 0.0;
    m_rmsCount = 0;
    m_dirty = false;
}

Analysis SpectrumAnalyzer::analyze()
{
    Analysis out;

    // 1. Unroll the ring buffer oldest to newest and apply the Hann window.
    for (int i = 0; i < kFftSize; ++i) {
        const int idx = (m_writePos + i) % kFftSize;
        m_scratch[i] = { m_mono[idx] * m_window[i], 0.f };
    }
    fft(m_scratch);

    // 2. Band levels. |X| * 2 / N gives the sine amplitude for a rectangular window;
    //    the Hann coherent gain of 0.5 doubles that, hence 4 / N. The band takes its
    //    loudest bin so a pure tone reads at its true amplitude in any band width.
    const double norm = 4.0 / kFftSize;
    for (int b = 0; b < kBandCount; ++b) {
        double amp = 0.0;
        for (int k = m_binEdges[b]; k < m_binEdges[b + 1]; ++k)
            amp = std::max(amp, std::abs(m_scratch[k]) * norm);
        const double db = 20.0 * std::log10(std::max(amp, 1e-6));
        out.bands[b] = float(std::clamp((db - kFloorDb) / -kFloorDb, 0.0, 1.0));
    }

    // 3. RMS of everything pushed since the previous analysis.
    if (m_rmsCount > 0) {
        out.rmsLeft = float(std::sqrt(m_sumSqLeft / double(m_rmsCount)));
        out.rmsRight = float(std::sqrt(m_sumSqRight / double(m_rmsCount)));
        const double denom = std::sqrt(m_sumSqLeft * m_sumSqRight);
        if (denom > 1e-12)
            out.correlation = float(std::clamp(m_sumLeftRight / denom, -1.0, 1.0));
    }
    m_sumSqLeft = m_sumSqRight = m_sumLeftRight = 0.0;
    m_rmsCount = 0;

    // 4. Scope: a window of kScopePoints * kScopeStride frames ending at the newest
    //    sample, decimated, from the scope path (low-pass filtered when enabled). Like a
    //    hardware scope, the window start is pulled back to the most recent rising zero
    //    crossing of that signal so periodic content holds still between frames.
    const int span = kScopePoints * kScopeStride;
    auto ringAt = [this](int offsetFromNewest) {          // 0 = newest frame
        return (m_writePos - 1 - offsetFromNewest + kFftSize * 2) % kFftSize;
    };
    int start = span - 1;                                  // offset (from newest) of the window's first frame
    for (int back = 0; back < kScopeTriggerSearch; ++back) {
        const int candidate = start + back;                // older start positions
        const float now = m_scopeMono[ringAt(candidate)];
        const float before = m_scopeMono[ringAt(candidate + 1)];
        if (before < 0.f && now >= 0.f) {
            start = candidate;
            out.scopeTriggered = true;
            break;
        }
    }
    for (int i = 0; i < kScopePoints; ++i) {
        const int idx = ringAt(start - i * kScopeStride);
        out.scopeLeft[i] = std::clamp(m_left[idx], -1.f, 1.f);
        out.scopeRight[i] = std::clamp(m_right[idx], -1.f, 1.f);
    }

    m_dirty = false;
    return out;
}

double SpectrumAnalyzer::bandLowerEdgeHz(int band)
{
    const int b = std::clamp(band, 0, kBandCount);
    return kBandLowHz * std::pow(kBandHighHz / kBandLowHz, double(b) / kBandCount);
}

double SpectrumAnalyzer::bandCenterHz(int band)
{
    return std::sqrt(bandLowerEdgeHz(band) * bandLowerEdgeHz(band + 1));
}

int SpectrumAnalyzer::bandIndexForFrequency(double hz)
{
    if (hz < kBandLowHz || hz >= kBandHighHz)
        return -1;
    const double pos = kBandCount * std::log(hz / kBandLowHz) / std::log(kBandHighHz / kBandLowHz);
    return std::clamp(int(std::floor(pos)), 0, kBandCount - 1);
}

QString SpectrumAnalyzer::formatFrequencyLabel(double hz)
{
    if (hz < 1000.0)
        return QString::number(int(std::lround(hz)));
    if (hz < 10000.0) {
        const int tenths = int(std::lround(hz / 100.0));       // 1250 -> 13 -> "1k3"
        const int k = tenths / 10;
        const int d = tenths % 10;
        return d ? QStringLiteral("%1k%2").arg(k).arg(d) : QStringLiteral("%1k").arg(k);
    }
    return QStringLiteral("%1k").arg(int(std::lround(hz / 1000.0)));
}

QStringList SpectrumAnalyzer::bandLabels()
{
    QStringList labels;
    labels.reserve(kBandCount);
    for (int b = 0; b < kBandCount; ++b)
        labels << formatFrequencyLabel(bandCenterHz(b));
    return labels;
}

void SpectrumAnalyzer::fft(std::vector<std::complex<float>>& a)
{
    const int n = int(a.size());
    if (n <= 1 || (n & (n - 1)) != 0)
        return;

    // Bit-reversal permutation.
    for (int i = 1, j = 0; i < n; ++i) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j)
            std::swap(a[i], a[j]);
    }

    // Butterflies.
    for (int len = 2; len <= n; len <<= 1) {
        const double ang = -2.0 * std::numbers::pi / len;
        const std::complex<float> wlen(float(std::cos(ang)), float(std::sin(ang)));
        for (int i = 0; i < n; i += len) {
            std::complex<float> w(1.f, 0.f);
            for (int k = 0; k < len / 2; ++k) {
                const std::complex<float> u = a[i + k];
                const std::complex<float> v = a[i + k + len / 2] * w;
                a[i + k] = u + v;
                a[i + k + len / 2] = u - v;
                w *= wlen;
            }
        }
    }
}

} // namespace cyber
