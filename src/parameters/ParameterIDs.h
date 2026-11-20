#pragma once

namespace ParamID
{

inline constexpr const char* glob_route     = "glob_route";
inline constexpr const char* glob_blend     = "glob_blend";
inline constexpr const char* glob_bands     = "glob_bands";
inline constexpr const char* glob_chmode    = "glob_chmode";
inline constexpr const char* glob_drive_db  = "glob_drive_db";
inline constexpr const char* glob_tone      = "glob_tone";
inline constexpr const char* glob_colorcomp = "glob_colorcomp";
inline constexpr const char* glob_heat      = "glob_heat";
inline constexpr const char* glob_spread    = "glob_spread";
inline constexpr const char* glob_motion    = "glob_motion";
inline constexpr const char* glob_listen    = "glob_listen";
inline constexpr const char* glob_fb_mode   = "glob_fb_mode";
inline constexpr const char* glob_fb_time   = "glob_fb_time";
inline constexpr const char* glob_fb_div    = "glob_fb_div";
inline constexpr const char* glob_fb_note_hz = "glob_fb_note_hz";
inline constexpr const char* glob_fb_amt    = "glob_fb_amt";
inline constexpr const char* glob_fb_safe   = "glob_fb_safe";
inline constexpr const char* glob_comp      = "glob_comp";
inline constexpr const char* glob_comp_hpf  = "glob_comp_hpf";
inline constexpr const char* glob_out_db    = "glob_out_db";
inline constexpr const char* glob_mix       = "glob_mix";
inline constexpr const char* glob_os        = "glob_os";
inline constexpr const char* glob_linphase  = "glob_linphase";
inline constexpr const char* glob_bypass    = "glob_bypass";

inline constexpr const char* xoverHz[] = {
    "xover_hz_1", "xover_hz_2", "xover_hz_3", "xover_hz_4", "xover_hz_5"
};
inline constexpr const char* xoverSlope[] = {
    "xover_slope_1", "xover_slope_2", "xover_slope_3", "xover_slope_4", "xover_slope_5"
};

inline constexpr const char* cOn[]        = { "c0_on", "c1_on", "c2_on", "c3_on", "c4_on", "c5_on" };
inline constexpr const char* cSolo[]      = { "c0_solo", "c1_solo", "c2_solo", "c3_solo", "c4_solo", "c5_solo" };
inline constexpr const char* cMute[]      = { "c0_mute", "c1_mute", "c2_mute", "c3_mute", "c4_mute", "c5_mute" };
inline constexpr const char* cStyle[]     = { "c0_style", "c1_style", "c2_style", "c3_style", "c4_style", "c5_style" };
inline constexpr const char* cDriveDb[]   = { "c0_drive_db", "c1_drive_db", "c2_drive_db", "c3_drive_db", "c4_drive_db", "c5_drive_db" };
inline constexpr const char* cDrivePan[]  = { "c0_drive_pan", "c1_drive_pan", "c2_drive_pan", "c3_drive_pan", "c4_drive_pan", "c5_drive_pan" };
inline constexpr const char* cBias[]      = { "c0_bias", "c1_bias", "c2_bias", "c3_bias", "c4_bias", "c5_bias" };
inline constexpr const char* cMix[]       = { "c0_mix", "c1_mix", "c2_mix", "c3_mix", "c4_mix", "c5_mix" };
inline constexpr const char* cDyn[]       = { "c0_dyn", "c1_dyn", "c2_dyn", "c3_dyn", "c4_dyn", "c5_dyn" };
inline constexpr const char* cBass[]      = { "c0_bass", "c1_bass", "c2_bass", "c3_bass", "c4_bass", "c5_bass" };
inline constexpr const char* cMid[]       = { "c0_mid", "c1_mid", "c2_mid", "c3_mid", "c4_mid", "c5_mid" };
inline constexpr const char* cTreble[]    = { "c0_treble", "c1_treble", "c2_treble", "c3_treble", "c4_treble", "c5_treble" };
inline constexpr const char* cPresence[]  = { "c0_presence", "c1_presence", "c2_presence", "c3_presence", "c4_presence", "c5_presence" };
inline constexpr const char* cFiltType[]  = { "c0_filt_type", "c1_filt_type", "c2_filt_type", "c3_filt_type", "c4_filt_type", "c5_filt_type" };
inline constexpr const char* cFiltHz[]    = { "c0_filt_hz", "c1_filt_hz", "c2_filt_hz", "c3_filt_hz", "c4_filt_hz", "c5_filt_hz" };
inline constexpr const char* cFiltQ[]     = { "c0_filt_q", "c1_filt_q", "c2_filt_q", "c3_filt_q", "c4_filt_q", "c5_filt_q" };
inline constexpr const char* cFiltPos[]   = { "c0_filt_pos", "c1_filt_pos", "c2_filt_pos", "c3_filt_pos", "c4_filt_pos", "c5_filt_pos" };
inline constexpr const char* cFbAmt[]     = { "c0_fb_amt", "c1_fb_amt", "c2_fb_amt", "c3_fb_amt", "c4_fb_amt", "c5_fb_amt" };
inline constexpr const char* cFbHz[]      = { "c0_fb_hz", "c1_fb_hz", "c2_fb_hz", "c3_fb_hz", "c4_fb_hz", "c5_fb_hz" };
inline constexpr const char* cLevelDb[]   = { "c0_level_db", "c1_level_db", "c2_level_db", "c3_level_db", "c4_level_db", "c5_level_db" };
inline constexpr const char* cPan[]       = { "c0_pan", "c1_pan", "c2_pan", "c3_pan", "c4_pan", "c5_pan" };

} // namespace ParamID
