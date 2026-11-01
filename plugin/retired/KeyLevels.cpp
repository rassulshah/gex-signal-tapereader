/********************************************************************************
 *  KeyLevels.cpp  --  Investor/RT RTX extension  lsKeyLevels  1.0.0  (2026-10-20)
 *  Rassul 22:20-22:41 CT: "create a seperate levels indicator without the open .. i want to use it for my hourly charts", "it should
 *  have levels that run across the charts", "have an option to show them [MenthorQ] but only show the price and volume levels by
 *  default", "the levels don't need liquidity amounts they just need a name and a price level", "place the badges right justified".
 *  Draws, on any chart (made for 60-min), a thin line ACROSS the chart at each key level and a badge "<NAME>  <price>" right-justified
 *  at the right edge of the price area, in his SessionPrices colours:
 *    price levels  PFH/PFL teal, PDH/PDL orange, PWH/PWL blue, CWH/CWL soft blue, ONH/ONL grey, LonHI/LonLO amber, AsiaHI/AsiaLO purple
 *    volume levels DPoc cyan, VAH/VAL red, WPoc beige
 *    MenthorQ      (off by default) CR red, PS green, HVL yellow, other gamma cyan - at MenthorQ's own prices, the whole set
 *  No Open, no hourly swings, no liquidity, no odds. Levels on one price are one line ("PFH · PDH").
 *  Source: lsFlexLevels\LRA-KeyLevels-<MKT>.csv, which lsDealerProfile writes from the market's 3-min chart at every new bar - the hourly
 *  chart's own bars cannot split 8:30 from 8:00, so the 3-min numbers are used (the same levels to the tick). Needs lsDealerProfile on
 *  a 3-min chart of the market (the status file says when the file is missing or old). MenthorQ: lsFlexLevels\MenthorQLevels.csv.
 *  Nothing heavy: a file stat at most every 2 s; drawing only lines in the visible price range.
 ********************************************************************************/
#ifndef NOMINMAX
#define NOMINMAX
#endif
#if defined(_WIN32)
#include <windows.h>
#endif
#include "irtsdk.h"
#include "PerfGuard.h"   // (PerfGuard 1.0.0, 2026-10-26) calc / draw timing, memory, lag, budgets -> lsFlexLevels\Perf
#ifndef LSPERF_PLUGIN
#define LSPERF_PLUGIN "KeyLevels"
#endif
#include "HostSlot.h"
#include "KeyLevelsHSettings.h"
#include "KeyLevelsHLogic.h"
#include "KeepOutDraw.h"   // (1.2.0) BoxDock.h 1.2.0 keep-out: nothing painted over the HOD/LOD table
#include "DealerProfileAuditLogic.h"
#include "SettingsHost.h"
#include "SettingsGuard.h"   // the window calls behind a fault guard (IRT faulted in other plugins' parmsLoad / parmsApply on a copy with no chart)
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>

static const char* KL_VERSION = "1.2.1";   // (1.2.0, 2026-10-21 16:05 CT, Rassul "fix this, it should be solid") level lines stop at the lsHodLod table's edges, a badge that would touch it is not painted (KeepOutDraw.h); (1.1.0, 2026-10-21) one right edge; a crowded group shrinks its font, then spreads (klh::layoutStack)
static long long klSteadyMs() { return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static std::string klFlex() { const char* up = std::getenv("USERPROFILE"); return up && *up ? std::string(up) + KLH_SEP "InvestorRT" KLH_SEP "rtx" KLH_SEP "lsFlexLevels" : std::string(); }
static void klMakeDir(const std::string& p) {
#if defined(_WIN32)
    CreateDirectoryA(p.c_str(), NULL);
#else
    mkdir(p.c_str(), 0700);
#endif
}
static bool klStat(const std::string& p, long long& mt, long long& sz) {
    struct stat st; if (p.empty() || stat(p.c_str(), &st) != 0) return false;
    mt = (long long)st.st_mtime; sz = (long long)st.st_size; return true;
}
static bool klRead(const std::string& p, size_t cap, std::string& out) {
    std::ifstream f(p.c_str(), std::ios::binary); if (!f.is_open()) return false;
    out.assign(cap, '\0'); f.read(&out[0], (std::streamsize)cap); out.resize((size_t)f.gcount());
    return true;
}

struct KlState {
    std::string mkt, sym;
    struct File { std::string path; long long mt = -1, sz = -1; };
    File key, mq;
    std::vector<klh::KeyIn> keys; long long keyAt = 0; std::string keyNote = "not read yet";
    std::vector<klh::MqIn> mqAll; std::string mqSym, mqNote = "off";
    long long lastCheckMs = -100000;
    int shrunk = 0, spread = 0;   // (1.1.0) badges drawn smaller / spread off their line on the last draw
    ss::Store setStore; ss::Values set = ss::defaults(klhset::spec(), -1);
    std::string statusBody;
    bdk::TableWatch keep;   // (1.2.0) the HOD/LOD table on this chart
};
static ss::Ids& klIds() { static ss::Ids I; return I; }

class KeyLevels : public cppExtension {
public:
    SS_SAFE_DIALOG(KeyLevels, "KeyLevels")   // (SettingsGuard 1.1.0) every window call guarded + logged; a chart-less copy opens filled
    HostSlot<KlState> slot_;
    virtual int draw(void);
    virtual int parmsLoadBody(void) { ss::guardedCall(this, &KeyLevels::loadBody, 0u); return RTX_OK; }
    virtual int parmsApplyBody(void) { ss::guardedCall(this, &KeyLevels::applyBody, 0u); return RTX_OK; }
    virtual int parmsUpdtBody(unsigned int i) { ss::guardedCall(this, &KeyLevels::updtBody, i); return RTX_OK; }
    void loadBody(unsigned) {
        if (!ss::inDialog(this)) return;
        ss::Store* st = dialogStore(); if (!st) return;
        ss::fillDialog(this, klhset::spec(), klIds(), st->load(ss::marketIndex(dlgMkt_), klSteadyMs(), true), snap_);
    }
    void applyBody(unsigned) { saveSettings(); }
    void updtBody(unsigned i) { ss::onUpdate(this, klhset::spec(), klIds(), (int)i, snap_); saveSettings(); }
    KlState* stateFor() { return slot_.get(this, false); }   // tests
    std::string chartMarket() { return ss::safeMarket(this); }   // a copy with no chart = "" (never faults)
    void refresh(KlState& s);
    void writeStatus(KlState& s, const std::vector<klh::Line>& L, size_t drawn);
private:
    ss::Snap snap_; ss::Store dlgStore_; std::string dlgMkt_;
    ss::Store* dialogStore() {
        const std::string mkt = chartMarket(); const std::string p = klhset::settingsPath(klFlex(), mkt); if (p.empty()) return nullptr;
        if (dlgStore_.path != p) { dlgStore_ = ss::Store(); dlgStore_.S = &klhset::spec(); dlgStore_.path = p; dlgMkt_ = mkt; }
        return &dlgStore_;
    }
    void saveSettings() {
        if (!ss::inDialog(this)) return;
        ss::Store* st = dialogStore(); if (!st) return;
        const ss::Values V = ss::readDialog(this, klhset::spec(), klIds(), st->load(ss::marketIndex(dlgMkt_), klSteadyMs(), true), snap_);
        klMakeDir(klFlex());
        if (st->save(V)) snap_.take(ss::dialogNow(this, klhset::spec(), klIds(), V));
    }
};

// Re-read a file only when its stamp / size changed; at most every 2 s. A torn or garbled read keeps the last good levels.
void KeyLevels::refresh(KlState& s) {
    const long long now = klSteadyMs();
    if (now - s.lastCheckMs < 2000 && now >= s.lastCheckMs) return;
    s.lastCheckMs = now;
    const std::string f = klFlex();
    const std::string sp = klhset::settingsPath(f, s.mkt);
    if (s.setStore.path != sp) { s.setStore = ss::Store(); s.setStore.S = &klhset::spec(); s.setStore.path = sp; }
    s.set = sp.empty() ? ss::defaults(klhset::spec(), -1) : s.setStore.load(ss::marketIndex(s.mkt), now);
    const std::string kp = f.empty() ? std::string() : f + KLH_SEP "LRA-KeyLevels-" + s.mkt + ".csv";
    if (kp != s.key.path) { s.key = KlState::File(); s.key.path = kp; s.keys.clear(); s.keyAt = 0; }
    long long mt = -1, sz = -1;
    const long long unixNow = (long long)time(nullptr);
    if (!klStat(kp, mt, sz)) { s.keys.clear(); s.keyNote = "missing - open lsDealerProfile on a 3-min " + s.mkt + " chart"; }
    else if (mt != s.key.mt || sz != s.key.sz) {
        std::string text; std::vector<klh::KeyIn> v; long long at = 0;
        if (sz < 65536 && klRead(kp, 65536, text) && klh::parseKey(text, unixNow, 80LL * 3600, v, at)) { s.keys.swap(v); s.keyAt = at; s.keyNote = std::to_string(s.keys.size()) + " levels"; s.key.mt = mt; s.key.sz = sz; }
        else if (s.keys.empty()) s.keyNote = "unreadable or older than 80 h";
    }
    if (s.keys.size() && unixNow - s.keyAt > 80LL * 3600) { s.keys.clear(); s.keyNote = "older than 80 h"; }   // never draw a week-old set
    if (!s.set.b(klhset::K_SHOW_MQ)) { s.mqNote = "off"; return; }
    const std::string mp = f.empty() ? std::string() : f + KLH_SEP "MenthorQLevels.csv";
    if (mp != s.mq.path) { s.mq = KlState::File(); s.mq.path = mp; s.mqAll.clear(); }
    if (!klStat(mp, mt, sz)) { s.mqAll.clear(); s.mqNote = "MenthorQLevels.csv missing"; return; }
    if (mt != s.mq.mt || sz != s.mq.sz) {
        std::string text;
        if (sz < 1024 * 1024 && klRead(mp, 1024 * 1024, text)) { std::vector<klh::MqIn> v; klh::parseMq(text, v); if (!v.empty() || s.mqAll.empty()) s.mqAll.swap(v); s.mq.mt = mt; s.mq.sz = sz; }
    }
}

void KeyLevels::writeStatus(KlState& s, const std::vector<klh::Line>& L, size_t drawn) {
    const std::string d = klFlex(); if (d.empty() || s.mkt.empty()) return;
    std::ostringstream o;
    o << "VERSION," << KL_VERSION << "\nMARKET," << s.mkt << "\nSYMBOL," << s.sym << "\nKEY_LEVELS," << s.keyNote;
    if (s.keyAt > 0) o << ",written " << ((long long)time(nullptr) - s.keyAt) / 60 << " min ago";
    o << "\nMENTHORQ," << s.mqNote << "\nLEVELS," << L.size() << " to draw, " << drawn << " on screen\n";
    o << "BADGES,one right edge,smaller font " << s.shrunk << ",spread off their line " << s.spread << "\n";   // (1.1.0)
    for (const klh::Line& q : L) o << "L," << q.name << "," << dpaudit::priceText(s.mkt, q.price) << "\n";
    o << "SETTINGS," << s.set.why << ",MenthorQ " << (s.set.b(klhset::K_SHOW_MQ) ? "on" : "off") << ",font " << s.set.i(klhset::K_FONT) << ",width " << s.set.i(klhset::K_WIDTH) << "\n" << bdk::keepLine();
    const std::string body = o.str(); if (body == s.statusBody) return;
    klMakeDir(d);
    std::ofstream f((d + KLH_SEP "KeyLevels.status-" + s.mkt + ".txt").c_str(), std::ios::binary | std::ios::trunc);
    if (f.is_open()) { f << body; s.statusBody = body; }
}

int KeyLevels::draw(void) { LSPERF_SCOPE(LSPERF_PLUGIN, "draw");
    KlState* s = slot_.get(this, true);
    if (!s) return RTX_OK;
    try {
        s->mkt = chartMarket();
        { const char* sy = getSymbol(); s->sym = sy ? sy : ""; }
        if (s->mkt.empty()) return RTX_OK;
        refresh(*s);
        s->keep.refresh(getSymbol(), getSecondsPerBar(), getChartLabel());   // (1.2.0) the HOD/LOD table: nothing painted over it this draw
        bdk::KeepScope keepScope(s->keep);
        const ss::Values& V = s->set;
        // the chart's MenthorQ contract (the same picker as lsDealerProfile): its own contract under either spelling, else the nearest expiry
        std::vector<klh::MqIn> mq;
        if (V.b(klhset::K_SHOW_MQ) && !s->mqAll.empty()) {
            std::vector<std::string> keys; for (const klh::MqIn& q : s->mqAll) if (std::find(keys.begin(), keys.end(), q.sym) == keys.end()) keys.push_back(q.sym);
            bool shift = false; const std::string pick = dpaudit::pickMQSymbol(keys, s->sym, s->mkt, std::string(), shift);
            for (const klh::MqIn& q : s->mqAll) if (q.sym == pick) mq.push_back(q);
            s->mqNote = pick.empty() ? "no " + s->mkt + " levels in MenthorQLevels.csv" : pick + ", " + std::to_string(mq.size()) + " levels";
        }
        const std::vector<klh::Line> L = klh::build(s->keys, mq, V);
        const long n = getBarCount();
        if (n < 1 || L.empty()) { writeStatus(*s, L, 0); return RTX_OK; }
        RCT pane; pane.getPaneRect(false);
        FONT ft; ft.id = HELVETICA; ft.size = (short)std::max(6, std::min(20, V.i(klhset::K_FONT))); ft.style = BOLD; setFont(ft);
        const int lw = std::max(1, std::min(4, V.i(klhset::K_WIDTH)));
        const int baseFont = ft.size, minFont = std::min(baseFont, 7), padX = 4;
        std::vector<int> ys; std::vector<size_t> idx; std::vector<std::string> txt;
        for (size_t k = 0; k < L.size(); ++k) {
            PNT p; p.set((int)(n - 1), (float)L[k].price); const int y = p.v;
            if (y <= pane.top || y >= pane.bottom) continue;               // off screen: not drawn
            idx.push_back(k); ys.push_back(y); txt.push_back(L[k].name + "  " + dpaudit::priceText(s->mkt, L[k].price));
        }
        // (1.1.0, Rassul 09:26 CT) ONE right edge for every badge; a crowded group shrinks its font, then spreads up / down only if needed
        const std::vector<klh::Slot> slot = klh::layoutStack(ys, baseFont, minFont, pane.top + 1, pane.bottom - 1);
        for (size_t j = 0; j < idx.size(); ++j) {                          // the lines first, across the chart, at the TRUE price
            const COLOR col = (COLOR)L[idx[j]].col;
            setPen(col, (short)lw, P_SOLID);
            ko::line(pane.left, ys[j], pane.right, ys[j]);                 // (1.2.0) cut at the HOD/LOD table
        }
        const COLOR bg = getBackgroundColor();
        s->shrunk = s->spread = 0;
        for (size_t j = 0; j < idx.size(); ++j) {                          // then the badges on top: name + price, right-justified
            const COLOR col = (COLOR)L[idx[j]].col;
            FONT fj = ft; fj.size = (short)slot[j].font; setFont(fj);
            const int w = (int)getTextWidth(txt[j].c_str(), -1) + 2 * padX, h = slot[j].h, yc = slot[j].y;
            const int x1 = pane.right - 3, x0 = x1 - w;
            if (slot[j].font < baseFont) ++s->shrunk;
            if (yc != ys[j]) {                                             // spread off its line: a short tick from the badge to its line
                ++s->spread; setPen(col, 1, P_SOLID);
                ko::line(x0 - 6, ys[j], x0, yc);
            }
            RCT rc; rc.set((short)x0, (short)(yc - h / 2), (short)x1, (short)(yc + h / 2));
            if (ko::off(rc)) continue;                                      // (1.2.0) a badge under the HOD/LOD table: not painted
            rc.draw(1, col, bg, DRAW_OPAQUE, PAT_SOLID);
            setTextColor((COLOR)dpaudit::liftForText(col));                 // a dark line colour (his pure blue) lifted to read on black
            rc.drawText(txt[j].c_str(), true, false);
        }
        setFont(ft);
        writeStatus(*s, L, idx.size());
    } catch (...) {}
    return RTX_OK;
}

int cppExtension::init(void) { return RTX_OK; }
int cppExtension::done(void) { return RTX_OK; }
int cppExtension::destroy(void) { static_cast<KeyLevels*>(this)->slot_.release(this); return RTX_OK; }
int cppExtension::setup(void) { ss::registerAll(this, klhset::spec(), klIds()); return RTX_OK; }
int cppExtension::calc(int) { return RTX_OK; }

extern "C" cppExtension* CreateExtension(void) {
    KeyLevels* p = new KeyLevels();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | FRONT_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE);
    p->setDescription("LRA Key Levels - for hourly charts. A line across the chart at each key level, with its name and price in a badge at the right edge. Price levels: prior full (PFH / PFL), prior day (PDH / PDL), prior week (PWH / PWL), this week (CWH / CWL), overnight (ONH / ONL), London (LonHI / LonLO), Asia (AsiaHI / AsiaLO). Volume levels: prior day POC (DPoc), value area (VAH / VAL), prior week POC (WPoc). MenthorQ levels (CR / PS / HVL / G) are off by default - tick the box to show them. No Open. Levels on one price share one line (PFH \xB7 PDH). The numbers are the ones lsDealerProfile computes on the market's 3-min chart (it must be open). Example: ES 60-min - an orange line at 7710.25 with the badge PDL  7710.25.");
    p->setVersion("1.2.1");   // KL_VERSION
    return p;
}
