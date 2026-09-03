// ============================================================================================
// test_ladder_header.js — (v15.49) THE COLUMNS WERE NOT MISSING; THEIR NAMES WERE.
//
// Operator, 2026-09-02: "alot of the columns are missing".
// MEASURED on his panel — every column present, correctly positioned, and inside the horizontal
// view. The HEADER ROW had scrolled off the top:
//     wrap.scrollTop 15.15 · header.topRel -15 · insideView FALSE · position 'relative'
// v15.28 made the view OPEN on the expected-move band rather than at the top of the content, so on
// most days the box is scrolled from the first render and a `relative` header leaves with it.
// ⚠ FROM THE OUTSIDE, "the columns are missing" AND "the column names are missing" ARE THE SAME
// REPORT. His was the accurate description of what he could see.
// ============================================================================================
const fs=require('fs');
const src=fs.readFileSync('./current/gex-signal-tapereader.user.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,200):''));} };

// ---- 1 · THE HEADER PINS ---------------------------------------------------------------------
const css=(src.match(/#gpts-body \.g3ladhd\{[^}]*\}/)||[''])[0];
ok(css.length>0, 'h1 the header rule is findable', css);
ok(/position:sticky/.test(css), 'h2 it is STICKY, so it survives a scrolled box', css);
ok(/top:0/.test(css), 'h2b ...pinned to the top of the scroll box');
ok(!/position:relative/.test(css), 'h2c ...and no longer scrolls away with the rows');
// ⚠ NOT DECORATION: without these the rows scroll THROUGH the header — a legible-looking header
// with node bars crossing it, which is worse than the fault being fixed.
ok(/z-index:\s*\d+/.test(css), 'h3 it sits ABOVE the rows', css);
ok(/background:#[0-9a-f]{6}/i.test(css), 'h3b ...on an opaque ground, so rows cannot show through');

// ---- 2 · THE BOX REALLY DOES OPEN SCROLLED ----------------------------------------------------
// ⚠ this is why a relative header could never work here — it is not an edge case, it is the design.
ok(/THE VIEW OPENS ON THE EXPECTED MOVE AND SCROLLS TO THE REST/.test(src),
   'h4 the view is documented as opening on the band, not at the top');
ok(/overflow-x:auto/.test(src) || /g3ladwrap\{[^}]*overflow/.test(src),
   'h4b ...inside a scrolling wrapper');

// ---- 3 · AND THE MARK COLUMN CAN NOW BE DIAGNOSED --------------------------------------------
// ⚠ MEASURED the same minute: ZERO marks, with a row 0.5 points from price and LVL_INPLAY_PTS=3.
// There was no way to tell whether levelMarkerOf RETURNED NULL or THREW, because the catch
// discarded both. An empty catch makes those two identical from the outside.
ok(/swallow\('levelMarker', eMk\)/.test(src),
   'm1 a throwing marker is now RECORDED, not silently discarded');
ok(/LVLMK_LAST\.push\(\{ k:P\.k, disp:P\.disp, now:now/.test(src),
   'm2 ...and every row records what the marker was ASKED');
ok(/m:mk\?mk\.m:null/.test(src), 'm2b ...and what it ANSWERED, including null');
// (v15.51) the threshold is now the ATR band when one exists, and the row says WHICH it used —
// a refusal at "thresh 3" and one at "thresh 0.48 (atr)" are different refusals.
ok(/d:\+Math\.abs\(P\.disp-now\)\.toFixed\(2\)/.test(src) && /thresh:\(_bd!=null\)\?\+_bd\.toFixed\(3\):LVL_INPLAY_PTS/.test(src) && /band:\(_bd!=null\)\?'atr':'legacy'/.test(src),
   'm2c ...with the distance AND the threshold it actually used (atr band, or legacy and says so), so a refusal explains itself');
ok(/window\.__gptsDebug\.mark = function/.test(src), 'm3 __gptsDebug.mark() answers it in one call');
// ⚠ one render, one record — otherwise the last N entries straddle two renders and the console
// shows two different prices as though they were one moment.
ok(/LVLMK_LAST\.length=0;/.test(src), 'm4 the record resets each render');
ok(src.indexOf('LVLMK_LAST.length=0')<src.indexOf('LVLMK_LAST.push'),
   'm4b ...before the rows are walked, not after');

// ---- 4 · THE THRESHOLD IS SCALE-DEPENDENT, AND THAT IS RECORDED, NOT SILENTLY CHANGED ---------
// ⚠ LVL_INPLAY_PTS is 3 CHART points. On a cash chart that is 3 SPY points — THREE strike gaps.
// On his ES chart it is 3 ES points — under HALF a strike gap. The same constant means two very
// different tests. Measured in the harness (cash): FIVE rows marked at once, d 0.15..2.65.
// ⚠ NOT CHANGED IN THIS BUILD: it alters what IN PLAY means, which is his call, not mine.
const m=/var LVL_INPLAY_PTS=(\d+)/.exec(src);
ok(!!m, 'x1 the constant is findable', m&&m[1]);
ok(+m[1]===3, 'x1b ...and unchanged at 3 — this build did NOT redefine IN PLAY', m&&+m[1]);
// executed: the same constant, two charts, two very different windows
const SPY_STRIKE=1, ES_STRIKE=5, ES_PER_SPY=10.035;
ok((3/SPY_STRIKE)===3, 'x2 EXECUTED: on cash, 3 points spans THREE strike gaps', 3/SPY_STRIKE);
ok((3/ES_STRIKE)<0.7, 'x2b ...on ES it spans under three quarters of ONE', +(3/ES_STRIKE).toFixed(2));
ok(Math.round(3*ES_PER_SPY/ES_STRIKE)===6,
   'x2c ...i.e. the cash window is ~6 ES strikes wide and the ES window is under one',
   Math.round(3*ES_PER_SPY/ES_STRIKE));

console.log('test_ladder_header: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
