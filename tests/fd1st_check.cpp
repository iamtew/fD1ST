// JUCE-free check for IDs and choice tables. Compile with plain cl.
#include "dsp/Names.h"
#include "dsp/Shaper.h"
#include "parameters/ParameterIDs.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <vector>

static void add (std::vector<const char*>& ids, const char* id)
{
    ids.push_back (id);
}

int main()
{
    assert (fd1st::kRoutes == 6);
    assert (std::strcmp (fd1st::kRouteNames[0], "One") == 0);
    assert (std::strcmp (fd1st::kRouteNames[5], "Cavity") == 0);
    assert (fd1st::kStyles == 34);
    assert (std::strcmp (fd1st::kStyleNames[0], "Glow") == 0);
    assert (std::strcmp (fd1st::kStyleNames[33], "Crush") == 0);
    assert (fd1st::kListenCount == 4);
    assert (fd1st::kOsCount == 4);

    std::vector<const char*> ids;
    add (ids, ParamID::glob_route);
    add (ids, ParamID::glob_blend);
    add (ids, ParamID::glob_bands);
    add (ids, ParamID::glob_chmode);
    add (ids, ParamID::glob_drive_db);
    add (ids, ParamID::glob_tone);
    add (ids, ParamID::glob_colorcomp);
    add (ids, ParamID::glob_heat);
    add (ids, ParamID::glob_spread);
    add (ids, ParamID::glob_motion);
    add (ids, ParamID::glob_listen);
    add (ids, ParamID::glob_fb_mode);
    add (ids, ParamID::glob_fb_time);
    add (ids, ParamID::glob_fb_div);
    add (ids, ParamID::glob_fb_note_hz);
    add (ids, ParamID::glob_fb_amt);
    add (ids, ParamID::glob_fb_safe);
    add (ids, ParamID::glob_comp);
    add (ids, ParamID::glob_comp_hpf);
    add (ids, ParamID::glob_out_db);
    add (ids, ParamID::glob_mix);
    add (ids, ParamID::glob_os);
    add (ids, ParamID::glob_linphase);
    add (ids, ParamID::glob_bypass);
    for (int i = 0; i < fd1st::kXovers; ++i)
    {
        add (ids, ParamID::xoverHz[i]);
        add (ids, ParamID::xoverSlope[i]);
    }
    for (int i = 0; i < fd1st::kLenses; ++i)
    {
        add (ids, ParamID::cOn[i]);
        add (ids, ParamID::cSolo[i]);
        add (ids, ParamID::cMute[i]);
        add (ids, ParamID::cStyle[i]);
        add (ids, ParamID::cDriveDb[i]);
        add (ids, ParamID::cDrivePan[i]);
        add (ids, ParamID::cBias[i]);
        add (ids, ParamID::cMix[i]);
        add (ids, ParamID::cDyn[i]);
        add (ids, ParamID::cBass[i]);
        add (ids, ParamID::cMid[i]);
        add (ids, ParamID::cTreble[i]);
        add (ids, ParamID::cPresence[i]);
        add (ids, ParamID::cFiltType[i]);
        add (ids, ParamID::cFiltHz[i]);
        add (ids, ParamID::cFiltQ[i]);
        add (ids, ParamID::cFiltPos[i]);
        add (ids, ParamID::cFbAmt[i]);
        add (ids, ParamID::cFbHz[i]);
        add (ids, ParamID::cLevelDb[i]);
        add (ids, ParamID::cPan[i]);
    }

    std::set<std::string> unique;
    for (auto* id : ids)
    {
        assert (id != nullptr && id[0] != '\0');
        const auto ok = unique.insert (id).second;
        assert (ok);
    }
    assert (unique.size() == ids.size());
    assert (std::strcmp (ParamID::cOn[0], "c0_on") == 0);
    assert (std::strcmp (ParamID::cOn[1], "c1_on") == 0);

    // Spec §11.3 at 0 dBFS (makeup table is 0 dBFS 200 Hz). Drive 0 vs 24 within 3 dB.
    {
        const float r0 = fd1st::glowProbeRms (0.f, 1.f) * fd1st::glowMakeup (0.f);
        const float r24 = fd1st::glowProbeRms (24.f, 1.f) * fd1st::glowMakeup (24.f);
        const float db = 20.f * std::log10 (std::max (1.0e-8f, r24 / std::max (1.0e-8f, r0)));
        assert (std::abs (db) <= 3.f);
    }

    std::puts ("fd1st_check ok");
    return 0;
}
