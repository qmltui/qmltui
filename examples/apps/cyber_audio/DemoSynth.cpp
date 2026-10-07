#include "DemoSynth.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace cyber {

void DemoSynth::reset()
{
    *this = DemoSynth();
}

DemoSynth::Frame DemoSynth::step()
{
    constexpr double kTwoPi = 2.0 * std::numbers::pi;

    ++m_tick;
    m_synthTime += 0.06;
    m_wavePhase = std::fmod(m_wavePhase + 0.18, kTwoPi);

    // 138 BPM: one beat every ~434 ms, i.e. 13 ticks at 33 ms.
    const int beatStep = (m_tick / 13) % 4;
    if (beatStep != m_beatIndex) {
        m_beatIndex = beatStep;
        m_beatPulse = 1.f;
    } else {
        m_beatPulse = std::max(0.f, m_beatPulse - 0.12f);
    }

    const bool isKick = (m_beatIndex == 0 || m_beatIndex == 2);
    const bool isSnare = (m_beatIndex == 1 || m_beatIndex == 3);

    Frame frame;
    frame.beatPulse = m_beatPulse;

    for (int i = 0; i < kBandCount; ++i) {
        double base;
        if (i < 6) {
            // Sub-bass & bass: kick drum plus bass line
            const double kickEnergy = isKick ? (0.75 - i * 0.08) * m_beatPulse : 0.2;
            const double bassLine = std::abs(std::sin(m_synthTime * 3.5 + i * 0.4)) * 0.45;
            base = kickEnergy + bassLine;
        } else if (i < 14) {
            // Low-mid & mid: snare plus melodic arp
            const double snareEnergy = isSnare ? 0.65 * m_beatPulse : 0.15;
            const double arpPhase = std::sin(m_synthTime * 6.0 + i * 0.8);
            const double synthArp = arpPhase > 0.3 ? arpPhase * 0.55 : 0.1;
            base = snareEnergy + synthArp;
        } else {
            // High-mid & treble: hi-hats, noise and air
            const double hiHat = std::abs(std::cos(m_synthTime * 12.0 + i)) * 0.4;
            const double air = std::abs(std::sin(m_synthTime * 2.0 + i * 0.5)) * 0.25;
            base = hiHat + air;
        }
        frame.bandTargets[i] = float(std::clamp(base, 0.03, 1.0));
    }

    // Synthetic stereo waveform: the right channel is shifted by a quarter turn so the
    // Lissajous view draws a rotating loop instead of a diagonal line.
    for (int i = 0; i < kScopePoints; ++i) {
        const double x = (double(i) / kScopePoints) * 4.0 * std::numbers::pi;
        const double l = std::sin(x + m_wavePhase) + 0.4 * std::sin(x * 2.2 - m_wavePhase * 1.4);
        const double r = std::sin(x + m_wavePhase + std::numbers::pi / 2)
                       + 0.4 * std::sin(x * 2.2 - m_wavePhase * 1.4 + std::numbers::pi / 2);
        frame.scopeLeft[i] = float(std::clamp(l / 1.4 * 0.85, -1.0, 1.0));
        frame.scopeRight[i] = float(std::clamp(r / 1.4 * 0.85, -1.0, 1.0));
    }

    return frame;
}

} // namespace cyber
