#pragma once

#include "SpectrumAnalyzer.h"

#include <array>

namespace cyber {

// Deterministic procedural synthwave source: 138 BPM kick / snare / hi-hat / arp.
// Produces per-band targets (already in the 0..1 level domain) and a synthetic
// stereo waveform for the oscilloscope. One step() equals one 33 ms engine tick.
class DemoSynth {
public:
    struct Frame {
        std::array<float, kBandCount> bandTargets{};
        float beatPulse = 0.f;
        std::array<float, kScopePoints> scopeLeft{};
        std::array<float, kScopePoints> scopeRight{};
    };

    void reset();
    Frame step();

private:
    int m_tick = 0;
    int m_beatIndex = -1;          // -1 so the very first tick lands on a beat
    float m_beatPulse = 0.f;
    double m_synthTime = 0.0;
    double m_wavePhase = 0.0;
};

} // namespace cyber
