#pragma once

#include <QString>
#include <QStringList>

#include <array>
#include <complex>
#include <vector>

namespace cyber {

constexpr int kBandCount = 24;
constexpr int kFftSize = 2048;       // power of two
constexpr int kScopePoints = 256;
constexpr int kScopeStride = 4;      // scope covers kScopePoints * kScopeStride frames (~21 ms at 48 kHz)
constexpr int kScopeTriggerSearch = kFftSize - kScopePoints * kScopeStride;  // frames searched for a trigger
constexpr double kBandLowHz = 40.0;
constexpr double kBandHighHz = 18000.0;
constexpr double kFloorDb = -60.0;   // maps to level 0.0; 0 dBFS maps to 1.0

struct Analysis {
    std::array<float, kBandCount> bands{};          // 0..1, dB mapped
    float rmsLeft = 0.f;                            // raw RMS of the frames since the last analyze()
    float rmsRight = 0.f;
    float correlation = 1.f;                        // normalized L/R correlation, 1 = mono, 0 = unrelated
    std::array<float, kScopePoints> scopeLeft{};    // trigger-aligned window (optionally low-passed), -1..1
    std::array<float, kScopePoints> scopeRight{};
    bool scopeTriggered = false;                    // a rising zero crossing anchored the window
};

// Pure C++ spectrum analyzer. No Qt Multimedia dependency so it can be unit tested
// and used by the demo source alike.
class SpectrumAnalyzer {
public:
    explicit SpectrumAnalyzer(int sampleRate = 48000);

    void setSampleRate(int rate);
    int sampleRate() const { return m_sampleRate; }

    // Low-pass applied to the scope display path only (the FFT stays unfiltered).
    // 0 disables the filter and shows the raw samples.
    void setScopeLowPassHz(double cutoffHz);
    double scopeLowPassHz() const { return m_scopeLowPassHz; }

    // Interleaved float frames. Mono is duplicated to both channels; more than two
    // channels use the first two.
    void pushInterleaved(const float* samples, int frameCount, int channels);
    bool hasNewData() const { return m_dirty; }
    void clear();

    // Windows and transforms the newest kFftSize mono frames.
    Analysis analyze();

    // Log-spaced band geometry, shared by the QML labels and the unit tests.
    static double bandLowerEdgeHz(int band);           // band 0..kBandCount
    static double bandCenterHz(int band);
    static int bandIndexForFrequency(double hz);       // -1 outside [kBandLowHz, kBandHighHz)
    static QString formatFrequencyLabel(double hz);    // at most 3 cells: "45", "1k3", "12k"
    static QStringList bandLabels();

    // In-place iterative radix-2 FFT; size must be a power of two.
    static void fft(std::vector<std::complex<float>>& data);

private:
    // Second-order Butterworth low-pass (Direct Form I).
    struct Biquad {
        float b0 = 1.f, b1 = 0.f, b2 = 0.f, a1 = 0.f, a2 = 0.f;
        float x1 = 0.f, x2 = 0.f, y1 = 0.f, y2 = 0.f;
        void design(double cutoffHz, double sampleRate);
        void reset() { x1 = x2 = y1 = y2 = 0.f; }
        float process(float x)
        {
            const float y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = x;
            y2 = y1; y1 = y;
            return y;
        }
    };

    void rebuildBinTable();
    void designLowPass();

    int m_sampleRate;
    double m_scopeLowPassHz = 0.0;
    std::array<float, kFftSize> m_mono{};          // unfiltered, feeds the FFT
    std::array<float, kFftSize> m_left{};          // scope display path (filtered when enabled)
    std::array<float, kFftSize> m_right{};
    std::array<float, kFftSize> m_scopeMono{};     // scope-path mono, feeds the trigger
    Biquad m_lowPassLeft;
    Biquad m_lowPassRight;
    int m_writePos = 0;
    bool m_dirty = false;
    double m_sumSqLeft = 0.0;
    double m_sumSqRight = 0.0;
    double m_sumLeftRight = 0.0;
    long long m_rmsCount = 0;
    std::array<float, kFftSize> m_window{};
    std::array<int, kBandCount + 1> m_binEdges{};
    std::vector<std::complex<float>> m_scratch;
};

} // namespace cyber
