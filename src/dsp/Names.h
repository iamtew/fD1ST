#pragma once

// Choice labels and counts. JUCE-free so tests/fd1st_check.cpp can compile it with cl.

namespace fd1st
{

inline constexpr int kLenses = 6;
inline constexpr int kXovers = 5;
inline constexpr int kStyles = 34;
inline constexpr int kRoutes = 6;

inline const char* const kRouteNames[kRoutes] = {
    "One", "Stack", "Split", "Prism", "Axis", "Cavity"
};

inline const char* const kListenNames[] = { "Wet", "Dry", "Delta", "Caustic" };
inline constexpr int kListenCount = 4;

inline const char* const kOsNames[] = { "Off", "Fast", "Good", "Superb" };
inline constexpr int kOsCount = 4;

inline const char* const kChmodeNames[] = { "Stereo", "Axis" };
inline constexpr int kChmodeCount = 2;

inline const char* const kFbModeNames[] = { "Time", "Sync", "Triplet", "Dotted", "Note" };
inline constexpr int kFbModeCount = 5;

inline const char* const kFbDivNames[] = { "1/32", "1/16", "1/8", "1/4", "1/2", "1/1" };
inline constexpr int kFbDivCount = 6;

inline const char* const kSlopeNames[] = { "6", "12", "24", "48" };
inline constexpr int kSlopeCount = 4;

inline const char* const kFiltTypeNames[] = {
    "Off", "LP", "BP", "HP", "Notch", "Peak", "Morph", "Comb", "Resample"
};
inline constexpr int kFiltTypeCount = 9;

inline const char* const kFiltPosNames[] = { "Pre", "Post" };
inline constexpr int kFiltPosCount = 2;

inline const char* const kStyleNames[kStyles] = {
    "Glow", "Clear", "Ember", "Crack",
    "Hissless", "Print", "Pack", "Oxide",
    "Tweedish", "Stack", "BritCrunch", "BritChime",
    "SmoothAmp", "CrunchAmp", "LeadAmp", "ScreamAmp", "PowerAmp",
    "TfLite", "TfGentle", "TfWarm",
    "SatLite", "SatSoft", "SatHeavy",
    "SoftSine", "Diode", "Poly", "TriFold", "Shards", "Inject",
    "Smear", "Drop", "Foldback", "Rectify", "Crush"
};

inline const float kXoverDefaultHz[kXovers] = { 200.f, 2000.f, 5000.f, 8000.f, 12000.f };

} // namespace fd1st
