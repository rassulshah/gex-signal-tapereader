// study-deltacadence.js — HOW OFTEN SHOULD THE DELTA PROFILE CHANGE ITS MIND?
//
// Operator, 2026-09-01: "i see that the values constantly change... i need this feature of the delta
// profile to be stable and not changing every few seconds but at the same time be informative.
// figure out the updating time interval that is valid for identifying nodes that are growing and
// decreasing that can represent future support and resistance and magnets and kings."
//
// ⚠ THE QUESTION IS NOT AESTHETIC. A state that flips every render is not merely annoying — it is
// not a claim about anything. The measurable version: if a node is BUILDING now, does it still hold
// mass later? Two numbers decide the interval:
//    CHURN     how often the state flips between one read and the next  (lower = readable)
//    EDGE      does BUILDING at t predict more mass at t+30m than a coin flip (higher = informative)
// The interval to ship is the smallest one whose churn is low and whose edge survives.
//
//     node tools/study-deltacadence.js
const fs = require('fs');
const DAYS = fs.readdirSync('data').filter(f => /^2026-\d\d-\d\d\.json$/.test(f)).map(f => f.replace('.json', ''));
const BUILD = 4, WEAK = -8;             // LVL_BUILD_P15 / LVL_WEAK_P15
const HORIZON_MIN = 30;                 // "does it still matter half an hour later"

// the panel's own measure: change in MASS over a window, as a percentage
function stateAt(series, i, winMin) {
  const now = series[i];
  let j = i;
  while (j > 0 && (now.t - series[j].t) / 60000 < winMin) j--;
  if (j === i) return null;
  const prior = Math.abs(series[j].cur);
  if (!(prior > 0)) return null;
  const pct = 100 * (Math.abs(now.cur) - prior) / prior;
  return pct >= BUILD ? 'BUILDING' : (pct <= WEAK ? 'WEAKENING' : 'HOLDING');
}

function run(winMin, everyMin) {
  let flips = 0, reads = 0, hit = 0, tot = 0;
  for (const d of DAYS) {
    const J = JSON.parse(fs.readFileSync('data/' + d + '.json', 'utf8'));
    const FR = (J.snaps && J.snaps.SPY) || [];
    const rth = FR.filter(f => { const dd = new Date(f.t - 5 * 3600000);
      const so = dd.getUTCHours() * 60 + dd.getUTCMinutes(); return so >= 510 && so < 900; });
    if (rth.length < 40) continue;
    // per strike, the series of (t, cur)
    const byK = {};
    rth.forEach(f => ((f.vend && f.vend.rows) || []).forEach(r => {
      if (!(r[0] > 0) || typeof r[1] !== 'number') return;
      (byK[r[0]] = byK[r[0]] || []).push({ t: f.t, cur: r[1] });
    }));
    for (const k of Object.keys(byK)) {
      const s = byK[k];
      if (s.length < 12) continue;
      let last = null, lastT = -1e15;
      for (let i = 0; i < s.length; i++) {
        if ((s[i].t - lastT) / 60000 < everyMin) continue;   // only read on the cadence
        const st = stateAt(s, i, winMin);
        lastT = s[i].t;
        if (!st) continue;
        reads++;
        if (last !== null && st !== last) flips++;
        last = st;
        // EDGE: BUILDING now → is |cur| higher HORIZON_MIN later?
        if (st === 'BUILDING') {
          let j = i;
          while (j < s.length - 1 && (s[j].t - s[i].t) / 60000 < HORIZON_MIN) j++;
          if ((s[j].t - s[i].t) / 60000 >= HORIZON_MIN * 0.7) {
            tot++; if (Math.abs(s[j].cur) > Math.abs(s[i].cur)) hit++;
          }
        }
      }
    }
  }
  return { winMin, everyMin, reads,
           churnPct: reads ? +(100 * flips / reads).toFixed(1) : null,
           edgePct: tot ? +(100 * hit / tot).toFixed(1) : null, edgeN: tot };
}

console.log('sessions: ' + DAYS.length + ' files, RTH only, ' + DAYS[0] + '..' + DAYS[DAYS.length-1]);
console.log('\nwindow  every   reads    churn%   BUILDING still bigger 30m later');
[[15,1],[15,3],[15,5],[15,10],[15,15],[30,3],[30,5],[30,10],[30,15],[60,5],[60,15]].forEach(c => {
  const r = run(c[0], c[1]);
  console.log(String(r.winMin).padStart(5) + 'm ' + String(r.everyMin).padStart(5) + 'm ' +
    String(r.reads).padStart(8) + '   ' + String(r.churnPct).padStart(6) + '   ' +
    String(r.edgePct).padStart(6) + '%  (n=' + r.edgeN + ')');
});
console.log('\n⚠ churn = how often the state differs from the previous READ at that cadence.');
console.log('⚠ edge  = of the BUILDING calls, how many held MORE mass ' + HORIZON_MIN + ' minutes later. 50% = a coin.');

// ---- DOES A BIGGER MOVE PREDICT BETTER? ------------------------------------------------------
// The headline above is uncomfortable: BUILDING is 51-53% against a coin at every window and
// cadence. Before concluding the signal is weak, ask whether the THRESHOLD is simply too low —
// a 4%/15m change may be noise where a 40% change is positioning.
console.log('\n\n---- edge by SIZE of the 15m mass change, and by distance from spot ----');
function edgeBuckets(winMin, everyMin) {
  const B = [[4,10],[10,25],[25,50],[50,100],[100,1e9]];
  const acc = B.map(() => ({ hit:0, tot:0 }));
  const near = { hit:0, tot:0 }, far = { hit:0, tot:0 };
  for (const d of DAYS) {
    const J = JSON.parse(fs.readFileSync('data/' + d + '.json', 'utf8'));
    const FR = (J.snaps && J.snaps.SPY) || [];
    const rth = FR.filter(f => { const dd = new Date(f.t - 5*3600000);
      const so = dd.getUTCHours()*60 + dd.getUTCMinutes(); return so >= 510 && so < 900; });
    if (rth.length < 40) continue;
    const byK = {}, spot = {};
    rth.forEach(f => { spot[f.t] = (f.xm && f.xm.SPXW && f.xm.SPXW.px) || null;
      ((f.vend && f.vend.rows) || []).forEach(r => {
        if (!(r[0] > 0) || typeof r[1] !== 'number') return;
        (byK[r[0]] = byK[r[0]] || []).push({ t: f.t, cur: r[1] }); }); });
    for (const k of Object.keys(byK)) {
      const s = byK[k]; if (s.length < 12) continue;
      let lastT = -1e15;
      for (let i = 0; i < s.length; i++) {
        if ((s[i].t - lastT) / 60000 < everyMin) continue;
        lastT = s[i].t;
        let j = i; while (j > 0 && (s[i].t - s[j].t)/60000 < winMin) j--;
        if (j === i) continue;
        const prior = Math.abs(s[j].cur); if (!(prior > 0)) continue;
        const pct = 100 * (Math.abs(s[i].cur) - prior) / prior;
        if (pct < 4) continue;
        let f2 = i; while (f2 < s.length-1 && (s[f2].t - s[i].t)/60000 < HORIZON_MIN) f2++;
        if ((s[f2].t - s[i].t)/60000 < HORIZON_MIN*0.7) continue;
        const win = Math.abs(s[f2].cur) > Math.abs(s[i].cur);
        for (let bi = 0; bi < B.length; bi++)
          if (pct >= B[bi][0] && pct < B[bi][1]) { acc[bi].tot++; if (win) acc[bi].hit++; }
        const sp = spot[s[i].t];
        if (sp) { const dist = Math.abs(+k - sp);
          const bucket = dist <= 25 ? near : far; bucket.tot++; if (win) bucket.hit++; }
      }
    }
  }
  console.log('15m change      still bigger 30m later');
  B.forEach((b, i) => { const a = acc[i];
    console.log(('+' + b[0] + '..' + (b[1] > 1e8 ? '∞' : b[1]) + '%').padEnd(14) +
      (a.tot ? (100*a.hit/a.tot).toFixed(1) + '%  (n=' + a.tot + ')' : '—')); });
  console.log('\nwithin 25pts of spot: ' + (near.tot ? (100*near.hit/near.tot).toFixed(1)+'%  (n='+near.tot+')' : '—'));
  console.log('further out        : ' + (far.tot ? (100*far.hit/far.tot).toFixed(1)+'%  (n='+far.tot+')' : '—'));
}
edgeBuckets(15, 5);

// ---- HYSTERESIS: how long must a new state HOLD before the face changes its mind? -------------
// ⚠ The churn table above measures how often the RAW state differs from the previous read. What the
// operator sees is the DISPLAYED state, and those are only the same thing if the face switches the
// moment the raw value does. Requiring a new state to persist converts a flicker into a claim —
// the same rule the king lane's dwell uses, applied to the level engine.
console.log('\n\n---- hysteresis: a new state must HOLD this long before it is displayed ----');
function hyst(winMin, everyMin, holdMin) {
  let changes = 0, reads = 0, hit = 0, tot = 0;
  for (const d of DAYS) {
    const J = JSON.parse(fs.readFileSync('data/' + d + '.json', 'utf8'));
    const FR = (J.snaps && J.snaps.SPY) || [];
    const rth = FR.filter(f => { const dd = new Date(f.t - 5*3600000);
      const so = dd.getUTCHours()*60 + dd.getUTCMinutes(); return so >= 510 && so < 900; });
    if (rth.length < 40) continue;
    const byK = {};
    rth.forEach(f => ((f.vend && f.vend.rows) || []).forEach(r => {
      if (!(r[0] > 0) || typeof r[1] !== 'number') return;
      (byK[r[0]] = byK[r[0]] || []).push({ t: f.t, cur: r[1] }); }));
    for (const k of Object.keys(byK)) {
      const s = byK[k]; if (s.length < 12) continue;
      let shown = null, pend = null, pendT = 0, lastT = -1e15;
      for (let i = 0; i < s.length; i++) {
        if ((s[i].t - lastT)/60000 < everyMin) continue;
        lastT = s[i].t;
        const raw = stateAt(s, i, winMin);
        if (!raw) continue;
        reads++;
        if (raw === shown) { pend = null; }
        else if (pend !== raw) { pend = raw; pendT = s[i].t; }
        else if ((s[i].t - pendT)/60000 >= holdMin) { shown = raw; pend = null; changes++; }
        if (shown === 'BUILDING') {
          let j = i; while (j < s.length-1 && (s[j].t - s[i].t)/60000 < HORIZON_MIN) j++;
          if ((s[j].t - s[i].t)/60000 >= HORIZON_MIN*0.7) { tot++; if (Math.abs(s[j].cur) > Math.abs(s[i].cur)) hit++; }
        }
      }
    }
  }
  return { holdMin, changePct: reads ? +(100*changes/reads).toFixed(1) : null,
           edgePct: tot ? +(100*hit/tot).toFixed(1) : null, n: tot };
}
console.log('hold   state changes per read   BUILDING edge 30m later');
[0,1,2,3,5,10].forEach(hm => { const r = hyst(15, 1, hm);
  console.log(String(r.holdMin).padStart(3) + 'm ' + String(r.changePct).padStart(14) + '%' +
    String(r.edgePct).padStart(18) + '%  (n=' + r.n + ')'); });
