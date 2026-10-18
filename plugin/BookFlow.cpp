/********************************************************************************
 *  BookFlow.cpp  --  Investor/RT RTX extension  lsBookFlow  1.3.0  (2026-10-18; 1.0.0 2026-10-16)
 *  1.3.0 (2026-10-18, Rassul approved the audit-A fixes; colour by side decided by Rassul tonight):
 *   - BRIGHT + LABEL FROM THE NET THAT IS DRAWN: a side is a spike when its |net| is >= "Spike vs average" (2.0x) the plain average of
 *     that side's |net| over the prior "Ratio bars" (20) closed candles - THE spike rule of both order-flow panes (Aggression the same),
 *     decided once at the close (BookFlowLogic.h netStat). The "1.5x the prior biggest" jump is gone from the drawn rule (it stays in the
 *     old StackStats / PullStats record words only). Was: the added-only / pulled-only part's spike, so +900 / -880 drew a tiny +20 bright.
 *   - NO REPAINT: the forming candle is drawn dim with no label until it closes.
 *   - COLOUR BY SIDE: bid-side bars green (buyers), offer-side bars red (sellers), above (arriving) or below (leaving) the grey zero line;
 *     the meaning is in the description. Label: the ratio (yellow) and, for size arriving, the pace (grey) with no "p", outside the bar.
 *   - Settings: "Spike vs prior biggest", "Spike lookback" and "Box shows" (did nothing) removed (parameter version 3); the box Net row
 *     shows the net and its ratio. Description rewritten.
 *   - MARKET STATE (MarketState.h): SRC "BOOK" facts of the last closed candle (bid_net, ask_net, bid_net_x, ask_net_x, bid_state,
 *     ask_state, spike, rebuilt_pct) posted once per closed candle, re-posted unchanged every 15 s, cleared on removal. Behind the scenes.
 *   The pane draws itself (draw() below) instead of OrderFlowFeed's drawNetBars (shared header untouched).
 *  Rassul 2026-10-16 07:45-07:49 CT: "give me a design that combines the pulling and stacking indicator" / mockup / "1 net bar. make the
 *  zero line gray so i can clearly see the separation. dont keep the old panes, replace them. lets build this".
 *
 *  WHO IS ARRIVING AND WHO IS LEAVING, in one pane (replaces lsPullStats 1.7.1 and lsStackStats 1.5.1).
 *  Per candle, per side: ONE NET BAR = lots ADDED near price minus lots PULLED near price (market-by-order feed, the same engine and the
 *  same "near price" band as before). Above the grey zero line = size arriving (stacking); below = size leaving (pulling). Bids green (left
 *  bar), offers red (right bar). Bright = a spike of the bigger part (added when up, pulled when down; same spike rule as the old panes).
 *  Spike label: ratio / pace / lots (pace only for added). Letters and the refill dash are not drawn; the records keep them.
 *  Top: the Order Flow read line (the louder of the two old reads, "Also: ..." when the other agrees), the blue options line when fresh,
 *  the pane title "Book Flow - Who is arriving or leaving?". Settings: as lsPullStats (Ratio bars, Position, Spike vs average, Spike vs
 *  prior biggest, Spike lookback, Near ticks, Record order feed, Offset X / Y, Box shows).
 *
 *  KEPT WORKING FOR THE TAPE READER AND THE NIGHTLY STUDIES (nothing else had to change):
 *   - lsFlexLevels\StackStats\<MKT>-<CONTRACT>-<spb>-<session>.csv and lsFlexLevels\PullStats\... session records, written in the old format
 *     (same columns, same message words) from the same candle numbers; the pull record's pace column is the ADDED pace (no pull pace here).
 *   - lsFlexLevels\PullStats\...-zoneread-<session>.csv: the arrival read after price reaches a key / gamma level (ZoneRead.h), shown as
 *     the read line only once it locks (3 min), always "?". The old stacking read at a level ("bids stacking 2.3x normal ?") is not shown.
 *   - "Record order feed" (default ON): every order-by-order event to data\irt\<session>\<MKT>\mbo_HH.csv (MboRecord.h) - the nightly MBO archive.
 *   - lsFlexLevels\BookFlow\... : its own record (same columns) and status-<MKT>-<CONTRACT>-<spb>.txt.
 *  Rules: BookFlowLogic.h; engine, feed, drawing, params, records: OrderFlowFeed.h 1.7.0.
 ********************************************************************************/
#ifndef NOMINMAX
#define NOMINMAX
#endif
#if defined(_WIN32)
#include <windows.h>
#endif
#include "irtsdk.h"
#ifdef far
#undef far
#endif
#ifdef near
#undef near
#endif
#include "HostSlot.h"
#define OFF_SDK 1
#include "OrderFlowFeed.h"
#include "BookFlowLogic.h"
#include "MboRecord.h"              // Record order feed
#include "MarketState.h"            // (1.3.0) SRC "BOOK" facts for the read

#define BF_VERSION "1.3.0"          // keep equal to bf::VERSION and the setVersion literal below

// (1.3.0) the dialog rows (parameter version 3): every row is read by the chart
static const int BF_RATIO = 0, BF_POS = 1, BF_AVG = 2, BF_NEAR = 3, BF_RECORD = 4, BF_OFFX = 5, BF_OFFY = 6;

class BookFlow : public off::MboPane<bf::Logic> {
public:
    BookFlow() : off::MboPane<bf::Logic>() {}
    virtual bool barMarks() const { return false; }                        // no letters / dash drawn; hover + records keep them
    virtual bool netBars() const { return true; }
    virtual bool textOff() const { return false; }
    virtual std::string boxKeep() const { return "Net;Pace"; }            // (1.2.1, Rassul 22:54) the box: Net and Pace only (1.3.0: Net = "+120 (2.4x)")
    virtual bool boxOnly() const { return true; }                         // (1.2.0, Rassul 2026-10-16 22:00) the left-justified numbers box in the pane, no read / options lines                          // (1.1.0, Rassul 2026-10-16 18:14) no read / numbers / options lines on the pane; the records and the tape reader keep every number
    virtual long long netOf(const off::Rec& r, int s) const { return bf::netOf(r, s); }
    // the label numbers of one net bar: ratio / pace / lots of the part that spiked (up = added, down = pulled; pace only for added)
    virtual void netLook(const off::PaneBar& b, int s, off::BarLook& k) const {
        const bool up = bf::netOf(b.r, s) >= 0;
        k.ratioOk[s] = up ? b.m[0].ratioOk[s] : b.m[1].ratioOk[s];
        k.ratio[s] = up ? b.m[0].ratio[s] : b.m[1].ratio[s];
        k.p100[s] = up ? b.r.pace100[s] : 0;
        k.q[s] = up ? b.r.addC[s] : b.r.pullC[s];
    }
    virtual int zonePane() const { return zread::P_PULLSTATS; }            // the arrival read (kept in PullStats\ so the nightly keeps reading it)
    virtual std::string zoneDirName() const { return "PullStats"; }
    virtual int parmsTitle(char* pStr, int size) { return off::copyTitle(pStr, size, bf::Logic::title(), BF_VERSION) ? RTX_OK : RTX_FAIL; }
    // Record order feed: the recorder and the two legacy session records live in this chart's pane state
    virtual bool recorderPane() const { return true; }
    struct Ext : off::PaneExt {
        mrec::Recorder r; off::SessionFile stack, pull;
        // (1.3.0) the net decisions of closed candles (decided once; only a Ratio bars / Spike vs average change re-decides), the chart
        // label + identity they belong to, the Market State facts posted
        std::map<long long, bf::NetStat> net; int netRb = -1; long long netAvg = -1; std::string identity, chart;
        long long mstBar = 0, mstCt = 0, mstMs = -1, mstPosts = 0; std::map<std::string, std::string> mstKv;
    };
    Ext* ext(S_& S) {
        if (!S.ext) S.ext.reset(new (std::nothrow) Ext());
        return static_cast<Ext*>(S.ext.get());
    }
    mrec::Recorder* rec(S_& S) {
        Ext* e = ext(S); if (!e) return nullptr;
        e->r.setOn(S.set.record != 0);
        if (!S.market.empty()) e->r.setMarket(S.market);
        return &e->r;
    }
    virtual void onRaw(S_& S, const DEPTH_ORDER& o) {
        mrec::Recorder* r = rec(S); if (!r || !r->on) return;
        if (!o.timestamp) { ++r->dropped; return; }
        if (o.timestamp != r->tsKey) {                                     // one clock conversion per second, not per event
            struct tm t; std::memset(&t, 0, sizeof(t));
            if (!getLocaltime(o.timestamp, &t)) { ++r->dropped; return; }
            r->tsKey = o.timestamp; r->tsTm = t;
        }
        const struct tm& t = r->tsTm;
        mrec::Ev e; e.y = t.tm_year + 1900; e.mo = t.tm_mon + 1; e.d = t.tm_mday; e.h = t.tm_hour; e.mi = t.tm_min; e.s = t.tm_sec;
        e.action = o.action; e.price = (double)o.price; e.prev = (double)o.previousPrice; e.size = (unsigned long)o.size;
        e.oid = (long long)o.orderID; e.aid = (long long)o.aggressorOrderID; e.side = o.buySell == 1 ? 1 : o.buySell == 0 ? 0 : -1;
        e.bid = (double)o.bid; e.ask = (double)o.ask;
        r->add(e);
    }
    virtual void afterPump(S_& S, long long now, bool fromDraw) { mrec::Recorder* r = rec(S); if (r) r->tick(now, fromDraw); post(S, now); }
    virtual void afterTimer(S_& S, long long now) { mrec::Recorder* r = rec(S); if (r) r->tick(now, false); }
    virtual void statusExtra(S_& S, std::ostream& o) {
        mrec::Recorder* r = rec(S); if (r) r->status(o); else o << "\nMBO_RECORD,unavailable (out of memory)";
        Ext* e = ext(S);
        o << "\nSPIKE_RULE,|net| >= " << off::tenthsText(avgT(S)) << (avgT(S) > 0 ? "" : " (off)") << " the plain " << S.ser.ratioBars << "-candle average of that side's |net|, decided at the close; forming candle dim, no label; colour by side"
          << "\nMARKET_STATE,BOOK " << (!e || e->chart.empty() ? std::string("waiting for the chart label") : "bar " + std::to_string(e->mstBar ? e->mstCt : 0) + ",posts " + std::to_string(e->mstPosts) + ",keys " + std::to_string(e->mstKv.size()));
    }
    // the old panes' session records (StackStats\ and PullStats\), opened with the session, one row per decided candle, saved with the main record
    virtual void onLoad(S_& S, const std::string& flex, const std::string& stem) {
        Ext* e = ext(S); if (!e || flex.empty()) return;
        const std::string cols = std::string("\nbar_time|ct_time|dbo|") + off::REC_COLS + "|letters_left|letters_right|spike_left|spike_right|dash_left|dash_right|implication|level|message|numbers\n";
        off::makeDir(flex);
        const std::string ds = flex + "\\StackStats", dp = flex + "\\PullStats";
        off::makeDir(ds); off::makeDir(dp);
        const std::string tail = " " + S.market + " " + S.symbol + " " + std::to_string(S.spb);
        e->stack.open(S.curSession, ds + "\\" + stem + "-" + S.curSession + ".csv", std::string(), std::string("# StackStats ") + st::VERSION + tail + cols, (size_t)off::REC_NUMS);
        e->pull.open(S.curSession, dp + "\\" + stem + "-" + S.curSession + ".csv", std::string(), std::string("# PullStats ") + ps::VERSION + tail + cols, (size_t)off::REC_NUMS);
        e->stack.load(); e->pull.load();
    }
    static off::Row legacyRow(const off::Row& base, const off::PaneBar& pb) {
        off::Row r = base;
        for (int s = 0; s < 2; ++s) { r.let[s] = pb.let[s]; r.spike[s] = pb.spike[s] ? 1 : 0; r.dash[s] = pb.dash[s]; }
        r.impl = pb.msg.impl; r.line1 = pb.msg.line1; r.line2 = pb.msg.line2;
        return r;
    }
    virtual void onDecide(S_& S, long long key, const off::PaneBar& nb, const off::Row& base) {
        Ext* e = ext(S); if (!e || e->stack.path.empty()) { decidedNet(S, key); return; }
        const off::Params& P = S.params.load.p;
        off::PaneBar sk = bf::stackView(nb); st::Logic::decide(sk, P, false); e->stack.put(legacyRow(base, sk));
        off::PaneBar pk = bf::pullView(nb);  ps::Logic::decide(pk, P, false); e->pull.put(legacyRow(base, pk));
        decidedNet(S, key);                                                // (1.3.0) the net spike -> the BookFlow record + the Market State
    }
    virtual void onSave(S_& S) { Ext* e = ext(S); if (!e) return; e->stack.save(); e->pull.save(); }

    // ---- (1.3.0) the net spike, decided once per closed candle -------------------------------------------------------------------
    // the spike threshold in force: the nightly params file wins where it sets it, else the dialog's (0 = off) - as Aggression
    static long long avgT(const S_& S) { const off::Params& P = S.params.load.p; return P.file("spikeAvgX") ? P.spikeAvgT : S.dlgSpike.avgT; }
    void sync(S_& S, Ext& e) {                                             // a new chart identity or a settings change: start the decisions over
        if (e.identity != S.identity) {
            if (!e.chart.empty() && !e.identity.empty() && !e.mstKv.empty()) mst::clear("BOOK", symOf(e.identity), spbOf(e.identity), e.chart);
            e.identity = S.identity; e.net.clear(); e.mstBar = 0; e.mstCt = 0; e.mstMs = -1; e.mstKv.clear();
        }
        if (e.netRb != S.ser.ratioBars || e.netAvg != avgT(S)) { e.net.clear(); e.netRb = S.ser.ratioBars; e.netAvg = avgT(S); }
    }
    static std::string symOf(const std::string& id) { const size_t a = id.find('|'), b = a == std::string::npos ? a : id.find('|', a + 1); return b == std::string::npos ? std::string() : id.substr(a + 1, b - a - 1); }
    static long spbOf(const std::string& id) { const size_t c = id.rfind('|'); return c == std::string::npos ? 0 : std::atol(id.c_str() + c + 1); }
    const bf::NetStat* netAt(S_& S, long long t) {                         // a closed candle's decision (computed once, then kept)
        Ext* e = ext(S); if (!e) return nullptr;
        sync(S, *e);
        std::map<long long, bf::NetStat>::const_iterator it = e->net.find(t);
        if (it != e->net.end()) return &it->second;
        const bf::NetStat n = bf::netStatAt(S.ser.bars, t, S.ser.ratioBars, avgT(S));
        if (!n.ok) return nullptr;
        while (e->net.size() > 6000) e->net.erase(e->net.begin());
        return &(e->net[t] = n);
    }
    // a closed candle was decided: its net spike goes into the record row (spike_left / spike_right) and the Market State facts
    void decidedNet(S_& S, long long key) {
        Ext* e = ext(S); if (!e) return;
        const bf::NetStat* n = netAt(S, key); if (!n) return;
        std::map<long long, off::PaneBar>::iterator b = S.ser.bars.find(key);
        if (b != S.ser.bars.end()) for (int s = 0; s < 2; ++s) b->second.spike[s] = n->spike[s];
        std::map<long long, off::Row>::iterator rw = S.file.rows.find(key);
        if (rw != S.file.rows.end()) for (int s = 0; s < 2; ++s) rw->second.spike[s] = n->spike[s] ? 1 : 0;
        if (b != S.ser.bars.end() && key >= e->mstBar) facts(S, *e, key, b->second, *n);
    }
    void facts(S_& S, Ext& e, long long key, const off::PaneBar& b, const bf::NetStat& n) {
        (void)S; struct tm x; std::memset(&x, 0, sizeof(x));
        if (!getLocaltime((RTDATE)key, &x)) return;
        e.mstKv = bf::bookFacts(b, n); e.mstBar = key; e.mstMs = -1;
        e.mstCt = off::civilDays(x.tm_year + 1900, (unsigned)x.tm_mon + 1, (unsigned)x.tm_mday) * 86400 + x.tm_hour * 3600 + x.tm_min * 60 + x.tm_sec;
    }
    // SRC BOOK: posted when new, re-posted unchanged every 15 s (readers ignore an entry older than 60 s)
    void post(S_& S, long long now) {
        Ext* e = ext(S); if (!e || e->chart.empty() || S.symbol.empty()) return;
        sync(S, *e);
        if (e->mstKv.empty() && S.lastClosedKey > 0) {                     // after a restart: the last closed candle read back from the record
            std::map<long long, off::PaneBar>::const_iterator b = S.ser.bars.find(S.lastClosedKey);
            const bf::NetStat* n = b != S.ser.bars.end() && b->second.ok ? netAt(S, S.lastClosedKey) : nullptr;
            if (n) facts(S, *e, S.lastClosedKey, b->second, *n);
        }
        if (e->mstKv.empty()) return;
        if (e->mstMs >= 0 && now >= e->mstMs && now - e->mstMs < 15000) return;
        e->mstMs = now; e->mstPosts++;
        mst::publish("BOOK", S.symbol, S.spb, e->chart, now, e->mstCt, e->mstKv);
    }
    void release() {                                                       // the Market State entry goes with the indicator
        S_* S = testState();
        if (S) { Ext* e = static_cast<Ext*>(S->ext.get()); if (e && !e->chart.empty() && !S->symbol.empty()) mst::clear("BOOK", S->symbol, S->spb, e->chart); }
        off::MboPane<bf::Logic>::release();
    }

    // ---- (1.3.0) the settings window: Ratio bars, Position, Spike vs average, Near ticks, Record order feed, Offset X, Offset Y ---------
    // The per-market file keeps OrderFlowFeed's layout (paneSettingsLine): the removed rows' fields (jump, lookback, Box shows) carry the
    // file's values unchanged. Loading never saves; a control he did not touch keeps the file's value (DlgSnap).
    bp::DlgSnap snapBf_;
    std::string placeFile(const std::string& mkt) { const std::string f = off::flexDir(); return f.empty() || mkt.empty() ? std::string() : f + "\\BookFlow.place-" + mkt + ".txt"; }
    std::vector<long long> dlgNow() {
        std::vector<long long> v;
        v.push_back(getIntegerValue(BF_RATIO)); v.push_back(getListIndex(BF_POS)); v.push_back(off::tenthsOf(getFloatValue(BF_AVG)));
        v.push_back(getIntegerValue(BF_NEAR)); v.push_back(isBoxChecked(BF_RECORD) ? 1 : 0); v.push_back(getIntegerValue(BF_OFFX)); v.push_back(getIntegerValue(BF_OFFY));
        return v;
    }
    virtual int parmsLoad(void) {
        if (!inDialog()) return RTX_OK;
        const off::PaneSettings f = fileSettings(chartMarket());
        setIntegerValue(BF_RATIO, f.ratioBars); setListIndex(BF_POS, f.pos); setFloatValue(BF_AVG, (float)f.spike.avgT / 10.0f);
        setIntegerValue(BF_NEAR, f.nearTicks); checkBox(BF_RECORD, f.record != 0); setIntegerValue(BF_OFFX, f.dx); setIntegerValue(BF_OFFY, f.dy);
        snapBf_.take(dlgNow());
        return RTX_OK;
    }
    void saveBf() {
        if (!inDialog()) return;
        const std::string mkt = chartMarket();
        const off::PaneSettings fs = fileSettings(mkt);
        const std::vector<long long> now = dlgNow();
        const long long fv[7] = { fs.ratioBars, fs.pos, fs.spike.avgT, fs.nearTicks, fs.record, fs.dx, fs.dy };
        long long v[7]; for (size_t k = 0; k < 7; ++k) v[k] = snapBf_.pick(k, now[k], fv[k]);
        off::PaneSettings s = fs;                                          // jump / lookback / Box shows: the file's values
        s.ratioBars = (int)v[0]; s.pos = bp::posOk((int)v[1]) ? (int)v[1] : fs.pos; s.spike.avgT = (int)v[2]; s.nearTicks = (int)v[3]; s.record = v[4] ? 1 : 0;
        s.dx = bp::offOk(v[5]) ? (int)v[5] : fs.dx; s.dy = bp::offOk(v[6]) ? (int)v[6] : fs.dy;
        off::PaneSettings chk;
        if (s.spike.avgT < 0 || s.nearTicks < 0 || !off::parsePaneSettings(off::paneSettingsLine(s), chk)) return;   // never overwrite with garbage
        const std::string p = placeFile(mkt); if (p.empty()) return;
        off::makeDir(off::flexDir());
        off::publishFile(p, off::paneSettingsLine(s) + "\n");
        snapBf_.take(now);
        S_* S = testState();
        if (S) { S->settingsCheckMs = -1; S->settingsStamp = -1 - 3; }    // re-read on the next pass (not "first")
    }
    virtual int parmsApply(void) { saveBf(); return RTX_OK; }
    virtual int parmsUpdt(unsigned int) { saveBf(); return RTX_OK; }

    // ---- (1.3.0) the pane: the numbers box (Net, Pace) + one net bar per side per candle, coloured by side ----------------------------
    virtual int draw(void) {
        {   S_* S0 = testState();                                          // the chart label (Market State key), read in the draw as BigBidAsk
            if (S0) { Ext* e = ext(*S0); const char* cl = getChartLabel(); const std::string c = cl ? cl : "";
                      if (e && !c.empty() && c != e->chart) { if (!e->chart.empty() && !S0->symbol.empty()) mst::clear("BOOK", S0->symbol, S0->spb, e->chart); e->chart = c; e->mstMs = -1; } } }
        pump(true);                                                        // from the draw: a recorder buffers, never writes
        S_* S = testState();
        if (S) { S->hov.left(); followBoard(*S); boxLeft(*S); }
        if (S) { try { renderNet(*S); } catch (...) { S->state = "draw error"; } }
        return RTX_OK;
    }
    void renderNet(S_& S) {
        RCT pane; pane.getPaneRect(false);
        const long count = getBarCount();
        if (count < 2 || pane.bottom - pane.top < 24 || pane.right - pane.left < 60) return;
        RTARRAYI times(barDateTime);
        if (times.count < count) return;
        const int last = (int)count - 1;
        auto xOf = [&](int bar) { PNT p; p.set(bar, 0.0f, kBarCenter); return (int)p.h; };
        int v0 = 0, v1 = last;
        if (getVisibleBars(&v0, &v1) != RTX_OK || v0 < 0 || v1 < v0 || v1 > last) { v1 = last; v0 = last; while (v0 > 0 && xOf(v0 - 1) >= pane.left) --v0; }
        int slot = (int)getPixelsPerBar();
        if (slot <= 0 || slot > 400) slot = v1 > v0 ? std::max(2, std::abs(xOf(v1) - xOf(v1 - 1))) : 8;
        off::PaneBar lb; bool haveLive = false;                            // the forming candle: its numbers as of now (drawn dim, never judged)
        {   std::map<long long, off::Acc>::const_iterator it = S.accs.find(S.formingKey);
            if (it != S.accs.end() && it->second.events > 0 && (long long)times[last] == S.formingKey) { lb = S.ser.make(S.formingKey, S.e.numbers(it->second, S.formingKey == S.firstBarKey, S.params.load.p)); haveLive = lb.ok; } }
        auto barOf = [&](int i) -> const off::PaneBar* {
            if (i == last) return haveLive ? &lb : nullptr;
            std::map<long long, off::PaneBar>::const_iterator it = S.ser.bars.find((long long)times[i]);
            return it != S.ser.bars.end() && it->second.ok ? &it->second : nullptr;
        };
        long long top = 0;
        for (int i = v0; i <= v1; ++i) { const off::PaneBar* b = barOf(i); if (b) top = std::max(top, magOf(b->r)); }
        if (S.axisTop > top) top = S.axisTop;
        S.paneH = pane.bottom - pane.top;
        const off::PaneGeo g = off::paneGeo(this, gmode(S.set.mode));
        const int hov = S.hov.hover.bar >= 0 && S.hov.hover.bar < last ? S.hov.hover.bar : -1;
        const int mb = hov >= 0 ? hov : last - 1;
        const long long mt = (long long)times[mb];
        std::map<long long, off::PaneBar>::const_iterator mi = S.ser.bars.find(mt);
        const off::PaneBar* mbar = mi != S.ser.bars.end() && mi->second.ok ? &mi->second : nullptr;
        off::Info info = bf::info(mbar, S.params.load.p, mbar ? netAt(S, mt) : nullptr);
        {   const std::string keep = ";" + boxKeep() + ";"; std::vector<off::InfoRow> kept;   // (1.2.1) Net and Pace only
            for (size_t k = 0; k < info.rows.size(); ++k) if (keep.find(";" + info.rows[k].label + ";") != std::string::npos) kept.push_back(info.rows[k]);
            info.rows.swap(kept); }
        { struct tm x; std::memset(&x, 0, sizeof(x)); info.head = hov < 0 ? std::string("Live") : (mt > 0 && getLocaltime((RTDATE)mt, &x) ? off::clock12(x.tm_hour, x.tm_min) : std::string("n/a")); }
        bp::Place pl; pl.pos = S.set.pos; pl.dx = S.set.dx; pl.dy = S.set.dy;
        const bp::Box area = bp::mk(g.left + 3, g.top + 3, g.edge - 3, g.bottom - 3);
        const zread::Line zl = S.zr.e.line(S.zr.pane, off::steadyMs());
        const off::PanelPlan P = off::planPanel(area, g.top, off::MODE_NUMBERS, pl, info, std::string(), std::string(), g.reserve, off::measureWith(this));
        S.lastBox = P.ok ? P.box : bp::Box();
        std::vector<off::LBox> avoid;
        if (P.ok) avoid.push_back(off::LBox{ P.box.l, P.box.t, P.box.r, P.box.b, 0, 0 });
        for (size_t k = 0; k < P.textRects.size(); ++k) avoid.push_back(off::LBox{ P.textRects[k].l, P.textRects[k].t, P.textRects[k].r, P.textRects[k].b, 0, 0 });
        drawNet(S, g, v0, v1, last, slot, top, xOf, barOf, avoid);
        const COLOR vcol[2] = { 0x0022C55E, 0x00EF4444 };                 // values: bids (buyers) green, offers (sellers) red
        off::drawPanel(this, P, info, vcol, off::implOf(zl), false);
    }
    // the net bars (OrderFlowFeed drawNetBars' geometry, 1.3.0 rules): the grey zero line, one bar per side, colour BY SIDE (bid green,
    // offer red), bright + label only for a CLOSED candle's net spike; labels biggest first, a label that would sit on another is skipped
    template <class XOf, class BarOf>
    void drawNet(S_& S, const off::PaneGeo& g, int v0, int v1, int last, int slot, long long top, XOf xOf, BarOf barOf, const std::vector<off::LBox>& avoid) {
        if (top <= 0) return;
        const COLOR normal[2] = { off::C_GREEN_BAR, off::C_RED_BAR }, spike[2] = { off::C_GREEN_SPIKE, off::C_RED_SPIKE };
        const off::NetGeo n = off::netGeoFor(g.bottom - g.top, g.reserve);
        const int y0 = g.top + n.y0;
        if (g.edge > g.left + 4) off::fillFlat(this, g.left, y0, g.edge, y0 + 1, off::C_NET_ZERO);
        struct Cand { int cx = 0, hb = 0; bool up = true; long long mag = 0; std::string a, b; };
        std::vector<Cand> cands;
        RTARRAYI times(barDateTime);
        for (int i = v0; i <= v1; ++i) {
            const off::PaneBar* pb = barOf(i); if (!pb) continue;
            const int xc = xOf(i); if (xc < g.left || xc > g.right) continue;
            const bool closed = i != last;
            const bf::NetStat* ns = closed ? netAt(S, (long long)times[i]) : nullptr;
            const off::Pair p = off::pairIn(xc, slot);
            for (int s = 0; s < 2; ++s) {
                const bf::NetLook k = bf::netLookOf(pb, ns, s, closed);
                if (k.v == 0) continue;
                const bool up = k.v > 0; const long long mg = up ? k.v : -k.v;
                const int h = off::barHeight(mg, top, n.maxPx);
                const COLOR c = k.bright ? spike[k.col] : normal[k.col];
                if (up) off::fillFlat(this, p.l[s], y0 - h, p.r[s], y0 - 1, c); else off::fillFlat(this, p.l[s], y0 + 2, p.r[s], y0 + 1 + h, c);
                if (k.bright && (!k.ratio.empty() || !k.pace.empty())) { Cand q; q.cx = (p.l[s] + p.r[s]) / 2; q.hb = h; q.up = up; q.mag = mg; q.a = k.ratio; q.b = k.pace; cands.push_back(q); }
            }
        }
        if (cands.empty()) return;
        std::stable_sort(cands.begin(), cands.end(), [](const Cand& a, const Cand& b) { return a.mag > b.mag; });
        FONT sf; sf.id = HELVETICA; sf.size = off::LABEL_PT; sf.style = PLAIN; setFont(sf);
        std::vector<off::LBox> placed(avoid);
        for (size_t k = 0; k < cands.size(); ++k) {
            const Cand& c = cands[k];
            std::string lr[2]; COLOR lc[2] = { off::C_LBL_RATIO, off::C_LBL_PACE }; int nR = 0;
            if (!c.a.empty()) { lr[nR] = c.a; lc[nR++] = off::C_LBL_RATIO; }
            if (!c.b.empty()) { lr[nR] = c.b; lc[nR++] = off::C_LBL_PACE; }
            const int yEdge = c.up ? y0 - c.hb : y0 + 1 + c.hb;
            const int t0 = c.up ? yEdge - 2 - nR * off::LABEL_LH : yEdge + 2, b0 = t0 + nR * off::LABEL_LH;
            if (t0 < g.top + g.reserve || b0 > g.bottom) continue;
            int w = 0; for (int r = 0; r < nR; ++r) w = std::max(w, (int)getTextWidth(lr[r].c_str(), -1));
            const off::LBox me = { c.cx - w / 2 - 1, t0, c.cx - w / 2 + w + 1, b0 - 1, 0, 0 };
            if (me.l < g.left + 1 || me.r > g.edge - 1) continue;
            bool hit = false; for (size_t o = 0; o < placed.size() && !hit; ++o) hit = off::overlaps(me, placed[o]);
            if (hit) continue;
            placed.push_back(me);
            for (int r = 0; r < nR; ++r) {
                setTextColor(lc[r]); const int tw = (int)getTextWidth(lr[r].c_str(), -1); const int y = t0 + r * off::LABEL_LH;
                RCT rc; rc.set(off::coord(c.cx - tw / 2), off::coord(y), off::coord(c.cx - tw / 2 + tw + 2), off::coord(y + off::LABEL_LH)); rc.drawText(lr[r].c_str(), false, false);
            }
        }
    }
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { static_cast<BookFlow*>(this)->release(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<BookFlow*>(this)->release(); return RTX_OK; }
int cppExtension::setup(void) {
    // (1.3.0) parameter version 3: "Spike vs prior biggest", "Spike lookback" (not part of the spike rule any more) and "Box shows" (did
    // nothing) removed; rows BF_RATIO .. BF_OFFY, all read by the chart (via the per-market file)
    setParameterVersion(3);
    setParameterDialogHeight(3);
    setIntegerParameter("Ratio bars", off::RATIO_DEFAULT, 50);                                          // 0
    setListParameter("Position", (short)off::POS_DEFAULT, off::POS_CHOICES, 110, kParmAppendSameLine); // 1
    setFloatParameter("Spike vs average", 2.0f, 50, kParmAppendSameLine);                               // 2
    setIntegerParameter("Near ticks (0 = auto)", 0, 50);                                                // 3
    setBoolParameter("Record order feed", true, kParmAppendSameLine);      // 4: data\irt\<session>\<MKT>\mbo_HH.csv
    setIntegerParameter("Offset X", 0, 50);                                // 5
    setIntegerParameter("Offset Y", 0, 50, kParmAppendSameLine);           // 6
    return RTX_OK;
}
int cppExtension::calc(int) { static_cast<BookFlow*>(this)->calcHook(); return RTX_OK; }

extern "C" cppExtension* CreateExtension(void) {
    BookFlow* p = new BookFlow();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | NO_UI | TRACK_MOUSE);                   // its own pane, draws itself; mouse events even when not selected
    p->setExtendedFlags(CALL_CONTINUOUSLY);                            // the order feed keeps coming while no trade prints
    p->setDescription("LRA Book Flow: who is ARRIVING and who is LEAVING the order book near price (market-by-order feed). Per candle, ONE NET bar per side = lots ADDED near price minus lots PULLED near price. Left bar = the bid side, always green (buyers); right bar = the offer side, always red (sellers). Above the grey zero line = size arriving (stacking), below = size leaving (pulling): a green bar above = bids arriving (support building), green below = bids leaving (support pulled), red above = offers arriving (resistance building), red below = offers leaving (resistance pulled). A CLOSED candle is drawn bright, with its ratio (yellow, e.g. 2.4 = 2.4x) and, for size arriving, its pace (grey, lots added per second vs the same time of day) beyond the bar's end, only when that side's net is at least Spike vs average (default 2.0x) the plain average of its net size over the prior Ratio bars (default 20) closed candles - decided once at the close, never changed. Other candles are dim with no label; the forming candle stays dim until it closes. The numbers box in the pane: Net (with its ratio) and Pace for bids and offers of the candle under the mouse, else the last closed candle. Settings: Ratio bars (the average), Position and Offset X / Y (the box), Spike vs average (0 = no bright bars), Near ticks (0 = auto: how close to price counts), Record order feed (every order-by-order event to the LRA data folder for the nightly MBO archive).");
    p->setVersion("1.3.0");   // BF_VERSION
    return p;
}
