// ============================================================================================
// test_hodlod.js — (v14.57) ⓪a DAY, HOD/LOD
//
// Spec: `mockuphodlodv2.html` (repo root, the operator's approved design). His question, verbatim:
// "determine if a lod or hod is in and we are going to the other extremity."
//
// ⚠ THESE TESTS EXECUTE THE FUNCTIONS. A grep cannot tell a correct rate from a wrong one, and this
// project has shipped fourteen assertions guarding an export that could not catch a wrong PRICE.
// Everything below runs `hodLod` over synthetic sessions with known answers.
// ============================================================================================
const fs = require('fs');
const src = fs.readFileSync('./v10.js', 'utf8');
let pass = 0, fail = 0;
const ok = (c, m, g) => { if (c) { pass++; console.log('PASS ' + m); }
                          else { fail++; console.log('FAIL ' + m + (g !== undefined ? ' -> ' + JSON.stringify(g) : '')); } };
function ex(n){ const re=new RegExp('function\\s+'+n+'\\s*\\(','g'); const m=re.exec(src);
  if(!m) return ''; let i=src.indexOf('{',m.index),d=0,e=-1;
  for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){d--; if(d===0){e=k;break;}} }
  return src.slice(m.index,e+1); }
function val(n){ const m=new RegExp('(?:var\\s+)?\\b'+n+'\\s*=\\s*([\\s\\S]*?);\\n').exec(src); return m?eval('('+m[1]+')'):undefined; }

// ---- harness ---------------------------------------------------------------------------------
global.mul=(a,b)=>a/(1/b);
global.two=x=>{x=''+x;return x.length<2?'0'+x:x;};
global.HODLOD_BASE = val('HODLOD_BASE');
global.ES_USD_PER_PT = 50;
let NOWSEC = 15*3600;
global.ctNowSecOfDay = () => NOWSEC;
global.inReplay = () => false;
global.showingStaleBook = () => false;
// the underlying->chart converter. closedCandles() returns the UNDERLYING book (SPY ~765); the
// section's E row is in CHART points (ES). Without this the two halves of one row are different
// instruments — which is exactly how v14.57 shipped "5.1pts" beside an expected "56.5pts".
let FUT = true, RR = 10.04;
global.dispIsFut = () => FUT;
global.dispR = () => RR;
let CANDLES = [];
global.closedCandles = () => CANDLES;
global.HLBASE_KEY='gpts_hodlod_base_v1';
global.HLBASE_MIN_SESSIONS=120; global.HLBASE_MIN_BUCKET=50;
// (v14.59) the base rates are couriered now, so hlTier/hodLod read hodlodBase() rather than the
// frozen literal. With no localStorage in this harness it returns the baked-in copy, which is
// exactly what these tests were already asserting against.
global.localStorage={ getItem:()=>null, setItem:()=>{}, removeItem:()=>{} };
eval(ex('hlBaseNormalise')); eval(ex('hodlodBase'));
eval(ex('hlClock')); eval(ex('hlDur')); eval(ex('hlTier')); eval(ex('hodLod'));

const OPEN = 8*3600+30*60;
// a session builder: bars at one-minute spacing from the open
function session(spec){   // spec: [{m, h, l}]  minutes-from-open
  return spec.map(x => ({ so: OPEN + x.m*60, o: x.o!=null?x.o:x.l, h: x.h, l: x.l, c: x.c!=null?x.c:x.h }));
}

// ---- 1 · THE BASE RATES ARE MEASURED, AND MATCH THE COMMITTED STUDY --------------------------
{
  const B = global.HODLOD_BASE;
  ok(!!B && B.n === 284, 'b1 the corpus is the 284-session ES set the mockup header names', B && B.n);
  ok(Array.isArray(B.ladder) && B.ladder.length === 5, 'b2 five holding windows, as the mockup draws');
  ok(B.ladder.every(L => L.n > 0 && L.rate > 0), 'b3 every rung carries its own n — no bare percentage');
  // ⚠ MONOTONE OR THE WHOLE PREMISE IS WRONG. The section's one claim is "the longer it stands, the
  // likelier it is the day's". If the rates are not increasing, that claim is not supported and the
  // ladder must not ship.
  const rates = B.ladder.map(L => L.rate);
  ok(rates.every((r, i) => i === 0 || r >= rates[i-1]),
     'b4 the survival rates INCREASE with holding time — the section\'s only predictive claim', rates);
  // and it must agree with the file the study wrote, or the panel and the evidence have diverged
  const J = JSON.parse(fs.readFileSync('./data/es-1min/BASERATES.json', 'utf8'));
  const both = J.ladder.both;
  ok(B.ladder.every(L => both[String(L.w)] && both[String(L.w)].rate === L.rate && both[String(L.w)].n === L.n),
     'b5 the baked ladder EQUALS data/es-1min/BASERATES.json — panel and evidence cannot drift');
  ok(B.n === J.corpus.sessions && B.rngPts === J.expected.rng_pts,
     'b6 ...as do the corpus size and the expected range');
}

// ---- 2 · hlTier awards the HIGHEST window actually earned -------------------------------------
{
  ok(hlTier(29) === null, 't1 under 30 minutes earns no rate at all');
  ok(hlTier(30).w === 30,  't2 exactly 30 earns the 30m rung');
  ok(hlTier(100).w === 90, 't3 100 minutes reads the 90m rate, NOT the 120m it has not earned', hlTier(100).w);
  ok(hlTier(9999).w === 180, 't4 ...and nothing beyond the top rung');
}

// ---- 3 · hodLod on a session with a KNOWN answer ----------------------------------------------
{
  // low at +10m (7000), high at +200m (7100); flat elsewhere
  CANDLES = session([{m:0,h:7050,l:7040},{m:10,h:7045,l:7000},{m:100,h:7060,l:7050},
                     {m:200,h:7100,l:7090},{m:300,h:7080,l:7070}]);
  NOWSEC = OPEN + 330*60;
  const D = hodLod('SPY');
  ok(D.ok, 'h1 a populated session reads');
  ok(D.lod === 7000 && D.hod === 7100, 'h2 HOD and LOD are the true session extremes', {hod:D.hod, lod:D.lod});
  ok(D.first === 'LOD', 'h3 the LOD printed first, so it is the standing extremity');
  ok(D.second === 'HOD', 'h4 ...and the HOD is the other side');
  ok(Math.round(D.took) === 10, 'h5 TOOK is measured from the RTH OPEN, not from the first bar', D.took);
  ok(Math.round(D.gap) === 190, 'h6 HL GAP is the distance between the two extremes', D.gap);
  // ⚠⚠ THE UNIT BUG v14.57 SHIPPED. closedCandles() is the UNDERLYING book; the range must be
  // converted to CHART space BEFORE the ES multiplier, or points and dollars describe an instrument
  // the number never measured — and the E row beside it is in chart points.
  ok(Math.abs(D.rngPts - 100*RR) < 1e-6,
     'h7 the range is converted to CHART points, not left in underlying points', D.rngPts);
  ok(Math.abs(D.rngUsd - 100*RR*50) < 1e-6,
     'h8 ...and the $50 ES multiplier is applied to CHART points, never to underlying ones', D.rngUsd);
  { FUT = false;
    const F = hodLod('SPY');
    ok(Math.abs(F.rngPts - 100) < 1e-9, 'h8b on a non-futures chart the ratio is 1', F.rngPts);
    ok(F.rngUsd === null,
       'h8c ...and NO dollar figure is printed, because the ES multiplier does not describe it', F.rngUsd);
    FUT = true; }
  // ⚠ STOOD IS MEASURED TO NOW, NOT TO THE OTHER EXTREME. The ladder asks how long the standing low
  // has survived up to this moment; measuring to the HOD would freeze it and the rung would stop
  // advancing while the low went on holding.
  ok(Math.round(D.stood) === 320, 'h9 STOOD runs from the standing extreme to NOW, not to the other one', D.stood);
  ok(D.tier && D.tier.w === 180, 'h10 ...so a low standing 5h20m has earned the top rung');
}

// ---- 4 · the reversed case -------------------------------------------------------------------
{
  CANDLES = session([{m:0,h:7100,l:7090},{m:5,h:7120,l:7100},{m:250,h:7040,l:7000}]);
  NOWSEC = OPEN + 260*60;
  const D = hodLod('SPY');
  ok(D.first === 'HOD' && D.second === 'LOD', 'r1 when the high prints first it is the standing one');
  ok(D.hod === 7120 && D.lod === 7000, 'r2 ...and the extremes are still right', {hod:D.hod, lod:D.lod});
}

// ---- 5 · REFUSALS — absence of data is not a reading ------------------------------------------
{
  CANDLES = [];
  ok(hodLod('SPY').ok === false, 'x1 no candles refuses');
  CANDLES = session([{m:-120,h:7000,l:6990}]);   // pre-open only
  NOWSEC = OPEN + 5*60;
  const D = hodLod('SPY');
  ok(D.ok === false && /RTH/.test(D.why || ''), 'x2 pre-open bars alone refuse, and say why', D.why);
}

// ---- 6 · REPLAY AND THE FROZEN BOOK USE THE BAR CLOCK, NOT THE WALL CLOCK ---------------------
// ⚠ In a replay the wall clock is not the session clock. Using it would inflate STOOD by hours and
// hand the top rung to an extreme that never earned it — a fabricated 84%.
{
  CANDLES = session([{m:0,h:7050,l:7000},{m:60,h:7060,l:7040}]);
  NOWSEC = 23*3600;
  global.inReplay = () => true;
  const R = hodLod('SPY');
  global.inReplay = () => false;
  const Lv = hodLod('SPY');
  ok(Math.round(R.stood) === 60, 'p1 a replay measures STOOD to the LAST BAR', R.stood);
  ok(Lv.stood > R.stood, 'p2 ...where the live path would have used the wall clock', {live:Lv.stood, replay:R.stood});
}

// ---- 7 · THE WICK FAMILY (v14.60) — DEFINED BY THE OPERATOR, CONFIRMED ON THE TAPE ----------
// ⚠ These five were PENDING through v14.59 because no definition existed. He supplied them on
// 2026-08-28 and they were then verified bar-by-bar against the live ES tape for 2026-08-27, where
// the two fields independent of extremity timing matched exactly (Wick% 26, W.End 8:42am).
{
  const SD = ex('secDay');
  ok(/WICK%/.test(SD) && /W\.END/.test(SD) && /MUD/.test(SD) && /BOP/.test(SD),
     'n1 the wick columns are on the face now, not named as pending');
  ok(!/pending a definition/.test(SD),
     'n1b ...and the old PENDING refusal is gone with them');
  // the whole session, reconstructed: open 100, LOD at +5m, reclaim at +8m, HOD at +100m
  const bars = [];
  for (let m = 0; m <= 120; m++) {
    let o = 100, h = 100.5, l = 99.5, c = 100;
    if (m === 5) { l = 90; c = 92; }               // the LOD
    if (m > 5 && m < 8) { c = 95; h = 96; }        // still under the open
    if (m === 8) { c = 101; h = 101; }             // FIRST CLOSE back through the open -> W.End
    if (m === 100) { h = 130; c = 129; }           // the HOD
    bars.push({ so: OPEN + m*60, o: m===0?100:o, h: h, l: l, c: c });
  }
  CANDLES = bars; NOWSEC = OPEN + 120*60;
  const W = hodLod('SPY');
  ok(W.first === 'LOD' && Math.round(W.took) === 5, 'n2 TOOK is open -> first extremity', W.took);
  ok(W.wend === OPEN + 8*60, 'n3 W.END is the first bar to CLOSE back through the open (not touch)',
     W.wend && (W.wend - OPEN)/60);
  ok(Math.round(W.bop) === 3, 'n4 BOP is first extremity -> W.END', W.bop);
  ok(Math.round(W.wick) === 8 && Math.round(W.wick) === Math.round(W.took + W.bop),
     'n5 WICK is open -> W.END, and equals TOOK + BOP', [W.wick, W.took, W.bop]);
  ok(Math.round(W.mud) === 92, 'n6 MUD is W.END -> the second extremity', W.mud);
  // open 100, LOD 90, HOD 130 -> range 40, excursion 10 -> 25%
  ok(W.wickPct === 25, 'n7 WICK% is a PRICE ratio: |open - extremity| / range', W.wickPct);
  // ⚠ THE UNIT TRAP THIS SECTION ALREADY SHIPPED ONCE. Wick% must be scale-free: the same session
  // on a futures chart must give the SAME percentage, or it is v14.57's 5.1-vs-56.5 bug again.
  RR = 1; const W1 = hodLod('SPY'); const pctSpy = W1.wickPct;
  RR = 10.04;
  const W2 = hodLod('SPY');
  ok(W2.wickPct === pctSpy, 'n8 WICK% is scale-free — identical on a futures chart', [pctSpy, W2.wickPct]);
  ok(Math.abs(W2.rngPts - W1.rngPts*10.04) < 0.01 && W2.rngPts !== W1.rngPts,
     'n8b ...while the RANGE does convert with rr, proving the scale really changed',
     [W1.rngPts, W2.rngPts]);
  RR = 10.04;
  // NEVER RECLAIMED is not zero. A session that opens at its low and runs has no completed wick.
  // ⚠ A GENUINE never-reclaim is narrower than it first looks, and building this case taught me
  // that: price must set the LOD first, then make its HIGH afterwards, and still never CLOSE back
  // through the open. If the low is on the opening bar and that bar closes at the open, the wick is
  // a real ZERO — which is his rule, not a miss.
  CANDLES = [{ so: OPEN, o: 100, h: 100, l: 99, c: 99.5 }];
  for (let m = 1; m <= 120; m++) {
    let h = 99.8, l = 99, c = 99.5;
    if (m === 5) { l = 90; c = 92; }              // the LOD, first
    if (m === 100) { h = 100.5; c = 99.9; }       // the HOD, later — but it never CLOSES >= 100
    CANDLES.push({ so: OPEN + m*60, o: 99.5, h: h, l: l, c: c });
  }
  const NR = hodLod('SPY');
  ok(NR.first === 'LOD', 'n8c the never-reclaim fixture really is LOD-first', NR.first);
  ok(NR.reclaimed === false && NR.wend === null && NR.wick === null && NR.mud === null,
     'n9 never closing back through the open leaves W.END/WICK/MUD NULL — not 0', 
     {reclaimed:NR.reclaimed, wick:NR.wick});
  ok(/\\u2014/.test(ex('secDay')) || /hlv\(/.test(ex('secDay')),
     'n9b ...and the face renders them as em-dash through hlv(), never as a number');
  CANDLES = bars; NOWSEC = OPEN + 120*60;
  ok(!/BOP:|mudMin/.test(src), 'n2b no stray duplicate implementation of these fields');
  // VWAP is one of the mockup's five chips and this codebase has none.
  ok(/NOT AVAILABLE/.test(SD) && /g3daychip\.na/.test(src),
     'n3 VWAP renders UNAVAILABLE with its own style — never a failing tick');
  ok(/no VWAP|NO VWAP/.test(src), 'n4 ...and the absence is documented, not silent');
}

// ---- 8 · the honesty furniture ---------------------------------------------------------------
{
  const SD = ex('secDay');
  ok(/coin-flip/.test(SD), 'f1 the section states on its face that bare sequence carries nothing');
  ok(/every rate carries its n/.test(SD), 'f2 ...and that no bare percentage appears');
  ok(/descriptive/.test(SD) && /no entries\/stops/.test(SD), 'f3 ...and the scope boundary holds');
  ok(/when it did not hold/.test(SD), 'f4 THE MISS CASE IS PRINTED beside the odds of being right');
  ok(/This is not a reading, it is no reading/.test(SD), 'f5 a refusal reads as a refusal, never as calm');
  ok(!/\bwill\b|\bshould\b|likely/.test(SD.replace(/\/\/[^\n]*/g, '')),
     'f6 no forecast vocabulary — gamma and structure describe, they do not predict');
}

// ---- 9 · IB60 is new, and IB30 survives it ---------------------------------------------------
{
  ok(/IB60_MIN_S\s*=\s*3600/.test(src), 'i1 IB60 is a 60-minute initial balance');
  ok(/IB_MIN_S/.test(src) && /ib60Set/.test(src), 'i2 ...and IB30 is still computed beside it');
  const SL = ex('sessionLevels');
  ok(/out\.ib60H=/.test(SL) && /out\.ibH=/.test(SL), 'i3 both are emitted from sessionLevels');
  ok(/out\.open=/.test(SL), 'i4 the session OPEN is captured — the OPEN chip needs it');
}

// ---- 8 · ONE ROW, ONE STATISTIC (v14.62) ----------------------------------------------------
// ⚠ THIS TEST EXISTS BECAUSE I GOT IT WRONG. v14.61 made the wick columns trimmed MEANS and left
// TOOK / HL GAP / HL RNG as MEDIANS, because those had been verified that way against an older
// mockup. The operator: "i thought they were all averages." A table where two columns are means and
// three are medians is one nobody can reason about, and nothing on the face said which was which.
{
  const B = global.HODLOD_BASE;
  const BR = JSON.parse(fs.readFileSync('./data/es-1min/BASERATES.json', 'utf8'));
  ok(/trimmed mean/i.test(BR.expected.statistic || ''),
     's1 the study declares its statistic inside the artefact', BR.expected.statistic);
  [['tookMin','took_min'],['gapMin','gap_min'],['rngPts','rng_pts'],
   ['firstClock','first_clock'],['secondClock','second_clock']].forEach(([bk, sk]) => {
    ok(Math.abs(B[bk] - BR.expected[sk]) < 0.51,
       's2 ' + bk + ' is baked from the trimmed MEAN', [B[bk], BR.expected[sk]]);
  });
  const mc = BR.expected.median_for_contrast;
  ok(B.tookMin !== mc.took_min && B.rngPts !== mc.rng_pts,
     's3 ...and differs from the median, proving the statistic actually changed',
     [B.tookMin, mc.took_min, B.rngPts, mc.rng_pts]);
  ok(typeof B.medTook === 'number' && typeof B.medRng === 'number',
     's4 the medians are still carried so the hover can disclose the choice');
  ok(BR.expected.rng_p25 < BR.expected.rng_pts && BR.expected.rng_p75 > BR.expected.rng_pts,
     's5 p25/p75 stay TRUE percentiles bracketing the mean — a trimmed quantile describes nothing',
     [BR.expected.rng_p25, BR.expected.rng_pts, BR.expected.rng_p75]);
  ok(/EVERY field is a TRIMMED MEAN/.test(ex('secDay')),
     's6 the face states that every field is the same statistic');
  const W = BR.wickFamily.median;
  ok(Math.abs(B.wick.bop - W.bop) < 0.51 && B.wick.bop !== W.bop_median,
     's7 the wick columns use that same statistic', [B.wick.bop, W.bop, W.bop_median]);
}

// ---- 9 · THE LOD/HOD CALL (v14.64) — the table, not a model ---------------------------------
// ⚠ Built AFTER a 5-feature logistic was measured against a plain 2-axis lookup and found to add
// 0.0008 AUC on an identical Brier. The table ships because it is inspectable. FINDINGS F-4.
{
  global.HLTAB = val('HLTAB');
  global.HLTAB_META = val('HLTAB_META');
  eval(ex('hlCell')); eval(ex('lodhodCall'));

  ok(Array.isArray(HLTAB) && HLTAB.length === 8 && HLTAB[0].length === 9,
     't1 the table is 8 posr octiles x 9 time blocks', HLTAB && [HLTAB.length, HLTAB[0].length]);
  ok(HLTAB_META.sessions === 284 && HLTAB_META.thresh === 70,
     't2 it declares its corpus and its threshold on the face of the constant');

  // EVERY populated cell must carry a real n — a percentage without one is what this project bans
  let cells = 0, thin = 0, bad = 0;
  HLTAB.forEach(r => r.forEach(c => {
    if (c[1] === null) { thin++; return; }
    cells++;
    if (!(c[0] >= 25) || c[1] < 0 || c[1] > 100) bad++;
  }));
  ok(bad === 0, 't3 every populated cell has n>=25 and a sane percentage', bad);
  ok(cells >= 50, 't4 the table is actually populated', cells);
  ok(thin > 0, 't5 ...and thin cells are NULL rather than guessed', thin);

  // MONOTONE IN BOTH AXES — the table's whole claim. Not asserted cell-by-cell (noise), but the
  // corners must obey it or the thing is not measuring what it says.
  const g = (p, t) => HLTAB[p][t][1];
  ok(g(7, 8) > g(0, 8), 't6 at the same hour, further from the extreme scores higher',
     [g(0, 8), g(7, 8)]);
  ok(g(7, 8) > g(7, 1), 't7 at the same distance, later in the session scores higher',
     [g(7, 1), g(7, 8)]);
  ok(g(0, 1) < 25, 't8 sitting ON a fresh extreme early is a LOW probability, as it must be', g(0, 1));

  // hlCell clamps rather than throwing, and refuses thin cells
  ok(hlCell(0.5, 200) && hlCell(0.5, 200).p !== null, 't9 a normal lookup returns a rate');
  ok(hlCell(1.5, 9999) !== null, 't10 out-of-range inputs clamp instead of throwing',
     hlCell(1.5, 9999));
  ok(hlCell('x', 100) === null, 't11 a non-numeric input returns null, not a number');
  // ⚠ A THIN CELL MUST REFUSE, NOT GUESS. The 08:30 block has under 25 observations at every
  // distance, because the session has barely started. Added after a mutation showed nothing
  // asserted this directly - "absence of data is not a reading" has to be tested, not just written.
  const thinCell = hlCell(0.5, 10);
  ok(thinCell && thinCell.p === null && /too few/.test(thinCell.why || ''),
     't11b a thin cell returns p=null AND says why', thinCell);
  ok(thinCell && typeof thinCell.n === 'number',
     't11c ...and still reports how many samples it does have', thinCell && thinCell.n);

  // the CALL: threshold, refusal, and the "still ahead" companion
  const D = ok0 => ({ ok: true, posr: ok0.posr, clock: OPEN + ok0.mins * 60, first: 'LOD',
                      rngPts: 50, far: 1 - ok0.posr });
  const hi = lodhodCall(D({ posr: 0.95, mins: 300 }));
  ok(hi && hi.p >= 70 && hi.in === true, 't12 a high cell crosses the 70% threshold', hi && hi.p);
  const lowc = lodhodCall(D({ posr: 0.02, mins: 50 }));
  ok(lowc && lowc.p < 70 && lowc.in === false,
     't13 sitting on a fresh extreme does NOT call it in', lowc && lowc.p);
  ok(Math.abs(hi.far - 0.05) < 1e-9,
     't14 `far` is what is still ahead toward the other extreme', hi.far);
  ok(lodhodCall({ ok: false }) === null, 't15 no read -> no call');
  ok(lodhodCall({ ok: true, posr: null, clock: OPEN }) === null,
     't16 ...and a missing posr refuses rather than defaulting to a number');

  // the threshold is the one the DATA picked (F-5), not a preference
  ok(HLTAB_META.thresh === 70,
     't17 the operating point is 70% — where F-5 measured half the range still ahead');

  // posr must be scale-free: the same session on a futures chart gives the SAME posr
  CANDLES = session([{m:0,h:101,l:100,o:100,c:100},{m:5,h:101,l:90,c:92},
                     {m:60,h:130,l:120,c:129},{m:120,h:130,l:125,c:128}]);
  NOWSEC = OPEN + 120*60;
  RR = 1;    const a1 = hodLod('SPY');
  RR = 10.04; const a2 = hodLod('SPY');
  ok(a1.posr != null && Math.abs(a1.posr - a2.posr) < 1e-9,
     't18 posr is scale-free — identical on a futures chart', [a1.posr, a2.posr]);
  ok(a1.far != null && Math.abs((a1.posr + a1.far) - 1) < 1e-9,
     't19 posr + far = 1, both on the running range');
  ok(a2.rngPts !== a1.rngPts, 't19b ...while rngPts DOES convert, proving the scale changed');

  // the face
  const SD = ex('secDay');
  ok(/lodhodCall/.test(SD), 't20 the section calls the table');
  ok(/thin cell/.test(SD), 't21 ...and says so when the cell is too thin to rate');
  ok(/still ahead/.test(SD), 't22 the remaining range is on the face beside the probability');
  ok(/UNPROVEN/.test(src) && /48%/.test(src),
     't23 SWP is demoted to unproven WITH its measured 48%', /UNPROVEN/.test(src));
  ok(/PROVISIONAL/.test(SD), 't24 the hover admits it is a backtest, not a forward test');

  // enrolment — the 2026-08-17 mandate
  ok(/key:'lodhod'/.test(src), 't25 the feature is in the FEATURES registry');
  const rules = JSON.parse(fs.readFileSync('./learning/rules.json', 'utf8'));
  ok(!!rules.rules.lodhod, 't26 ...and has a rule in learning/rules.json');
  ok(rules.rules.lodhod.n === 0 && rules.rules.lodhod.rate === null,
     't27 which starts UNMEASURED — the backtest is not a live rate', rules.rules.lodhod);
  ok(/lodhod_table_calibrated/.test(src), 't28 with a question that forward-tests the table itself');
}

console.log('test_hodlod: ' + pass + ' passed, ' + fail + ' failed');
process.exit(fail ? 1 : 0);
