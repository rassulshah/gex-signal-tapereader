/********************************************************************************
 *  DealerRead.cpp  --  Investor/RT RTX extension  lsDealerRead  (v1.0, 2026-09-29)
 *
 *  THE DEALER READ: one compact strip docked in a corner of the price pane (mockup v11, ~1,470 x 176 px at font 10):
 *    left    the key level, the phase tabs APPROACH / SWEEP / RECLAIM / RETEST / TRADE, and WHAT TO DO
 *    top     the TRADE line (stop, target, R:R, record) and the flag chips (news, data age)
 *    middle  the LEVEL checklist, titled by its MenthorQ level ("CR0 4,200 - 0DTE CALL RESISTANCE"): SPEED, COLOR, ZOMMA
 *    right   the FUEL checklist: COVER, GAMMA, VANNA
 *    each row: a check box (ok / bad / watch / waiting), the greek, a centred meter (- weaker | + stronger) with its value,
 *    and the reason in plain words ("walking into gamma peak 4,210").
 *  Dashed border = still learning for this market (every checklist is scored nightly before it is trusted).
 *
 *  Data: %USERPROFILE%\InvestorRT\rtx\lsFlexLevels\LRA-Dealer-<MKT>.csv (LRA analytics/lra/dealer_irt.py); grammar in
 *  DealerLogic.h. Parameters read only in the parms callbacks. Never black lines: the chart is black.
 ********************************************************************************/
#include "irtsdk.h"
#include "DealerLogic.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>

static const COLOR C_BLUE  = 0x003987E5;
static const COLOR C_AMBER = 0x00D99A1E;
static const COLOR C_GREEN = 0x0022C55E;
static const COLOR C_RED   = 0x00EF4444;
static const COLOR C_YEL   = 0x00FCD34D;
static const COLOR C_GREY  = 0x0064748B;
static const COLOR C_INK   = 0x00E5E7EB;
static const COLOR C_MUTED = 0x009CA3AF;
static const COLOR C_GROUND = 0x00000000;   // the strip's FILL (matches the chart); no line is ever this colour
static const COLOR C_BOXBG = 0x0005070C;
static const COLOR C_TRACK = 0x001F2937;
static const COLOR C_DOBG  = 0x000B1A2E;
static const COLOR C_DOBRD = 0x001E3A5F;
static const COLOR C_DOTXT = 0x00BFDBFE;
static const COLOR C_TABON = 0x001E3A5F;
static const COLOR C_TABONB = 0x0093C5FD;
static const COLOR C_TABOFF = 0x000B0F19;
static const COLOR C_TABOFB = 0x00374151;
static const COLOR C_BORDER = 0x0064748B;

static COLOR colOf(char c) { return c == 'G' ? C_GREEN : c == 'R' ? C_RED : c == 'Y' ? C_YEL : c == 'A' ? C_AMBER : c == 'B' ? C_BLUE : c == 'N' ? C_MUTED : C_INK; }
static COLOR stCol(const std::string& st) { return st == "ok" ? C_GREEN : st == "bad" ? C_RED : st == "watch" ? C_YEL : C_GREY; }
static const char* stWord(const std::string& st, bool wall)
{
    if (st == "ok") return wall ? "STRONGER" : "HELPING";
    if (st == "bad") return wall ? "WEAKER" : "AGAINST";
    if (st == "watch") return "WATCH";
    return "WAITING";
}

struct PIdx { int market, corner, font, trade, todo, clock, layout; };
static PIdx PX;
struct Settings { int market = 0, corner = 0, font = 10, clock = 0, layout = 0; bool trade = true, todo = true; };   // layout 0 Stacked, 1 Compact, 2 Mini, 3 Full

class DealerRead : public cppExtension {
public:
    DealerRead();
    virtual int parmsLoad(void);
    virtual int parmsApply(void);
    virtual int parmsUpdt(unsigned int iParmNumber);
    virtual int draw(void);

    Settings cfg;
    dl::Data D;
    std::string mkt, root;
    float u;   // scale: font / 10

    bool dialogReady();
    void readSettings(Settings& S);
    void load();
    void render(const Settings& S);
    void renderFull(const Settings& S);
    void renderSmall(const Settings& S);
    void miniBoxes(short& x, short y, const std::vector<dl::Row>& rows, short sz);
    std::string fit(const std::string& s, int maxW, int sz, bool bold);
    short U(float v) { return (short)(v * u + 0.5f); }
    void fill(short l, short t, short r, short b, COLOR c);
    void frame(short l, short t, short r, short b, COLOR c, bool dashed);
    void line(short x1, short y1, short x2, short y2, COLOR c, int w);
    int  textW(const char* s, int sz, bool bold);
    void text(short x, short y, const char* s, COLOR col, int sz, bool bold, int just = 0);   // 0 left, 1 centre, 2 right; y = middle
    void checkBox(short x, short y, short sz, const std::string& st);
    void meter(short x, short y, short w, short h, const dl::Row& r);
    void checklist(short x, short y, short w, short h, const std::string& title, COLOR tcol, const std::string& score, char scol,
                   const std::vector<dl::Row>& rows, bool wall, int fs);
    void writeStatus(const char* what);
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::calc(int)     { return RTX_OK; }
int cppExtension::done(void)    { return RTX_OK; }
int cppExtension::destroy(void) { return RTX_OK; }

DealerRead::DealerRead() : cppExtension() { u = 1.0f; }
bool DealerRead::dialogReady() { int i = getListIndex(PX.market); return i >= 0 && i <= 7; }
int DealerRead::parmsLoad(void)  { if (dialogReady()) readSettings(cfg); return RTX_OK; }
int DealerRead::parmsApply(void) { if (dialogReady()) readSettings(cfg); return RTX_OK; }
int DealerRead::parmsUpdt(unsigned int) { if (dialogReady()) readSettings(cfg); return RTX_OK; }

int cppExtension::setup(void)
{
    setParameterVersion(1);
    setParameterDialogHeight(10);
    const short SL = kParmAppendSameLine;
    int pc = 0;
    // IRT keeps a saved instance's values BY POSITION: append new rows at the end, never reorder
    PX.market = pc++; setListParameter("Market", 0, "Auto;ES;NQ;CL;GC;HG;NG;EU");
    PX.corner = pc++; setListParameter("Dock at", 0, "Bottom left;Bottom right;Top left;Top right", 0, SL);
    PX.font   = pc++; setIntegerParameter("Font size (pt)", 10, 0);
    PX.trade  = pc++; setBoolParameter("TRADE line", true, SL);
    PX.todo   = pc++; setBoolParameter("WHAT TO DO box", true);
    PX.clock  = pc++; setIntegerParameter("Clock offset (min)", 0, 0, SL);
    PX.layout = pc++; setListParameter("Layout", 0, "Stacked;Compact;Mini;Full (wide)");      // (1.1) appended LAST: saved positions stay put
    return RTX_OK;
}

void DealerRead::readSettings(Settings& S)
{
    S.market = getListIndex(PX.market);
    S.corner = getListIndex(PX.corner); if (S.corner < 0 || S.corner > 3) S.corner = 0;
    S.font = getIntegerValue(PX.font); if (S.font < 7) S.font = 10;   // (1.0.1) a first dialog can show ??? / a wrong number if (S.font > 20) S.font = 20;
    S.trade = isBoxChecked(PX.trade) != 0; S.todo = isBoxChecked(PX.todo) != 0;
    S.clock = getIntegerValue(PX.clock); if (S.clock < -720) S.clock = -720; if (S.clock > 720) S.clock = 720;
    S.layout = getListIndex(PX.layout); if (S.layout < 0 || S.layout > 3) S.layout = 0;
}

void DealerRead::load()
{
    D = dl::Data();
    char buf[32] = {0};
    const char* rs = getRootSymbol(buf);
    root = rs ? rs : "";
    mkt = dl::marketFor(cfg.market, root);
    if (mkt.empty()) return;
    const char* up = getenv("USERPROFILE"); if (!up) return;
    std::string path = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\LRA-Dealer-" + mkt + ".csv";
    std::ifstream f(path.c_str()); if (!f.is_open()) return;
    std::stringstream ss; ss << f.rdbuf();
    D = dl::parseText(ss.str());
}

void DealerRead::fill(short l, short t, short r, short b, COLOR c) { RCT rc; rc.set(l, t, r, b); rc.draw(0, c, c, DRAW_OPAQUE, PAT_SOLID); }
void DealerRead::line(short x1, short y1, short x2, short y2, COLOR c, int w)
{
    setPen(c, (short)w, P_SOLID);
    PNT a; a.set(0, 0.0f); a.h = x1; a.v = y1; a.setDrawPosition();
    PNT b; b.set(0, 0.0f); b.h = x2; b.v = y2; b.drawLineTo();
}
void DealerRead::frame(short l, short t, short r, short b, COLOR c, bool dashed)
{
    setPen(c, 1, dashed ? P_DOT : P_SOLID);
    PNT p; p.set(0, 0.0f);
    p.h = l; p.v = t; p.setDrawPosition(); p.h = r; p.drawLineTo(); p.v = b; p.drawLineTo(); p.h = l; p.drawLineTo(); p.v = t; p.drawLineTo();
}
int DealerRead::textW(const char* s, int sz, bool bold)
{
    FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f);
    return (int)getTextWidth(s, -1);
}
void DealerRead::text(short x, short y, const char* s, COLOR col, int sz, bool bold, int just)
{
    FONT f; f.id = HELVETICA; f.size = (short)sz; f.style = bold ? BOLD : PLAIN; setFont(f);
    setTextColor(col);
    short yy = (short)(y - (short)(sz * 0.45f + 0.5f));        // the rect draw baselines low (GammaProfile v0.73)
    int w = (int)getTextWidth(s, -1);
    short l = just == 0 ? x : (just == 1 ? (short)(x - w / 2) : (short)(x - w));
    RCT rc; rc.set(l, (short)(yy - sz), (short)(l + w + 4), (short)(yy + sz));
    rc.drawText(s, false, false);
}
void DealerRead::checkBox(short x, short y, short sz, const std::string& st)
{
    COLOR c = stCol(st);
    frame(x, (short)(y - sz / 2), (short)(x + sz), (short)(y + sz / 2), c, false);
    short m = (short)(sz / 4 > 2 ? sz / 4 : 2);
    if (st == "ok") {        // a tick
        line((short)(x + m), y, (short)(x + sz / 2 - 1), (short)(y + sz / 2 - m), c, 2);
        line((short)(x + sz / 2 - 1), (short)(y + sz / 2 - m), (short)(x + sz - m), (short)(y - sz / 2 + m), c, 2);
    } else if (st == "bad") { // a cross
        line((short)(x + m), (short)(y - sz / 2 + m), (short)(x + sz - m), (short)(y + sz / 2 - m), c, 2);
        line((short)(x + m), (short)(y + sz / 2 - m), (short)(x + sz - m), (short)(y - sz / 2 + m), c, 2);
    } else if (st == "watch") {
        text((short)(x + sz / 2), y, "!", c, (int)(sz * 0.8f), true, 1);
    } else {
        fill((short)(x + sz / 2 - 1), (short)(y - 1), (short)(x + sz / 2 + 1), (short)(y + 1), c);
    }
}
void DealerRead::meter(short x, short y, short w, short h, const dl::Row& r)
{
    fill(x, (short)(y - h / 2), (short)(x + w), (short)(y + h / 2), C_TRACK);
    short mid = (short)(x + w / 2);
    int fs = (int)(8 * u + 0.5f);
    if (!r.hasV) { text(mid, y, r.lab.c_str(), C_MUTED, fs, false, 1); return; }
    short ln = (short)(w / 2 * std::fabs(r.v) / 100.0f);
    COLOR c = r.v >= 0 ? C_GREEN : C_RED;
    if (r.v >= 0) fill(mid, (short)(y - h / 2), (short)(mid + ln), (short)(y + h / 2), c);
    else          fill((short)(mid - ln), (short)(y - h / 2), mid, (short)(y + h / 2), c);
    line(mid, (short)(y - h / 2 - 1), mid, (short)(y + h / 2 + 1), 0x0094A3B8, 1);
    int tw = textW(r.lab.c_str(), fs, true);
    if (r.v >= 0) {
        if (ln > tw + 6) text((short)(mid + ln - 3), y, r.lab.c_str(), 0x00052E16, fs, true, 2);
        else text((short)(mid + ln + 3), y, r.lab.c_str(), C_INK, fs, true, 0);
    } else text((short)(mid - ln - 3), y, r.lab.c_str(), C_INK, fs, true, 2);
}
void DealerRead::checklist(short x, short y, short w, short h, const std::string& title, COLOR tcol, const std::string& score, char scol,
                           const std::vector<dl::Row>& rows, bool wall, int fs)
{
    fill(x, y, (short)(x + w), (short)(y + h), C_BOXBG);
    frame(x, y, (short)(x + w), (short)(y + h), tcol, false);
    short ty = (short)(y + U(16));
    text((short)(x + U(10)), ty, title.c_str(), tcol, fs + 1, true, 0);
    text((short)(x + w - U(10)), ty, score.c_str(), colOf(scol), fs + 1, true, 2);
    short ry = (short)(y + U(48));
    for (size_t i = 0; i < rows.size() && i < 3; i++) {
        const dl::Row& r = rows[i];
        checkBox((short)(x + U(10)), ry, U(16), r.st);
        text((short)(x + U(34)), ry, r.n.c_str(), C_INK, fs - 1, true, 0);
        meter((short)(x + U(96)), ry, U(130), U(14), r);
        std::string why = std::string(stWord(r.st, wall)) + "  " + r.why;
        // the effect word in its colour, then the cause in ink
        const char* word = stWord(r.st, wall);
        text((short)(x + U(236)), ry, word, stCol(r.st), fs - 1, true, 0);
        int ww = textW(word, fs - 1, true);
        std::string cause = r.why;
        int room = (int)(x + w - U(8)) - (int)(x + U(236) + ww + U(6));
        while (!cause.empty() && textW(cause.c_str(), fs - 1, false) > room) cause = cause.substr(0, cause.size() - 1);
        text((short)(x + U(236) + ww + U(6)), ry, cause.c_str(), C_INK, fs - 1, false, 0);
        ry = (short)(ry + U(34));
    }
}

void DealerRead::renderFull(const Settings& S)
{
    u = S.font / 10.0f;
    int fs = S.font;
    RCT pane; pane.getPaneRect(false);
    RCT scale; scale.getScaleRect();
    short paneR = pane.right;
    if (scale.left > pane.left && scale.left < pane.right && scale.right >= scale.left) paneR = (short)(scale.left - 2);
    bool top = S.trade;
    short W = U(1466), H = top ? U(176) : U(150);
    if (!D.hasLevel) { W = U(900); H = U(44); }
    short x0 = (S.corner == 1 || S.corner == 3) ? (short)(paneR - W - U(6)) : (short)(pane.left + U(6));
    short y0 = (S.corner <= 1) ? (short)(pane.bottom - H - U(6)) : (short)(pane.top + U(6));
    fill(x0, y0, (short)(x0 + W), (short)(y0 + H), C_GROUND);
    frame(x0, y0, (short)(x0 + W), (short)(y0 + H), C_BORDER, D.learn);

    char b[200];
    if (!D.hasPrice) {
        sprintf_s(b, sizeof(b), "Dealer Read: no data for %s (root %s) - is the LRA Reader running?", mkt.empty() ? "?" : mkt.c_str(), root.c_str());
        text((short)(x0 + U(10)), (short)(y0 + H / 2), b, C_MUTED, fs, false, 0);
        return;
    }
    if (!D.hasLevel) {
        sprintf_s(b, sizeof(b), "%s  Dealer Read - no key level within 0.6 EM", mkt.c_str());
        text((short)(x0 + U(10)), (short)(y0 + U(14)), b, C_INK, fs, true, 0);
        if (!D.bookLine.empty()) text((short)(x0 + U(10)), (short)(y0 + U(32)), D.bookLine.c_str(), D.book < 0 ? C_AMBER : C_BLUE, fs - 1, false, 0);
        return;
    }
    // ---- left column
    sprintf_s(b, sizeof(b), "%s %s  %s", mkt.c_str(), dl::fmtPx(D.lvlPx, mkt).c_str(), D.lvlLabel.c_str());
    short ly = (short)(y0 + U(18));
    text((short)(x0 + U(10)), ly, b, C_INK, fs + 1, true, 0);
    text((short)(x0 + U(10)), (short)(ly + U(22)), D.ctx.c_str(), C_MUTED, fs - 1, false, 0);
    short tabY = (short)(ly + U(44)), tabW = U(72), tabH = U(18);
    for (int i = 0; i < 5; i++) {
        short tx = (short)(x0 + U(10) + i * (tabW + U(3)));
        bool on = i == D.phase, done = i < D.phase;
        fill(tx, (short)(tabY - tabH / 2), (short)(tx + tabW), (short)(tabY + tabH / 2), on ? C_TABON : C_TABOFF);
        frame(tx, (short)(tabY - tabH / 2), (short)(tx + tabW), (short)(tabY + tabH / 2), on ? C_TABONB : C_TABOFB, false);
        text((short)(tx + tabW / 2), tabY, dl::phaseName(i), on ? C_INK : (done ? C_GREEN : C_MUTED), fs - 3, true, 1);
    }
    if (S.todo) {
        short dx = (short)(x0 + U(10)), dy = (short)(tabY + tabH / 2 + U(8)), dw = U(366), dh = (short)(y0 + H - U(8) - dy);
        fill(dx, dy, (short)(dx + dw), (short)(dy + dh), C_DOBG);
        frame(dx, dy, (short)(dx + dw), (short)(dy + dh), C_DOBRD, false);
        text((short)(dx + U(8)), (short)(dy + U(10)), "WHAT TO DO", C_MUTED, fs - 3, true, 0);
        int maxW = dw - U(16);
        struct Wd { DealerRead* s; int f; int operator()(const std::string& t) const { return s->textW(t.c_str(), f, true); } } wd = { this, fs - 1 };
        std::vector<std::string> ls = dl::wrap(D.doText, maxW, wd);
        short yy = (short)(dy + U(26));
        for (size_t i = 0; i < ls.size() && yy < dy + dh - U(4); i++) { text((short)(dx + U(8)), yy, ls[i].c_str(), C_DOTXT, fs - 1, true, 0); yy = (short)(yy + U(15)); }
    }
    // ---- the two checklists (and the TRADE line on top of them)
    short cx = (short)(x0 + U(390)), cw = U(530), gap = U(8);
    short cy = (short)(y0 + (top ? U(30) : U(6))), ch = (short)(y0 + H - U(6) - cy);
    if (top) {
        short bx = cx, bw = (short)(2 * cw + gap), by = (short)(y0 + U(4)), bh = U(22);
        fill(bx, by, (short)(bx + bw), (short)(by + bh), C_DOBG);
        frame(bx, by, (short)(bx + bw), (short)(by + bh), C_DOBRD, false);
        short my = (short)(by + bh / 2), tx = (short)(bx + U(10));
        text(tx, my, "TRADE", C_MUTED, fs - 2, true, 0); tx = (short)(tx + U(48));
        for (size_t i = 0; i < D.trade.size(); i++) {
            text(tx, my, D.trade[i].k.c_str(), C_MUTED, fs - 2, false, 0); tx = (short)(tx + textW(D.trade[i].k.c_str(), fs - 2, false) + U(6));
            text(tx, my, D.trade[i].v.c_str(), colOf(D.trade[i].col), fs - 1, true, 0); tx = (short)(tx + textW(D.trade[i].v.c_str(), fs - 1, true) + U(18));
        }
        short fx = (short)(bx + bw - U(6));
        for (int i = (int)D.flags.size() - 1; i >= 0; i--) {
            int w = textW(D.flags[(size_t)i].k.c_str(), fs - 2, true) + U(14);
            fx = (short)(fx - w);
            COLOR c = colOf(D.flags[(size_t)i].col);
            frame(fx, (short)(my - U(8)), (short)(fx + w), (short)(my + U(8)), c, false);
            text((short)(fx + w / 2), my, D.flags[(size_t)i].k.c_str(), c, fs - 2, true, 1);
            fx = (short)(fx - U(4));
        }
    }
    COLOR wcol = (D.wallCol == 'A' || D.wall.empty()) ? C_AMBER : C_BLUE;
    checklist(cx, cy, cw, ch, D.wallTitle, wcol, D.wallScore, D.wallCol, D.wall, true, fs);
    checklist((short)(cx + cw + gap), cy, cw, ch, "FUEL", C_AMBER, D.fuelScore, D.fuelCol, D.fuel, false, fs);
    // stale
    RTDATE now = currentDate(); struct tm t; memset(&t, 0, sizeof(t)); getLocaltime(now, &t);
    double age = dl::staleMin(D.asofSo + S.clock * 60.0, t.tm_hour * 3600.0 + t.tm_min * 60.0 + t.tm_sec);
    if (age > 10.0) {
        sprintf_s(b, sizeof(b), "STALE %dm", (int)(age + 0.5));
        text((short)(x0 + U(376)), (short)(y0 + U(18)), b, C_RED, fs, true, 2);
    }
}


// ---- (1.1) operator 2026-09-29: "too much space" -> D STACKED is the default: the level on its own row on top, one line of
// WHAT TO DO, then the two checklists side by side; the strip fits the pane width. Compact (2 lines) / Mini (1 line) too.
std::string DealerRead::fit(const std::string& s, int maxW, int sz, bool bold)
{
    std::string t = s;
    if (maxW <= 0) return "";
    while (!t.empty() && textW(t.c_str(), sz, bold) > maxW) t = t.substr(0, t.size() - 1);
    return t;
}
void DealerRead::miniBoxes(short& x, short y, const std::vector<dl::Row>& rows, short sz)
{
    for (size_t i = 0; i < rows.size() && i < 3; i++) { checkBox(x, y, sz, rows[i].st); x = (short)(x + sz + U(4)); }
}
void DealerRead::render(const Settings& S) { if (S.layout == 3) renderFull(S); else renderSmall(S); }

void DealerRead::renderSmall(const Settings& S)
{
    u = S.font / 10.0f;
    int fs = S.font;
    RCT pane; pane.getPaneRect(false);
    RCT scale; scale.getScaleRect();
    short paneR = pane.right;
    if (scale.left > pane.left && scale.left < pane.right && scale.right >= scale.left) paneR = (short)(scale.left - 2);
    short avail = (short)(paneR - pane.left - U(12));
    short W = U(860); if (W > avail) W = avail; if (W < U(400)) W = U(400);
    short H = S.layout == 0 ? U(150) : (S.layout == 1 ? U(44) : U(22));
    if (!D.hasLevel || !D.hasPrice) H = U(22);
    short x0 = (S.corner == 1 || S.corner == 3) ? (short)(paneR - W - U(6)) : (short)(pane.left + U(6));
    short y0 = (S.corner <= 1) ? (short)(pane.bottom - H - U(6)) : (short)(pane.top + U(6));
    fill(x0, y0, (short)(x0 + W), (short)(y0 + H), C_GROUND);
    frame(x0, y0, (short)(x0 + W), (short)(y0 + H), C_BORDER, D.learn);
    char b[200];
    // stale (drawn at the right end of the first line)
    RTDATE now = currentDate(); struct tm tt; memset(&tt, 0, sizeof(tt)); getLocaltime(now, &tt);
    double age = dl::staleMin(D.asofSo + S.clock * 60.0, tt.tm_hour * 3600.0 + tt.tm_min * 60.0 + tt.tm_sec);
    std::string stale;
    if (D.hasPrice && age > 10.0) { if (age >= 90) sprintf_s(b, sizeof(b), "STALE %dh", (int)(age / 60 + 0.5)); else sprintf_s(b, sizeof(b), "STALE %dm", (int)(age + 0.5)); stale = b; }
    if (!D.hasPrice) {
        sprintf_s(b, sizeof(b), "Dealer Read: no data for %s (root %s)", mkt.empty() ? "?" : mkt.c_str(), root.c_str());
        text((short)(x0 + U(8)), (short)(y0 + H / 2), b, C_MUTED, fs - 1, false, 0);
        return;
    }
    if (!D.hasLevel) {
        sprintf_s(b, sizeof(b), "%s  no key level within 0.6 EM   -   %s", mkt.c_str(), D.bookLine.c_str());
        text((short)(x0 + U(8)), (short)(y0 + H / 2), fit(b, W - U(90), fs - 1, false).c_str(), C_MUTED, fs - 1, false, 0);
        if (!stale.empty()) text((short)(x0 + W - U(8)), (short)(y0 + H / 2), stale.c_str(), C_RED, fs - 1, true, 2);
        return;
    }
    std::string trade;
    for (size_t i = 0; i < D.trade.size(); i++) {
        const std::string& k = D.trade[i].k;
        if (k == "RECORD" || k == "SWEEP ROOM") continue;
        std::string v = D.trade[i].v; size_t zp = v.find(" zone edge"); if (zp != std::string::npos) v = v.substr(0, zp);
        size_t pp = v.find(" ("); if (k == "TARGET" && pp != std::string::npos) v = v.substr(0, pp);
        trade += (trade.empty() ? "" : "  ") + std::string(k == "TARGET" ? "TGT" : k.c_str()) + " " + v;
    }
    sprintf_s(b, sizeof(b), "%s %s  %s", mkt.c_str(), dl::fmtPx(D.lvlPx, mkt).c_str(), D.lvlLabel.c_str());
    std::string lvl = b;
    COLOR wcol = (D.wallCol == 'A' || D.wall.empty()) ? C_AMBER : C_BLUE;
    short box = U(12);

    if (S.layout == 2) {                                   // ---- MINI: one line
        short y = (short)(y0 + H / 2), x = (short)(x0 + U(8));
        sprintf_s(b, sizeof(b), "%s - %s", lvl.c_str(), dl::phaseName(D.phase));
        text(x, y, b, C_INK, fs - 1, true, 0); x = (short)(x + textW(b, fs - 1, true) + U(12));
        text(x, y, "LEVEL", wcol, fs - 1, true, 0); x = (short)(x + textW("LEVEL", fs - 1, true) + U(6));
        miniBoxes(x, y, D.wall, box); x = (short)(x + U(4));
        std::string ws = D.wallScore; text(x, y, ws.c_str(), colOf(D.wallCol), fs - 1, true, 0); x = (short)(x + textW(ws.c_str(), fs - 1, true) + U(14));
        text(x, y, "FUEL", C_AMBER, fs - 1, true, 0); x = (short)(x + textW("FUEL", fs - 1, true) + U(6));
        miniBoxes(x, y, D.fuel, box); x = (short)(x + U(4));
        text(x, y, D.fuelScore.c_str(), colOf(D.fuelCol), fs - 1, true, 0); x = (short)(x + textW(D.fuelScore.c_str(), fs - 1, true) + U(14));
        int room = (int)(x0 + W - U(10) - (stale.empty() ? 0 : textW(stale.c_str(), fs - 1, true) + U(10))) - x;
        text(x, y, fit(trade, room, fs - 2, false).c_str(), C_INK, fs - 2, false, 0);
        if (!stale.empty()) text((short)(x0 + W - U(8)), y, stale.c_str(), C_RED, fs - 1, true, 2);
        return;
    }
    if (S.layout == 1) {                                   // ---- COMPACT: two lines
        short y1 = (short)(y0 + U(12)), y2 = (short)(y0 + U(32)), x = (short)(x0 + U(8));
        text(x, y1, lvl.c_str(), C_INK, fs - 1, true, 0); x = (short)(x + textW(lvl.c_str(), fs - 1, true) + U(8));
        text(x, y1, dl::phaseName(D.phase), C_TABONB, fs - 2, true, 0); x = (short)(x + textW(dl::phaseName(D.phase), fs - 2, true) + U(12));
        std::string wt = fit(D.wallTitle, U(170), fs - 2, true);
        text(x, y1, wt.c_str(), wcol, fs - 2, true, 0); x = (short)(x + textW(wt.c_str(), fs - 2, true) + U(8));
        for (size_t i = 0; i < D.wall.size() && i < 3; i++) { checkBox(x, y1, box, D.wall[i].st); x = (short)(x + box + U(3)); text(x, y1, D.wall[i].n.c_str(), C_MUTED, fs - 3, false, 0); x = (short)(x + textW(D.wall[i].n.c_str(), fs - 3, false) + U(8)); }
        text(x, y1, "FUEL", C_AMBER, fs - 2, true, 0); x = (short)(x + textW("FUEL", fs - 2, true) + U(6));
        for (size_t i = 0; i < D.fuel.size() && i < 3; i++) { checkBox(x, y1, box, D.fuel[i].st); x = (short)(x + box + U(3)); text(x, y1, D.fuel[i].n.c_str(), C_INK, fs - 3, false, 0); x = (short)(x + textW(D.fuel[i].n.c_str(), fs - 3, false) + U(8)); }
        text((short)(x0 + W - U(8)), y1, D.fuelScore.c_str(), colOf(D.fuelCol), fs - 2, true, 2);
        x = (short)(x0 + U(8));
        text(x, y2, trade.c_str(), C_INK, fs - 2, false, 0); x = (short)(x + textW(trade.c_str(), fs - 2, false) + U(14));
        int room = (int)(x0 + W - U(10) - (stale.empty() ? 0 : textW(stale.c_str(), fs - 2, true) + U(10))) - x;
        text(x, y2, fit(">> " + D.doText, room, fs - 2, true).c_str(), C_DOTXT, fs - 2, true, 0);
        if (!stale.empty()) text((short)(x0 + W - U(8)), y2, stale.c_str(), C_RED, fs - 2, true, 2);
        return;
    }
    // ---- STACKED (D): row 1 the level, row 2 WHAT TO DO, then the two checklists side by side
    short r1t = (short)(y0 + U(4)), r1h = U(22), y1 = (short)(r1t + r1h / 2);
    fill((short)(x0 + U(4)), r1t, (short)(x0 + W - U(4)), (short)(r1t + r1h), C_DOBG);
    frame((short)(x0 + U(4)), r1t, (short)(x0 + W - U(4)), (short)(r1t + r1h), C_DOBRD, false);
    short x = (short)(x0 + U(10));
    text(x, y1, lvl.c_str(), C_INK, fs - 1, true, 0); x = (short)(x + textW(lvl.c_str(), fs - 1, true) + U(14));
    short tabW = U(58), tabH = U(14);
    for (int i = 0; i < 5; i++) {
        bool on = i == D.phase, done = i < D.phase;
        fill(x, (short)(y1 - tabH / 2), (short)(x + tabW), (short)(y1 + tabH / 2), on ? C_TABON : C_TABOFF);
        frame(x, (short)(y1 - tabH / 2), (short)(x + tabW), (short)(y1 + tabH / 2), on ? C_TABONB : C_TABOFB, false);
        text((short)(x + tabW / 2), y1, dl::phaseName(i), on ? C_INK : (done ? C_GREEN : C_MUTED), fs - 3, true, 1);
        x = (short)(x + tabW + U(3));
    }
    x = (short)(x + U(12));
    // flags (news) and stale at the right end, the trade numbers between
    short rx = (short)(x0 + W - U(10));
    if (!stale.empty()) { text(rx, y1, stale.c_str(), C_RED, fs - 2, true, 2); rx = (short)(rx - textW(stale.c_str(), fs - 2, true) - U(10)); }
    for (int i = (int)D.flags.size() - 1; i >= 0; i--) {
        const dl::KV& f = D.flags[(size_t)i];
        if (f.col == 'G') continue;                    // only what needs attention (a news release soon, old data)
        text(rx, y1, f.k.c_str(), colOf(f.col), fs - 2, true, 2); rx = (short)(rx - textW(f.k.c_str(), fs - 2, true) - U(10));
    }
    text(x, y1, fit(trade, rx - x, fs - 2, false).c_str(), C_INK, fs - 2, false, 0);
    short y2 = (short)(r1t + r1h + U(12));
    if (S.todo) text((short)(x0 + U(10)), y2, fit(">> " + D.doText, W - U(20), fs - 2, true).c_str(), C_DOTXT, fs - 2, true, 0);
    short ct = (short)(y2 + U(10)), cb = (short)(y0 + H - U(4)), cw = (short)((W - U(12)) / 2);
    for (int k = 0; k < 2; k++) {
        short cx = (short)(x0 + U(4) + k * (cw + U(4)));
        const std::vector<dl::Row>& rows = k == 0 ? D.wall : D.fuel;
        COLOR tc = k == 0 ? wcol : C_AMBER;
        std::string sc = k == 0 ? D.wallScore : D.fuelScore; char scol = k == 0 ? D.wallCol : D.fuelCol;
        fill(cx, ct, (short)(cx + cw), cb, C_BOXBG);
        frame(cx, ct, (short)(cx + cw), cb, tc, false);
        short ty = (short)(ct + U(11));
        int scW = textW(sc.c_str(), fs - 1, true);
        std::string title = fit(k == 0 ? D.wallTitle : std::string("FUEL"), cw - scW - U(24), fs - 1, true);
        text((short)(cx + U(6)), ty, title.c_str(), tc, fs - 1, true, 0);
        text((short)(cx + cw - U(6)), ty, sc.c_str(), colOf(scol), fs - 1, true, 2);
        short ry = (short)(ct + U(33));
        for (size_t i = 0; i < rows.size() && i < 3; i++) {
            const dl::Row& r = rows[i];
            checkBox((short)(cx + U(6)), ry, box, r.st);
            text((short)(cx + U(24)), ry, r.n.c_str(), C_INK, fs - 2, true, 0);
            meter((short)(cx + U(78)), ry, U(84), U(10), r);
            short wx = (short)(cx + U(172));
            text(wx, ry, fit(r.why, cw - U(178), fs - 2, false).c_str(), C_INK, fs - 2, false, 0);
            ry = (short)(ry + U(24));
        }
    }
}

void DealerRead::writeStatus(const char* what)
{
    const char* up = getenv("USERPROFILE"); if (!up) return;
    std::string path = std::string(up) + "\\InvestorRT\\rtx\\lsFlexLevels\\DealerRead.status.txt";
    std::ofstream f(path.c_str(), std::ios::trunc); if (!f.is_open()) return;
    f << "VERSION,1.1\nROOT," << root << "\nMARKET," << mkt << "\nLEVEL," << (D.hasLevel ? D.lvlLabel : "none") << "\nPHASE," << D.phase << "\nSTATE," << what << "\n";
}

int DealerRead::draw(void)
{
    load();
    render(cfg);
    writeStatus(D.hasPrice ? "drawn" : "no data");
    return RTX_OK;
}

extern "C" cppExtension *CreateExtension(void)
{
    DealerRead *p = new DealerRead();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE);
    p->setDescription("LRA Dealer Read: level + fuel checklists, phase, TRADE line (reads lsFlexLevels\\LRA-Dealer-<MKT>.csv)");
    p->setVersion("1.1");
    return p;
}
