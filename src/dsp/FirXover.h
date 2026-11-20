#pragma once

#include <cmath>
#include <vector>

namespace fd1st
{

// Complementary linear-phase split: HP = delayed − LP. Tap count is vs 48 kHz host,
// not OS rate (32× would explode). Cutoff still uses the processing sample rate.
struct FirSplit
{
    std::vector<float> ir, hist[2];
    int taps = 0, w[2] = {};
    int latency = 0;

    void design (int nTaps, float hz, float sr)
    {
        if (nTaps < 15)
            nTaps = 15;
        if ((nTaps & 1) == 0)
            ++nTaps;
        taps = nTaps;
        latency = (taps - 1) / 2;
        ir.assign ((size_t) taps, 0.f);
        const double fc = (double) hz / (double) sr;
        const double mid = 0.5 * (double) (taps - 1);
        double sum = 0;
        for (int i = 0; i < taps; ++i)
        {
            const double x = (double) i - mid;
            double sinc = (std::abs (x) < 1.0e-12) ? 2.0 * fc : std::sin (2.0 * 3.14159265358979323846 * fc * x)
                                                                    / (3.14159265358979323846 * x);
            const double win = 0.54 - 0.46 * std::cos (2.0 * 3.14159265358979323846 * (double) i / (double) (taps - 1));
            ir[(size_t) i] = (float) (sinc * win);
            sum += (double) ir[(size_t) i];
        }
        if (std::abs (sum) > 1.0e-12)
            for (auto& v : ir)
                v = (float) ((double) v / sum);
        for (int c = 0; c < 2; ++c)
        {
            hist[c].assign ((size_t) taps, 0.f);
            w[c] = 0;
        }
    }

    void split (float x, int ch, float& lo, float& hi)
    {
        if (taps < 3 || ch < 0 || ch > 1)
        {
            lo = x;
            hi = 0;
            return;
        }
        hist[ch][(size_t) w[ch]] = x;
        float acc = 0;
        int p = w[ch];
        for (int i = 0; i < taps; ++i)
        {
            acc += ir[(size_t) i] * hist[ch][(size_t) p];
            if (--p < 0)
                p = taps - 1;
        }
        const int d = w[ch] - latency;
        const float delayed = hist[ch][(size_t) ((d % taps + taps) % taps)];
        w[ch] = (w[ch] + 1) % taps;
        lo = acc;
        hi = delayed - acc;
    }
};

} // namespace fd1st
