// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
var NCHART_MIN=90;                 // minutes of session in view
var NCHART_H=132;                  // px


// ---- (v11.36) SKEW — THEIRS, NOT OURS ----
// Their page publishes "Δ Skew +3.3 pts · puts rich, downside bid" and a 25-delta variant. It is
// directional and, unlike anything open-interest based, it moves intraday because IV does.
// Index skew is PERSISTENTLY negative-ish — puts are always bid — so the raw level would print the
// same verdict every day. The vote is the level against its own recent range.
// ⚠⚠ (v11.95) THE SKEW BADGE WAS PERMANENTLY GREY, AND THE CAUSE WAS TWO FAULTS STACKED.
// Live 2026-08-24: v=0.11, lo=2.2, hi=2.2, n=240.
//  1. THE HISTORY AND THE VALUE ARE DIFFERENT METRICS. v11.37 switched the reading from their
//     published header skew (~2.2) to a computed 25-delta put-call IV spread (~0.11) — and kept
//     appending to the SAME store. The current value was being ranked against a range built from a
//     quantity it is not. Pattern 1, mislabeling, in its purest form.
//  2. THE DIRECTION TEST IS GATED ON `if(hi>lo)`, so a degenerate range silently yields dir 0 for
//     ever. A dead range read exactly like a neutral market.
// The key is versioned by SOURCE. A history recorded against a different metric is not history, it is
// noise wearing history's name, so changing the source starts a new range and SAYS it is rebuilding.
var SKEW_SRC='skew25';                       // bump this string whenever the metric changes
var SKEW_HIST_KEY='gpts_skew_hist_v2_'+SKEW_SRC, SKEW_HIST_MAX=400, SKEW_BAND=0.25;
