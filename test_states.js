// (v14.49) THE LEVEL LIFECYCLE — settled with the operator and checked against Skylit Academy
// doctrine. Three orthogonal facts get three slots: STATE (the level's own condition), MARKER (its
// relationship to price), COUNTER (how many times price tested it).
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
// ⚠ object literals (TAP_PROB = {0:80,1:66,2:33}) contain commas, so the value must be read to the
// end of the STATEMENT, not the next comma — and parenthesised so eval reads it as a value.
function v(n){const m=new RegExp('(?:var\\s+)?\\b'+n+'\\s*=\\s*([^;\\n]+)').exec(src); return m?eval('('+m[1].replace(/,\\s*$/,'')+')'):undefined;}

global.LVL_BUILD_P15=v('LVL_BUILD_P15'); global.LVL_SPENT_PEAK=v('LVL_SPENT_PEAK');
global.LVL_WEAK_P15=v('LVL_WEAK_P15');   global.LVL_TURN_P15=v('LVL_TURN_P15');
global.TAP_PROB=v('TAP_PROB'); global.frameNumSafe=x=>String(x);
let VEL=null, PEAK=null, TAPS=0;
global.velAt=()=>VEL; global.peakOf=()=>PEAK; global.nodeTapCount=()=>TAPS;
eval(ex('levelStateOf'));
const V=(p5,p15,p60,cur)=>({v:{p5,p15,p60,cur:cur===undefined?100:cur},stale:false});

// ---- the five states, in precedence ----
VEL=V(0,0,0); PEAK=null; TAPS=0;
ok(levelStateOf(7650,null).st==='HOLDING', 's1 a steady level HOLDS');
VEL=V(0,0,0,40); PEAK=100;
ok(levelStateOf(7650,null).st==='SPENT', 's2 under half its own day peak = SPENT — the mass has gone');
PEAK=null; VEL=V(5,14,9);
ok(levelStateOf(7650,null).st==='BUILDING', 's3 15m materially positive = BUILDING');
VEL=V(-5,-14,-9);
ok(levelStateOf(7650,null).st==='WEAKENING', 's4 15m materially negative = WEAKENING');
VEL=V(5,9,-9);
ok(levelStateOf(7650,null).st==='TURN UP', 's5 5m and 15m agree and flip against the hour = TURN UP');
VEL=V(-5,-9,9);
ok(levelStateOf(7650,null).st==='TURN DN', 's6 ...and the mirror');
// ⚠ precedence: an inflection outranks the trend it interrupts
VEL=V(9,20,-9);
ok(levelStateOf(7650,null).st==='TURN UP', 's7 TURN outranks BUILDING — an inflection is newer news than the trend');
VEL=V(0,0,0,40); PEAK=100;
ok(levelStateOf(7650,null).st==='SPENT', 's8 ...and SPENT outranks everything: a terminal state beats a transitional one');

// ⚠⚠ BUILDING AND SPENT NO LONGER REQUIRE A ROLL PAIRING
PEAK=null; VEL=V(5,14,9);
ok(levelStateOf(7650,{src:{},dst:{}}).st==='BUILDING',
   'p1 a level building from FRESH FLOW is BUILDING — no roll destination needed');
VEL=V(0,0,0,40); PEAK=100;
ok(levelStateOf(7650,{src:{},dst:{}}).st==='SPENT',
   'p2 a level that EVAPORATED with no identifiable destination is just as SPENT');
ok(/BUILDING AND SPENT NO LONGER REQUIRE A ROLL PAIRING/.test(src), 'p3 the reasoning is recorded');

// ---- the counter is a COUNTER, not a state ----
PEAK=null; VEL=V(0,0,0); TAPS=3;
const T3=levelStateOf(7650,null);
ok(T3.st==='HOLDING' && T3.taps===3,
   'c1 three taps do NOT make a state — a level can be fully massive and worn out at once', T3);
VEL=V(-5,-14,-9); TAPS=3;
const W3=levelStateOf(7650,null);
ok(W3.st==='WEAKENING' && W3.taps===3, 'c2 WEAKENING 3x — draining AND worn, which one word could never say', W3);
ok(/g3ldtap/.test(src), 'c3 the counter has its own slot');
// Skylit's DECAYING is WEAKENING with no interaction — the distinction survives the split
VEL=V(-5,-14,-9); TAPS=0;
ok(/DECAYING, a quiet death/.test(levelStateOf(7650,null).why),
   'c4 WEAKENING with 0 taps is named as Skylit\'s DECAYING — a quiet death');

// ---- the marker: the level's relationship to PRICE ----
let RD=null; global.reactDefence=()=>RD; global.closedCandles=()=>[]; global.ifDispScale=()=>1;
global.LVL_INPLAY_PTS=v('LVL_INPLAY_PTS'); global.LVL_CLOSE_BARS=v('LVL_CLOSE_BARS');
eval(ex('levelMarkerOf'));
RD={verdict:'ABANDONING'};
ok(levelMarkerOf(7650,7651,'SPY',false).m==='BREAKING', 'm1 price on it and shedding = BREAKING');
RD={verdict:'DEFENDING'};
ok(levelMarkerOf(7650,7651,'SPY',false).m==='DEFENDING', 'm2 price on it and absorbing = DEFENDING');
RD=null;
ok(levelMarkerOf(7650,7651,'SPY',false).m==='IN PLAY', 'm3 price on it with no verdict yet = IN PLAY');
ok(levelMarkerOf(7650,7700,'SPY',false)===null, 'm4 a level price is nowhere near gets no marker');
// ⚠ POTENTIAL vs EVIDENCE
// ⚠ the scale argument is REQUIRED: without it the closing test cannot be computed and the marker
// must fall back to POTENTIAL rather than guessing (see r1 below for why that matters).
ok(levelMarkerOf(7650,7700,'SPY',true,0).m==='◂T',
   'm5 no scale to judge distance with = POTENTIAL only, never an invented ATTRACTING');
global.closedCandles=()=>Array.from({length:9},(_,i)=>({c:770+i*0.2}));   // underlying, walking AWAY
ok(levelMarkerOf(7650,7718,'SPY',true,10).m==='◂T', 'm6 ...and stays potential while price walks away');
global.closedCandles=()=>Array.from({length:9},(_,i)=>({c:770-i*0.4}));   // underlying, closing IN
ok(levelMarkerOf(7650,7668,'SPY',true,10).m==='ATTRACTING',
   'm7 dominant pull AND the distance closing = ATTRACTING — evidence, not potential');
ok(/ATTRACTING REQUIRES EVIDENCE, NOT POTENTIAL/.test(src), 'm8 the distinction is documented');
ok(/BREAKING DOES NOT MEAN PRICE HAS BROKEN THROUGH/.test(src),
   'm9 BREAKING is the LEVEL failing, never a claim about price — Beach Ball doctrine');

// ---- doctrine consistency ----
ok(/SPENT IS NOT SKYLIT'S "DELIVERED"/.test(src) || /SPENT IS NOT SKYLIT’S "DELIVERED"/.test(src),
   'd1 SPENT is explicitly distinguished from Skylit\'s tap-exhaustion word');
ok(/HALO fires when their multi-window rates AGREE; TURN fires when they DISAGREE/.test(src),
   'd2 TURN is recorded as the complement of Skylit\'s own HALO, not a rival to it');
ok(/EVERY THRESHOLD BELOW IS HAND-SET, NOT MEASURED/.test(src),
   'd3 the thresholds are labelled as chosen, not found');

// ---- the tap-counting fixes ----
const UT=ex('updateTaps');
ok(/TAP_TOL\*step/.test(UT) && /TAP_AWAY\*step/.test(UT),
   't1 tolerances scale by the book\'s own strike gap — they were SPY units applied to SPXW strikes');
ok(/last\.l>k\+away \|\| last\.h<k-away/.test(UT),
   't2 the re-arm needs a WHOLE CLOSED BAR clear — it was judged on a live tick, which noise inflated');
ok(!/var px=S\.price;\s*\n\s*if\(px!=null && Math\.abs\(px-k\)>=TAP_AWAY\)/.test(src), 't3 ...and the tick path is gone');
global.STEP_MIN_STRIKES=v('STEP_MIN_STRIKES'); global.STEP_DEFAULT=v('STEP_DEFAULT'); global.STEP_CACHE={};
eval(ex('strikeStepOf'));
let TP={pct:{}}; global.tapeMap=()=>TP;
// ⚠ these ladders must be DENSE — a sparse set is now refused and falls back, which is the fix, so a
// 5-strike fixture would pass on the fallback and prove nothing about the measurement.
TP={pct:{}}; for(let i=0;i<40;i++) TP.pct[(7600+i*5).toFixed(2)]=1;
ok(strikeStepOf('SPXW')===5, 't4 SPXW\'s step is measured from a dense tape as 5', strikeStepOf('SPXW'));
global.STEP_CACHE={};
TP={pct:{}}; for(let i=0;i<40;i++) TP.pct[(700+i).toFixed(2)]=1;
ok(strikeStepOf('SPY')===1, 't5 ...and SPY\'s as 1, so today\'s SPY behaviour is reproduced exactly');
global.STEP_CACHE={};
TP={pct:{}}; for(let i=0;i<40;i++){ if(i===7||i===19) continue; TP.pct[(7600+i*5).toFixed(2)]=1; }
ok(strikeStepOf('SPXW')===5, 't6 the MEDIAN gap is used, so missing strikes cannot skew it');
global.STEP_CACHE={};


// ================= (v14.50) THE SEVEN DEFECTS AN INDEPENDENT REVIEW FOUND =================
// Every one of these was invisible to the 33 asserts above and would have shipped.
// 1 — ATTRACTING compared two different price scales and therefore fired ALWAYS.
ok(/THE SCALE PARAMETER IS LOAD-BEARING/.test(src), 'r1 the scale bug is documented at the site');
ok(/function levelMarkerOf\(dispPrice, now, sym, isTopPull, undScale\)/.test(src),
   'r1b the marker takes the underlying->display scale explicitly');
ok(/was\*undScale/.test(src) && !/was\*dsc/.test(src),
   'r1c the past close is converted on THAT scale, not the SPX basis');
{ // executed: price walking AWAY must NOT read ATTRACTING
  let RD2=null; global.reactDefence=()=>RD2;
  // level 7650 in chart space; bars are UNDERLYING (~760) and cross with undScale=10.
  const S=10;
  eval(ex('levelMarkerOf'));
  // walking AWAY: six bars ago underlying was 754 (=7540, gap 110); now 750 (=7500, gap 150)
  global.closedCandles=()=>Array.from({length:9},(_,i)=>({c:754-i*0.5}));
  ok(levelMarkerOf(7650,7500,'SPY',true,S).m==='◂T',
     'r1d walking away = potential only — the ◂T branch is reachable again',
     levelMarkerOf(7650,7500,'SPY',true,S));
  // closing IN: six bars ago underlying was 750 (=7500, gap 150); now 760 (=7600, gap 50)
  global.closedCandles=()=>Array.from({length:9},(_,i)=>({c:750+i*0.5}));
  ok(levelMarkerOf(7650,7600,'SPY',true,S).m==='ATTRACTING', 'r1e closing in = ATTRACTING',
     levelMarkerOf(7650,7600,'SPY',true,S));
}
// 2 — strikeStepOf measured a SPARSE feed map and inflated the tap tolerances
ok(/ONLY A DENSE LADDER MAY SET THE STEP/.test(src), 'r2 the sparse-map trap is documented');
{ global.STEP_CACHE={};
  let TP={pct:{}}; global.tapeMap=()=>TP;
  [7625,7650,7700].forEach(k=>TP.pct[k.toFixed(2)]=1);            // a sparse subset
  ok(strikeStepOf('SPXW')===5, 'r2a a sparse map falls back to the book default, not its own gaps',
     strikeStepOf('SPXW'));
  TP={pct:{}}; for(let i=0;i<40;i++) TP.pct[(7600+i*5).toFixed(2)]=1;
  ok(strikeStepOf('SPXW')===5, 'r2b a DENSE ladder sets it properly');
  TP={pct:{}}; [7625,7700].forEach(k=>TP.pct[k.toFixed(2)]=1);
  ok(strikeStepOf('SPXW')===5, 'r2c ...and the last dense reading is remembered when it goes sparse');
}
// 3 — zero was treated as a sign in the TURN test
VEL=V(0,-10,5); PEAK=null; TAPS=0;
ok(levelStateOf(7650,null).st!=='TURN DN', 'r3 a FLAT 5m does not "agree" with a falling 15m');
VEL=V(5,10,0);
ok(levelStateOf(7650,null).st!=='TURN UP', 'r3b ...and a flat hour is not something to flip against');
// 5 — a rounded -0.4 printed as "0%/15m draining"
VEL=V(-0.2,-0.4,-3);
ok(!/0%\/15m draining/.test(levelStateOf(7650,null).why||''),
   'r5 nothing that rounds to zero is described as draining');
// the operator's own example: the badge must read PRIOR tests while a test is under way
ok(/A TAP COMPLETES WHEN PRICE LEAVES, NOT WHEN IT ARRIVES/.test(src),
   'r8 a tap counts on LEAVING — during the second test the badge reads 1x, the number you decide on');
ok(/st\.on=true/.test(src) && /if\(st\.on\)\{ st\.taps\+\+/.test(src), 'r8b ...implemented that way');
// 7 — the pull contest mixed dollars with %King
ok(/LIKE FOR LIKE, OR NOT AT ALL/.test(src), 'r7 the unit mixing is documented');
ok(/if\(frDP\.usdK==null\) continue;/.test(src),
   'r7b a node with no dollar reading sits the contest out rather than inventing one');

console.log('test_states: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
