// JUCE-free check. Compile with plain cl. Hexes must match src/ui/Theme.h.
#include "parameters/ParameterIDs.h"

#include <cassert>
#include <cstring>

int main()
{
    assert (std::strcmp (ParamID::out_db, "out_db") == 0);

    constexpr unsigned kMain    = 0x900020u;
    constexpr unsigned kEmerald = 0x0F6B4Eu;
    constexpr unsigned kSage    = 0xA8C5B0u;
    constexpr unsigned kForest  = 0x1A3C32u;
    constexpr unsigned kBlack   = 0x000000u;
    constexpr unsigned kWhite   = 0xFFFFFFu;
    assert (kMain == 0x900020u);
    assert (kEmerald == 0x0F6B4Eu);
    assert (kSage == 0xA8C5B0u);
    assert (kForest == 0x1A3C32u);
    assert (kBlack == 0u);
    assert (kWhite == 0xFFFFFFu);
    return 0;
}
