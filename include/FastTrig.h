#pragma once

#include <cmath>

namespace FastTrig
{
    inline constexpr double kC0 = 1.5702428873469307;
    inline constexpr double kC1 = -0.6417109210157308;
    inline constexpr double kC2 = 0.07146803366879997;

    inline double SinTurns(double turns)
    {
        double t = turns - std::floor(turns + 0.5);

        if (t > 0.25)
            t = 0.5 - t;
        else if (t < -0.25)
            t = -0.5 - t;

        const double x = 4.0 * t;
        const double x2 = x * x;
        return x * (kC0 + x2 * (kC1 + x2 * kC2));
    }

    inline double CosTurns(double turns)
    {
        return SinTurns(turns + 0.25);
    }
}
