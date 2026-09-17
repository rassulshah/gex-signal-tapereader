// test_kingtracker_rows.js — Gate A for lsKingTracker: the panel's King-journey sampler (ktrkSample) and the KINGTRACK /
// KINGNOW rows it writes. Executed with stubs, mutation-checked.   node test_kingtracker_rows.js
'use strict';
const fs = require('fs');
const SRC = fs.readFileSync(__dirname + '/current/gex-signal-tapereader.user.js', 'utf8');
function extract(name) {
  const i = SRC.indexOf('function ' + name + '(');
  if (i < 0) throw new Error('missing ' + name);
  let d = 0, j = SRC.indexOf('{', i);
  for (; j < SRC.length; j++) { if (SRC[j] === '{') d++; else if (SRC[j] === '}') { d--; if (d === 0) break; } }
  return SRC.slice(i, j + 1);
}
function extractVar(name) { const m = SRC.match(new RegExp('\\nvar ' + name + '\\s*=[^\\n]*?;')); if (!m) throw new Error('no var ' + name); return m[0].replace(/\/\/.*$/, '') + '\n'; }
const CODE = [extractVar('KTRK_CONFIRM_N'), extractVar('KTRK_HYST'), extractVar('KTRK_OSC_LOOKBACK'), extract('ktrkConfirmFresh'), extract('ktrkFresh'), extract('ktrkIfKing'), extract('ktrkSample')].join('\n');

// the world: four books, the tape King per book supplied by the test; the clock and the day supplied by the test
const W = { so: 30780, day: '2026-9-16', king: {}, saved: 0, ifc: null };
function build(code) {
  const pre = `
    var CFG={ irt:{ on:true } };
    var KTRK_BOOKS=[ { book:'SPX', fam:'ES', fut:'ES1', srcs:['SPXW','SPX'] }, { book:'SPY', fam:'ES', fut:'ES1', srcs:['SPY'] }, { book:'QQQ', fam:'NQ', fut:'NQ1', srcs:['QQQ'] }, { book:'NDX', fam:'NQ', fut:'NQ1', srcs:['NDXP','NDX'] },
                     { book:'IF', fam:'ES', fut:'ES1', srcs:[], ifSym:'SPX', conv:'SPXW' }, { book:'IFQ', fam:'NQ', fut:'NQ1', srcs:[], ifSym:'QQQ', conv:'QQQ' } ];
    function todayKey(){ return __W.day; }
    function ctNowSecOfDay(){ return __W.so; }
    function ktrkTapeKing(B){ return __W.king[B.book] || null; }
    function futDerBookKing(){ return null; }
    function saveKingTrack(){ __W.saved++; }
    function ifChain(sym){ return (__W.ifc && __W.ifc[sym]) || null; }
    function skylitFutPx(fut, book, px){ return { px: px * (book === 'SPXW' ? 1.0094 : (book === 'QQQ' ? 41.2 : 1)), ratio: 1 }; }
    var KTRK=ktrkFresh(), KTRK_NOW={ SPX:null, SPY:null, QQQ:null, NDX:null, IF:null, IFQ:null }, KTRK_CONFIRM=ktrkConfirmFresh();
  `;
  return new Function('__W', pre + code + '\nreturn { sample:ktrkSample, state:()=>({KTRK,KTRK_NOW,KTRK_CONFIRM}), N:KTRK_CONFIRM_N, LB:KTRK_OSC_LOOKBACK, setOn:(v)=>{ CFG.irt.on=v; } };')(W);
}
let fails = 0, n = 0;
function ok(c, msg, got) { n++; if (!c) { fails++; console.log('FAIL ' + n + ': ' + msg + (got !== undefined ? ' -> ' + JSON.stringify(got) : '')); } }
function tick(M, spxKing, secs) { W.so += (secs === undefined ? 30 : secs); W.king = { SPX: spxKing }; M.sample(); return M.state(); }

// ---- the journey of 2026-09-16 on the SPX book: 7500 at the open -> 7600 at 09:19 -> 7685 at 09:47, with chatter
let M = build(CODE); W.so = 30780; W.day = '2026-9-16'; W.saved = 0;
let S = tick(M, { k: 7504.19, strike: 7500, pct: 100, ageS: 1 }, 0);
ok(S.KTRK.SPX.length === 1 && S.KTRK.SPX[0].strike === 7500 && S.KTRK.SPX[0].es === 7504.19 && S.KTRK.SPX[0].so === 30780, 'the first sample SEEDS the journey (7500 @ 08:33)', S.KTRK.SPX);
ok(S.KTRK_NOW.SPX && S.KTRK_NOW.SPX.strike === 7500, 'KINGNOW = the seed');
ok(W.saved === 1, 'the seed is saved');
S = tick(M, { k: 7504.19, strike: 7500, pct: 100 }); S = tick(M, { k: 7504.19, strike: 7500, pct: 100 });
ok(S.KTRK.SPX.length === 1, 'the same strike again is not a step');
// a challenger must be seen KTRK_CONFIRM_N times in a row before it is a roll
for (let i = 1; i < M.N; i++) { S = tick(M, { k: 7604.25, strike: 7600, pct: 100 }); ok(S.KTRK.SPX.length === 1, 'challenger 7600 sample ' + i + ' of ' + M.N + ': not yet a roll (dwell)', S.KTRK.SPX.length); }
ok(S.KTRK_NOW.SPX.strike === 7600, 'KINGNOW already shows the challenger while it dwells (the live right edge)');
S = tick(M, { k: 7604.25, strike: 7600, pct: 100 });
ok(S.KTRK.SPX.length === 2 && S.KTRK.SPX[1].strike === 7600 && S.KTRK.SPX[1].es === 7604.25, 'the ' + M.N + 'th sample confirms the roll: step 2 = 7600', S.KTRK.SPX);
// a break in the dwell resets the count
S = tick(M, { k: 7688.37, strike: 7685, pct: 100 }); S = tick(M, { k: 7688.37, strike: 7685, pct: 100 }); S = tick(M, { k: 7604.25, strike: 7600, pct: 100 });
ok(S.KTRK.SPX.length === 2 && S.KTRK_CONFIRM.SPX.n === 0, 'two samples of 7685 then back to 7600: no roll, the dwell count resets');
for (let i = 0; i < M.N; i++) S = tick(M, { k: 7688.37, strike: 7685, pct: 100 });
ok(S.KTRK.SPX.length === 3 && S.KTRK.SPX[2].strike === 7685, 'a full dwell on 7685 -> step 3 (the 09:47 King)');
// ANTI-OSCILLATION (v16.15): a "new" King that is a strike the line sat on in the last KTRK_OSC_LOOKBACK steps is chatter
for (let i = 0; i < M.N + 1; i++) S = tick(M, { k: 7604.25, strike: 7600, pct: 100 });
ok(S.KTRK.SPX.length === 3 && S.KTRK_NOW.SPX.strike === 7685 && S.KTRK_NOW.SPX.es === 7688.37, 'back to 7600 (a strike within the last ' + M.LB + ' steps) is chatter: no step, KINGNOW holds the incumbent');
for (let i = 0; i < M.N; i++) S = tick(M, { k: 7757.30, strike: 7750, pct: -100 });
ok(S.KTRK.SPX.length === 4 && S.KTRK.SPX[3].strike === 7750 && S.KTRK_NOW.SPX.pct === -100, 'a genuinely new strike (7750, negative King) rolls after the dwell; the pct rides on KINGNOW');
// the SPY book is independent and a missing King leaves it untouched
ok(!S.KTRK_NOW.SPY && S.KTRK.SPY.length === 0, 'no SPY King supplied -> the SPY journey stays empty');
// a new day resets the journey
W.day = '2026-9-17'; S = tick(M, { k: 7700, strike: 7700, pct: 100 });
ok(S.KTRK.day === '2026-9-17' && S.KTRK.SPX.length === 1 && S.KTRK.SPX[0].strike === 7700, 'a new day starts a fresh journey');
// the export is off -> nothing samples
{ const M2 = build(CODE); M2.setOn(false); W.day = '2026-9-16'; W.king = { SPX: { k: 7500, strike: 7500 } }; M2.sample(); ok(M2.state().KTRK.SPX.length === 0, 'export off -> no sampling'); }

// ---- (v16.38) THE IF MAGNET BOOKS — the same sampler on InsiderFinance's 0DTE chain (the largest |net| strike of gexProf)
{ const Mi = build(CODE); W.day = '2026-9-16'; W.so = 30780; W.king = {}; W.saved = 0;
  const chain = (rows, extra) => Object.assign({ err: null, stale: false, ageMin: 2, dte0: { lv: { gexProf: rows } } }, extra || {});
  W.ifc = { SPX: chain([[7600, -2e8, -1e8], [7650, 1e8, 5e7], [7675, 3e8, 1.5e8], [7700, 1e8, 0]]), QQQ: chain([[600, 1e7, 2e7], [605, -5e7, -2e7]]) };
  let s = tick(Mi, undefined, 0);
  ok(s.KTRK.IF.length === 1 && s.KTRK.IF[0].strike === 7675 && Math.abs(s.KTRK.IF[0].es - 7675 * 1.0094) < 0.01 && s.KTRK_NOW.IF.pct === 100, 'IF: the Magnet is the largest |net| strike (7675, calls + puts) at strike x the SPXW ratio, +100', s.KTRK.IF);
  ok(s.KTRK.IFQ.length === 1 && s.KTRK.IFQ[0].strike === 605 && Math.abs(s.KTRK.IFQ[0].es - 605 * 41.2) < 0.01 && s.KTRK_NOW.IFQ.pct === -100, 'IFQ: the QQQ chain\'s Magnet (605, net negative -> -100) on the NQ scale', s.KTRK.IFQ);
  ok(s.KTRK.SPX.length === 0, 'the tape books are untouched by the chain (no tape King supplied)');
  // a new Magnet must dwell like any King
  W.ifc.SPX = chain([[7600, -2e8, -1e8], [7650, 1e8, 5e7], [7675, 3e8, 1.5e8], [7700, 5e8, 0]]);
  for (let i = 1; i < Mi.N; i++) s = tick(Mi);
  ok(s.KTRK.IF.length === 1 && s.KTRK_NOW.IF.strike === 7700, 'IF: a new Magnet (7700) dwells ' + Mi.N + ' samples before it is a step; KINGNOW shows it meanwhile');
  s = tick(Mi); ok(s.KTRK.IF.length === 2 && s.KTRK.IF[1].strike === 7700, 'IF: the roll confirms on the ' + Mi.N + 'th sample');
  // a stale or missing chain samples nothing — no phantom step, no repaint
  W.ifc.SPX = chain([[7600, -9e8, 0]], { stale: true }); s = tick(Mi);
  ok(s.KTRK.IF.length === 2 && s.KTRK_NOW.IF.strike === 7700, 'IF: a STALE chain does not sample (the line holds 7700, no 7600 step)');
  W.ifc.SPX = chain([[7600, -9e8, 0]], { err: 'down' }); s = tick(Mi);
  ok(s.KTRK.IF.length === 2, 'IF: a chain in error does not sample');
  W.ifc = null; s = tick(Mi);
  ok(s.KTRK.IF.length === 2 && s.KTRK.IFQ.length === 1, 'IF: no companion at all -> both IF books hold');
  W.ifc = { SPX: chain([]) }; s = tick(Mi); ok(s.KTRK.IF.length === 2, 'IF: an empty gexProf does not sample');
  ok(/out\.push\('KINGTRACK,'\+KB\.fam\+','\+KB\.book/.test(SRC) && /\{ book:'IF',\s+fam:'ES'/.test(SRC) && /\{ book:'IFQ', fam:'NQ'/.test(SRC), 'the export loop writes the IF / IFQ books with the same KINGTRACK / KINGNOW grammar (KingTrackerLogic.h parses them)');
  ok(/AU\.rolls=\{[^}]*IF:\(\(KTRK&&KTRK\.IF\)\|\|\[\]\)\.length-1/.test(SRC), 'the audit carries the IF roll count');
  W.ifc = null; }

// ---- the rows the export writes (wiring — the logic above is executed)
ok(/out\.push\('KINGTRACK,'\+KB\.fam\+','\+KB\.book\+','\+gpI\(P\.so\)\+','\+gpF2\(P\.es\)\+','\+gpI\(P\.strike\)\)/.test(SRC), 'KINGTRACK,<fam>,<book>,<so>,<es>,<strike> — the grammar lsKingTracker parses (KingTrackerLogic.h)');
ok(/out\.push\('KINGNOW,'\+KB\.fam\+','\+KB\.book\+','\+gpF2\(NW\.es\)\+','\+gpI\(NW\.strike\)\+','\+gpI\(NW\.pct\)\)/.test(SRC), 'KINGNOW,<fam>,<book>,<es>,<strike>,<pct>');
ok(/AU\.rolls=\{ SPX:\(\(KTRK&&KTRK\.SPX\)\|\|\[\]\)\.length-1/.test(SRC), 'the audit carries the roll count (steps - 1)');

// ---- mutations
function mutated(re, rep) { const c = CODE.replace(re, rep); if (c === CODE) throw new Error('mutation did not apply: ' + re); return build(c); }
{ const Mx = mutated(/if\(cf\.n>=KTRK_CONFIRM_N\)\{/, 'if(true){'); W.day = '2026-9-16'; W.so = 30780; tick(Mx, { k: 7504, strike: 7500 }, 0); const s = tick(Mx, { k: 7604, strike: 7600 });
  ok(s.KTRK.SPX.length === 2, 'mutation: dropping the dwell rolls on the first sample (the cases above would fail)'); }
{ const Mx = mutated(/if\(oscBack\)\{/, 'if(false){'); W.day = '2026-9-16'; W.so = 30780; tick(Mx, { k: 7504, strike: 7500 }, 0);
  for (let i = 0; i < 4; i++) tick(Mx, { k: 7604, strike: 7600 }); for (let i = 0; i < 4; i++) tick(Mx, { k: 7504, strike: 7500 });
  ok(Mx.state().KTRK.SPX.length === 3, 'mutation: dropping the anti-oscillation lets 7500 -> 7600 -> 7500 record three steps'); }
console.log((fails ? 'FAIL ' : 'PASS ') + (n - fails) + '/' + n + ' assertions (King tracker rows — the panel side, v16.38 IF books)');
process.exit(fails ? 1 : 0);
