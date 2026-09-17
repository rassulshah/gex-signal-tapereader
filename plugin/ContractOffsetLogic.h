// ContractOffsetLogic.h — THE ANCHOR BAR for the contract offset, shared by lsGammaProfile and lsKingTracker. NO SDK.
//
// The panel's book is priced on Skylit's ES1 spot (SCALEREF); the chart may be another contract. The offset is
// (chart close at the anchor bar) − SCALEREF, and everything about correctness lives in WHICH bar is the anchor:
//   · 0.46 / KT 0.7  the bar at ASOF (the export time) — right during RTH, wrong after hours
//   · 0.48 / KT 0.8  the last RTH-stamped bar at or before ASOF — right at the close, wrong on FOMC day (the quote
//                    had frozen at 14:26, not 15:00)
//   · 0.49 / KT 0.9  the bar of the SCALEREF quote's OWN minute (panel 16.34 stamps it) — the only comparable bar
// Both plugins failed twice on 2026-09-16 because the rule lived inside each .cpp behind the SDK. It lives here now,
// once, on plain arrays, and plugin/test_contractoffset_logic.cpp pins every case above.
#pragma once

namespace col {

struct Bar { int y, m, d; double sod; float close; };   // local date, CT sec-of-day, close

const double RTH_A = 8 * 3600.0 + 30 * 60.0, RTH_B = 15 * 3600.0;

inline bool dateBefore(int y1, int m1, int d1, int y2, int m2, int d2) { return y1 < y2 || (y1 == y2 && (m1 < m2 || (m1 == m2 && d1 < d2))); }
inline bool dateSame  (int y1, int m1, int d1, int y2, int m2, int d2) { return y1 == y2 && m1 == m2 && d1 == d2; }

// Returns the index of the anchor bar, or -1 for "use the live close". bars[0..n) in chart order (oldest first).
//   scaleRefSo >= 0 with a date: the last bar stamped at or before that minute ON that date (searching back up to
//     6000 bars); if the chart has no bar on that date, fall through.
//   else if asofSo >= 0: the last bar stamped inside RTH (08:30–15:00) at or before ASOF on the last bar's date,
//     or the previous session's last RTH bar (earlier dates, RTH stamps only), up to 3000 bars back.
//   else: -1.
inline int anchorIndex(const Bar* bars, int n, double asofSo, double scaleRefSo, int sy, int sm, int sd)
{
    if (n < 1) return -1;
    if (scaleRefSo >= 0 && sy > 0) {
        for (int i = n - 1; i >= 0 && i >= n - 6000; i--) {
            const Bar& b = bars[i];
            if (dateBefore(sy, sm, sd, b.y, b.m, b.d)) continue;          // after the quote's day
            if (dateBefore(b.y, b.m, b.d, sy, sm, sd)) break;             // before it: the date is not on this chart
            if (b.sod > scaleRefSo + 1.0) continue;
            if (b.close > 0) return i;
            break;
        }
    }
    if (asofSo >= 0) {
        const Bar& last = bars[n - 1];
        for (int i = n - 1; i >= 0 && i >= n - 3000; i--) {
            const Bar& b = bars[i];
            bool sameDay = dateSame(b.y, b.m, b.d, last.y, last.m, last.d);
            if (sameDay && b.sod > asofSo + 1.0) continue;
            if (b.sod < RTH_A || b.sod > RTH_B + 1.0) continue;
            if (b.close > 0) return i;
            break;
        }
    }
    return -1;
}

// the offset itself, with the plausibility clamp both plugins apply (an NQ chart against an ES book, a bad anchor)
inline bool offsetFor(float chartClose, float scaleRef, float& off)
{
    if (!(chartClose > 0) || !(scaleRef > 0)) return false;
    off = chartClose - scaleRef;
    if (off < -300.0f || off > 300.0f) return false;
    return true;
}

} // namespace col
