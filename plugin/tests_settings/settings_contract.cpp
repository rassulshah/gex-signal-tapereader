// settings_contract.cpp  1.0.0  (2026-10-18)  -  the settings contract every IRT indicator must pass (SettingsSpec.h).
// SDK-free and portable: Linux g++ (run_tests.sh) AND the PC's cl (tools\gex-build.bat runs it before building; a FAIL stops the build).
//   settings_contract <plugin folder>      exit 0 = PASS, 1 = FAIL (one line per failure on stdout)
// Per table: the table checks (keys, labels without '?', defaults valid for all 7 markets, ranges, groups, every window line within
// the width budget), save -> load round trip for every market (random valid values), garbage / wrong-market / other-plugin / unknown /
// missing keys -> defaults (never garbage), each legacy file sample migrates to the expected values, group masters switch their
// items, colours pass through unchanged (0x00RRGGBB - the value the window shows is the value drawn), and against the plugin's .cpp:
// every row is READ (its K_ name used outside the table), the window is registered from the table (no hand-written set*Parameter
// rows, no getIntegerValue / setIntegerValue on parameters), parmsLoad / parmsApply / parmsUpdt go through the layer, and the
// setVersion literal equals the table's version.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "../SettingsSpec.h"
#include "../HodLodSettings.h"
#include "../DeltaProfileSettings.h"
#include "../TapeReaderSettings.h"
#include "../OrderFlowPaneSettings.h"
#include "../LiquidityProfileSettings.h"
#include "../CVDSettings.h"
#include "../CobImpulseSettings.h"
#include "../TradeManagerSettings.h"
#include "../DealerProfileSettings.h"
#include "../SessionVWAPSettings.h"
#include "../DefenseLinesSettings.h"
#include "../TurnOrderSettings.h"
#include "../KeyLevelsHSettings.h"
#include "../BigBidAskSettings.h"
#include "../ReversalStageSettings.h"
#include "../DealerHedgeSettings.h"
#include "../SweepRefillSettings.h"
#include "../StopRunMarkSettings.h"

struct LegacyCase { const char* text; int market; const char* key; double want; };   // key == nullptr: must NOT be recognised
struct Entry {
    const ss::Spec* S;
    const char* cpp;                       // the indicator's source (setVersion literal + reads checked)
    ss::LegacyFn legacy;
    std::vector<LegacyCase> cases;
    std::vector<std::string> extraSources; // headers that also read the values (e.g. OrderFlowFeed.h for the panes)
    int mappedRows = -1;                   // >= 0: rows are read through a mapping (not K_ names) - the non-label row count it must cover
};

static int g_fail = 0, g_checks = 0;
static void fail(const std::string& m) { std::printf("FAIL %s\n", m.c_str()); ++g_fail; }
static void ok(bool c, const std::string& m) { ++g_checks; if (!c) fail(m); }

static bool readAll(const std::string& p, std::string& out) { std::ifstream f(p.c_str(), std::ios::binary); if (!f) return false; std::stringstream ss_; ss_ << f.rdbuf(); out = ss_.str(); return true; }
static size_t countOf(const std::string& s, const std::string& w) { size_t n = 0, p = 0; while ((p = s.find(w, p)) != std::string::npos) { ++n; p += w.size(); } return n; }
// a K_ name used as a whole word (K_POS must not match K_POSITION)
static size_t wordCount(const std::string& s, const std::string& w) {
    size_t n = 0, p = 0;
    while ((p = s.find(w, p)) != std::string::npos) {
        const bool l = p == 0 || !(std::isalnum((unsigned char)s[p - 1]) || s[p - 1] == '_');
        const size_t e = p + w.size(); const bool r = e >= s.size() || !(std::isalnum((unsigned char)s[e]) || s[e] == '_');
        if (l && r) ++n;
        p = e;
    }
    return n;
}
static std::string upperKey(const char* k) { std::string s = "K_"; for (const char* c = k; *c; ++c) s += (char)std::toupper((unsigned char)*c); return s; }

static unsigned long long g_rng = 0x9E3779B97F4A7C15ull;
static double rnd01() { g_rng ^= g_rng << 13; g_rng ^= g_rng >> 7; g_rng ^= g_rng << 17; return (double)(g_rng % 1000003ull) / 1000003.0; }
static double randomValid(const ss::Row& r) {
    switch (r.type) {
        case ss::T_BOOL: return rnd01() < 0.5 ? 0 : 1;
        case ss::T_INT: case ss::T_LIST: return std::floor(r.lo + rnd01() * (r.hi - r.lo + 1 - 1e-9));
        case ss::T_FLOAT: { const double x = r.lo + rnd01() * (r.hi - r.lo); return std::floor(x * 100.0) / 100.0; }
        case ss::T_COLOR: return std::floor(rnd01() * 0xFFFFFF);
        default: return 0;
    }
}

static void checkEntry(const Entry& E, const std::string& dir) {
    const ss::Spec& S = *E.S; const std::string P = S.plugin;
    for (const std::string& m : ss::check(S)) fail(m);
    ++g_checks;
    // round trip, every market
    for (int mkt = 0; mkt < ss::M_N; ++mkt) {
        for (int rep = 0; rep < 20; ++rep) {
            ss::Values V = ss::defaults(S, mkt);
            if (rep) {
                for (int k = 0; k < S.n; ++k) {
                    if (S.rows[k].type == ss::T_TEXT) { std::string t; const int n = (int)(rnd01() * std::min(40.0, S.rows[k].hi)); for (int c = 0; c < n; ++c) t += (char)('a' + (int)(rnd01() * 26)); if (rep % 3 == 0 && !t.empty()) t[0] = ','; V.t[(size_t)k] = t; }
                    else if (S.rows[k].type != ss::T_LABEL) V.v[(size_t)k] = randomValid(S.rows[k]);
                }
            }
            ss::Values W; const bool okp = ss::parse(S, mkt, ss::format(S, V), W);
            ok(okp, P + " round trip parse " + ss::MARKETS[mkt]);
            for (int k = 0; k < S.n; ++k) if (S.rows[k].type == ss::T_TEXT) ok(W.s(k) == V.s(k), P + " round trip text " + S.rows[k].key + " '" + V.s(k) + "' -> '" + W.s(k) + "'");
            for (int k = 0; k < S.n; ++k) if (S.rows[k].type != ss::T_LABEL && S.rows[k].type != ss::T_TEXT)
                ok(std::fabs(W.v[(size_t)k] - V.v[(size_t)k]) < 1e-6, P + " round trip " + S.rows[k].key + " " + ss::MARKETS[mkt] + " " + ss::fmt(S.rows[k], V.v[(size_t)k]) + " -> " + ss::fmt(S.rows[k], W.v[(size_t)k]));
        }
        // defaults: never 0 / garbage where the row has a real default, and valid
        const ss::Values D = ss::defaults(S, mkt);
        for (int k = 0; k < S.n; ++k) ok(S.rows[k].type == ss::T_LABEL || ss::valid(S.rows[k], D.v[(size_t)k]), P + " default " + S.rows[k].key + " " + ss::MARKETS[mkt]);
        // garbage / wrong files -> defaults
        const char* bad[] = { "", "garbage", "SPEC1|Other|1|ES\nposition=3\n", "SPEC1|X\n", "\x01\x02\x03" };
        for (const char* b : bad) { ss::Values W; ss::parse(S, mkt, b, W); for (int k = 0; k < S.n; ++k) ok(W.v[(size_t)k] == D.v[(size_t)k], P + " bad file -> default " + S.rows[k].key); }
        {   // another market's file is not applied
            ss::Values V = D; const int other = (mkt + 1) % ss::M_N; V.market = other;
            for (int k = 0; k < S.n; ++k) if (S.rows[k].type == ss::T_BOOL) V.v[(size_t)k] = 1 - V.v[(size_t)k];
            ss::Values W; ok(!ss::parse(S, mkt, ss::format(S, V), W), P + " other market's file rejected");
        }
        {   // invalid values / unknown keys / missing keys -> default, the rest kept
            std::string t = std::string("SPEC1|") + S.plugin + "|0|" + ss::MARKETS[mkt] + "\nnot_a_key=5\n";
            for (int k = 0; k < S.n; ++k) { const ss::Row& r = S.rows[k]; if (r.type == ss::T_LABEL) continue;
                t += std::string(r.key) + "=" + (r.type == ss::T_TEXT ? std::string((size_t)r.hi + 5, 'x') : std::string(r.type == ss::T_COLOR ? "ZZZZZZ" : (r.type == ss::T_FLOAT ? "nan" : "999999999"))) + "\n"; }
            ss::Values W; ss::parse(S, mkt, t, W);
            for (int k = 0; k < S.n; ++k) ok(W.v[(size_t)k] == D.v[(size_t)k] && W.s(k) == D.s(k), P + " invalid value -> default " + S.rows[k].key);
        }
    }
    // colours pass through unchanged
    for (int k = 0; k < S.n; ++k) if (S.rows[k].type == ss::T_COLOR) {
        ss::Values V = ss::defaults(S, 0); V.v[(size_t)k] = 0x22C55E; ok(V.c(k) == 0x22C55Eul, P + " colour " + S.rows[k].key + " unchanged");
        ss::Values W; ss::parse(S, 0, ss::format(S, V), W); ok(W.c(k) == 0x22C55Eul, P + " colour " + S.rows[k].key + " file round trip (RRGGBB)");
    }
    // group masters
    for (int k = 0; k < S.n; ++k) if (S.rows[k].master) {
        ss::Values V = ss::defaults(S, 0); V.v[(size_t)k] = 0; ss::masterClick(S, V, k);
        for (int j : ss::itemsOf(S, k)) { if (S.rows[j].type == ss::T_BOOL) ok(!V.b(j), P + " master " + S.rows[k].key + " off unchecks " + S.rows[j].key); ok(!ss::effective(S, V, j), P + " master off -> " + S.rows[j].key + " not in effect"); }
        V.v[(size_t)k] = 1; ss::masterClick(S, V, k);
        for (int j : ss::itemsOf(S, k)) if (S.rows[j].type == ss::T_BOOL) ok(V.b(j), P + " master " + S.rows[k].key + " on checks " + S.rows[j].key);
    }
    // legacy samples
    for (const LegacyCase& c : E.cases) {
        ss::Values V = ss::defaults(S, c.market);
        const bool rec = E.legacy && E.legacy(S, c.market, c.text, V);
        if (!c.key) { ok(!rec, P + " legacy text must not be recognised: '" + std::string(c.text).substr(0, 40) + "'"); continue; }
        ok(rec, P + " legacy recognised: '" + std::string(c.text).substr(0, 40) + "'");
        const int k = ss::find(S, c.key); ok(k >= 0, P + " legacy key " + c.key);
        if (k >= 0) ok(std::fabs(V.v[(size_t)k] - c.want) < 1e-6, P + " legacy " + c.key + " = " + ss::fmt(S.rows[k], V.v[(size_t)k]) + " want " + ss::fmt(S.rows[k], c.want));
        for (int j = 0; j < S.n; ++j) ok(S.rows[j].type == ss::T_LABEL || ss::valid(S.rows[j], V.v[(size_t)j]), P + " legacy -> valid " + S.rows[j].key);
    }
    // the source: every row read, the window from the table, version literal
    if (dir.empty()) return;
    std::string src; if (!readAll(dir + "/" + E.cpp, src)) { fail(P + " cannot read " + E.cpp); return; }
    std::string all = src; for (const std::string& x : E.extraSources) { std::string t; if (readAll(dir + "/" + x, t)) all += t; else fail(P + " cannot read " + x); }
    if (E.mappedRows >= 0) {   // e.g. BigBidAsk: every row through controls() / toVec (test_bba116 proves the 75-value round trip)
        int n = 0; for (int k = 0; k < S.n; ++k) if (S.rows[k].type != ss::T_LABEL) ++n;
        ok(n == E.mappedRows, P + " the mapping covers " + std::to_string(E.mappedRows) + " rows, the table has " + std::to_string(n));
    } else for (int k = 0; k < S.n; ++k) { const ss::Row& r = S.rows[k]; if (r.type == ss::T_LABEL) continue;
        ok(wordCount(all, upperKey(r.key)) > 0, P + " dead setting: '" + r.key + "' (" + upperKey(r.key) + ") is never read by " + E.cpp); }
    ok(countOf(src, "ss::registerAll(") == 1, P + " the window must be registered by ss::registerAll (once)");
    ok(countOf(src, "ss::fillDialog(") >= 1, P + " parmsLoad must fill the window with ss::fillDialog");
    ok(countOf(src, "ss::readDialog(") >= 1, P + " parmsApply must read the window with ss::readDialog");
    ok(countOf(src, "ss::onUpdate(") >= 1, P + " parmsUpdt must call ss::onUpdate (group masters)");
    const char* banned[] = { "setBoolParameter(", "setIntegerParameter(", "setFloatParameter(", "setColorParameter(", "setListParameter(", "setLabelParameter(", "getIntegerValue(", "setIntegerValue(", "getParameterColor(", "setParameterColor(", "isBoxChecked(", "checkBox(", "getListIndex(", "setListIndex(", "getFloatValue(", "setFloatValue(", "getParameterText(", "setParameterText(", "setStringParameter(" };
    for (const char* b : banned) ok(countOf(src, b) == 0, P + " " + E.cpp + " calls " + b + " by hand (use the table / SettingsHost.h)");
    const std::string want = std::string("setVersion(\"") + S.version + "\")";
    ok(countOf(src, want) == 1, P + " setVersion literal must be " + want);
}

int main(int argc, char** argv) {
    const std::string dir = argc > 1 ? argv[1] : "";
    std::vector<Entry> E;
    {   // (SettingsSpec 1.2.0) a changed default: the old default written by the version that had it reads as the new default; his own value stays
        ss::Values V = ss::defaults(dpset::spec(), ss::M_ES); V.v[dpset::K_FONT] = 9; dpset::upgrade("2.18.2", V); if (V.i(dpset::K_FONT) != 10) fail("DeltaProfile upgrade: 2.18.2 font 9 -> 10");
        V.v[dpset::K_FONT] = 9; dpset::upgrade("2.18.3", V); if (V.i(dpset::K_FONT) != 9) fail("DeltaProfile upgrade: a 2.18.3 font 9 is his choice");
        V.v[dpset::K_FONT] = 12; dpset::upgrade("2.18.2", V); if (V.i(dpset::K_FONT) != 12) fail("DeltaProfile upgrade: font 12 kept");
        ss::Values T = ss::defaults(tmset::spec(), ss::M_ES); T.v[tmset::K_FONT] = 8; tmset::upgrade("1.3.1", T); if (T.i(tmset::K_FONT) != 10) fail("TradeManager upgrade: 1.3.1 font 8 -> 10");
        T.v[tmset::K_FONT] = 8; tmset::upgrade("1.3.2", T); if (T.i(tmset::K_FONT) != 8) fail("TradeManager upgrade: a 1.3.2 font 8 is his choice");
    }
    E.push_back(Entry{ &hlset::spec(), "HodLod.cpp", hlset::legacy, {
        { "7,0,0,1,1,1,-1,-1,-1,-1,-1\n", ss::M_ES, "position", 7 },          // his ES / NQ file on 10-18
        { "7,0,0,1,1,1,-1,-1,-1,-1,-1\n", ss::M_ES, "table_bg", 0x0F1520 },    // -1 = the default colour, never black
        { "3,10,-20,0,1,1,255,-1,-1,-1,-1", ss::M_NQ, "position", 3 },     // (2.4.4) the old offsets are skipped
        { "3,10,-20,0,1,1,255,-1,-1,-1,-1", ss::M_NQ, "show_table", 0 },
        { "3,10,-20,0,1,1,255,-1,-1,-1,-1", ss::M_NQ, "table_bg", 255 },
        { "5", ss::M_CL, "position", 5 },
        { "99,0,0", ss::M_CL, "position", 7 },                                // out of range -> default
        { "1,2", ss::M_CL, nullptr, 0 },                                       // 2 fields: not a place file
    }, {} });
    E.push_back(Entry{ &dpset::spec(), "DeltaProfile.cpp", dpset::legacy, {
        { "EPZ26|0|50|0|9|0|0|0|0|0|1|0|90|2|0\nNQ|0|50|0|9|0|0|0|0|0|1|0|90|2|0\n", ss::M_ES, "width", 110 },  // pre-2.8 default 50 -> the default (drawn 110 either way)
        { "EPZ26|0|50|0|9|0|0|0|0|0|1|0|90|2|0\n", ss::M_ES, "font", 10 },     // (2.18.3) 9 = the old default -> the new default 10
        { "EPZ26|0|50|0|12|0|0|0|0|0|1|0|90|2|0\n", ss::M_ES, "font", 12 },    // his own choice kept
    }, {} });
    E.push_back(Entry{ &trset::spec(), "TapeReader.cpp", trset::legacy, {
        { "7", ss::M_ES, "position", 7 },
        { "4,10,-5,0", ss::M_NQ, "background", 0 },
        { "4,10,-5,0", ss::M_NQ, "position", 4 },                              // (1.3.2) Offset X / Y removed: a saved offset is ignored
    }, {} });
    // (BidAskStats 1.13.2 / BookFlow 1.3.2) the window on the layer; the place files keep the panes' own format (no legacy reader needed)
    E.push_back(Entry{ &ofset::bidAskSpec(), "BidAskStats.cpp", nullptr, {}, {} });
    E.push_back(Entry{ &ofset::bookFlowSpec(), "BookFlow.cpp", nullptr, {}, {} });
    // (LiquidityProfile / LiquidityFlow 0.45.0) one source, two builds; the three files keep their strict formats
    E.push_back(Entry{ &lpset::profileSpec(), "LiquidityProfile.cpp", nullptr, {}, {} });
    E.push_back(Entry{ &lpset::flowSpec(), "LiquidityProfile.cpp", nullptr, {}, {} });
    E.push_back(Entry{ &tmset::spec(), "TradeManager.cpp", tmset::legacy, {
        { "2\n", ss::M_ES, "position", 2 }, { "7", ss::M_NQ, "position", 7 }, { "9", ss::M_NQ, "position", 2 }, { "abc", ss::M_ES, nullptr, 0 } }, {} });
    // (DealerProfile 2.17.1) his real 10-05 file (root + chart keys) and a 2.11 (c2) row; lsHodLod 2.4.3 reads "banner" through the same reader
    E.push_back(Entry{ &dprset::spec(), "DealerProfile.cpp", dprset::legacy, {
        { "CPEZ26~3 Minutes*~|0|50|9|0|0|0|0|0|cob_bull,cob_bear\nEPZ26~3 Minutes*~|0|50|9|0|0|0|0|0|cob_bull,cob_bear\nNQ~3 Minutes*~|0|50|9|0|0|0|0|0|cob_bull,cob_bear\n", ss::M_HG, nullptr, 0 },   // his 10-05 file: per-chart keys, never matched -> defaults (unchanged)
        { "CPEZ26|0|50|9|0|0|0|0|0|cob_bull,cob_bear\n", ss::M_HG, "width", 80 },          // an old root row: bar length 50 -> the 80 px column
        { "CPEZ26|0|50|9|0|0|0|0|0|cob_bull,cob_bear\n", ss::M_HG, "font", 10 },           // 9 pt was drawn as 10 (the 10 pt minimum)
        { "CPEZ26|0|50|9|0|0|0|0|0|cob_bull,cob_bear\n", ss::M_HG, "labels", 0 },
        { "EPZ26~3 Minutes*~|0|50|9|0|0|0|0|0|x\nES|0|130|11|0|1|1|1|1|cob_bull,cob_bear|c2|60|9|1\n", ss::M_ES, "levels_w", 60 },   // the c2 row wins
        { "EPZ26~3 Minutes*~|0|50|9|0|0|0|0|0|x\nES|0|130|11|0|1|1|1|1|cob_bull,cob_bear|c2|60|9|1\n", ss::M_ES, "banner", 1 },     // HodLod's banner flag
        { "EPZ26~3 Minutes*~|0|50|9|0|0|0|0|0|x\nES|0|130|11|0|1|1|1|1|cob_bull,cob_bear|c2|60|9|1\n", ss::M_ES, "width", 130 },
        { "NQ|0|300|10|0|1|1|1|0|x\n", ss::M_NQ, "width", 250 },                                     // old row: clamped into 40-250
        { "NQ|0|300|10|0|1|1|1|0|x\n", ss::M_ES, nullptr, 0 },                                        // no ES row: not recognised (defaults)
    }, {} });
    E.push_back(Entry{ &svset::spec(), "SessionVWAP.cpp", nullptr, {}, {} });   // (1.6.1) the file keeps its own keyed-style format (SessionVWAPStyle.h)
    E.push_back(Entry{ &cvdset::spec(), "CVD.cpp", nullptr, {}, {} });              // (1.3.0, his decision list) show / hide + colour per drawn element
    E.push_back(Entry{ &dlset::spec(), "DefenseLines.cpp", nullptr, {}, {} });   // (2026-10-20) lsDefenseLines 1.0.0: lines / marks show + colours, line minutes (also read by BookFlow.cpp)
    E.push_back(Entry{ &klhset::spec(), "KeyLevels.cpp", nullptr, {}, { "KeyLevelsHLogic.h" } });   // (2026-10-20 22:41) lsKeyLevels 1.0.0: hourly-chart key levels - show + colour per group, MenthorQ (off), font, line width
    E.push_back(Entry{ &ciset::spec(), "CobImpulse.cpp", nullptr, {}, {} });
    E.push_back(Entry{ &bbaset::spec(), "BigBidAsk.cpp", nullptr, {}, {} });   // (1.19.0, Rassul 21:51) Biggest in, skip first / last, font, number / bid / ask colours        // (0.5.0, his decision list) labels / impulse box: show + colour
    E.push_back(Entry{ &torset::spec(), "TurnOrder.cpp", nullptr, {}, {} });   // (2026-10-22) lsTurnOrder 1.0.0: show + bid / offer colours, the confirmation gate (ATR), min size
    E.push_back(Entry{ &dhset::spec(), "DealerHedge.cpp", nullptr, {}, {} });   // (2026-10-22) lsDealerHedge 0.1.0 oH zones: show + buy / sell colours, band %, pulse, labels, hover, width, flow x, repeats, refills
    E.push_back(Entry{ &swrset::spec(), "SweepRefill.cpp", nullptr, {}, {} });   // (2026-10-22) lsSweepRefill 1.1.0 Sr: show bullish / bearish + colours, ZigZag show + colour, swing line, gap, font (the detection numbers: LRA-StopSweep-<MKT>.csv)
    E.push_back(Entry{ &srmset::spec(), "StopRunMark.cpp", nullptr, {}, {} });   // (2026-10-22) lsStopRun 1.2.0 St / Sta: show + colours, min orders, font (the detection numbers: LRA-StopSweep-<MKT>.csv)
    E.push_back(Entry{ &rsset::spec(), "ReversalStage.cpp", nullptr, {}, {} });   // (2026-10-22) lsReversalStage 1.0.0: steps show + low / high colours, stage names, Now box show + Position
    if (argc > 3 && std::string(argv[2]) == "--print") {   // the expected window per indicator, for the real-dialog check (tools\settings-check)
        const int mkt = ss::marketIndex(argv[3]);
        for (const Entry& e : E) {
            const ss::Spec& S = *e.S; std::printf("== %s %s (parameter version %u) - %s defaults\n", S.plugin, S.version, S.paramVersion, mkt >= 0 ? ss::MARKETS[mkt] : "ALL");
            const ss::Values D = ss::defaults(S, mkt);
            for (const ss::Line& L : ss::lines(S)) {
                std::string t; for (int k : L.rows) { const ss::Row& r = S.rows[k]; std::string v = ss::fmt(r, D.v[(size_t)k]);
                    if (r.type == ss::T_LABEL) v = ""; else if (r.type == ss::T_BOOL) v = D.b(k) ? "[x]" : "[ ]"; else if (r.type == ss::T_TEXT) v = "'" + D.s(k) + "'"; else if (r.type == ss::T_COLOR) v = "#" + v; else if (r.type == ss::T_LIST) { std::vector<std::string> c = ss::split(r.choices, ';'); v = D.i(k) < (int)c.size() ? c[(size_t)D.i(k)] : v; }
                    t += std::string(t.empty() ? "" : "  |  ") + r.label + " " + v; }
                std::printf("   ~%3dpx  %s\n", L.px, t.c_str());
            }
            for (int k = 0; k < S.n; ++k) if (S.rows[k].hidden) std::printf("   (hidden, kept in the file) %s = %s\n", S.rows[k].key, (S.rows[k].type == ss::T_TEXT ? D.s(k) : ss::fmt(S.rows[k], D.v[(size_t)k])).c_str());
        }
        return 0;
    }
    for (const Entry& e : E) {
        // a table only binds its .cpp once the .cpp is migrated (it includes the table's header)
        std::string src; const bool have = !dir.empty() && readAll(dir + "/" + e.cpp, src);
        std::string hdr = std::string(e.S->plugin) + "Settings.h";
        const bool migrated = have && (src.find("Settings.h\"") != std::string::npos) && (src.find("ss::registerAll(") != std::string::npos);
        checkEntry(e, migrated ? dir : std::string());
        if (have && !migrated) std::printf("NOTE %s: table checked; %s not migrated to the layer yet (source checks skipped)\n", e.S->plugin, e.cpp);
    }
    std::printf("%s settings contract: %d checks, %d failures (%d tables)\n", g_fail ? "FAIL" : "PASS", g_checks, g_fail, (int)E.size());
    return g_fail ? 1 : 0;
}
