// ============================================================================================
// test_lane_axis.js — (v15.42) THE KING LANE'S AXIS RAN TO THE WALL CLOCK, AND THE ROLL LANE
// WENT BLANK WITHOUT SAYING WHY.
//
// Operator, 2026-09-02: "the king node paths are still empty. the roll column is also empty."
//
// ⚠ MEASURED on his panel at 19:50 CT, a 24px lane holding six SPXW runs of 15/103/32/105/102
// minutes plus the one still holding since 14:27:
//     widths  1.0 · 2.5 · 1.0 · 2.5 · 2.4 · 11.4 px
// The axis spanned 08:30 → 19:50, so the crown that was merely STILL THERE took 57% of the lane and
// five real migrations rendered as one-to-two-pixel ticks. At 1px "empty" is the correct reading.
// ⚠⚠ THE FAULT GROWS EVERY HOUR THE TAB STAYS OPEN, which is exactly why it looked fine intraday.
// ============================================================================================
const fs=require('fs');
const src=fs.readFileSync('./current/gex-signal-tapereader.user.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,200):''));} };

// ===== 1 · THE ARITHMETIC, EXECUTED — this is a geometry bug, so measure geometry =============
// the lane maps [openMs, nowMs] onto [x+2, x+w-3]; his lane is 24px wide
const LANE_W=24, X=2;
function widths(openMs, nowMs, ptsMin){          // ptsMin = migration times, minutes after the open
  const span=nowMs-openMs, usable=LANE_W-5;
  const xAt=t=>{ let f=(t*60000)/span; f=Math.max(0,Math.min(1,f)); return X+2+f*usable; };
  return ptsMin.map((t,i)=> +( (i+1<ptsMin.length ? xAt(ptsMin[i+1]) : X+LANE_W-2) - xAt(t) ).toFixed(1));
}
const PTS=[0,15,118,150,255,357];                // 08:30, 08:45, 10:28, 11:00, 12:45, 14:27
const OPEN=0, CLOSE=390*60000, EVENING=680*60000;  // 15:00 and 19:50, in ms after the open

const bad=widths(OPEN, EVENING, PTS);
const good=widths(OPEN, CLOSE, PTS);
ok(bad.filter(w=>w<3).length>=4,
   'a1 EXECUTED: on the WALL clock, four+ runs render under 3px — his 1.0/2.5/1.0/2.5', bad);
ok(bad[bad.length-1]/bad.reduce((s,w)=>s+w,0) > 0.5,
   'a1b ...and the "still there" run alone takes over half the lane', +(bad[5]/bad.reduce((s,w)=>s+w,0)).toFixed(2));
// ⚠ WHAT THE CLAMP ACTUALLY BUYS, stated as measured rather than as hoped. My first cut asserted
// "at most one run under 3px" and it failed at [0.7, 5, 1.6, 5.1, 5, 2.6] — TWO are under 3px, and
// that is the truth: a 15-minute run in a 24px lane spanning 390 minutes IS sub-pixel, and no clamp
// can change it. The clamp fixes what it can fix, and the residual is a property of the lane's
// width, not a bug. Asserting the hope would have hidden the limit.
ok(good[1]>4 && good[3]>4 && good[4]>4,
   'a2 EXECUTED: the 103, 105 and 102-minute runs become comfortably visible', [good[1],good[3],good[4]]);
// ⚠ 1.7x, not 2x. I wrote "roughly double" and the assertion said 1.7 — so the claim is 1.7.
// Rounding a measurement up to a nicer number in a test is how a changelog ends up overstating a fix.
ok(good[1]/bad[1] > 1.7 && good[4]/bad[4] > 1.7,
   'a2b ...about 1.7x what the wall clock gave them', [+(good[1]/bad[1]).toFixed(2), +(good[4]/bad[4]).toFixed(2)]);
const shareBad=bad[5]/bad.reduce((s,w)=>s+w,0), shareGood=good[5]/good.reduce((s,w)=>s+w,0);
ok(shareBad>0.5 && shareGood<0.25,
   'a2c ...and the "still there" run stops eating the lane: 57% -> under a quarter',
   {wallClock:+(shareBad*100).toFixed(0)+'%', clamped:+(shareGood*100).toFixed(0)+'%'});
// ⚠ THE RESIDUAL, ASSERTED SO IT IS NOT MISTAKEN FOR FIXED. A short run stays sub-pixel; the code
// floors every run at 1px so it is never invisible, but 15 minutes of 390 cannot be legible in 24px.
ok(good[0]<1.5 && good[2]<3,
   'a2d ...while SHORT runs stay sub-pixel — a real limit of a 24px lane, not something clamping fixes',
   {run15min:good[0], run32min:good[2]});
ok(/Math\.max\(1,\(x1-x0\)\)/.test(src),
   'a2e ...and every run is floored at 1px, so a short one is faint but never absent');
// ⚠ THE FAULT GROWS. A test that only checked 19:50 would pass at 15:30 and fail at midnight.
const later=widths(OPEN, 900*60000, PTS);
ok(later[1] < bad[1] && bad[1] < good[1],
   'a3 the longer the tab stays open the worse it gets — 15:00 > 19:50 > midnight', [good[1],bad[1],later[1]]);

// ===== 2 · THE CLAMP IS DATA, NOT CLOCK ARITHMETIC =============================================
const lane=(src.match(/var openMs=null, lastMs=null, nowMs=clockNow\(\);[\s\S]{0,420}/)||[''])[0];
ok(lane.length>0, 'a4 the lane reads clockNow() and the series together');
ok(/lastMs=cs\[cs\.length-1\]\.t/.test(lane),
   'a4b ...taking the END of the day from the LAST CLOSED BAR — the same series the start comes from');
ok(/!_P\.rth && lastMs!=null && nowMs>lastMs\) nowMs=lastMs/.test(lane),
   'a4c ...and clamping only once RTH is over, so nothing changes intraday');
ok(!/openMs\+.*\*60000/.test(lane) && !/P\.close-P\.open/.test(lane),
   'a4d ...with NO clock arithmetic and no timezone maths — the end of the day is DATA');
// ⚠ intraday the axis must still track the live clock, or the lane stops growing during the session
ok(/nowMs>lastMs/.test(lane), 'a4e ...and it only ever SHRINKS the axis, never extends it');

// ===== 3 · THE ROLL LANE NAMES ITS SILENCE =====================================================
// ⚠ "no rolls today", "retired at the close" and "the latch is empty" all rendered as NOTHING.
const empty=(src.match(/if\(!rolls \|\| !rolls\.length\)\{[\s\S]{0,1400}?\n    \}/)||[''])[0];
ok(empty.length>0, 'r1 the empty branch now returns an element instead of an empty string');
ok(/ROLL ARROWS \\u2014 EMPTY, AND HERE IS WHY/.test(empty), 'r1b ...carrying a reason');
['replayed session','close-of-session book','Outside RTH','noise floor'].forEach(w=>
  ok(empty.indexOf(w)>=0, 'r1·'+w+' ...distinguishing the "'+w+'" case'));
ok(/ROLL_SIG_N/.test(empty), 'r1c ...and naming the actual threshold, not a vague one');

// ===== 4 · THE ARROWS BELONG TO THE BOOK ON SCREEN =============================================
// ⚠ After the close the face serves the CLOSE-OF-SESSION book (v14.55) — it drew that book's nodes,
// states and ROC and blanked its ROLLS. One face, two opinions about which session is on screen.
const rl=(src.match(/function rollsLive\(\)\{[\s\S]{0,320}?\n\}/)||[''])[0];
ok(/showingStaleBook\(\)\) return true/.test(rl),
   'r2 rolls are served whenever the frozen close-of-session book is');
ok(/replayOn\(\)\) return true/.test(rl), 'r2b ...and in replay, as before');
ok(/P&&P\.rth/.test(rl), 'r2c ...and during RTH, as before');
// ⚠ NOT A NEW CLAIM: rollLatched() still refuses to draw today's arrows over a replayed bar.
ok(/if\(typeof replayOn==='function' && replayOn\(\)\) return replayRolls/.test(src),
   'r3 rollLatched still routes replay to the frame’s own rolls — no live leak');

console.log('test_lane_axis: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
