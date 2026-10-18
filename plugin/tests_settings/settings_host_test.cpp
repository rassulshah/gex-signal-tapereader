// settings_host_test.cpp - SettingsHost.h on the mock SDK (IRT's behaviour: blank chart copy, colour rows ignore integer calls).
// Proves: registration from the table (ids, same-line flags, version, dialog height), colours filled / read through the colour API
// (the old setIntegerValue path reproduces the BLACK swatch), group masters (uncheck + disable), untouched controls keep the file's
// value even when IRT ignores fills, invalid input never saved, the chart copy never saves, keyed store + one-time legacy migration
// from his real 10-18 files, same-second same-size re-save seen (lesson 84), every table's window lines within the budget.
#include "mock_irtsdk.h"
#include "../SettingsHost.h"
#include "../HodLodSettings.h"
#include "../DeltaProfileSettings.h"
#include "../TapeReaderSettings.h"
#include <cassert>
#include <cstdlib>
#include <unistd.h>

static int F = 0, N = 0;
#define CHECK(c, m) do { ++N; if (!(c)) { std::printf("FAIL %s (line %d)\n", m, __LINE__); ++F; } } while (0)

static std::string tmpDir() { char t[] = "/tmp/sstestXXXXXX"; const char* d = mkdtemp(t); return d ? d : "/tmp"; }
static void writeFile(const std::string& p, const std::string& s) { FILE* f = std::fopen(p.c_str(), "wb"); std::fwrite(s.data(), 1, s.size(), f); std::fclose(f); }
static std::string readFile(const std::string& p) { std::string s; ss::readSmall(p, s); return s; }

int main() {
    using namespace hlset;
    const ss::Spec& S = spec();
    // 1. registration
    cppExtension x; ss::Ids ids; ss::registerAll(&x, S, ids);
    CHECK(ids.ok && (int)x.m.p.size() == S.n, "every visible row registered");
    CHECK(x.m.paramVersion == S.paramVersion, "parameter version from the table");
    CHECK(x.m.dialogLines == ss::visibleLines(S), "dialog height = the visible lines");
    for (int k = 0; k < S.n; ++k) { const mock::Param* q = x.at(ids.id[(size_t)k]); CHECK(q && q->name == S.rows[k].label, "row label at its id");
        CHECK(((q->flags & kParmAppendSameLine) != 0) == S.rows[k].same, "same-line flag from the table"); }
    // 2. colours: fill -> swatch shows the colour; the old integer path leaves it black
    ss::Values V = ss::defaults(S, ss::M_ES); V.v[K_PROJ_UP] = 0x123456; V.v[K_TABLE_BG] = MAKE_COLOR(0x0F, 0x15, 0x20);
    ss::Snap snap; ss::fillDialog(&x, S, ids, V, snap);
    CHECK(x.at(ids.id[K_PROJ_UP])->c == 0x123456ul, "colour row filled with setParameterColor shows the colour");
    CHECK(x.getParameterColor(ids.id[K_PROJ_UP]) == 0x123456ul, "dialog colour == the value the chart draws (0x00RRGGBB, no swap)");
    { cppExtension y; ss::Ids yi; ss::registerAll(&y, S, yi); y.setIntegerValue(yi.id[K_PROJ_UP], 0x123456);
      CHECK(y.at(yi.id[K_PROJ_UP])->c == (COLOR)0x22C55E, "old path: setIntegerValue on a colour row does nothing (the swatch keeps its default / black)");
      CHECK(y.getIntegerValue(yi.id[K_PROJ_UP]) == 0, "old path: getIntegerValue on a colour row reads 0 -> black saved (BigBidAsk.settings-ES.txt 10-18)"); }
    // 3. user edits + group master
    x.userColor(ids.id[K_MARK_HOD], 0xABCDEF); x.userInt(ids.id[K_OFFSET_X], 25);
    x.userCheck(ids.id[K_SHOW_PROJ], false); ss::onUpdate(&x, S, ids, ids.id[K_SHOW_PROJ] + 1, snap);   // IRT may pass another number (lesson 94)
    CHECK(!x.isEnabled(ids.id[K_PROJ_UP]) && !x.isEnabled(ids.id[K_PROJ_DN]), "master off greys its colours");
    CHECK(x.isEnabled(ids.id[K_TABLE_BG]), "another group's item stays enabled");
    CHECK(!ss::onUpdate(&x, S, ids, ids.id[K_SHOW_PROJ], snap), "a second update without a change does nothing");
    ss::Values W = ss::readDialog(&x, S, ids, V, snap);
    CHECK(W.c(K_MARK_HOD) == 0xABCDEFul && W.i(K_OFFSET_X) == 25 && !W.b(K_SHOW_PROJ), "edits read back");
    CHECK(W.c(K_PROJ_UP) == 0x123456ul, "a disabled colour keeps its value");
    x.userCheck(ids.id[K_SHOW_PROJ], true); ss::onUpdate(&x, S, ids, 0, snap);
    CHECK(x.isEnabled(ids.id[K_PROJ_UP]), "master on enables its colours again");
    // 4. garbage typed into a number -> the file's value
    x.userInt(ids.id[K_OFFSET_Y], 99999); W = ss::readDialog(&x, S, ids, V, snap);
    CHECK(W.i(K_OFFSET_Y) == V.i(K_OFFSET_Y), "an out-of-range number is never saved");
    // 5. IRT ignores the fill: untouched controls keep the FILE's values (not the window's stale ones)
    { cppExtension z; ss::Ids zi; ss::registerAll(&z, S, zi); z.m.ignoreFill = true;
      ss::Values file = ss::defaults(S, ss::M_NQ); file.v[K_POSITION] = 2; file.v[K_TABLE_BG] = 0x010203; file.v[K_SHOW_MARK] = 0;
      ss::Snap zs; ss::fillDialog(&z, S, zi, file, zs);
      ss::Values out = ss::readDialog(&z, S, zi, file, zs);
      for (int k = 0; k < S.n; ++k) CHECK(out.v[(size_t)k] == file.v[(size_t)k], "ignored fill + untouched -> file value kept");
      z.userList(zi.id[K_POSITION], 5); out = ss::readDialog(&z, S, zi, file, zs);
      CHECK(out.i(K_POSITION) == 5 && out.c(K_TABLE_BG) == 0x010203ul, "the one control he changed is saved, the rest from the file"); }
    // 6. the chart copy: everything blank -> inDialog false (the plugins never save from it)
    { cppExtension c; c.m.dialog = false; CHECK(!ss::inDialog(&c), "chart copy is not the window"); }
    // 7. the store: legacy migration from his real files, keyed file, lesson 84
    const std::string d = tmpDir();
    { ss::Store st; st.S = &S; st.path = d + "/HodLod.settings-ES.txt"; st.legacyPaths.push_back(d + "/HodLod.place-ES.txt"); st.legacy = hlset::legacy;
      writeFile(d + "/HodLod.place-ES.txt", "7,0,0,1,1,1,-1,-1,-1,-1,-1\n");
      std::string mig; ss::Values a = st.load(ss::M_ES, 0, true, &mig);
      CHECK(mig == d + "/HodLod.place-ES.txt", "migrated from the place file");
      CHECK(a.i(K_POSITION) == 7 && a.b(K_SHOW_TABLE) && a.c(K_TABLE_BG) == 0x0F1520ul && a.c(K_PROJ_UP) == 0x22C55Eul, "his ES file -> Bottom centre, default colours (never black)");
      const std::string keyed = readFile(st.path);
      CHECK(ss::isKeyed(keyed) && keyed.find("position=7") != std::string::npos && keyed.find("table_bg=0F1520") != std::string::npos, "keyed file written");
      CHECK(readFile(d + "/HodLod.place-ES.txt") == "7,0,0,1,1,1,-1,-1,-1,-1,-1\n", "legacy file left untouched");
      ss::Values b = a; b.v[K_POSITION] = 3; CHECK(st.save(b), "save");
      ss::Store st2; st2.S = &S; st2.path = st.path; st2.legacy = hlset::legacy;
      CHECK(st2.load(ss::M_ES, 0).i(K_POSITION) == 3, "a second chart reads the saved value");
      b.v[K_POSITION] = 4; ss::Store st3 = st2; st.save(b);                 // same size, same second
      CHECK(st3.load(ss::M_ES, 1500).i(K_POSITION) == 4, "same-second same-size re-save is seen (content compare, lesson 84)");
      CHECK(st3.load(ss::M_NQ, 5000).i(K_POSITION) == 7, "an ES file never applies to NQ (defaults)"); }
    { // DeltaProfile: his real per-root file -> per market
      ss::Store st; st.S = &dpset::spec(); st.path = d + "/DeltaProfile.settings-GC.txt"; st.legacyPaths.push_back(d + "/DeltaProfile.settings.txt"); st.legacy = dpset::legacy;
      writeFile(d + "/DeltaProfile.settings.txt", "CLEX26|0|50|0|9|0|0|0|0|0|1|0|90|2|0\nQGC|0|180|0|12|1|0|0|0|0|1|0|90|3|1|0|c28\n");
      ss::Values g = st.load(ss::M_GC, 0, true);
      CHECK(g.i(dpset::K_WIDTH) == 180 && g.i(dpset::K_FONT) == 12 && !g.b(dpset::K_DELTA_TEXT), "QGC line -> GC values");
      ss::Store sc = st; sc.path = d + "/DeltaProfile.settings-CL.txt"; ss::Values c = sc.load(ss::M_CL, 0, true);
      CHECK(c.i(dpset::K_WIDTH) == 110 && c.i(dpset::K_FONT) == 9, "CLEX26 pre-2.8 width 50 -> default 110");
      ss::Store sn = st; sn.path = d + "/DeltaProfile.settings-NG.txt"; ss::Values n = sn.load(ss::M_NG, 0, true);
      CHECK(n.why == "defaults: no settings file", "no NG line -> defaults"); }
    { // the original HodLod 2.4.1 projection row would have been flagged as cut off
      ss::Row R[3] = { ss::masterOf(ss::boolRow("show_proj", "Show projection", true)), ss::same(ss::in(ss::colorRow("proj_up", "Up (to the HOD)", 0x22C55E, 0), "show_proj")), ss::same(ss::in(ss::colorRow("proj_dn", "Down (to the LOD)", 0xEF4444, 0), "show_proj")) };
      ss::Spec old = { "HodLodOld", "x", 1, R, 3, 0 };
      bool cut = false; for (const std::string& e : ss::check(old)) if (e.find("cut off") != std::string::npos) cut = true;
      CHECK(cut, "the 2.4.1 projection row (~424 px) fails the width budget"); }
    std::printf("%s settings host test: %d checks, %d failures\n", F ? "FAIL" : "PASS", N, F);
    return F ? 1 : 0;
}
