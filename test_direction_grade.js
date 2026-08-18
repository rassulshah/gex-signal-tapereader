// (v10.51) DIRECTION GRADE — the ONE direction verdict per bar, now HIERARCHICAL:
// the SMA-50 five-state IS the direction, GEX/VEX drift only confirms or diverges, and
// with no confirmed trend the read is TENTATIVE (capped at C). The v10.50 weighted-lean
// assertions are SUPERSEDED and rewritten here; the two hard caps (mid-range, chop) are
// unchanged and still override everything.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m+(g!==undefined?' -> '+g:''));} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+g:''));} };

// ---- mocks: each spine input is injectable so we can test the hierarchy in isolation ----
// (v10.57) drift is in SHADOW mode live (DRIFT_LIVE=false). These pins exercise the hierarchy AS IT RUNS
// WHEN DRIFT IS PROMOTED, so the flag is forced on here; test_drift_shadow.js pins the live shadow behaviour.
global.DRIFT_LIVE=true;
global.STATE={SPY:{price:774.0, candles:[], lastClosedB:1}, QQQ:{price:null, candles:[]}};
var TV={state:'up', up:16, dn:1, win:20, slope:0.42};
var DRIFT={verdict:'AGREE-UP', dir:1, gvwap:773.9, vvwap:775.0, overlap:true};
var NET={bias:'support-heavy', dir:1, ratio:2.0, decisive:true};
var RG={tag:'trend', er:0.6};
var MODEL={ok:true, px:774.0, flr:{k:772}, ceil:{k:778}, levels:[], kingK:775};
var MINS=10*60+30;                                  // 10:30 CT = 'morning' (odds not capped)
global.trendVerdict=function(){ return TV; };
global.driftRead=function(){ return DRIFT; };
global.netPositioning=function(){ return NET; };
global.regimeTag=function(){ return RG; };
global.nodeMapModel=function(){ return MODEL; };
global.closedCandles=function(){ return []; };
global.ctMinutesSinceMidnight=function(){ return MINS; };
global.ctNow=function(){ return new Date(2026,7,17,11,0); };   // a Monday, not OPEX
global.ruleTier=function(){ return '⚖'; };
global.PAL={sub:'#8b98a9'};

eval(['rangePosOf','gradeOfScore','gradeDisp','isOpexDay','sessionBucket','directionGrade'].map(ex).join('\n'));

// ================= 1. rangePosOf =================
ok(rangePosOf({flr:{k:772},ceil:{k:778}}, 772.5).zone==='lower', '1a near the floor -> lower');
ok(rangePosOf({flr:{k:772},ceil:{k:778}}, 777.5).zone==='upper', '1b near the ceiling -> upper');
ok(rangePosOf({flr:{k:772},ceil:{k:778}}, 775.0).zone==='mid',   '1c halfway -> mid');
ok(rangePosOf({flr:{k:772},ceil:{k:778}}, 774.1).zone==='mid',   '1d 0.35 boundary is inside mid', rangePosOf({flr:{k:772},ceil:{k:778}},774.1).pos);
ok(rangePosOf({flr:{k:772},ceil:{k:778}}, 773.9).zone==='lower', '1e just under 0.35 -> lower');
ok(rangePosOf({flr:{k:772},ceil:{k:778}}, 776.0).zone==='upper', '1f just over 0.65 -> upper');
ok(rangePosOf(null, 774).zone==='unknown',                       '1g no model -> unknown');
ok(rangePosOf({flr:{k:775},ceil:{k:775}}, 775).zone==='unknown', '1h degenerate range -> unknown');

// ================= 2. gradeOfScore thresholds =================
ok(gradeOfScore(6)==='A' && gradeOfScore(5)==='A', '2a A at >=5');
ok(gradeOfScore(4)==='B' && gradeOfScore(3)==='B', '2b B at >=3');
ok(gradeOfScore(2)==='C' && gradeOfScore(-1)==='C','2c C below 3');
// (v10.54, audit 25) THE HALF-GRADE IS GONE. gradeDisp hardcoded the cut points as 5
// and 3, so once a promoted threshold moved gradeA the "A−" landed on the wrong score
// entirely. A score AT the threshold is the grade: A means A.
ok(gradeDisp('A',5)==='A' && gradeDisp('A',6)==='A', '2d a score AT the A threshold renders A, not A−');
ok(gradeDisp('B',3)==='B' && gradeDisp('C',0)==='C', '2d2 ...and the same for B and C');

// ================= 3. the A case: confirmed trend CONFIRMED by an overlapping drift ====
global.STATE.SPY.price=772.6;                       // lower third of 772..778
var d=directionGrade('SPY');
ok(d.dir==='UP',        '3a direction UP — set by the CONFIRMED UPTREND, not by drift', d.dir);
ok(d.score===5,         '3b score 5 = confirmed trend 3 + drift agrees with overlap 2', d.score);
ok(d.grade==='A',       '3c grade A', d.grade);
ok(d.disp==='A',        '3d displayed as A (v10.54: no half-grade at the threshold)', d.disp);
ok(d.relation==='confirmed', '3e relation = confirmed', d.relation);
ok(d.tentative===false, '3f a confirmed trend is never tentative');
ok(d.trendState==='up', '3g the five-state is reported verbatim', d.trendState);
ok(d.capped==null,      '3h nothing capped it');
ok(d.noOdds===false,    '3i odds permitted in the morning bucket');
ok(d.inputs.trend.state==='up' && d.inputs.trend.up===16 && d.inputs.trend.dn===1 &&
   d.inputs.trend.win===20, '3j the trend inputs are recorded {state,up,dn,win,slope}');
ok(d.inputs.drift.verdict==='AGREE-UP' && d.inputs.drift.dir===1, '3k the drift inputs are recorded');
ok(d.inputs.structAsym.bias==='support-heavy' && d.inputs.rangePos.zone==='lower' &&
   d.inputs.regime.tag==='trend', '3l structure/range/regime still recorded (structure no longer VOTES)');
ok(d.inputs.session.bucket==='morning', '3m session bucket recorded on the grade', d.inputs.session.bucket);
ok(d.dirWeightsSource==='hand-set', '3n the model source is declared hand-set', d.dirWeightsSource);

// ================= 4. MID-RANGE HARD CAP (the Skylit midpoint rule) ==================
global.STATE.SPY.price=775.0;                       // dead centre of 772..778
var mid=directionGrade('SPY');
ok(mid.score===5,        '4a the raw score is still 5', mid.score);
ok(mid.grade==='C',      '4b ...but mid-range HARD-CAPS the grade to C', mid.grade);
ok(mid.capped==='mid-range', '4c the cap is named', mid.capped);
ok(mid.noOdds===true,    '4d and no odds may be quoted mid-range');
ok(mid.dir==='UP',       '4e the direction itself is unchanged (only the grade is capped)', mid.dir);

// ================= 5. CHOP HARD CAP =================
global.STATE.SPY.price=772.6; RG={tag:'chop', er:0.18};
var ch=directionGrade('SPY');
ok(ch.grade==='C',   '5a chop HARD-CAPS to C', ch.grade);
ok(ch.dir==='SIDE',  '5b chop also forces the direction to SIDE', ch.dir);
ok(ch.noOdds===true, '5c chop drops the odds sentence');
ok(/chop/.test(ch.capped||''), '5d the cap is named', ch.capped);
RG={tag:'trend', er:0.6};

// ================= 6. the B cases and the C cases (SUPERSEDES the v10.50 lean math) ====
// 6.1 trend-only: drift SPLIT has nothing to confirm with — a confirmed trend alone is a B.
DRIFT={verdict:'SPLIT', dir:0};
var b=directionGrade('SPY');
ok(b.score===3 && b.grade==='B', '6a confirmed trend + SPLIT drift = trend-only, score 3 = B', b.score+'/'+b.grade);
ok(b.dir==='UP',                 '6b the trend still names the side with no drift help', b.dir);
ok(b.relation==='trend-only',    '6c relation = trend-only', b.relation);
// 6.2 LEAN (same side, bands do not overlap) is worth one point, not two.
DRIFT={verdict:'LEAN-UP', dir:1, overlap:false};
var lean=directionGrade('SPY');
ok(lean.score===4 && lean.grade==='B', '6d agreement without band overlap = +1, not +2', lean.score+'/'+lean.grade);
ok(lean.relation==='confirmed',        '6e still a confirmed relation', lean.relation);
// 6.3 DIVERGENCE: drift opposes the trend. The trend KEEPS the direction; the grade dies.
DRIFT={verdict:'AGREE-DN', dir:-1, overlap:true};
var opp=directionGrade('SPY');
ok(opp.dir==='UP',              '6f drift NEVER flips a confirmed trend — direction stays UP', opp.dir);
ok(opp.relation==='divergence', '6g relation = divergence', opp.relation);
ok(opp.score===1,               '6h 3 − 2 = 1', opp.score);
ok(opp.grade==='C',             '6i a diverging trend can never grade above C', opp.grade);
// 6.4 TENTATIVE: no confirmed trend at all.
TV={state:'flat', up:4, dn:5, win:20, slope:0.01};
DRIFT={verdict:'AGREE-UP', dir:1, overlap:true};
var tent=directionGrade('SPY');
ok(tent.dir==='UP',            '6j with no trend, drift supplies the provisional lean', tent.dir);
ok(tent.relation==='tentative' && tent.tentative===true, '6k relation = tentative', tent.relation);
ok(tent.score===1 && tent.grade==='C', '6l tentative is score 1 and hard-capped at C', tent.score+'/'+tent.grade);
DRIFT={verdict:'NONE', dir:0};
var none=directionGrade('SPY');
ok(none.dir==='SIDE' && none.grade==='C', '6m no trend AND no drift -> SIDE / C', none.dir+'/'+none.grade);
TV={state:'up', up:16, dn:1, win:20, slope:0.42};
DRIFT={verdict:'AGREE-UP', dir:1, overlap:true};

// ================= 7. an A needs a side =================
TV={state:'flat', up:3, dn:3, win:20, slope:0}; DRIFT={verdict:'NONE', dir:0};
var sideless=directionGrade('SPY');
ok(sideless.dir==='SIDE' && sideless.grade!=='A', '7a a SIDE read can never be graded A', sideless.dir+'/'+sideless.grade);
TV={state:'up', up:16, dn:1, win:20, slope:0.42}; DRIFT={verdict:'AGREE-UP', dir:1, overlap:true};

// ================= 8. session buckets + power-hour odds cap =================
MINS=8*60+0;   ok(sessionBucket().bucket==='pre-open',   '8a 08:00 CT -> pre-open');
MINS=9*60+0;   ok(sessionBucket().bucket==='open-drive', '8b 09:00 CT -> open-drive');
MINS=10*60+30; ok(sessionBucket().bucket==='morning',    '8c 10:30 CT -> morning');
MINS=12*60+0;  ok(sessionBucket().bucket==='midday',     '8d 12:00 CT -> midday');
MINS=14*60+0;  ok(sessionBucket().bucket==='afternoon',  '8e 14:00 CT -> afternoon');
MINS=14*60+45; ok(sessionBucket().bucket==='power',      '8f 14:45 CT -> power');
ok(sessionBucket().capOdds===true, '8g power hour caps odds');
var pw=directionGrade('SPY');
ok(pw.noOdds===true, '8h ...and directionGrade carries that through', pw.noOdds);
MINS=10*60+30;
ok(isOpexDay(new Date(2026,7,21))===true,  '8i 2026-08-21 is the 3rd Friday -> OPEX');
ok(isOpexDay(new Date(2026,7,14))===false, '8j 2026-08-14 is the 2nd Friday -> not OPEX');
ok(isOpexDay(new Date(2026,7,20))===false, '8k a Thursday is never OPEX');

// ================= 9. defensive =================
global.nodeMapModel=function(){ throw new Error('boom'); };
var safe=directionGrade('SPY');
ok(safe && safe.grade, '9a a throwing input never breaks the grade', safe.grade);
global.nodeMapModel=function(){ return MODEL; };
global.trendVerdict=function(){ throw new Error('boom'); };
var safe2=directionGrade('SPY');
ok(safe2.trendState==='na' && safe2.relation==='tentative', '9b a throwing trend degrades to tentative, never to a false trend', safe2.relation);
global.trendVerdict=function(){ return TV; };
global.STATE.SPY.price=null;
ok(directionGrade('SPY').grade!=null, '9c no price -> still returns a grade object');
global.STATE.SPY.price=772.6;

// ================= 10. source-level guards =================
ok(/HARD-CAP|HARD-CAPS/.test(src), '10a the mid-range cap is documented in source');
ok(/DECISION_MATRIX/.test(src),    '10b the matrix exists');
ok(!/directionGrade[^]{0,3000}(buy|sell|long side entry)/i.test(src.slice(src.indexOf('function directionGrade'), src.indexOf('function directionGrade')+2000)), '10c no trade words inside directionGrade');
ok(/drift NEVER overrides the direction/i.test(src), '10d the hierarchy rule is stated in source');
ok(/@version\s+11\.1\.1/.test(src), '10e version pinned to 10.56');

console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
