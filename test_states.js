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
ok(levelMarkerOf(7650,7700,'SPY',true).m==='◂T',
   'm5 dominant pull with no closing distance is POTENTIAL only — the marker says where flow points');
global.closedCandles=()=>Array.from({length:9},(_,i)=>({c:7700+i*2}));   // price walking AWAY
ok(levelMarkerOf(7650,7718,'SPY',true).m==='◂T', 'm6 ...and stays potential while price walks away');
global.closedCandles=()=>Array.from({length:9},(_,i)=>({c:7700-i*4}));   // price closing IN
ok(levelMarkerOf(7650,7668,'SPY',true).m==='ATTRACTING',
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
eval(ex('strikeStepOf'));
let TP={pct:{}}; global.tapeMap=()=>TP;
[7625,7630,7635,7650,7655].forEach(k=>TP.pct[k.toFixed(2)]=1);
ok(strikeStepOf('SPXW')===5, 't4 SPXW\'s step is measured from the tape as 5', strikeStepOf('SPXW'));
TP={pct:{}}; [760,761,762,763].forEach(k=>TP.pct[k.toFixed(2)]=1);
ok(strikeStepOf('SPY')===1, 't5 ...and SPY\'s as 1, so today\'s SPY behaviour is reproduced exactly');
TP={pct:{}}; [7625,7630,7640,7645].forEach(k=>TP.pct[k.toFixed(2)]=1);   // one strike missing
ok(strikeStepOf('SPXW')===5, 't6 the MEDIAN gap is used, so a missing strike cannot skew it');

console.log('test_states: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
