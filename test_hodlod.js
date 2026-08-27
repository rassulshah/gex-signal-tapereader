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

// ---- 7 · WHAT IS DELIBERATELY NOT BUILT, AND SAYS SO -----------------------------------------
// ⚠ BOP/WICK/W.END/WICK%/MUD have no definition anywhere — not the mockup, not the Academy, not a
// spec. The doctrine gate says say so and get agreement rather than invent. Printing five guessed
// timing statistics beside measured ones would make the whole row untrustworthy.
{
  const SD = ex('secDay');
  ok(/pending a definition/.test(SD), 'n1 the unbuilt columns are named as PENDING on the face');
  ok(!/BOP:|wick%|mudMin/.test(src), 'n2 ...and none of them is silently computed anyway');
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

console.log('test_hodlod: ' + pass + ' passed, ' + fail + ' failed');
process.exit(fail ? 1 : 0);
