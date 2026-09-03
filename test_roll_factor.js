// (v10.55 PART B/D) THE ROLL FACTOR — inside the hierarchy, and STRICTLY on the score.
//
// The user's rule, and the one thing this file exists to enforce: THE SMA OWNS THE
// DIRECTION. A confirmed roll of pullback nodes in the trend's own direction is the
// Academy's strongest presumptive evidence, so it is worth +1 of CONFIDENCE; a pullback
// node that rolls the other way is worth −1 and is called weakening. Neither may ever
// change `dir` — not by one bar, not on any input combination.
//
// Also pins the multi-session half (PART D): rolling floors/ceilings out of FCHIST only
// VOTE once three sessions exist; before that they are recorded and the Testing tab says
// how far off the vote is.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m+(g!==undefined?' -> '+g:''));} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+g:''));} };

// ---------------- mocks (the same shape test_dir_hierarchy uses) ----------------
// (v10.57) drift is in SHADOW mode live (DRIFT_LIVE=false). These pins exercise the hierarchy AS IT RUNS
// WHEN DRIFT IS PROMOTED, so the flag is forced on here; test_drift_shadow.js pins the live shadow behaviour.
global.DRIFT_LIVE=true;
global.STATE={SPY:{price:772.6, candles:[], lastClosedB:1}, QQQ:{price:null, candles:[]}};
var TV={state:'dn', up:1, dn:16, win:20, slope:-0.4};
var DRIFT={verdict:'SPLIT', dir:0, gvwap:773.9, vvwap:775.0, overlap:false};
var NET={bias:'balanced', dir:0, ratio:1, decisive:false};
var RG={tag:'trend', er:0.6};
var MODEL={ok:true, px:772.6, flr:{k:770}, ceil:{k:776}, levels:[], kingK:773};
var LEG=null, SESS=null;
global.trendVerdict=function(){ return TV; };
global.driftRead=function(){ return DRIFT; };
global.netPositioning=function(){ return NET; };
global.regimeTag=function(){ return RG; };
global.nodeMapModel=function(){ return MODEL; };
global.closedCandles=function(){ return []; };
global.ctMinutesSinceMidnight=function(){ return 10*60+30; };
global.ctNow=function(){ return new Date(2026,7,17,11,0); };
global.ruleTier=function(){ return '⚖'; };
global.legEngine=function(){ return LEG; };
global.sessionRoll=function(){ return SESS; };
global.PAL={sub:'#8b98a9'};
global.fmtNum=function(x){ return (Math.round(x*100)/100).toString(); };
global.fmtLvl=function(x){ return global.fmtNum(x); };
global.LEG_ROLL_SIGNAL=2; global.LEG_ROLL_CONFIRM=3;

eval(['rangePosOf','gradeOfScore','gradeDisp','isOpexDay','sessionBucket','directionGrade',
      'rollRun','sessionRollSeries'].map(ex).join('\n'));

function leg(dir, count, opts){
  opts=opts||{};
  return { dir:dir, phase:opts.phase||'PB', magnet:{k:773}, lastPB:{k:775},
           roll:{ side:dir==='dn'?'ceil':'flr', steps:[], count:count,
                  signal:count>=2, confirmed:count>=3, weakening:!!opts.weakening },
           pbDetected:{k:775}, invalidations:{trendBreak:false,pbBreak:false} };
}
function G(){ return directionGrade('SPY'); }

// ================= 1. A CONFIRMED ROLL WITH THE TREND IS +1 ========================
LEG=null; SESS=null;
var base=G();
ok(base.dir==='DN' && base.score===3, '1a baseline: a confirmed downtrend alone, drift split', base.score);
LEG=leg('dn',3);
var withRoll=G();
ok(withRoll.score===base.score+1, '1b a CONFIRMED roll in the trend direction adds exactly +1', withRoll.score);
ok(withRoll.inputs.roll.vote===1 && withRoll.inputs.roll.confirmed===true,
   '1c ...and the vote is recorded as such', withRoll.inputs.roll.vote);
LEG=leg('dn',2);
ok(G().score===base.score, '1d a roll at SIGNAL strength (2 steps) does NOT vote yet — only confirmation does', G().score);
LEG=leg('dn',1);
ok(G().score===base.score, '1e a single pullback node is not a roll at all', G().score);

// ================= 2. A ROLL AGAINST THE TREND IS −1, AND SAYS WEAKENING ==========
LEG=leg('dn',1,{weakening:true});
var weak=G();
ok(weak.score===base.score-1, '2a a pullback node that rolled AGAINST the trend costs a point', weak.score);
ok(weak.weakening===true && weak.inputs.roll.weakening===true, '2b ...and the read is flagged weakening', weak.weakening);
ok(weak.dir==='DN', '2c ...without changing the direction — the SMA still owns it', weak.dir);
// (v15.53) removed: readBlock44 archived (A-dead-else); the hover word lives in readState now

// ================= 3. THE ROLL MAY NEVER CHANGE THE DIRECTION =====================
// Every combination: trend up/down, roll up/down, confirmed or weakening. `dir` is the
// SMA's answer in all of them.
['up','dn'].forEach(function(t){
  TV = (t==='up') ? {state:'up',up:16,dn:1,win:20,slope:0.4} : {state:'dn',up:1,dn:16,win:20,slope:-0.4};
  var want=(t==='up')?'UP':'DN';
  ['up','dn'].forEach(function(l){
    [1,2,3].forEach(function(c){
      [false,true].forEach(function(w){
        LEG=leg(l,c,{weakening:w});
        ok(G().dir===want, '3·trend '+t+' · roll '+l+' ×'+c+(w?' weakening':'')+' -> dir stays '+want, G().dir);
      });
    });
  });
});
TV={state:'dn', up:1, dn:16, win:20, slope:-0.4};
// and with NO trend the roll cannot supply one either
TV={state:'flat', up:4, dn:4, win:20, slope:0};
LEG=leg('dn',3);
var flat=G();
ok(flat.dir==='SIDE', '3z with no confirmed trend a confirmed roll does NOT invent a direction', flat.dir);
ok(flat.grade==='C', '3z2 ...and the tentative cap still holds', flat.grade);
TV={state:'dn', up:1, dn:16, win:20, slope:-0.4};

// ================= 4. THE MULTI-SESSION HALF (PART D) =============================
LEG=null;
SESS={ sessions:2, ready:false, vote:-1, flr:{count:0,dir:0}, ceil:{count:2,dir:-1}, note:'needs 3 sessions, have 2' };
ok(G().score===base.score, '4a with only 2 recorded sessions the day-over-day roll does NOT vote', G().score);
ok(G().inputs.roll.session.ready===false && /needs 3 sessions, have 2/.test(G().inputs.roll.session.note),
   '4b ...and the record says exactly what it is waiting on', G().inputs.roll.session.note);
SESS={ sessions:4, ready:true, vote:-1, flr:{count:0,dir:0}, ceil:{count:3,dir:-1}, note:null };
ok(G().score===base.score+1, '4c with 3+ sessions a ceiling rolling DOWN in a downtrend votes +1', G().score);
SESS={ sessions:4, ready:true, vote:1, flr:{count:3,dir:1}, ceil:{count:0,dir:0}, note:null };
ok(G().score===base.score-1, '4d ...and a floor rolling UP against that downtrend costs one', G().score);
ok(G().dir==='DN', '4e ...still without touching the direction', G().dir);

// the run counter itself
ok(rollRun([770,771,772]).count===2 && rollRun([770,771,772]).dir===1,
   '4f rollRun counts CONSECUTIVE same-direction migrations (3 sessions = 2 migrations)', rollRun([770,771,772]).count);
ok(rollRun([776,775.5,775]).dir===-1, '4g ...and signs them (ceilings rolling down = bearish)');
ok(rollRun([770,771,770.5]).count===1, '4h a reversal breaks the run', rollRun([770,771,770.5]).count);
ok(rollRun([771,771,771]).count===0, '4i standing still is not a migration');
ok(rollRun([772]).count===0 && rollRun([]).count===0, '4j one session (or none) can never be a roll');

// ================= 5. DOCTRINE: 2 = SIGNAL, 3 = CONFIRMATION ======================
ok(/LEG_ROLL_SIGNAL=2/.test(src) && /LEG_ROLL_CONFIRM=3/.test(src),
   '5a the Academy count rule is the constant, not a magic number in a branch');
var SR=ex('sessionRoll');
ok(/out\.ready=\(sess\.length>=LEG_ROLL_CONFIRM\)/.test(SR), '5b the vote gate is 3 recorded sessions');
ok(/floors rolling UP = bullish/.test(SR), '5c floors up bullish / ceilings down bearish, per the doctrine');
ok(/needs '\+LEG_ROLL_CONFIRM\+' sessions, have '/.test(SR), '5d the honest "have N" note is generated, not hardcoded');
ok(/multi-session roll vote/.test(ex('unlockRowsHtml')), '5e ...and it shows up in the Testing tab coverage');

// ================= 6. THE FACTOR IS RECORDED, NOT JUST USED =======================
ok(/key:'leg\.roll'/.test(src), '6a leg.roll is an enrolled FEATURE with its own outcome');
ok(/key:'leg\.roll'/.test(ex('dirFactorGroups')), '6b ...and has a vote-split row in the Direction-factors table');
ok(/order:\['confirmed','signal','weakening','none'\]/.test(ex('dirFactorGroups')),
   '6c ...split by confirmed / signal / weakening so the doctrine can be falsified per state');
ok(/session:\{ sessions:sr\?sr\.sessions:0/.test(src) && /session:\{ sessions:sessR\?sessR\.sessions:0/.test(ex('directionGrade')),
   '6d the multi-session read rides on the direction record too');

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
