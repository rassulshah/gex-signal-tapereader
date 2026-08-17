// (v10.49 C) DIRECTION GRADE — the ONE direction verdict per bar. Fused from four
// inputs (drift · structure asymmetry · range position · regime), with two HARD CAPS
// that no amount of good input can override: mid-range and chop both force C.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m+(g!==undefined?' -> '+g:''));} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+g:''));} };

// ---- mocks: each spine input is injectable so we can test the fusion in isolation ----
global.STATE={SPY:{price:774.0, candles:[], lastClosedB:1}, QQQ:{price:null, candles:[]}};
var DRIFT={verdict:'AGREE-UP', dir:1, gvwap:773.9, vvwap:775.0};
var NET={bias:'support-heavy', dir:1, ratio:2.0, decisive:true};
var RG={tag:'trend', er:0.6};
var MODEL={ok:true, px:774.0, flr:{k:772}, ceil:{k:778}, levels:[], kingK:775};
var MINS=10*60+30;                                  // 10:30 CT = 'morning' (odds not capped)
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
ok(gradeDisp('A',5)==='A−' && gradeDisp('A',6)==='A', '2d exactly-at-threshold renders A−');

// ================= 3. the A case: everything aligned, price out of the middle =========
global.STATE.SPY.price=772.6;                       // lower third of 772..778
var d=directionGrade('SPY');
ok(d.dir==='UP',   '3a direction UP (drift up + support-heavy)', d.dir);
ok(d.score===5,    '3b score 5 = drift agree 2 + struct aligned 2 + trend 1', d.score);
ok(d.grade==='A',  '3c grade A', d.grade);
ok(d.disp==='A−',  '3d displayed as A− (reached exactly at the threshold)', d.disp);
ok(d.capped==null, '3e nothing capped it');
ok(d.noOdds===false, '3f odds permitted in the morning bucket');
ok(d.inputs.drift.verdict==='AGREE-UP' && d.inputs.structAsym.bias==='support-heavy' &&
   d.inputs.rangePos.zone==='lower' && d.inputs.regime.tag==='trend', '3g all four inputs recorded');
ok(d.inputs.session.bucket==='morning', '3h session bucket recorded on the grade', d.inputs.session.bucket);

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

// ================= 6. the B case and the C case =================
DRIFT={verdict:'SPLIT', dir:0};                     // drift no longer agrees
var b=directionGrade('SPY');
ok(b.score===3 && b.grade==='B', '6a split drift + aligned structure + trend = B', b.score+'/'+b.grade);
ok(b.dir==='UP', '6b structure alone still names the side', b.dir);
DRIFT={verdict:'AGREE-DN', dir:-1};                 // drift OPPOSES the structure
var opp=directionGrade('SPY');
ok(opp.dir==='DN', '6c drift is weighted 2× structure, so it wins the side', opp.dir);
ok(opp.score===2, '6d drift agree 2 + struct opposed -1 + trend 1 = 2', opp.score);
ok(opp.grade==='C', '6d2 ...which is only a C: a fight between drift and structure is not a read', opp.grade);
DRIFT={verdict:'NONE', dir:0};
NET={bias:'balanced', dir:0, ratio:1, decisive:false};
RG={tag:'mixed', er:0.35};
var c=directionGrade('SPY');
ok(c.score===1 && c.grade==='C', '6e nothing agreeing -> C', c.score+'/'+c.grade);
ok(c.dir==='SIDE', '6f ...and no side is claimed', c.dir);
ok(c.inputs.drift.verdict==='NONE', '6g a missing VEX book scores +1 (nothing contradicts), not +2');

// ================= 7. an A needs a side =================
DRIFT={verdict:'NONE', dir:0}; NET={bias:'balanced',dir:0,ratio:1,decisive:false}; RG={tag:'trend',er:0.7};
var sideless=directionGrade('SPY');
ok(sideless.dir==='SIDE' && sideless.grade!=='A', '7a a SIDE read can never be graded A', sideless.dir+'/'+sideless.grade);

// ================= 8. session buckets + power-hour odds cap =================
DRIFT={verdict:'AGREE-UP',dir:1}; NET={bias:'support-heavy',dir:1,ratio:2,decisive:true};
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
global.STATE.SPY.price=null;
ok(directionGrade('SPY').grade!=null, '9b no price -> still returns a grade object');

// ================= 10. source-level guards =================
ok(/HARD-CAP|HARD-CAPS/.test(src), '10a the mid-range cap is documented in source');
ok(/DECISION_MATRIX/.test(src),    '10b the matrix exists');
ok(!/directionGrade[^]{0,3000}(buy|sell|long side entry)/i.test(src.slice(src.indexOf('function directionGrade'), src.indexOf('function directionGrade')+2000)), '10c no trade words inside directionGrade');

console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
