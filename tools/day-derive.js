// day-derive.js — re-derive the day model's CSV rows from the audit's AU.day block (panel >= 16.37) with the PANEL'S OWN
// functions (gpDayModel, hodlodCondE, gpOpenWindow, the EM / cond tables), so tools/gp-regress.py --indicator daymodel |
// daystats can diff them against GammaProfile.csv at the same moment.
//     node tools/day-derive.js GammaProfile.audit.json        -> JSON on stdout
// The functions are extracted from current/gex-signal-tapereader.user.js exactly as the Gate A tests do; nothing here is
// a re-implementation. What IS re-derived here (the pre-EM range stages, the clamp, the body lean, the DAYSE clocks) is the
// export's own arithmetic, kept in step with gammaProfileBuild by test_daymodel_em.js / test_daystats_cond.js.
'use strict';
const fs = require('fs'), path = require('path');
const SRC = fs.readFileSync(path.join(__dirname, '..', 'current', 'gex-signal-tapereader.user.js'), 'utf8');
function extract(name) {
  const i = SRC.indexOf('function ' + name + '(');
  if (i < 0) throw new Error('missing ' + name);
  let d = 0, j = SRC.indexOf('{', i);
  for (; j < SRC.length; j++) { if (SRC[j] === '{') d++; else if (SRC[j] === '}') { d--; if (d === 0) break; } }
  return SRC.slice(i, j + 1);
}
function extractVar(name) {
  const i = SRC.indexOf('\nvar ' + name + '=');
  if (i < 0) throw new Error('no var ' + name);
  let d = 0, j = SRC.indexOf('{', i), started = false;
  for (; j < SRC.length; j++) { if (SRC[j] === '{') { d++; started = true; } else if (SRC[j] === '}') { d--; if (started && d === 0) break; } }
  return SRC.slice(i + 1, j + 1) + ';\n';
}
const CODE = [extractVar('GP_EM_MODEL'), extractVar('GP_COND_FALLBACK'), extractVar('SECONDIN_BASE'), extract('gpOpenWindow'), extract('hodlodCondE'), extract('gpDayModel'), extract('siFeats'), extract('siPredict'), extract('siDbin')].join('\n');
const M = new Function(CODE + '\nreturn { gpDayModel, hodlodCondE, GP_EM_MODEL, GP_COND_FALLBACK, SECONDIN_BASE, siPredict, siDbin };')();

const au = JSON.parse(fs.readFileSync(process.argv[2], 'utf8'));
const Y = au.day;
if (!Y || Y.err) { console.log(JSON.stringify({ err: Y ? Y.err : 'no AU.day on the audit (panel < 16.37?)' })); process.exit(0); }
const O = Y.open, out = { open: O, elapsed: Y.elapsed };
const n1 = x => (x == null || isNaN(x)) ? '' : (+x).toFixed(1);
const f2 = x => (x == null || isNaN(x)) ? '' : (+x).toFixed(2);
const I = x => (x == null || isNaN(x)) ? '' : String(Math.round(+x));
const E = Y.base;
if (E) {
  // ---- the range stages the export runs before the EM (gammaProfileBuild, "THE ADAPTIVE EXPECTED MODEL")
  const PM = E.predict || null;
  const Cx = (PM && PM.exante) ? PM.exante : { a: 41.95, b: 0.351 };
  const C3 = (PM && PM.open30) ? PM.open30 : { a: 26.48, b: 1.441 };
  const C6 = (PM && PM.open60) ? PM.open60 : { a: 25.47, b: 1.099 };
  const scl = Y.scl > 0 ? Y.scl : 1, el = Y.elapsed;
  let rng = E.rngPts, basis = 'weekday', drive = 0, stageNote = '';
  if (el >= 30 && Y.w30 && Y.w30.complete) {
    // (16.37) the export's stages run on gpOpenWindow's windows (bars ending <= 30 / 60 min, the studies' definition)
    if (typeof Y.w30.close === 'number') { const d = Y.w30.close - Y.w30.open; drive = d > 0 ? 1 : (d < 0 ? -1 : 0); } else drive = null;
    if (el >= 60 && Y.w60 && Y.w60.complete) { rng = C6.a + C6.b * (Y.w60.h - Y.w60.l) * scl; basis = 'open60'; }
    else { rng = C3.a + C3.b * (Y.w30.h - Y.w30.l) * scl; basis = 'open30'; }
  } else if (Y.pdR != null) { rng = Cx.a + Cx.b * Y.pdR; basis = 'exante'; }
  const DM = M.gpDayModel({ elapsed: el, w30: Y.w30, w60: Y.w60, scl: scl, emEs: Y.emEs, rng: rng, basis: basis });
  rng = DM.rng; basis = DM.basis;
  if (E.rngPts > 0) { const lo = E.rngPts * 0.4, hi = E.rngPts * 2.5; if (rng < lo) rng = lo; if (rng > hi) rng = hi; }
  if (!(rng > 0)) { rng = E.rngPts; basis = 'weekday'; }
  const f = DM.f, eHi = O + f * rng, eLo = O - (1 - f) * rng;
  const drv = (drive === null) ? (Y.model ? Y.model.drive : 0) : drive;
  let eClose = O + (drv !== 0 ? drv * rng * 0.25 : 0); if (eClose > eHi) eClose = eHi; if (eClose < eLo) eClose = eLo;
  out.model = { rng, basis, f, drive: drv, driveReDerived: drive !== null, hi: eHi, lo: eLo, close: eClose, stageNote };
  out.DAYEXP = [f2(O), f2(eHi), f2(eLo), f2(eClose)];
  out.EXPMODEL = [basis, n1(rng), String(drv), f.toFixed(2), Y.emEs != null ? (+Y.emEs).toFixed(1) : '', Y.em ? (Y.em.est ? 'est' : 'pin') : ''];
  // ---- the conditional E row and the DAYSE clocks
  const baseForCond = { condstats: E.hasCond ? E.condstats : null };
  const D = Y.actual ? Object.assign({ ok: true }, Y.actual) : null;
  const CE = M.hodlodCondE(baseForCond, el, Y.w30, Y.w60, D, Y.call);
  out.cond = CE;
  out.CONDE = [CE.basis, n1(CE.t1), n1(CE.t2), I(CE.lodPct), I(CE.n), I(CE.lastHrPct), I(CE.last30Pct), I(CE.t2P20), I(CE.t2P50), I(CE.t2P80)];   // (16.45) + the 2ND clock's p20 / p50 / p80
  // (16.46) READ2 — the second extreme's read, re-derived from the audit's recorded features through the BAKED weights (the
  // audit says which source the panel used; a nightly refit would differ, and the compare below tolerates that by noting src)
  if (Y.second && Y.second.ok) {
    const S2 = Y.second, p = M.siPredict(M.SECONDIN_BASE, S2.d, S2.ml, S2.age, S2.share);
    const tb = M.SECONDIN_BASE.timing[String(M.siDbin(M.SECONDIN_BASE, S2.d))];
    out.second = { side: S2.side, p, arrival: (tb && typeof tb.med === 'number') ? tb.med : null, src: S2.src };
    out.READ2 = [S2.side, String(Math.round(100 * p)), S2.d.toFixed(3), n1(S2.ml), n1(S2.age), S2.share.toFixed(3), (tb && typeof tb.med === 'number') ? I(tb.med) : '', I(M.SECONDIN_BASE.n), S2.src === 'nightly' ? 'nightly' : 'baked'];
  }
  const openSec = 8 * 3600 + 30 * 60, firstClock = openSec + CE.t1 * 60, secondClock = openSec + CE.t2 * 60;
  const lodFirst = CE.lodPct >= 50, eFirst = lodFirst ? 'LOD' : 'HOD', eSecond = lodFirst ? 'HOD' : 'LOD';
  const W = E.wick || {}, wend = (typeof W.wick === 'number') ? openSec + W.wick * 60 : null, mudUsd = Math.round(rng * (Y.ptUsd || 50));
  out.DAYSE = [eFirst, f2(eFirst === 'LOD' ? eLo : eHi), I(firstClock), n1(CE.t1), n1(W.bop), n1(W.wick), I(wend), I(W.wickPct), n1(W.mud),
               eSecond, f2(eSecond === 'LOD' ? eLo : eHi), I(secondClock), n1(CE.t2 - CE.t1), n1(rng), I(mudUsd), n1(E.rngP25), n1(E.rngP75)];
  out.clocks = { firstClock, secondClock, eHodClk: lodFirst ? secondClock : firstClock, eLodClk: lodFirst ? firstClock : secondClock };
}
if (Y.actual) {
  const A = Y.actual;
  out.DAYSA = [A.first, f2(A.first === 'LOD' ? Y.lo : Y.hi), I(A.firstT), n1(A.took), n1(A.bop), n1(A.wick), I(A.wend), I(A.wickPct), n1(A.mud),
               A.second, f2(A.second === 'LOD' ? Y.lo : Y.hi), I(A.secondT), n1(A.gap), n1(A.rngPts), I(A.rngUsd), '', ''];
}
out.DAYACT = [f2(O), f2(Y.hi), f2(Y.lo), f2(Y.close)];
console.log(JSON.stringify(out));
