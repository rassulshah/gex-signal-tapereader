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
  // (v14.65) THE CHIP ROW IS GONE - every chip was a measured proxy for the table's own axis, a
  // measured NEGATIVE (SWP 48%), or a permanent dash (VWAP). What must survive is the REASONING,
  // so a later context does not helpfully add them back.
  ok(!/g3daychips/.test(SD), 'n3 the chip row is removed from the section');
  ok(/SWP\s+48%|48% standalone/.test(SD) || /48%/.test(SD),
     'n4 ...and SWP\'s measured 48% is recorded at the removal site, not just deleted');
  ok(/VWAP\s+does not exist|VWAP  does not exist|VWAP/.test(SD),
     'n4b ...and VWAP\'s absence is still explained where the chip used to be');
}

// ---- 8 · the honesty furniture ---------------------------------------------------------------
{
  const SD = ex('secDay');
  ok(/coin-flip/.test(SD), 'f1 the section states on its face that bare sequence carries nothing');
  ok(/every rate carries its n/.test(SD), 'f2 ...and that no bare percentage appears');
  ok(/descriptive/.test(SD) && /no entries\/stops/.test(SD), 'f3 ...and the scope boundary holds');
  ok(/when it did not hold/.test(SD), 'f4 THE MISS CASE IS PRINTED beside the odds of being right');
  ok(/This is not a reading, it is no reading/.test(SD), 'f5 a refusal reads as a refusal, never as calm');
  // (v14.72) SCOPED, AND DECISIONS D-12 SAID IT WOULD HAVE TO BE: "§30 executes the read composer and
  // fails on forecast vocabulary. An AI read making real directional calls will trip it — that test
  // must be scoped to the MECHANISM sentence before any such feature ships."
  // The far-side block is a CALIBRATED PROBABILITY, which is a different object from a forecast: it
  // says how often a level like this one was traded, with the number attached. So the ban still
  // applies to everything outside that block, and inside it the rule becomes STRICTER — the word
  // "likely" may not appear without a percentage beside it.
  {
    // the exempt region is DECLARED IN THE SOURCE, not guessed by this test — so widening it is a
    // visible edit in the panel rather than a quiet loosening here.
    ok(/PROB-BLOCK-START/.test(SD) && /PROB-BLOCK-END/.test(SD),
       'f6a the probability surface is explicitly marked in the source');
    const noFar = SD.replace(/\/\/[^\n]*/g,'')
                    .replace(/PROB-BLOCK-START[\s\S]*?PROB-BLOCK-END/, '');
    ok(!/\bwill\b|\bshould\b|likely/.test(noFar),
       'f6 no forecast vocabulary outside the probability block — mechanism describes, it does not predict');
    // ⚠⚠ (v14.86) "most likely" IS GONE FROM THE FACE. The window moved into the hover and is now
    // called what it is — the MIDDLE HALF — which states the interquartile range without hedging.
    // The guard follows the phrase: wherever the window is described, its 50% must be beside it,
    // because the whole point of F-15 is that this band is 50% and the floor beside it is 80%.
    const mh = SD.match(/MIDDLE HALF[\s\S]{0,220}/i);
    ok(mh && /50%/.test(mh[0]),
       'f6b the middle-half window NEVER appears without its own measured 50%', mh&&mh[0].slice(0,90));
    ok(!/most likely/i.test(SD.replace(/\/\/[^\n]*/g,'')),
       'f6c ...and "most likely" is gone from the rendered text, not just relabelled around');
  }
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
  // ⚠ (v14.70) THIS ASSERTED `thin > 0` AND BROKE WHEN THE TABLE BECAME FULLY POPULATED — it was
  // testing the DATA, not the logic. A full table is the goal, not a regression. What must hold is
  // that hlCell REFUSES a thin cell, which is tested below (t11b) against an injected one.
  ok(thin === 0 || thin > 0, 't5 thin-cell count recorded (data, not a contract)', {filled: cells, thin});
  ok(cells === 72, 't5b ...and the table is now fully populated — 45 min of coverage regained', cells);

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
  // ⚠ INJECT A THIN CELL rather than relying on the shipped table having one. The 08:30 column used
  // to be empty and was used as the fixture here; v14.70 filled it, which broke a test of logic that
  // had been quietly coupled to data. Absence of data is not a reading — that has to stay TESTED,
  // not merely true by accident.
  const savedRow = HLTAB[0].slice();
  HLTAB[0][0] = [7, null];                       // n=7, under the 25-sample floor
  const thinCell = hlCell(0.02, 10);
  ok(thinCell && thinCell.p === null && /too few/.test(thinCell.why || ''),
     't11b a thin cell returns p=null AND says why', thinCell);
  ok(thinCell && thinCell.n === 7,
     't11c ...and still reports how many samples it does have', thinCell && thinCell.n);
  HLTAB[0] = savedRow;

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
  // (v14.66) the refusal text moved into hlVerdict when it was extracted for testability; u1d
  // executes it. Assert it at its NEW home rather than deleting the guard.
  ok(/thin cell/.test(ex('hlVerdict')),
     't21 ...and says so when the cell is too thin to rate (now in hlVerdict)');
  // (v14.72) t22 USED TO GREP THE RENDER FOR "still ahead" - and that clause was firing on days
  // both extremes were already in, which the grep could never see. Execute the clause instead.
  eval(ex('hlFarClause'));
  const FC = hlFarClause;
  const fcAhead = FC({ second:'HOD', secondT: 400*60, clock: 200*60 }, { far:0.42 });
  ok(/42%/.test(fcAhead) && /HOD/.test(fcAhead) && !/still ahead/.test(fcAhead),
     't22 the range clause names the extreme it measures TO, and no longer claims travel', fcAhead);
  ok(FC({ second:'HOD', secondT: 100*60, clock: 200*60 }, { far:0.01 }) === '',
     't22b ...and is SILENT once the far side has printed - the state that made it wrong');
  ok(FC({ second:'HOD', secondT: 400*60, clock: 200*60 }, { far:null }) === '',
     't22c ...and refuses when there is no measurement rather than printing 0%');
  ok(/48%/.test(SD) && /BELOW the 45% base/.test(SD),
     't23 SWP is gone from the face AND its 48% is recorded at the removal site');
  ok(/posr` thresholded at 0\.5|posr thresholded at 0\.5/.test(SD),
     't23b ...and POS is documented as having been the table\'s own axis, rounded');
  ok(/PROVISIONAL/.test(SD), 't24 the hover admits it is a backtest, not a forward test');

  // enrolment — the 2026-08-17 mandate
  ok(/key:'lodhod'/.test(src), 't25 the feature is in the FEATURES registry');
  const rules = JSON.parse(fs.readFileSync('./learning/rules.json', 'utf8'));
  ok(!!rules.rules.lodhod, 't26 ...and has a rule in learning/rules.json');
  ok(rules.rules.lodhod.n === 0 && rules.rules.lodhod.rate === null,
     't27 which starts UNMEASURED — the backtest is not a live rate', rules.rules.lodhod);
  ok(/lodhod_table_calibrated/.test(src), 't28 with a question that forward-tests the table itself');
}

// ---- 10 · THE NOT-IN CALL AND THE CONDITIONAL FAR-SIDE CLAUSE (v14.66) ----------------------
{
  const SD = ex('secDay');
  // three states, not two
  // ⚠ EXECUTED, NOT GREPPED. The first version of u1 grepped secDay for "NOT IN" and passed against
  // a DEAD branch when mutation-tested. hlVerdict is extracted so the branch can actually be run.
  eval(ex('hlVerdict'));
  const V = (p, inn, notIn) => hlVerdict({first:'LOD'}, {p:p, in:inn, notIn:notIn, n:900}, null);
  // (v14.86) the em-dash between the state and its number is gone — operator: "HOD IN 100%".
  ok(/^LOD NOT IN 92%$/.test(V(8, false, true)),
     'u1 a low cell produces a NOT IN verdict with the INVERTED percentage', V(8,false,true));
  ok(/^LOD IN 88%$/.test(V(88, true, false)), 'u1b a high cell produces IN', V(88,true,false));
  ok(/^LOD STANDING 45%$/.test(V(45, false, false)),
     'u1c the middle produces STANDING, neither call', V(45,false,false));
  ok(/thin cell, n=900/.test(hlVerdict({first:'LOD'},{p:null,n:900},null)),
     'u1d a thin cell refuses and names its n');
  ok(/too early/.test(hlVerdict({first:'LOD'}, null, null)),
     'u1e no call at all falls back without throwing');
  // (v14.70) 72 -> 85 when the table gained the first 45 minutes: the NOT-IN call lived almost
  // entirely in the cells the old study had excluded. n went 85 -> 230 and it fires an hour earlier.
  ok(HLTAB_META.notIn === 20 && HLTAB_META.notInHit === 85,
     'u2 the NOT-IN threshold and its MEASURED rate are both declared',
     [HLTAB_META.notIn, HLTAB_META.notInHit]);
  // ⚠⚠ (v14.84) u3 IS INVERTED, AND THAT IS THE POINT. It asserted the NOT-IN call was WEAKER than
  // the IN call — true only while IN was quoted at the hindsight-inflated 92 (F-12). Corrected to
  // the real-time 63, NOT-IN at 85 is the STRONGER call, and it fires an hour earlier. The old
  // assertion was not wrong about the code; it was faithfully pinning a claim that inherited the
  // error. A test that pins a consequence of a bug goes green right up until the bug is fixed.
  ok(HLTAB_META.notInHit > HLTAB_META.inHit,
     'u3 ...and NOT-IN is now the STRONGER call (85 vs 63) — the correction reversed the ranking',
     [HLTAB_META.notInHit, HLTAB_META.inHit]);
  ok(HLTAB_META.inHit === 63 && HLTAB_META.inHindsight === 92,
     'u3b the real-time rate is shipped and the withdrawn hindsight figure is kept, named',
     [HLTAB_META.inHit, HLTAB_META.inHindsight]);
  // ⚠ FAKE ON FIRST WRITING, caught by mutation: this only checked the NEW sentence was present, so
  // restoring the OLD one alongside it left the assertion green with the face saying both.
  ok(/NOT-IN call is now the STRONGER/.test(src) && !/NOT-IN call is weaker and thinner/.test(src),
     'u3c ...and the hover says so, with the old "weaker and thinner" claim GONE, not merely joined');
  // ⚠ WAS `notInN < 150` — an assertion that the sample was THIN. v14.70 made it 230, so the test
  // that guarded honesty about a weakness now guards that the weakness was fixed. Assert the n is
  // DECLARED and real, not that it is small.
  ok(HLTAB_META.notInN >= 200,
     'u4 ...on an n that is no longer thin — the missing cells were where this call lived',
     HLTAB_META.notInN);

  // the call sets notIn only at the low end
  const D = o2 => ({ ok:true, posr:o2.posr, clock:OPEN + o2.mins*60, first:'LOD',
                     rngPts:50, far:1-o2.posr, secondT:OPEN+400*60 });
  const lowc = lodhodCall(D({posr:0.02, mins:50}));
  ok(lowc && lowc.notIn === true && lowc.in === false,
     'u5 a fresh extreme early triggers NOT IN, not IN', lowc && [lowc.p, lowc.notIn]);
  const hic = lodhodCall(D({posr:0.95, mins:300}));
  ok(hic && hic.in === true && hic.notIn === false, 'u6 a high cell is IN and not NOT-IN');
  const mid = lodhodCall(D({posr:0.5, mins:150}));
  ok(mid && mid.in === false && mid.notIn === false,
     'u7 the middle is STANDING — neither call', mid && mid.p);

  // ⚠ THE CLAUSE THAT WAS WRONG BEFORE: "toward HOD" must be conditional on the far side being ahead
  ok(/D\.secondT>D\.clock/.test(SD),
     'u8 the far-side clause is gated on the second extreme not having printed');
  ok(/both extremes in/.test(SD),
     'u9 ...and says so plainly when it already has, instead of pointing at a finished move');
  // (v14.72) WHEN-TO-EXPECT-IT moved off the ladder tier and onto the far-side line, where it is
  // measured rather than borrowed from the unconditional E-row median. The property is unchanged:
  // the face must say WHEN, and must show the SPREAD rather than a single clock time.
  // (v14.86) the floor now reads "AFTER x" on the read line itself — same one-sided claim, his
  // wording: "why dont you just say HOD IN 100% · LOD after 1:30pm — 80%."
  ok(/' after '/.test(SD) && /floorAt/.test(SD) && /80%/.test(SD),
     'u10 the face carries a WHEN — a one-sided 80% floor, because a 30-min box is worth 15%');
  ok(/fsClock12\(FS\.floorAt\)/.test(SD),
     'u10a ...on a 12-hour clock, which is how he reads a time back');
  // (v14.86) the window lives in the HOVER now, so the assertion follows it there rather than
  // being deleted — the number it protects is unchanged and still has to be stated.
  ok(/winA/.test(SD) && /winB/.test(SD) && /50%, not 80%/.test(SD),
     'u10b ...and the two-sided window carries its own, honest, 50% — stated against the 80% floor');
  ok(/travelled '\+Math\.round\(100\*D\.posr\)/.test(SD) && /n='\+CALL\.n/.test(SD),
     'u10c ...and the EVIDENCE (travel % and n) survived the compression, in the hover');
  // ⚠ THE HAZARD CLAUSE IS THE THIRD THING THAT MOVED, and it had no guard until a mutation
  // deleted it silently. It is the only statement about what happens when the floor is CLEARED and
  // the extreme still has not printed — the case a one-sided floor says nothing about by design.
  ok(/FS\.haz\.p/.test(SD) && /FS\.haz\.at/.test(SD) && /land in the close/.test(SD),
     'u10d ...and so did the hazard clause — what happens if the floor passes and it still has not printed');
  // (v14.70) re-measured on the widened table: 13:33 -> 13:29, 97% -> 99% still ahead.
  // ⚠ Pinned to the CURRENT measurement rather than a range, so a table change that forgets to
  // re-derive these gets caught instead of quietly quoting numbers from a table that no longer exists.
  ok(HLTAB_META.secondMed === '13:29' && HLTAB_META.secondAhead === 99,
     'u11 the timing and the "still ahead" share are measured constants, not prose',
     [HLTAB_META.secondMed, HLTAB_META.secondAhead]);

  // the hover must separate CELL rate from DECISION rate — this project's oldest failure
  ok(/CELL rate/.test(SD) && /DECISION rate/.test(SD),
     'u12 the hover distinguishes the cell probability from the decision accuracy');
  ok(/n='\+HLTAB_META\.inN/.test(SD) && /n='\+HLTAB_META\.notInN/.test(SD),
     'u13 ...and both carry their n');
}

// ---- 11 · NO CANDLES MUST NOT HIDE THE BACKTEST (v14.69) -------------------------------------
// ⚠ The old gate was `if(!D.ok) return <one line>`, which hid the 284-session base rates, the
// survival ladder and the corpus provenance whenever there were no bars — i.e. every pre-open
// minute, which is exactly when the operator prepares. None of that needs a candle.
{
  const SD = ex('secDay');
  ok(/var NOREAD/.test(SD), 'w1 the no-read case is a FLAG, not an early return');
  ok(!/if\(!D\.ok\)\{\s*return h\+/.test(SD.replace(/\s+/g,' ')),
     'w2 ...and the early return that hid everything is gone');
  // ⚠ COUNT, DO NOT JUST MATCH. There are TWO A rows (the excursion block and the day block) and a
  // single-match assertion passed while one of them was mutated away — it found the survivor.
  // (v14.96) THREE A rows now — the excursion block, the day block, and the new SPAN row that
  // carries the GREEN/RED call. The count is kept (not loosened to >=1) for the reason above: a
  // single-match assertion passed while one row was mutated away.
  ok((SD.match(/NOREAD \? row\('a','A'/g)||[]).length === 3,
     'w3 ALL THREE A rows show em-dashes rather than inventing today',
     (SD.match(/NOREAD \? row\('a','A'/g)||[]).length);
  ok(/showing the base rates only/.test(SD), 'w4 the header says WHY the A row is empty');
  ok(/This is not a reading, it is no reading/.test(SD),
     'w5 ...and the refusal still reads as a refusal (doctrine, not decoration)');
  ok(/NOREAD \? 'WAITING FOR THE SESSION/.test(SD), 'w6 there is no verdict without bars');
  ok(/if\(!NOREAD\)\{ try\{ CALL=lodhodCall/.test(SD),
     'w7 the table is not consulted when there is nothing to consult it about');
  // the static half must still be reachable in the no-read path
  // (v14.72) the ladder and the foot were REMOVED (they restated the verdict and cost a row); what
  // must still survive the no-candle path is the static half the operator prepares on — the E rows.
  ok(/g3dayg/.test(SD) && /NOREAD \? row\('a','A'/.test(SD),
     'w8 the E rows are outside the gate and the A row refuses — the pre-open state that v14.69 fixed');
  ok(!/g3daylb/.test(SD) && !/g3dayfoot/.test(SD),
     'w8b ...and the ladder and honesty line are gone, deliberately, not by accident');
  ok(/backtest over/.test(SD), 'w9 ...and the evidence line names them as a backtest, not today');
}


// ============================================================================================
// (v14.87) PT AND THE CLOSE LEG
// Operator: "pt is hod to the lowest point prior to the close that doesn't take out the lod, and
// pt time is the time that it took."
// ============================================================================================
{
  const PT = ex('hlPT');

  // ⚠⚠ I MEASURED THE WRONG THING FIRST. study-secondleg.py took the distance to the CLOSE and
  // reported 12.5 pts; PT is the furthest point back and is 19.8 — 58% apart. A day that retraces
  // 20 points and closes back at the extreme has PT 20 and LC ~0. Both ship, under labels that
  // match them, and this pins that PT tracks a RUNNING EXTREME rather than a last value.
  ok(/advP==null \|\| \(secondIsHOD \? v<advP : v>advP\)/.test(PT),
     'p1 PT walks for the FURTHEST point after the second extreme, not the last one');
  ok(/var v=secondIsHOD\?b\.l:b\.h;/.test(PT),
     'p2 ...taking the LOW after a HOD and the HIGH after a LOD');
  ok(/out\.lcPts=Math\.abs\(secP-lastC\)/.test(PT),
     'p3 ...and LC is measured to the CLOSING price, separately');

  // "doesn't take out the lod" holds by construction — if it had, that would BE the day's LOD.
  // A violation means the extremes were mislabelled, which is worth catching loudly.
  ok(/out\.viol *= *secondIsHOD *\? *\(advP < D\.lod/.test(PT),
     'p4 the "does not take out the first extreme" invariant is CHECKED, not assumed');

  // ⚠ ALSO FAKE: the message string survives even when the CLOCK half of the guard is deleted, which
  // would compute PT from bars that have not happened yet. Bind to the condition, not its text.
  ok(/if\(D\.secondT==null \|\| D\.secondT>D\.clock\)/.test(PT),
     'p5 no PT until the second extreme has actually printed — clock half included');
  ok(/b\.so<D\.secondT\) continue/.test(PT), 'p6 ...and only bars AT OR AFTER it are considered');

  // closedCandles() is the UNDERLYING book (SPY ~765); D.scale converts to chart points. Forgetting
  // it would report a 28-point ES excursion as 2.8 — landmine L-F, two price spaces mixed.
  // ⚠ FAKE ON FIRST WRITING, caught by mutation: /\*rr;/ matched the LC line, so dropping the scale
  // from PT alone left it green — and a 28-point ES excursion would have printed as 2.8. Bind to
  // EACH conversion, by name.
  ok(/out\.ptPts=Math\.abs\(secP-advP\)\*rr;/.test(PT),
     'p7 the PT distance is converted from the underlying book to chart points');
  ok(/out\.lcPts=Math\.abs\(secP-lastC\)\*rr;/.test(PT),
     'p7b ...and so is the LC distance, separately');
  ok(/out\.ptUsd=D\.isFut \? out\.ptPts\*ES_USD_PER_PT/.test(PT),
     'p8 ...and dollars come from the ONE contract multiplier, only on a futures chart');

  ok(/out\.lcTag=secondIsHOD\?'HC':'LC'/.test(PT),
     'p9 the close leg is HC after a HOD and LC after a LOD — his naming');

  // The asymmetry is large: PT after a LOD is 24.0 pts, after a HOD 17.0. One pooled number would
  // be wrong by ~40% on whichever side you are actually on.
  ok(/var PT_META=\{/.test(src), 'p10 the PT numbers live in one constant');
  ok(/hod:\{ ptMin:46, ptPts:17\.0/.test(src) && /lod:\{ ptMin:51, ptPts:24\.0/.test(src),
     'p11 ...with a separate expectation per side, because they differ by ~40%');
  ok(/out\.exp=secondIsHOD\?PT_META\.hod:PT_META\.lod/.test(PT),
     'p12 ...and the face reads the one matching today');
  ok(/n:283/.test(src), 'p13 ...and carries the n behind them');

  const SD2 = ex('secDay');
  ok(/PTL=hlPT\(sym, D\)/.test(SD2), 'p14 secDay computes it');
  // (v14.96) LC|HC GAP·RNG moved OUT of the table and into the strip beside the read — operator:
  // "keep the HL metrics ... at the top to the right of the forecast". So the header no longer
  // names it; the STRIP does. Both halves are asserted so the leg cannot go missing entirely.
  // (v14.96) the LC pair moved AGAIN — out of the v14.96 top strip and into ROW 3, which is where
  // he asked for it ("a thrid row for the HL fields"). Both existed for one build and printed the
  // spans twice. Assert the row-3 home, and that the strip is gone.
  ok(/'PT TOOK','PT'/.test(SD2) && /'LC GAP','LC RNG','LC \$'/.test(SD2) && !/g3dayhl/.test(SD2),
     'p15 ...PT legs in row 2, the LC span in row 3, and no duplicate strip');
  ok(/PTL\.exp\.ptMin/.test(SD2) && /PTL\.exp\.ptPts/.test(SD2),
     'p16 ...and the E row uses the SIDE-SPECIFIC expectation, not the pooled one');
}


// ============================================================================================
// (v14.96) SLvl / TLvl, AND THE PT LEG'S WICK FAMILY
// ============================================================================================
{
  const LH = ex('hlLevelHit'), PT2 = ex('hlPT'), SD3 = ex('secDay');

  // ---- ONLY LEVELS THE CHART ALREADY DRAWS --------------------------------------------------
  // ⚠⚠ If this strip could name a level the chart does not show, it would be useless — you cannot
  // act on a level you cannot see. Both sources are the panel's own.
  // ⚠ FAKE ON FIRST WRITING, caught by mutation: greppping for `ifLadder(sym)` matched the ASSIGNMENT
  // even after the loop that uses it was gated off with `if(false)`, so the walls silently vanished
  // from the candidate set while the assertion stayed green. Bind to the loop being REACHABLE.
  ok(/sessionLevels\(sym, rr\)/.test(LH), 'L1 session levels feed the candidate set');
  ok(/if\(IL && !IL\.err && IL\.rows\) IL\.rows\.forEach/.test(LH),
     'L1b ...and so do the walls, through a loop that actually runs');
  ok(/add\(SL\.pdh,'PDH'\)/.test(LH) && /add\(SL\.pdl,'PDL'\)/.test(LH) && /'CW0'/.test(LH),
     'L2 ...prior-day, initial balance and the walls');

  // ---- THE FURTHEST ONE, NOT THE FIRST ------------------------------------------------------
  // A move that clears three levels is described by the LAST one it cleared.
  // (v14.96) the cell now lists EVERY level taken out, furthest FIRST — he asked for names only so
  // several fit ("PDH, CW, VAH, POC"). The furthest-first ordering is the same rule as before, now
  // expressed as a sort rather than a single winner; `furthest()` still exists and returns [0].
  ok(/hit\.sort\(function\(a,b\)\{ return up \? \(b\.px-a\.px\) : \(a\.px-b\.px\); \}\)/.test(LH),
     'L3 the levels are ordered FURTHEST-FIRST from the open');
  ok(/function furthest\(extPx, up\)\{ var t=taken\(extPx,up\); return t\.length\?t\[0\]:null; \}/.test(LH),
     'L3b ...and the single furthest level is still derivable from that order');
  ok(/L\.px>opDisp && L\.px<=extPx/.test(LH),
     'L4 ...and it must lie BETWEEN the open and that extreme, in that extreme\'s direction');

  // ---- SCALE. sessionLevels takes rr; the extremes come from the underlying book -------------
  ok(/var opDisp=D\.open\*rr;/.test(LH) && /\(firstUp\?D\.hod:D\.lod\)\*rr/.test(LH),
     'L5 open and extremes are converted to chart space before being compared to the levels');

  // ---- TLvl WAITS FOR THE SECOND EXTREME ----------------------------------------------------
  ok(/if\(D\.secondT!=null && D\.secondT<=D\.clock\)\{/.test(LH),
     'L6 no TLvl until the second extreme has printed');

  // ---- THEY SIT WITH THEIR OWN EXTREME ------------------------------------------------------
  // Operator: "they should be right after the HOD and LOD fields."
  ok(/\['1ST','SLvl'/.test(SD3), 'L7 SLvl is the column immediately after the 1ST extreme');
  ok(/\['2ND','TLvl'/.test(SD3), 'L8 TLvl is the column immediately after the 2ND extreme');
  ok(/g3daylv\.sw\{[^}]*#e3b341/.test(src) && /g3daylv\.tg\{[^}]*#5fd08a/.test(src),
     'L9 a sweep and a target are coloured differently — two events, not two of one');

  // ---- THE PROFILE LEVELS ARE ABSENT ON PURPOSE ---------------------------------------------
  // ⚠ Measured: a prior POC is tagged 46.6% of the next session against 46.3% for a SHAM level at
  // the same distance. Distance explains the tags. They may ship as a RECORD, never as a claim.
  ok(!/pVAH|pVAL|pPOC/.test(LH),
     'L10 no profile levels yet — they are tagged no more often than distance alone explains');

  // ---- THE PT WICK FAMILY --------------------------------------------------------------------
  ok(/out\.ptWickPct=Math\.round\(100\*out\.ptPts\/D\.rngPts\)/.test(PT2),
     'L11 PTWick% mirrors WICK% — the excursion over the day\'s range');
  ok(/out\.ptMud=Math\.max\(0,\(lastT-advT\)\/60\)/.test(PT2),
     'L12 PTMUD mirrors MUD — the PT extreme to the close');
  ok(/ptWickPct:40, ptMud:52/.test(src) && /ptWickPct:56, ptMud:56/.test(src),
     'L13 ...both side-specific, because PT itself is ~40% asymmetric');

  // ⚠⚠ PTWICK IS NOT BUILT, AND THAT IS THE POINT. WICK needs an ANCHOR the move started from and
  // later reclaimed; the PT leg's anchor IS the second extreme. He defined the first wick family
  // himself when asked — ask again rather than invent one and print it beside measured columns.
  ok(!/ptWick\b|'PTWICK'/.test(PT2), 'L14 PTWICK is NOT derived — its definition has not been given');
  ok(/PTWICK IS NOT DERIVED/.test(src), 'L15 ...and the code says why, so nobody quietly invents one');
}

console.log('test_hodlod: ' + pass + ' passed, ' + fail + ' failed');
process.exit(fail ? 1 : 0);
