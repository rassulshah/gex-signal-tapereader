// test_daystats_cond.js — (v16.32) the conditional E row: hodlodCondE + gpOpenWindow, executed, mutation-checked.
// node test_daystats_cond.js   (reads current/gex-signal-tapereader.user.js directly)
'use strict';
const fs = require('fs');
const SRC = fs.readFileSync(__dirname + '/current/gex-signal-tapereader.user.js', 'utf8');

function extract(name) {                       // brace-matched function body, top-level `function name(`
  const i = SRC.indexOf('function ' + name + '(');
  if (i < 0) throw new Error('missing ' + name);
  let d = 0, j = SRC.indexOf('{', i);
  for (; j < SRC.length; j++) { if (SRC[j] === '{') d++; else if (SRC[j] === '}') { d--; if (d === 0) break; } }
  return SRC.slice(i, j + 1);
}
function extractVar(name) {
  const i = SRC.indexOf('var ' + name + '=');
  const j = SRC.indexOf('};', i);
  return SRC.slice(i, j + 2);
}
const CODE = [extractVar('GP_COND_FALLBACK'), extract('gpOpenWindow'), extract('hodlodCondE')].join('\n');
function build(code) {
  return new Function(code + '\nreturn {gpOpenWindow, hodlodCondE, GP_COND_FALLBACK};')();
}
let fails = 0, n = 0;
function ok(c, msg) { n++; if (!c) { fails++; console.log('FAIL ' + n + ': ' + msg); } }

const O = 8 * 3600 + 30 * 60;
// tool bars (3-min, stamped by END) for a day that opens at the bottom of its first hour: open 7684, low 7683 at 08:33,
// rising to 7712 by 09:30 — Tue 15 Sep in shape (open in the top of the OR would be the mirror)
function bars(n, f) { const out = []; for (let k = 0; k < n; k++) out.push(Object.assign({ so: O + (k + 1) * 180 }, f(k))); return out; }
const upDay = bars(20, k => ({ o: 7684 + k, h: 7686 + k * 1.4, l: (k === 0 ? 7683 : 7684 + k * 0.9), c: 7685 + k }));
const M = build(CODE);

// ---- gpOpenWindow
const w30 = M.gpOpenWindow(upDay, O, 30), w60 = M.gpOpenWindow(upDay, O, 60);
ok(w30 && w30.n === 10 && w30.complete, 'w30 has 10 bars and is complete');
ok(w60 && w60.n === 20 && w60.complete, 'w60 has 20 bars and is complete');
ok(w30.l === 7683 && w30.tL === 3, 'w30 low 7683 at minute 3 (bar END, like the study)');
ok(Math.abs(w30.h - (7686 + 9 * 1.4)) < 1e-9 && w30.tH === 30, 'w30 high is the last bar, minute 30');
ok(w30.pos < 0.1, 'open sits at the bottom of the OR (pos ~0.03)');
ok(M.gpOpenWindow(upDay.slice(0, 5), O, 30).complete === false, 'a 15-minute window is not complete');
ok(M.gpOpenWindow([], O, 30) === null, 'no bars -> null');

// ---- hodlodCondE by stage
const base = { condstats: null };                                  // no courier tables -> the baked fallback
let e = M.hodlodCondE(base, 10, M.gpOpenWindow(upDay.slice(0, 3), O, 30), null, null, null);
ok(e.basis === 'pre-open' && e.t1 === 24 && e.t2 === 288 && e.lodPct === 51 && e.t2P20 === 147 && e.t2P50 === 288, 'pre-open: pooled medians / majority (fallback rebaked 2026-09-17, n=301) + the 2ND-clock percentiles (16.45)');
e = M.hodlodCondE(base, 33, w30, null, null, null);
ok(e.basis === 'pos30-bottom' && e.t1 === 21 && e.lodPct === 59, '30 min, open at the OR bottom: the bottom-tercile row');
e = M.hodlodCondE(base, 66, w30, w60, null, null);
ok(e.basis === 'pos60-bottom+orclock' && e.t1 === 3 && e.lodPct === 66 && e.t2 === 303, '60 min, outer third: 1ST clock = the window low\'s clock (minute 3), LOD 66%');
ok(e.t2 > e.t1, '2ND after 1ST');
// the mirror: open at the TOP of its first hour -> HOD first, clock of the window high
const dnDay = bars(20, k => ({ o: 7684 - k, h: (k === 0 ? 7685 : 7684 - k * 0.9), l: 7682 - k * 1.4, c: 7683 - k }));
e = M.hodlodCondE(base, 66, M.gpOpenWindow(dnDay, O, 30), M.gpOpenWindow(dnDay, O, 60), null, null);
ok(e.basis === 'pos60-top+orclock' && e.t1 === 3 && e.lodPct === 35, 'open at the top: HOD-first (LOD 35%), the window high\'s clock');
// middle third: no OR-clock rule, the table row
const midDay = bars(20, k => ({ o: 7684, h: 7690 + (k % 3), l: 7678 - (k % 2), c: 7684 + (k % 2) }));
e = M.hodlodCondE(base, 66, M.gpOpenWindow(midDay, O, 30), M.gpOpenWindow(midDay, O, 60), null, null);
ok(e.basis === 'pos60-middle' && e.t1 === 36 && e.lodPct === 52 && e.t2P20 === 192 && e.t2P50 === 306 && e.t2P80 === 378, 'middle third: the middle row, no OR-clock rule; its own percentiles (16.45)');
// 60 min elapsed but the 60-window incomplete (gap in bars) -> stays on the 30-min row
e = M.hodlodCondE(base, 66, w30, M.gpOpenWindow(upDay.slice(0, 12), O, 60), null, null);
ok(e.basis === 'pos30-bottom', 'incomplete 60-min window -> the 30-min row stands');
// READ IN: the 1ST is the actual first extreme
e = M.hodlodCondE(base, 95, w30, w60, { ok: true, first: 'LOD', firstT: O + 3 * 60 }, { in: true, p: 80 });
ok(e.basis === 'pos60-bottom+orclock' && e.readIn === 'LOD' && e.lodPct === 66, 'READ IN (v16.36): the E row keeps the stage\'s expectation; the call is only noted', e);
e = M.hodlodCondE(base, 95, w30, w60, { ok: true, first: 'HOD', firstT: O + 3 * 60 }, { in: false, p: 40 });
ok(e.basis === 'pos60-bottom+orclock', 'READ not IN: the table stands');
// courier tables win over the fallback
const courier = { condstats: { n: 400, t1Med: 20, t2Med: 280, lodPct: 55, pos30: [{ n: 100, t1: 18, t2: 270, lodPct: 62 }, { n: 100, t1: 30, t2: 290, lodPct: 50 }, { n: 100, t1: 19, t2: 280, lodPct: 40 }], pos60: [{ n: 100, t1: 15, t2: 300, lodPct: 68 }, { n: 100, t1: 35, t2: 305, lodPct: 52 }, { n: 100, t1: 20, t2: 265, lodPct: 33 }] } };
e = M.hodlodCondE(courier, 33, w30, null, null, null);
ok(e.t1 === 18 && e.lodPct === 62 && e.n === 100, 'courier condstats used when present');
e = M.hodlodCondE({ condstats: { t1Med: 20 } }, 10, null, null, null, null);
ok(e.t1 === 24, 'a half-formed courier block is refused -> fallback');
// a thin row (n<8) is refused -> the previous stage stands
const thin = JSON.parse(JSON.stringify(courier)); thin.condstats.pos60[0].n = 5;
e = M.hodlodCondE(thin, 66, w30, w60, null, null);
ok(e.basis === 'pos30-bottom', 'thin pos60 row (n<8) refused -> the 30-min row stands');

// ---- the export wiring exists (grep is only for wiring; the logic above is executed)
ok(/CONDE,'\+CE\.basis/.test(SRC), 'the export writes a CONDE row');
ok(/E\.firstClock=openSecGP\+CE\.t1\*60/.test(SRC), 'the E clocks are overridden from the conditional model');
ok(/condstats:\(j\.condstats/.test(SRC), 'hlBaseNormalise passes condstats');
ok(/CALLg=CALL;/.test(SRC), 'the READ is captured for the conditional model');

// ---- mutations: each must break at least one assertion above
function mutated(re, rep) { const c = CODE.replace(re, rep); if (c === CODE) throw new Error('mutation did not apply: ' + re); return build(c); }
function countFails(Mx) {
  let f = 0;
  const t = (c) => { if (!c) f++; };
  const e1 = Mx.hodlodCondE(base, 66, w30, w60, null, null); t(e1.basis === 'pos60-bottom+orclock' && e1.t1 === 3 && e1.lodPct === 66);
  const e2 = Mx.hodlodCondE(base, 33, w30, null, null, null); t(e2.basis === 'pos30-bottom' && e2.t1 === 21);
  const e3 = Mx.hodlodCondE(base, 95, w30, w60, { ok: true, first: 'LOD', firstT: O + 3 * 60 }, { in: true }); t(e3.readIn === 'LOD');
  const e4 = Mx.hodlodCondE(base, 66, w30, M.gpOpenWindow(upDay.slice(0, 12), O, 60), null, null); t(e4.basis === 'pos30-bottom');
  return f;
}
ok(countFails(mutated(/elapsed>=60 && w60 && w60\.complete/, 'elapsed>=60 && w60')) > 0, 'mutation: ignoring window completeness fires');
ok(countFails(mutated(/if\(W===60 && b!==1\)/, 'if(false)')) > 0, 'mutation: dropping the OR-clock rule fires');
ok(countFails(mutated(/CALL && CALL\.in &&/, 'CALL && false &&')) > 0, 'mutation: dropping the READ note fires');
ok(countFails(mutated(/return p<1\/3\?0:\(p<2\/3\?1:2\);/, 'return 1;')) > 0, 'mutation: collapsing the terciles fires');

console.log((fails ? 'FAIL ' : 'PASS ') + (n - fails) + '/' + n + ' assertions (conditional E row, v16.32)');
process.exit(fails ? 1 : 0);
