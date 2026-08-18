// (v10.54, audit 14) ONE TAP = ONE RECORD.
//
// The `act` feature decided whether to record the operator's last take/pass with:
//     fresh = (ctx.t - last.t) <= FEAT_FWD * CANDLE_MS
// FEAT_FWD*CANDLE_MS is the whole 30-minute forward window, so a SINGLE tap was
// re-recorded on all ten bars inside its own window. Selection quality — the one number
// that answers "is my hand filter adding anything?" — therefore counted one decision as
// ten, and a single lucky take could outweigh a week of real ones.
//
// The record is now written on the TAP BAR only: the first snapshot that closes after the
// tap. featStats de-duplicates on top of that, because older recorded days still contain
// the repeated records the pre-10.54 recorder wrote.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m+(g!==undefined?' -> '+g:''));} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+g:''));} };

// ---------------- mocks ----------------
let LS={};
global.localStorage={ getItem:k=>(k in LS?LS[k]:null), setItem:(k,v)=>{LS[k]=String(v);} };
global.window={ __gptsDebug:{} };
global.FEATURES=[]; global.TODAY='2026-08-18';
global.FEAT_FWD=10; global.DIR_PTS=0.5; global.DRIFT_PTS=0.5;
global.CANDLE_MS=180000;
global.RULE_UNLOCK_N=20;
global.RECORDER_KEY='gpts_rec_v1'; global.RECORDER_DAYS=4; global.RECORDER_SYMS=['SPY','QQQ'];
global.RESHUFFLE={SPY:false}; global.LASTFEED={SPY:null};
global.STATE={SPY:{price:774, candles:[], king:773, lastClosedB:1}, QQQ:{price:null,candles:[]}};
global.RULES=null; global.RULES_DOC=null; global.RULES_FETCHED=true; global.RULES_URL='';
global.RULES_KEY='gpts_rules_v1'; global.RULES2_KEY='gpts_rules_v2'; global.PROMO_KEY='gpts_promo_v1';
global.DIR_WEIGHTS={ trend:3, driftAgree:2, driftLean:1, diverge:-2, tentative:1, gradeA:5, gradeB:3 };
global.FEAT_KEEP_BARS=160;
global.ruleTier=function(){ return '⚖'; };
global.directionGrade=function(){ return {grade:'B',dir:'UP',score:3,tier:'⚖',capped:null,inputs:{drift:{verdict:'AGREE-UP'},structAsym:{bias:'support-heavy'},rangePos:{zone:'lower'},regime:{tag:'trend'},session:{bucket:'morning',opex:false}}}; };
global.driftRead=function(){ return {verdict:'AGREE-UP',dir:1,overlap:true,px:774}; };
global.nodeMapModel=function(){ return {ok:true,px:774,kingK:775,levels:[{k:773,isFlr:true,pos:true,taps:0,state:'Building',chg:12}]}; };
global.inPlayZone=function(){ return {k:773,isFlr:true,pos:true,taps:0,state:'Building',chg:12}; };
global.nodeGrade=function(){ return {grade:'A',score:5,tier:'⚖',inputs:{pol:'+',tap:0,rocNow:'Building',rocDay:{pct:20,label:'Acm'},conf:{q:true,v:true,holdDir:1}}}; };
global.decisionCell=function(){ return {cell:'B×A',text:'bounce play',dirGrade:'B',nodeGrade:'A',tier:'⚖',k:773}; };
global.tradeFrame=function(sym,L,dir){ return {zone:[772.75,773.25], inval:772, tgt:776, path:'wall', k:L?L.k:null, dir:dir||1}; };
global.accumCanon=function(){ return {m15:{pct:10,label:'Acm'},session:{pct:20,label:'Acm'}}; };
global.acmLabel=function(){ return 'Acm'; };
global.deflAnticipation=function(){ return {fired:true,grade:'A',dist:0.3}; };
global.reactionQuality=function(){ return {q:'confirmed',why:'wick'}; };
global.zoneMeaningful=function(){ return true; };
global.zoneRole=function(){ return 'Flr'; };
global.kingRoll=function(){ return 1; };
global.gatekeeper=function(){ return {ok:true,k:774}; };
global.trendVerdict=function(){ return {state:'up',up:16,dn:1,win:20,slope:0.4}; };
global.trendFast=function(){ return {p10:{state:'up',up:8,dn:1,win:10,slope:0.2}, p20:{state:'up',up:15,dn:2,win:20,slope:0.3}}; };
global.netPositioning=function(){ return {bias:'support-heavy',dir:1,ratio:2,decisive:true}; };
global.deriveFactors=function(){ return {ns:1,nm:1,reg:'pos',zg:770,imb:1.2}; };
global.futureStructureSummary=function(){ return {above:[],below:[]}; };
global.recNode=function(x){ return x; };
global.regimeTag=function(){ return {tag:'trend',er:0.6}; };
global.closedCandles=function(){ return []; };
global.spineOf=function(){ return {dir:directionGrade(), inPlay:inPlayZone(), node:nodeGrade(), decision:decisionCell()}; };
global.render=function(){};
global.ctNow=function(){ return new Date(2026,7,18,11,0); };
global.ctMinutesSinceMidnight=function(){ return 10*60+30; };
global.ctTodayStr=function(){ return '2026-08-18'; };
global.RULES_APPLIED_AT=null;
global.PAL={sub:'#8b98a9'};

// ---- the act log this test drives ----
var ACTS=[];
global.actToday=function(){ return ACTS; };

eval([ 'ruleSeed','rulesSeed','rulesNormalize','rulesLoad','rulesSave','ruleGet','effN','nTxt',
       'registerFeature','featureByKey','_fwdHitDir','_fwdHitNum','_trend5Vote','frameRR','_frameRecOf',
       'weightsHash','modelStamp','registerCoreFeatures','seedQuestions','isOpexDay','sessionBucket',
       'eventTagNow','featRegime','featureCtx','featRecordAll','featEnqueue',
       'recorderLoad','recorderSave','recorderDay','_fpct','featStats','nodeHoldDir' ].map(ex).join('\n'));

registerCoreFeatures();
const ACT=featureByKey('act');

// ================= 1. THE TAP BAR RECORDS IT ================================
const T0=Date.parse('2026-08-18T16:00:00Z');
ACTS=[{ t:T0, k:773, cell:'B×A', dirGrade:'B', nodeGrade:'A', action:'take' }];
var r0=ACT.record('SPY', { t:T0+1000, px:774, bar:1, n:1, spine:spineOf() });
ok(r0.action==='take', '1a the first snapshot after the tap RECORDS it', r0.action);
ok(r0.tapT===T0, '1b ...stamped with the tap time, which is what de-duplicates it later', r0.tapT);
ok(r0.k===773 && r0.cell==='B×A', '1c ...with the strike and cell the tap was made on');
ok(r0.tgt===776 && r0.inval===772, '1d ...and the frame the card was showing, so the tap is scoreable on it');

// ================= 2. EVERY LATER BAR DOES NOT ==============================
// bars 2..10 of the tap's own forward window: under v10.53 EVERY one of these recorded
// the same take again.
var reRecorded=0;
for(var b=1;b<=10;b++){
  var rn=ACT.record('SPY', { t:T0 + b*CANDLE_MS + 1000, px:774, bar:1+b, n:1+b, spine:spineOf() });
  if(rn.action) reRecorded++;
}
ok(reRecorded===0, '2a NONE of the next 10 bars re-records the same tap (v10.53 recorded all 10)', reRecorded);
ok(ACT.record('SPY',{ t:T0+CANDLE_MS, px:774, bar:2, n:2, spine:spineOf() }).action===null,
   '2b exactly one candle later it is already outside the tap bar');
ok(ACT.record('SPY',{ t:T0+CANDLE_MS-1, px:774, bar:1, n:1, spine:spineOf() }).action==='take',
   '2c ...while anything strictly inside the candle is still the tap bar');
ok(ACT.record('SPY',{ t:T0-5000, px:774, bar:0, n:0, spine:spineOf() }).action===null,
   '2d a snapshot BEFORE the tap never records it (no backdating)');
// no tap at all
ACTS=[];
ok(ACT.record('SPY',{ t:T0, px:774, bar:1, n:1, spine:spineOf() }).action===null,
   '2e with no taps logged the record is honestly empty, not a fabricated "pass"');

// ================= 3. THE SOURCE SAYS WHY ===================================
var FEAT=ex('registerCoreFeatures');
ok(/onTapBar = !!\(last && ctx && ctx\.t && \(ctx\.t-last\.t\)>=0 && \(ctx\.t-last\.t\)<CANDLE_MS\)/.test(FEAT),
   '3a the window is ONE CANDLE, not the whole forward window');
ok(!/fresh = !!\(last && ctx && ctx\.t && \(ctx\.t-last\.t\)<=\(FEAT_FWD\*CANDLE_MS\)\)/.test(FEAT),
   '3b the FEAT_FWD*CANDLE_MS window is gone');
ok(/tapT:onTapBar\?last\.t:null/.test(FEAT), '3c the tap time is carried on the record');

// ================= 4. featStats DE-DUPLICATES OLD DAYS TOO ==================
// Days recorded before v10.54 still hold ten copies of one tap. One tap must count once.
LS={};
var db=recorderLoad(); var day=recorderDay(db);
day.feat={ SPY:[] };
for(var i=0;i<10;i++){
  day.feat.SPY.push({ key:'act', t:T0+i*1000, bar:i, n:i, px:774, session:'morning',
    rec:{ action:'take', k:773, cell:'B×A', tapT:T0 }, hit:1, mfe:0.9, mae:-0.1, resolved:true });
}
day.feat.SPY.push({ key:'act', t:T0+99999, bar:40, n:40, px:774, session:'morning',
  rec:{ action:'pass', k:775, cell:'C×B', tapT:T0+99999 }, hit:1, mfe:0.1, mae:-0.05, resolved:true });
recorderSave(db);
var st=featStats('SPY');
ok(st.act.take.n===1, '4a ten legacy copies of ONE tap count as ONE take', st.act.take.n);
ok(st.act.take.hit===1, '4b ...and its outcome is counted once, not ten times', st.act.take.hit);
ok(st.act.pass.n===1, '4c a genuinely separate tap still counts', st.act.pass.n);
ok(st.act.take.mn===1, '4d MFE/MAE are averaged over taps, not over duplicated records', st.act.take.mn);
// legacy records with NO tapT fall back to a per-day / action / strike / cell key
LS={};
var db2=recorderLoad(); var d2=recorderDay(db2); d2.feat={ SPY:[] };
for(var j=0;j<10;j++){
  d2.feat.SPY.push({ key:'act', t:T0+j*1000, bar:j, n:j, px:774, session:'morning',
    rec:{ action:'take', k:773, cell:'B×A' }, hit:1, mfe:0.5, mae:0, resolved:true });
}
recorderSave(db2);
ok(featStats('SPY').act.take.n===1, '4e pre-10.54 records with no tapT are de-duplicated by day/action/strike/cell', featStats('SPY').act.take.n);

// ================= 5. THE UI COUNTS TAPS, NOT BAR-RECORDS ===================
var YC=ex('yourCallsHtml');
ok(/n='\+o\.n\+' taps/.test(YC), '5a "Your calls" labels its n as TAPS — an act record is already one independent observation');
ok(!/effN\(/.test(YC), '5b ...so it is NOT divided by the forward window, which would be wrong here');
ok(/not enough taps yet/.test(YC), '5c under 5 of each it says so instead of showing a gap it cannot support');
ok(/control group/.test(YC), '5d ...and the hover states why passes matter');

console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
