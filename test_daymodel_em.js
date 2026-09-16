// test_daymodel_em.js — (v16.33) the day model's EM + placement, the EM pin and the daily record: executed, mutation-checked.
// node test_daymodel_em.js   (reads current/gex-signal-tapereader.user.js directly)
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
const CODE = [extractVar('GP_EM_MODEL'), extractVar('GP_DAYEM_KEY'), extractVar('GP_DAYREC_KEY'), extract('gpOpenWindow'), extract('gpDayEmPin'),
  extract('gpDayModel'), extract('gpDayRecordAll'), extract('gpDayRecordGet'), extract('gpDayRecordTake')].join('\n');

// the environment the functions expect
const ENV = {
  store: {}, now: 8 * 3600 + 35 * 60, today: '2026-09-15', notToday: false,
};
function build(code) {
  const ls = { getItem: k => (k in ENV.store ? ENV.store[k] : null), setItem: (k, v) => { ENV.store[k] = String(v); } };
  const pre = 'var localStorage=__ls; var GPTS_VERSION="16.33"; var EM_MIN_FRAC=0.001;\n' +
    'function ctTodayStr(){ return __env.today; } function ctNowSecOfDay(){ return __env.now; } function dte0NotToday(){ return __env.notToday; }\n' +
    'function irtRatio(){ return {r:1.0094}; } function evCalLoad(){ return [{t:1,title:"FOMC Statement"}]; }\n';
  return new Function('__ls', '__env', pre + code + '\nreturn {gpOpenWindow, gpDayEmPin, gpDayModel, gpDayRecordTake, gpDayRecordGet, GP_EM_MODEL};')(ls, ENV);
}
let fails = 0, n = 0;
function ok(c, msg, got) { n++; if (!c) { fails++; console.log('FAIL ' + n + ': ' + msg + (got !== undefined ? ' -> ' + JSON.stringify(got) : '')); } }
const near = (a, b, t) => Math.abs(a - b) <= (t || 0.06);

const O = 8 * 3600 + 30 * 60;
function bars(n, f) { const out = []; for (let k = 0; k < n; k++) out.push(Object.assign({ so: O + (k + 1) * 180 }, f(k))); return out; }
const M = build(CODE);

// ---- Tue 15 Sep in shape: open 7684.25 at the top of its opening range (pos30 0.84, or30 16.75; pos60 0.92, or60 33)
// build windows directly (the study's numbers), not from bars — gpOpenWindow is pinned in test_daystats_cond
const w30 = { h: 7687, l: 7670.25, tH: 3, tL: 27, open: 7684.25, pos: 0.84, n: 10, complete: true };
const w60 = { h: 7687, l: 7654, tH: 3, tL: 57, open: 7684.25, pos: 0.92, n: 20, complete: true };
const EM = 37.7;
let r = M.gpDayModel({ elapsed: 5, w30: null, w60: null, scl: 1, emEs: EM, rng: 85.5, basis: 'exante' });
ok(r.basis === 'em-exante' && near(r.rng, 1.728 + 1.329 * EM) && r.f === 0.5, 'pre-open: EM replaces the prior-day range, symmetric', r);
r = M.gpDayModel({ elapsed: 33, w30, w60: null, scl: 1, emEs: EM, rng: 50.3, basis: 'open30' });
ok(r.basis === 'em-open30' && near(r.rng, 3.52 + 0.791 * 16.75 + 0.848 * EM) && near(r.f, 0.805 - 0.617 * 0.84, 0.001), '30 min: OR30 + EM, placed by pos30', r);
ok(near(r.f * r.rng, 14.1, 0.2) && near((1 - r.f) * r.rng, 34.6, 0.2), '30 min on Tue 15 Sep: +14.1 / -34.6 (the mockup)', [r.f * r.rng, (1 - r.f) * r.rng]);
r = M.gpDayModel({ elapsed: 66, w30, w60, scl: 1, emEs: EM, rng: 61.9, basis: 'open60' });
ok(r.basis === 'em-open60' && near(r.rng, 2.362 + 0.661 * 33 + 0.824 * EM) && near(r.f, 0.879 - 0.77 * 0.92, 0.001), '60 min: IB + EM, placed by pos60', r);
ok(near(r.f * r.rng, 9.5, 0.3) && near((1 - r.f) * r.rng, 45.7, 0.3), '60 min on Tue 15 Sep: ~+9.6 / -45.6 vs the actual +2.8 / -40.8', [r.f * r.rng, (1 - r.f) * r.rng]);
// no EM: the legacy stage's range stands, the placement still applies
r = M.gpDayModel({ elapsed: 66, w30, w60, scl: 1, emEs: null, rng: 61.9, basis: 'open60' });
ok(r.basis === 'open60' && r.rng === 61.9 && near(r.f, 0.879 - 0.77 * 0.92, 0.001), 'no EM pinned: legacy range, placement still on', r);
// incomplete 60-min window -> the 30-min stage
r = M.gpDayModel({ elapsed: 66, w30, w60: Object.assign({}, w60, { complete: false }), scl: 1, emEs: EM, rng: 50, basis: 'open30' });
ok(r.basis === 'em-open30', 'incomplete IB -> 30-min stage stands', r);
// the clamp on f
r = M.gpDayModel({ elapsed: 66, w30, w60: Object.assign({}, w60, { pos: 1.6 }), scl: 1, emEs: EM, rng: 50, basis: 'open60' });
ok(r.f === 0.05, 'upside share clamped at 0.05', r.f);
r = M.gpDayModel({ elapsed: 66, w30, w60: Object.assign({}, w60, { pos: -0.5 }), scl: 1, emEs: EM, rng: 50, basis: 'open60' });
ok(r.f === 0.95, 'upside share clamped at 0.95', r.f);
// scale: a SPY-scaled window is converted to ES points before the coefficients
r = M.gpDayModel({ elapsed: 66, w30, w60: { h: 768.7, l: 765.4, pos: 0.92, complete: true }, scl: 10, emEs: EM, rng: 61.9, basis: 'open60' });
ok(near(r.rng, 2.362 + 0.661 * 33 + 0.824 * EM), 'window range x scale before the coefficients', r.rng);

// ---- the EM pin
const chain = (em, spot) => ({ err: null, stale: false, spot: spot, dte0: { em: { em: em, k: 7685, pct: 0.49 }, exps: ['20260915'] } });
ENV.store = {}; ENV.now = O + 5 * 60;
let p = M.gpDayEmPin(chain(37.7, 7684));
ok(p && p.emSpx === 37.7 && p.est === false && p.atMin === 5 && p.date === '2026-09-15', 'pinned clean at 08:35', p);
ENV.now = O + 200 * 60;
p = M.gpDayEmPin(chain(20.0, 7684));
ok(p && p.emSpx === 37.7, 'the pin is ONCE: a later, decayed straddle does not replace it', p);
ENV.store = {}; ENV.now = O + 40 * 60;
p = M.gpDayEmPin(chain(33.1, 7684));
ok(p && p.est === true && p.atMin === 40, 'a pin after 15 min is flagged est', p);
ENV.store = {}; ENV.now = O + 61 * 60;
ok(M.gpDayEmPin(chain(33.1, 7684)) === null, 'after 60 min: no pin (the legacy stages stand)');
ENV.store = {}; ENV.now = O + 5 * 60;
ok(M.gpDayEmPin(chain(2.45, 7684)) === null, 'an expired book\'s residue (2.45 on 7684) is refused');
ENV.notToday = '20260916';
ok(M.gpDayEmPin(chain(37.7, 7684)) === null, 'front expiry not today -> refused'); ENV.notToday = false;
ok(M.gpDayEmPin(Object.assign(chain(37.7, 7684), { stale: true })) === null, 'a stale chain is refused');
ENV.now = O - 60;
ok(M.gpDayEmPin(chain(37.7, 7684)) === null, 'before the open: no pin');
ENV.store = {}; ENV.today = '2026-09-16'; ENV.now = O + 3 * 60;
ENV.store['gpts_dayem_v1'] = JSON.stringify({ date: '2026-09-15', emSpx: 37.7 });
p = M.gpDayEmPin(chain(44.5, 7615));
ok(p && p.emSpx === 44.5 && p.date === '2026-09-16', 'yesterday\'s pin does not serve today', p);

// ---- the daily record
ENV.store = {}; ENV.today = '2026-09-16'; ENV.now = O + 4 * 60;
const AU = { spot: { spx: 7615.2, src: 'trinity' }, flipSpx: 7590.25, king: 7550, kingNeg: true, regime: { sign: 'NEG', type: 'RANGE', conf: 'B', conflict: true },
  ifc: { payloadT: '2026-09-16T13:30:00Z', dte0: { cr: 7675, ps: 7550, netGEX: -1.5 } } };
let rec = M.gpDayRecordTake({ openEs: 7622.5, AU: AU, emRec: { emSpx: 44.47, est: false, atMin: 3 }, emEs: 44.89 });
ok(rec && rec.openEs === 7622.5 && rec.spotSpx === 7615.2 && rec.flipSpx === 7590.25 && rec.cwSpx === 7675 && rec.pwSpx === 7550, 'record: open, spot, flip, walls', rec);
ok(rec.regime && rec.regime.sign === 'NEG' && rec.regime.conflict === true && rec.king && rec.king.spx === 7550 && rec.king.neg === true, 'record: regime + King', rec);
ok(rec.emSpx === 44.47 && rec.emEs === 44.89 && rec.emEst === false && rec.events[0] === 'FOMC Statement' && rec.ratio > 1, 'record: EM, event, ratio', rec);
ok(M.gpDayRecordGet('2026-09-16').openEs === 7622.5, 'record persisted and readable by date');
// outside the window nothing changes
ENV.now = O + 120 * 60;
rec = M.gpDayRecordTake({ openEs: 7000, AU: {}, emRec: null, emEs: null });
ok(rec.openEs === 7622.5, 'outside the 15-min window the record is not re-taken', rec.openEs);
// inside the window a missing field is filled by a later export, the earlier fields kept
ENV.store = {}; ENV.now = O + 2 * 60;
M.gpDayRecordTake({ openEs: 7622.5, AU: { spot: {} }, emRec: null, emEs: null });
ENV.now = O + 9 * 60;
rec = M.gpDayRecordTake({ openEs: 7622.5, AU: AU, emRec: { emSpx: 44.47, est: false, atMin: 8 }, emEs: 44.89 });
ok(rec.flipSpx === 7590.25 && rec.emSpx === 44.47, 'a later export inside the window fills the fields the first one lacked', rec);

// ---- wiring (grep only; the logic above is executed)
ok(/dayRecord:\(function\(\)\{ try\{ return \(typeof gpDayRecordGet/.test(SRC), 'the day file carries dayRecord');
ok(/var eHi=O\+gpPlaceF\*eRng, eLo=O-\(1-gpPlaceF\)\*eRng/.test(SRC), 'the candle is placed by the upside share');
ok(/gpDayRecordTake\(\{ openEs:/.test(SRC), 'the export takes the record');
ok(/EXPMODEL,'\+eBasis\+','\+gpN1\(eRng\)\+','\+eDriveSign\+','\+gpPlaceF\.toFixed\(2\)/.test(SRC), 'EXPMODEL carries the upside share and the EM');
ok(/if\(eClose>eHi\) eClose=eHi; if\(eClose<eLo\) eClose=eLo;/.test(SRC), 'the leaned close stays inside the placed candle');

// ---- mutations
function mutated(re, rep) { const c = CODE.replace(re, rep); if (c === CODE) throw new Error('mutation did not apply: ' + re); return build(c); }
function fails60(Mx) {
  const q = Mx.gpDayModel({ elapsed: 66, w30, w60, scl: 1, emEs: EM, rng: 61.9, basis: 'open60' });
  return !(q.basis === 'em-open60' && near(q.rng, 2.362 + 0.661 * 33 + 0.824 * EM) && near(q.f, 0.879 - 0.77 * 0.92, 0.001));
}
ok(fails60(mutated(/out\.f=clampF\(M\.place60\.a\+M\.place60\.b\*w60\.pos\);/, 'out.f=0.5;')), 'mutation: dropping placement fires');
ok(fails60(mutated(/M\.open60\.c\*em/, 'M.open60.c*0')), 'mutation: dropping the EM term fires');
{ const Mx = mutated(/inp\.w60\.complete\)\?inp\.w60:null/, 'true)?inp.w60:null');
  const q = Mx.gpDayModel({ elapsed: 66, w30, w60: Object.assign({}, w60, { complete: false }), scl: 1, emEs: EM, rng: 50, basis: 'open30' });
  ok(q.basis !== 'em-open30', 'mutation: ignoring window completeness fires', q.basis); }
{ ENV.store = {}; ENV.now = O + 40 * 60; const Mx = mutated(/est:mins>15/, 'est:false');
  const q = Mx.gpDayEmPin(chain(33.1, 7684)); ok(q && q.est === false, 'mutation: dropping the est flag fires (est would have been true)', q && q.est); }

console.log((fails ? 'FAIL ' : 'PASS ') + (n - fails) + '/' + n + ' assertions (day model EM + placement, pin, daily record — v16.33)');
process.exit(fails ? 1 : 0);
