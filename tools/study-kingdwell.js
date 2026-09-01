// study-kingdwell.js — HOW MANY KING MIGRATIONS A DWELL THRESHOLD LEAVES, PER SESSION.
//
// Operator, 2026-09-01: "the movement of the kings in the king lanes doesn't make sense, it is too
// erratic. there should only be a couple of movements in a day."
//
// ⚠ THIS IS THE SCRIPT THAT CHOSE `KT_DWELL_MIN`. The constant ships with a number beside it and
// this is where that number comes from, so it can be re-derived when there are more sessions
// instead of being inherited on trust. Run it after any week of new recordings:
//
//     node tools/study-kingdwell.js
//
// It walks every data/YYYY-MM-DD.json, keeps RTH frames only (08:30–15:00 CT), and applies the SAME
// rule the panel does — a new strike must HOLD for N minutes before it counts as a migration.
// ⚠ Dwell is a DURATION here because it is a duration in the panel. Until v15.23 it was a COUNT of
// observations, which meant ~6 seconds live (per render) and 6 minutes in replay (per 3-min frame):
// one constant, two rules. A study measuring counts would have answered a question the face does
// not ask.
const fs = require('fs');
const DAYS = fs.readdirSync('data').filter(f => /^2026-\d\d-\d\d\.json$/.test(f)).map(f => f.replace('.json', ''));
const DWELLS = [0, 3, 6, 10, 15, 20, 30, 45];

function migrations(frames, book, dwellMin) {
  let cur = null, pend = null, n = 0;
  for (const f of frames) {
    const k = (f.tri && f.tri[book] && f.tri[book].king) || null;
    if (!(k > 0)) continue;
    if (cur === null) { cur = k; continue; }          // the seed is an origin, not a migration
    if (k === cur) { pend = null; continue; }
    if (!pend || pend.k !== k) { pend = { k: k, t0: f.t }; continue; }
    if ((f.t - pend.t0) / 60000 >= dwellMin) { cur = k; n++; pend = null; }
  }
  return n;
}

const acc = {}; DWELLS.forEach(d => acc[d] = { SPXW: [], SPY: [] });
const used = [];
for (const d of DAYS) {
  const J = JSON.parse(fs.readFileSync('data/' + d + '.json', 'utf8'));
  const FR = (J.snaps && J.snaps.SPY) || [];
  // ⚠ CT via a fixed -5h, the same convention the panel's recorder uses. A study on another clock
  // silently compares a different window.
  const rth = FR.filter(f => {
    const dd = new Date(f.t - 5 * 3600000);
    const so = dd.getUTCHours() * 60 + dd.getUTCMinutes();
    return so >= 510 && so < 900;
  });
  if (rth.length < 40) continue;                       // a stub of a session is not a session
  used.push(d + '(' + rth.length + ')');
  for (const dw of DWELLS) {
    acc[dw].SPXW.push(migrations(rth, 'SPXW', dw));
    acc[dw].SPY.push(migrations(rth, 'SPY', dw));
  }
}
const med = a => { const s = [...a].sort((x, y) => x - y); return s[Math.floor(s.length / 2)]; };
console.log('sessions used (' + used.length + '): ' + used.join(' '));
console.log('\ndwell(min)   SPXW median [min-max]      SPY median [min-max]');
for (const dw of DWELLS) {
  const f = a => med(a) + ' [' + Math.min(...a) + '-' + Math.max(...a) + ']';
  console.log(String(dw).padStart(6) + '       ' + f(acc[dw].SPXW).padEnd(24) + '  ' + f(acc[dw].SPY));
}
console.log('\n⚠ one instrument, one window — a measurement, not a law. Quote it with n and dates.');
