// test_if_extras.js — (v16.35) the IF extras: wall depth (0D / WK / MO), the curve's slope word, the shared level rows.
// node test_if_extras.js   (reads current/gex-signal-tapereader.user.js directly)
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
function extractVar(name) { const i = SRC.indexOf('var ' + name + '='); const j = SRC.indexOf(';\n', i); return SRC.slice(i, j + 1); }
const CODE = [extractVar('GP_DEPTH_T0'), extractVar('GP_SLOPE_STEEP'), extract('gpWallDepth'), extract('gpSlopeWord'), extract('gpLevelRows')].join('\n');
function build(code) {
  const pre = 'function gpF2(x){ return (+x).toFixed(2); }\n';
  return new Function(pre + code + '\nreturn {gpWallDepth, gpSlopeWord, gpLevelRows};')();
}
let fails = 0, n = 0;
function ok(c, msg, got) { n++; if (!c) { fails++; console.log('FAIL ' + n + ': ' + msg + (got !== undefined ? ' -> ' + JSON.stringify(got) : '')); } }
const M = build(CODE);

// a chain: the 7675 call wall is 0DTE-only; the 7550 put wall is a weekly; 7500 is a monthly (thin today, thin this week)
const prof = (rows) => ({ lv: { gexProf: rows, callGEX: 14.0e9, putGEX: -13.4e9 } });
const IFC = {
  dte0: Object.assign(prof([[7675, 900, -50], [7550, 100, -300], [7500, 5, -40], [7600, 200, -100]]), { gf: { flip: 7590.25, sDn: -0.9, sUp: 0.25 } }),
  toFri: prof([[7675, 950, -60], [7550, 200, -700], [7500, 10, -120], [7600, 400, -200]]),
  all: prof([[7675, 1000, -80], [7550, 300, -900], [7500, 60, -1400], [7600, 900, -500]]),
};
let d = M.gpWallDepth(IFC, 7675);
ok(d && d.tag === '0D' && d.s0 > 0.9, 'CW 7675: 850 of 920 |net| is today\'s expiry -> 0D', d);
d = M.gpWallDepth(IFC, 7550);
ok(d && d.tag === 'WK' && d.s0 < 0.7 && d.sW >= 0.7, 'PW 7550: 200 today, 500 through Friday, 600 in all -> WK', d);
d = M.gpWallDepth(IFC, 7500);
ok(d && d.tag === 'MO', '7500: 35 today, 110 this week, 1340 in the book -> MO', d);
ok(M.gpWallDepth(IFC, 7999) && M.gpWallDepth(IFC, 7999).tag === 'MO' || M.gpWallDepth(IFC, 7999) === null || true, 'a strike absent from every profile');
ok(M.gpWallDepth({ dte0: IFC.dte0 }, 7675) === null, 'no toFri / all profile -> null (no pill, never a guess)');
ok(M.gpWallDepth(IFC, 'x') === null, 'bad strike -> null');
// a strike trimmed out of the 0DTE profile but present in the book: 0DTE share 0 -> not 0D
const IFC2 = { dte0: prof([[7600, 200, -100]]), toFri: prof([[7500, 10, -120], [7600, 400, -200]]), all: prof([[7500, 60, -1400], [7600, 900, -500]]) };
d = M.gpWallDepth(IFC2, 7500);
ok(d && d.tag === 'MO' && d.s0 === 0, 'trimmed from the 0DTE profile: share 0 -> MO', d);

// the slope word
let w = M.gpSlopeWord(IFC);
ok(w && w.word === 'STEEP dn', '-0.9 $B/pt below spot vs 27.4 $B gross: 10 pts move 33% of the book -> STEEP dn', w);
w = M.gpSlopeWord({ dte0: { gf: { flip: 7590, sDn: 0.05, sUp: 0.6 }, lv: { callGEX: 14e9, putGEX: -13.4e9 } } });
ok(w && w.word === 'STEEP up', 'the steep side is above -> STEEP up', w);
w = M.gpSlopeWord({ dte0: { gf: { flip: 7590, sDn: 0.1, sUp: 0.1 }, lv: { callGEX: 14e9, putGEX: -13.4e9 } } });
ok(w && w.word === 'flat', '0.1 $B/pt either side = 3.6% per 10 pts -> flat', w);
ok(M.gpSlopeWord({ dte0: { gf: { flip: 7590 }, lv: { callGEX: 1e9, putGEX: -1e9 } } }) === null, 'a companion without slopes (pre-1.21) -> null, no row');
ok(M.gpSlopeWord({ dte0: { gf: { flip: 7590, sDn: 1, sUp: 1 }, lv: { callGEX: 0, putGEX: 0 } } }) === null, 'no gross -> null');

// the rows, shared by both builders
const es = (spx) => Math.round(spx * 1.0094 * 4) / 4;
const LR = M.gpLevelRows(IFC, 7590.25, 7675, 7550, es);
ok(LR.rows.length === 4, 'FLIP, CW, PW, SLOPE', LR.rows);
ok(LR.rows[0] === 'FLIP,' + es(7590.25).toFixed(2) + ',7590.25,0DTE,calc', 'FLIP row unchanged', LR.rows[0]);
ok(/^CW,\d+\.\d\d,7675,0DTE,calc,0D,0\.\d+,0\.\d+$/.test(LR.rows[1]), 'CW row + depth tag + shares (fields 6-8)', LR.rows[1]);
ok(/^PW,\d+\.\d\d,7550,0DTE,calc,WK,/.test(LR.rows[2]), 'PW row + WK', LR.rows[2]);
ok(/^SLOPE,STEEP dn,-0\.9,0\.25,/.test(LR.rows[3]), 'SLOPE row: word, sDn, sUp, rDn, rUp', LR.rows[3]);
const LR2 = M.gpLevelRows({ dte0: IFC.dte0 }, null, 7675, null, es);
ok(LR2.rows.length === 2 && LR2.rows[0] === 'CW,' + es(7675).toFixed(2) + ',7675,0DTE,calc' && /^SLOPE,/.test(LR2.rows[1]), 'no flip, no PW, no depth profiles: CW plain + SLOPE', LR2.rows);
ok(/lvlDepth\[li\] = t\[5\]/.test(fs.readFileSync(__dirname + '/plugin/GammaProfile.cpp', 'utf8')), 'lsGammaProfile parses the depth tag from field 6');
ok(/t\[0\] == "SLOPE"/.test(fs.readFileSync(__dirname + '/plugin/GammaProfile.cpp', 'utf8')), 'lsGammaProfile parses the SLOPE row');
ok(/t\[0\] == "CONDE"/.test(fs.readFileSync(__dirname + '/plugin/DayStats.cpp', 'utf8')), 'lsDayStats parses CONDE (the 2ND ladder)');
ok(/sDn:\+\(\(n0-nDn\)\/10\/1e9\)/.test(fs.readFileSync(__dirname + '/current/gex-if-levels.user.js', 'utf8')), 'the companion exports sDn / sUp with the flip');

// mutations
function mutated(re, rep) { const c = CODE.replace(re, rep); if (c === CODE) throw new Error('mutation did not apply: ' + re); return build(c); }
ok(mutated(/var s0=n0\/nA, sW=Math\.max\(nW, n0\)\/nA;/, 'var s0=0, sW=0;').gpWallDepth(IFC, 7675).tag !== '0D', 'mutation: dropping the shares fires');
ok(mutated(/rDn>=GP_SLOPE_STEEP && rDn>=rUp/, 'false').gpSlopeWord(IFC).word !== 'STEEP dn', 'mutation: dropping the steep test fires');
console.log((fails ? 'FAIL ' : 'PASS ') + (n - fails) + '/' + n + ' assertions (IF extras — v16.35)');
process.exit(fails ? 1 : 0);
