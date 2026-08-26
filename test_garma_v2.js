// (v14.41) GARMA V2 PHASE 0 — the corrections, and the refusals.
// Source: garma/claude_package_v2 (59 rules; 17 new, 11 materially corrected) + garma/V2-PHASE-PLAN.md.
// Phase 0 adds NOTHING new to the face except an air pocket on the path clause; the rest is gating
// what already shipped. These tests exist because "we already do that correctly" is a claim that
// rots — every one of them asserts a property that a future refactor could quietly break.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
// ⚠ the object literal `{0:80,...}` must be parenthesised or eval reads it as a BLOCK and throws
function v(n){const m=new RegExp('var\\s+'+n+'\\s*=\\s*([^;\\n]+)').exec(src); return m?eval('('+m[1]+')'):undefined;}

// ================= GM-MAP-004 — an air pocket is a PATHWAY, never a destination =================
global.AIRPOCKET_GAP_MULT=v('AIRPOCKET_GAP_MULT');
global.AIRPOCKET_VACUUM_MULT=v('AIRPOCKET_VACUUM_MULT');
global.AIRPOCKET_MIN_STRIKES=v('AIRPOCKET_MIN_STRIKES');
ok(global.AIRPOCKET_GAP_MULT===2.5 && global.AIRPOCKET_VACUUM_MULT===4.0,
   'a1 the Academy thresholds are reused verbatim, not re-invented',
   [global.AIRPOCKET_GAP_MULT, global.AIRPOCKET_VACUUM_MULT]);
eval(ex('railPockets')); eval(ex('pocketOnPath'));

// an evenly-spaced ladder has no pocket at all
const even=[7650,7655,7660,7665,7670,7675].map(x=>({disp:x}));
ok(railPockets(even,1).length===0, 'a2 regular spacing is not a pocket');

// one wide gap -> a pocket; a wider one -> a vacuum
const gap=[7650,7655,7660,7690,7695,7700].map(x=>({disp:x}));
let PK=railPockets(gap,1);
ok(PK.length===1 && PK[0].lo===7660 && PK[0].hi===7690, 'a3 the empty bracket between two nodes IS the pocket', PK);
ok(PK[0].vacuum===true, 'a4 30 pts against 5-pt spacing is an extended pocket = LIQUIDITY VACUUM', PK[0].span);
const modest=[7650,7655,7660,7675,7680,7685].map(x=>({disp:x}));
let PM=railPockets(modest,1);
ok(PM.length===1 && PM[0].vacuum===false, 'a5 ...while 15 pts is a pocket but NOT yet a vacuum', PM);

// the floor: nothing narrower than the absolute minimum is ever called a pocket
const tiny=[7650,7650.2,7650.4,7651.2,7651.4].map(x=>({disp:x}));
ok(railPockets(tiny,1).length===0, 'a6 a tight ladder never manufactures a pocket (absolute floor holds)');

// pocketOnPath answers the PATH question only
ok(pocketOnPath(PK,7655,7700)===PK[0], 'a7 a pocket between price and target is found');
ok(pocketOnPath(PK,7692,7700)==null,   'a8 ...and one that is NOT on the way is not claimed');
ok(pocketOnPath(PK,7700,7655)===PK[0], 'a9 direction-agnostic — a downward path finds it too');

// THE STRUCTURAL GUARANTEE: the destination comes from RAILPS (nodes), so a gap can never be one.
ok(/frBestP\s*=/.test(src) && /destination: '\+frameNum\(frBestP\.disp\)/.test(src),
   'a10 the destination is a PILE, never a gap — pockets are not in RAILPS by construction');
ok(/through '\+\(frPk\.vacuum\?'a LIQUIDITY VACUUM':'an AIR POCKET'\)/.test(src),
   'a11 the pocket rides the PATH clause, where doctrine puts it');
ok(/a pathway, not a target/.test(src), 'a12 ...and says so in the sentence itself');

// ================= GM-MAP-007 — a used level is a weaker level =================
let TAPN=0, VEL=null, PEAK=null;
global.nodeTapCount=()=>TAPN;
global.velAt=()=>VEL;
global.peakOf=()=>PEAK;
global.TAP_PROB=v('TAP_PROB');
global.LVL_WEAK_P15=v('LVL_WEAK_P15'); global.LVL_TURN_P15=v('LVL_TURN_P15'); global.LVL_DOOR_PEAK=v('LVL_DOOR_PEAK');
eval(ex('levelStateOf'));
const steady={v:{cur:100,d15:0,d60:0,p5:0,p15:0,p60:0},stale:false};
VEL=steady; TAPN=0;
ok(levelStateOf(7650,null).st==='HOLDING', 't1 an untouched steady level still HOLDS');
TAPN=1;
let L1=levelStateOf(7650,null);
// ⚠ TAP_PROB IS INDEXED BY TAPS ALREADY TAKEN, NOT BY WHICH TAP THIS IS. TAP_PROB[0]=80 is the
// UNTOUCHED level's first-test odds, so a level that has been tapped ONCE carries TAP_PROB[1]=66 as
// the odds of its NEXT test. Getting this backwards would overstate every used level by one grade.
ok(L1.st==='HOLDING' && /1 tap today/.test(L1.why) && /66%/.test(L1.why),
   't2 one tap taken -> the NEXT test is the ~66% one, not the ~80% one', L1);
TAPN=2;
ok(/33%/.test(levelStateOf(7650,null).why), 't3 two taps taken -> the next test is the ~33% graveyard read');
TAPN=3;
let L3=levelStateOf(7650,null);
ok(L3.st==='USED' && /33%/.test(L3.why), 't4 the THIRD tap is a regime change — USED, ~33% (graveyard)', L3);
ok(L3.st.length===4, 't5 USED fits the rail\'s 4-letter state slot without truncation');
// ranking: a live state outranks the tap count — fresh size arriving is the newer fact
VEL={v:{cur:100,d15:5,d60:0,p5:0,p15:0,p60:0},stale:false}; TAPN=5;
ok(levelStateOf(7650,{src:{},dst:{7650:1}}).st==='FORMING',
   't6 a level RECEIVING a roll reads FORMING even when tapped out — new size is the newer fact');
VEL={v:{cur:10,d15:0,d60:0,p5:0,p15:0,p60:0},stale:false}; PEAK=100; TAPN=9;
ok(levelStateOf(7650,{src:{7650:1},dst:{}}).st==='DOOR', 't7 ...and a drained source is still a DOOR');
VEL=steady; PEAK=null;
ok(/g3lvwUSED/.test(src) && /g3pile\.g3lvUSED/.test(src), 't8 USED has its own (deliberately dim) styling');

// ================= GM-EVENT-001 / GM-REG-002 — downgrade, never suspend =================
ok(/normal rules NOT suspended/.test(src), 'e1 the event clause states the cap explicitly');
ok(!/normal-rule confidence downgraded/.test(src), 'e2 the old ambiguous wording is gone');
global.eventTagLabel=()=>'FOMC'; global.isOpexDay=()=>false; global.trendVerdict=()=>({state:'na'});
global.evCalFetch=()=>{}; global.evCalActive=()=>null;
eval(ex('dayTypeOf'));
let EV=dayTypeOf('SPY',{ok:true,em:20,upExc:2,dnExc:3});
ok(EV.t==='EVENT' && /NOT suspended/.test(EV.why), 'e3 an event day caps confidence and says so', EV);
// the structural guarantee: the day type is ONE clause, composed with the others, never replacing them
ok(/FR\.push/.test(src) && /KING/.test(src) && /destination:/.test(src),
   'e4 King and Destination clauses are composed independently of the day type');
ok(!/if\s*\(\s*frDT\.t\s*===\s*'EVENT'\s*\)\s*return/.test(src),
   'e5 nothing short-circuits the read on an event day');

// ================= GM-TOOL-001/002 — the refusals =================
ok(/THREE FEATURES WE ARE DELIBERATELY NOT BUILDING/.test(src), 'x1 the tool-hierarchy refusal is recorded in the file');
ok(/GAMMA VWAP is OPTIONAL EXPERIMENTAL CONFLUENCE/.test(src) && /FALCON is not defined/.test(src),
   'x2 Gamma VWAP and Falcon are named as non-triggers');
ok(!/gammaVwap|gexVwap/i.test(src.replace(/\/\/[^\n]*/g,'')), 'x3 ...and no code reads them');

console.log('test_garma_v2: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
