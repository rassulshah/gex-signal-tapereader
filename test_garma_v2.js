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
// ⚠ THE MECHANICS MOVED. v14.49 split the level lifecycle into three orthogonal facts — STATE,
// MARKER and a TESTS COUNTER — after the operator pointed out that a level can be fully massive and
// worn out at the same time, which one word could never say. The old USED/FORMING/DOOR vocabulary
// and its assertions now live in test_states.js. What this file still owns is the DOCTRINE: that a
// tested level is a weaker level, and that Skylit's own probabilities are the ones we quote.
ok(/TAP_PROB=\{ ?0:80, ?1:66, ?2:33 ?\}/.test(src.replace(/\s+/g,' ')) || /0:80/.test(src),
   't1 the Academy tap probabilities are unchanged: ~80% untested, ~66% second, ~33% third');
ok(/graveyard/.test(src), 't2 ...including their own word for the third-tap state');
ok(/g3ldtap/.test(src) && /An untested level is the strong one and every test spends it/.test(src),
   't3 the tap count reaches the face as a counter, with the doctrine in its hover');
ok(/FRESH\/TESTED\/DELIVERED is the TAP axis and is exactly our counter/.test(src),
   't4 the counter is explicitly mapped onto Skylit\'s FRESH -> TESTED -> DELIVERED lifecycle');
ok(/WEAKENING with 0 taps IS their DECAYING/.test(src),
   't5 ...and Skylit\'s DECAYING (weakening with NO interaction) survives the split');
ok(/target FRESH positioning, not used levels/.test(src),
   't6 the Academy core rule is still recorded where the taps are counted');

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


// ================= (v14.42) PHASE A1 — DARK POOL CAPTURE =================
// The payload shape is NOT yet known (the endpoint 401s on a cold re-fetch and fires on chart
// mount), so the parser is tolerant BY DESIGN and these tests pin that tolerance: every shape the
// endpoint might plausibly return must yield the same rows, and an unknown shape must FAIL LOUDLY
// into DP_STATE.err rather than quietly producing zero levels that look like "no dark pools today".
global.localStorage={ _d:{}, getItem(k){return this._d[k]||null;}, setItem(k,v){this._d[k]=v;} };
eval(ex('dpTickerFromUrl')); eval(ex('dpNum')); eval(ex('dpParse'));
ok(dpTickerFromUrl('/fs/api/dark-pool/top-prints?ticker=SPY&top_n=3&lookback_days=45')==='SPY',
   'd1 the ticker is read off the URL — the payload is not trusted to name itself');
ok(dpTickerFromUrl('/fs/api/dark-pool/top-prints?top_n=3')==null, 'd2 ...and absent when it is absent');

const ROWS=[{price:661.5,size:1200000,notional:793800000,time:'2026-07-30T18:04:00Z'},
            {price:648.2,size:900000,notional:583380000,time:'2026-08-11T15:22:00Z'}];
let P1=dpParse(ROWS);
ok(P1.ok && P1.prints.length===2 && P1.prints[0].px===661.5, 'd3 a bare array parses', P1.prints);
ok(P1.prints[0].notional===793800000 && P1.prints[0].at>0, 'd4 ...with size, notional and a parsed timestamp');
ok(dpParse({data:ROWS}).prints.length===2,       'd5 ...as does {data:[...]}');
ok(dpParse({prints:ROWS}).prints.length===2,     'd6 ...and {prints:[...]}');
ok(dpParse({top_prints:ROWS}).prints.length===2, 'd7 ...and {top_prints:[...]}');
ok(dpParse({success:true,data:{prints:ROWS}}).prints.length===2, 'd8 ...and one level of nesting');
// alternative field names
ok(dpParse([{px:'661.5',shares:'1200000',dollar_volume:'793800000'}]).prints[0].px===661.5,
   'd9 numeric strings and alternative field names both survive');
// the honest failures
ok(dpParse(null).ok===false && /empty/.test(dpParse(null).why), 'd10 an empty payload says so');
ok(dpParse({weird:1}).ok===false && /no array/.test(dpParse({weird:1}).why),
   'd11 an unrecognised shape FAILS LOUDLY — never a silent zero-level "no dark pools"');
ok(dpParse([{foo:1}]).ok===false && /no priced rows/.test(dpParse([{foo:1}]).why),
   'd12 ...and an array with no prices is refused too');
ok(dpParse([{price:0}]).prints.length===0, 'd13 a zero price is not a level');

// capture is PASSIVE — we read the page's own response, never issue our own authenticated request
ok(/url\.indexOf\('dark-pool'\)!==-1/.test(src), 'd14 both hooks watch for the dark-pool response');
ok((src.match(/indexOf\('dark-pool'\)/g)||[]).length>=2, 'd15 ...fetch AND xhr, like every other feed');
ok(!/fetch\([^)]*dark-pool/.test(src), 'd16 we NEVER issue our own dark-pool request (the page holds the auth)');
ok(/DP_KEY='gpts_darkpool_v1'/.test(src) && /localStorage\.setItem\(DP_KEY/.test(src),
   'd17 the store is PERSISTED — the endpoint fires on mount, not on a timer');
ok(/TOP N PRINTS OVER A 45-DAY/.test(src) && /THEIR DEFINITION, NOT OURS/.test(src),
   'd18 their definition is recorded verbatim; we do no clustering of our own');
ok(/CASH-EQUITY PRINTS, AND THAT IS WHY ticker=SPY/.test(src), 'd19 the SPY-not-SPX reason is written down');
// (v15.53) removed: dark-pool lifecycle archived by his decision (K-dark-pool); emPosRail with the rail
ok(/g3llvdp/.test(src), 'd21 dark pools get their own colour on the levels line');
// (v15.53) removed: dark-pool lifecycle archived by his decision (K-dark-pool); emPosRail with the rail
// (v15.53) removed: dark-pool lifecycle archived by his decision (K-dark-pool); emPosRail with the rail
ok(/__gptsDebug\.dp=/.test(src) && /raw:DP_STATE\.raw/.test(src),
   'd24 a RAW SAMPLE is kept — reading the real shape is the whole point of A1');


// ================= (v14.43) THE FIRST LIVE CAPTURE, AND WHAT IT CAUGHT =================
// Real payload, verbatim from the operator's Atlas on 2026-08-26:
//   [{"price":754.6936,"notional":1811264639.99,"size":2400000,"ts":1784147283}, ...]
const REAL=[{price:754.6936,notional:1811264639.9999998,size:2400000,ts:1784147283},
            {price:750.8278,notional:1501655600,size:2000000,ts:1784232714},
            {price:743.9288,notional:1282525811.912,size:1723990,ts:1784577746}];
let PR=dpParse(REAL);
ok(PR.ok && PR.prints.length===3, 'r1 the REAL payload parses — a bare array of {price,notional,size,ts}', PR.prints.length);
ok(PR.prints[0].px===754.6936 && PR.prints[0].size===2400000, 'r2 price and size land intact');
// ⚠ THE BUG THE FIRST CAPTURE CAUGHT: ts is in SECONDS.
ok(PR.prints[0].at===1784147283000, 'r3 ts is SECONDS and is converted to ms at the parse', PR.prints[0].at);
const ageDays=Math.round((Date.parse('2026-08-26T20:00:00Z')-PR.prints[0].at)/86400000);
ok(ageDays>30 && ageDays<46, 'r4 ...which puts the oldest print inside the 45-day lookback, not in 1970', ageDays);
ok(/THEIR ts IS IN SECONDS/.test(src), 'r5 the unit is documented at the one place that converts it');
// a millisecond timestamp must still survive, in case they ever change it
ok(dpParse([{price:660,ts:1787702400000}]).prints[0].at===1787702400000, 'r6 a ms timestamp is left alone');

// ================= (v14.43) A CLAMPED POSITION IS A FALSE POSITION =================
// emPosRail CLAMPS to 0..100. The first live capture put three dark pools 115-220 pts below the
// rail, so all three pinned at 0% and MERGED with the (also off-frame) SPY King into one stack
// reading "SPY K·DP·DP·DP 7632" — four levels named, one price shown, belonging to none of them.
// (v15.53) removed: dark-pool lifecycle archived by his decision (K-dark-pool); emPosRail with the rail
// (v15.53) removed: dark-pool lifecycle archived by his decision (K-dark-pool); emPosRail with the rail
// (v15.53) removed: dark-pool lifecycle archived by his decision (K-dark-pool); emPosRail with the rail
// (v15.53) removed: dark-pool lifecycle archived by his decision (K-dark-pool); emPosRail with the rail
// (v15.53) removed: dark-pool lifecycle archived by his decision (K-dark-pool); emPosRail with the rail


// ================= (v14.44) PHASE A2 — THE DARK-POOL LIFECYCLE =================
// (v15.53) SECTION ARCHIVED with dpLifecycle/dpLifecycleOne/dpConfluence (K-dark-pool, his decision). The verbatim
// section is in archive/v15.53/tests/test_garma_v2-dark-pool-section.js.

// ================= (v14.45) PHASE B — ROLLING STRUCTURE =================
// V2 (GM-ROLL-001/2/3) unblocks a refusal that stood since v10.51: FCHIST has SAMPLED the dominant
// floor/ceiling every closed bar for months and deliberately computed nothing, believing rolling
// was day-over-day only. Official doctrine says migration across MAP UPDATES — 2 signal, 3 confirm.
global.ROLL_BUCKET_MIN=v('ROLL_BUCKET_MIN');
global.ROLL_MIN_BUCKETS=v('ROLL_MIN_BUCKETS');
global.LEG_ROLL_SIGNAL=v('LEG_ROLL_SIGNAL');
global.LEG_ROLL_CONFIRM=v('LEG_ROLL_CONFIRM');
eval(ex('rollRun'));
ok(global.LEG_ROLL_SIGNAL===2 && global.LEG_ROLL_CONFIRM===3,
   'b1 the SAME count rule as everywhere: 2 signal, 3 confirm — no second vocabulary');
ok(rollRun([660,661,662]).count===2 && rollRun([660,661,662]).dir===1, 'b2 two migrations up = a run of 2');
ok(rollRun([660,661,662,663]).count===3, 'b3 three = confirmation territory');
ok(rollRun([660,661,660]).count===1, 'b4 a reversal ends the run');
ok(rollRun([660,660,660]).count===0, 'b5 a flat floor is not rolling');
ok(rollRun([662,661,660]).dir===-1, 'b6 ...and downward migration is signed');

// bucketing — the cadence IS the design
global.TODAY='2026-08-26';
let FCROWS=[];
const tB0=Date.parse('2026-08-26T14:00:00Z');
// a floor that wobbles one strike back and forth every 3 minutes must NOT read as rolling
for(let i=0;i<20;i++) FCROWS.push({d:'2026-08-26', t:tB0+i*180000, flr:660+(i%2), ceil:700});
global.fcHistOf=()=>FCROWS;
eval(ex('rollBucketsOf')); eval(ex('intradayRoll'));
let IW=intradayRoll('SPY');
// ⚠ a wobble CAN leave a run of 1 — doctrine calls one print noise, and the threshold is what
// rejects it. Assert the thing that matters: it never reaches SIGNAL.
ok(IW.flr.count < global.LEG_ROLL_SIGNAL,
   'b7 a floor ticking back and forth every bar never reaches signal — one print is noise', IW.flr);
ok(rollingRead0().indexOf('ROLLING')<0, 'b7b ...and the read stays silent about it');
function rollingRead0(){ try{ eval(ex('rollingRead')); global.sessionRoll=()=>({ready:false}); return rollingRead('SPY').txt||''; }catch(e){ return ''; } }
// a floor genuinely climbing across buckets does. ⚠ THE ARITHMETIC MATTERS: one migration per
// bucket, so a SIGNAL (2) needs 3 buckets and a CONFIRMATION (3) needs 4. At 15m that is 45 and 60
// minutes. The first draft used 30m buckets and this test caught it: 90 minutes before the panel
// could say anything is most of a daytrading session spent silent.
ok(global.ROLL_BUCKET_MIN===15, 'b8a the bucket is 15 minutes, so a signal is 45m old and not 90m');
FCROWS=[];
for(let i=0;i<40;i++) FCROWS.push({d:'2026-08-26', t:tB0+i*180000, flr:660+Math.floor(i/5), ceil:700});
let IC=intradayRoll('SPY');
ok(IC.buckets>=4, 'b8b 40 bars of 3m = 8 buckets of 15m', IC.buckets);
ok(IC.ready && IC.flr.dir===1 && IC.flr.count>=2, 'b8 a floor climbing across buckets IS a roll', IC.flr);
// yesterday's rows never stitch into today's intraday read
FCROWS=[{d:'2026-08-25', t:tB0-86400000, flr:640, ceil:700},{d:'2026-08-26', t:tB0, flr:660, ceil:700}];
ok(intradayRoll('SPY').buckets===1, 'b9 intraday means TODAY — yesterday is not stitched in', intradayRoll('SPY').buckets);
ok(intradayRoll('SPY').ready===false && /needs 2 buckets/.test(intradayRoll('SPY').note||''),
   'b10 one bucket is not a reading, and it says how far off it is');

// the read
global.sessionRoll=()=>({ready:true, flr:{count:3,dir:1}, ceil:{count:0,dir:0}, sessions:7});
FCROWS=[]; for(let i=0;i<40;i++) FCROWS.push({d:'2026-08-26', t:tB0+i*180000, flr:660, ceil:700-Math.floor(i/5)});
eval(ex('rollingRead'));
let RR=rollingRead('SPY');
ok(/floor is ROLLING UP 3 sessions/.test(RR.txt), 'b11 the day-over-day roll finally reaches the read', RR.txt);
ok(/ceiling is ROLLING DOWN/.test(RR.txt) && /upside compressing/.test(RR.txt), 'b12 ...alongside the intraday one');
ok(/confirmed/.test(RR.txt), 'b13 3 migrations is labelled confirmed, not merely signal');
// GM-ROLL-003 — a ceiling rolling UP is a TARGET rule, never an entry rule
global.sessionRoll=()=>({ready:true, flr:{count:0,dir:0}, ceil:{count:2,dir:1}, sessions:7});
FCROWS=[{d:'2026-08-26', t:tB0, flr:660, ceil:700}];
let RU=rollingRead('SPY');
ok(/more room above/.test(RU.txt) && /not a reason to be long/.test(RU.txt),
   'b14 GM-ROLL-003: a rising ceiling widens the TARGET and is explicitly not an entry', RU.txt);
ok(RU.bull===0, 'b15 ...and it casts no bullish vote');
ok(/GM-ROLL-003 IS A TARGET RULE, NOT AN ENTRY RULE/.test(src), 'b16 the rule is written where it is enforced');
ok(/THE CADENCE IS THE WHOLE DESIGN/.test(src), 'b17 the bucketing rationale is recorded');
ok(/rollingRead\(sym\); if\(frRoll && frRoll\.txt\) frS2\.push/.test(src),
   'b18 rolling is pushed into the S&R clause — "new support and resistance forming"');

console.log('test_garma_v2: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
