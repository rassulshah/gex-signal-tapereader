/********************************************************************************
 *  BookFlow.cpp  --  Investor/RT RTX extension  lsBookFlow  1.0.0  (2026-10-16)
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

#define BF_VERSION "1.2.1"          // keep equal to bf::VERSION and the setVersion literal below

class BookFlow : public off::MboPane<bf::Logic> {
public:
    BookFlow() : off::MboPane<bf::Logic>() {}
    virtual bool barMarks() const { return false; }                        // no letters / dash drawn; hover + records keep them
    virtual bool netBars() const { return true; }
    virtual bool textOff() const { return false; }
    virtual std::string boxKeep() const { return "Net;Pace"; }            // (1.2.1, Rassul 22:54) the box: Net and Pace only
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
    struct Ext : off::PaneExt { mrec::Recorder r; off::SessionFile stack, pull; };
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
    virtual void afterPump(S_& S, long long now, bool fromDraw) { mrec::Recorder* r = rec(S); if (r) r->tick(now, fromDraw); }
    virtual void afterTimer(S_& S, long long now) { mrec::Recorder* r = rec(S); if (r) r->tick(now, false); }
    virtual void statusExtra(S_& S, std::ostream& o) { mrec::Recorder* r = rec(S); if (r) r->status(o); else o << "\nMBO_RECORD,unavailable (out of memory)"; }
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
        Ext* e = ext(S); if (!e || e->stack.path.empty()) return;
        (void)key;
        const off::Params& P = S.params.load.p;
        off::PaneBar sk = bf::stackView(nb); st::Logic::decide(sk, P, false); e->stack.put(legacyRow(base, sk));
        off::PaneBar pk = bf::pullView(nb);  ps::Logic::decide(pk, P, false); e->pull.put(legacyRow(base, pk));
    }
    virtual void onSave(S_& S) { Ext* e = ext(S); if (!e) return; e->stack.save(); e->pull.save(); }
};

int cppExtension::init(void)    { return RTX_OK; }
int cppExtension::done(void)    { static_cast<BookFlow*>(this)->release(); return RTX_OK; }
int cppExtension::destroy(void) { static_cast<BookFlow*>(this)->release(); return RTX_OK; }
int cppExtension::setup(void) {
    setParameterVersion(2);
    setParameterDialogHeight(4);
    setIntegerParameter("Ratio bars", off::RATIO_DEFAULT, 50);
    setListParameter("Position", (short)off::POS_DEFAULT, off::POS_CHOICES, 110, kParmAppendSameLine);
    setFloatParameter("Spike vs average", 2.0f, 50);
    setFloatParameter("Spike vs prior biggest", 1.5f, 50, kParmAppendSameLine);
    setIntegerParameter("Spike lookback", 10, 50, kParmAppendSameLine);
    setIntegerParameter("Near ticks (0 = auto)", 0, 50);
    setBoolParameter("Record order feed", true, kParmAppendSameLine);      // parameter 6: data\irt\<session>\<MKT>\mbo_HH.csv
    setIntegerParameter("Offset X", 0, 50);                                // 7
    setIntegerParameter("Offset Y", 0, 50, kParmAppendSameLine);           // 8
    setListParameter("Box shows", (short)off::MODE_READ, off::MODE_CHOICES, 90, kParmAppendSameLine);   // 9
    return RTX_OK;
}
int cppExtension::calc(int) { static_cast<BookFlow*>(this)->calcHook(); return RTX_OK; }

extern "C" cppExtension* CreateExtension(void) {
    BookFlow* p = new BookFlow();
    p->setArrayCount(1);
    p->setFlags(POST_DRAWING | NO_UI | TRACK_MOUSE);                   // its own pane, draws itself; mouse events even when not selected
    p->setExtendedFlags(CALL_CONTINUOUSLY);                            // the order feed keeps coming while no trade prints
    p->setDescription("LRA BookFlow: who is ARRIVING and who is LEAVING, in one pane (replaces PullStats and StackStats). Per candle, one NET bar per side = lots ADDED near price minus lots PULLED near price (market-by-order feed). Above the grey zero line = size arriving (stacking), below = size leaving (pulling). Green = bids (buyers), red = offers (sellers). Bright = a spike of the bigger part (added when up, pulled when down): >= Spike vs average x that side's average AND >= Spike vs prior biggest x the biggest bar of either side in the prior Spike lookback candles. Spike labels: the ratio, the pace (p = added lots per second vs the same time of day) and the lots. The read line (only at a key / gamma level, shown once it locks 3 minutes after price reached the level): was the size resting at the level cancelled or traded; always '?'. The top message is the louder of the stacking and pulling reads for the candle under the mouse. Settings: Ratio bars, Position, Spike vs average, Spike vs prior biggest, Spike lookback (0 = off), Near ticks (0 = auto), Record order feed (every order-by-order event to the LRA data folder for the nightly MBO archive), Offset X / Y, Box shows.");
    p->setVersion("1.2.1");   // BF_VERSION
    return p;
}
