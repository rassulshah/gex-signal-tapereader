// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
var PEAK_OFF_MARK = 8;       // show '↓peak' only when current is >= this many pts below session peak
// --- Adaptive proximity (relative trap read #2) ---
// 'Not far away' overhead/underfoot resistance is judged against how far a
// normal 12-30 min move actually travels NOW, derived from recent 3m bar range,
// clamped so it never misbehaves on a freak bar.
var PROX_BARS = 10;          // lookback bars for average 3m range
var PROX_MOVE_BARS = 6;      // a typical bounce spans ~this many 3m bars
var PROX_MIN_STRIKES = 1;    // floor: never consider resistance closer than this irrelevant
var PROX_MAX_STRIKES = 5;    // ceiling: never flag resistance beyond this as 'in reach'
// --- #1 target scoring weights (confluence) ---
var TS_W_STRENGTH = 1.0;     // weight on %King strength
var TS_W_VELOCITY = 1.4;     // weight on accumulation velocity (build rate)
var TS_W_PROXIMITY = 0.8;    // weight on nearness to price
var TS_W_STRUCT = 1.2;       // structural bonus for King / Gatekeeper roles
var TS_MIN_PCT = 20;         // a strike must be at least this %King to be a target candidate
var GK_MAX_DIST = 5;
