// (v10.53 C / v10.54 audit 10) REGIME TAG ON EVERY OUTCOME — AND THE 1-WAY FLAG THAT
// STOPS A ONE-DIRECTIONAL FACTOR FROM MASQUERADING AS EDGE.
//
// A rule that works in trend and fails in chop averages to "meh" and nobody can see why.
// So EVERY FEATURES record now carries regime:{tag, opex, event}, written in one place
// (featRecordAll) so a newly enrolled feature gets it for free — no per-feature wiring
// to forget. This pins: the tag comes from regimeTag(), opex from the session bucket,
// event from an actual event-tag state (never a guess), the object survives into the
// recorder queue, and the Analysis factor table splits its rows by it.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m+(g!==undefined?' -> '+g:''));} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+g:''));} };

// ---------------- mocks (same shape as test_feature_enrollment) ----------------
var LS={};
global.localStorage={ getItem:function(k){ return (k in LS)?LS[k]:null; }, setItem:function(k,v){ LS[k]=String(v); } };
global.window={ __gptsDebug:{} };
global.TODAY='2026-08-21';
global.FEATURES=[];
global.RULES=null; global.RULES_DOC=null; global.RULES_FETCHED=true;
global.RULES_KEY='gpts_rules_v1'; global.RULES2_KEY='gpts_rules_v2'; global.PROMO_KEY='gpts_promo_v1';
global.RULE_UNLOCK_N=20; global.RULES_URL='';
global.FEAT_FWD=10; global.DIR_PTS=0.5; global.DRIFT_PTS=0.5;
global.CANDLE_MS=180000;
global.STATE={SPY:{price:774, candles:[], king:773, lastClosedB:1}, QQQ:{price:null,candles:[]}};
global.RESHUFFLE={SPY:false,QQQ:false};
global.LASTFEED={SPY:null}; global.LASTVEX={SPY:null};
global.RECORDER_KEY='gpts_rec_v1'; global.RECORDER_DAYS=4; global.RECORDER_SYMS=['SPY','QQQ'];
global.directionGrade=function(){ return {grade:'B',dir:'UP',score:3,tier:'⚖',capped:null,inputs:{drift:{verdict:'AGREE-UP'},structAsym:{bias:'support-heavy'},rangePos:{zone:'lower'},regime:{tag:'trend'},session:{bucket:'morning',opex:false}}}; };
global.driftRead=function(){ return {verdict:'AGREE-UP',gvwap:773.9,vvwap:775,gLo:772.6,gHi:775.2,vLo:773.2,vHi:776.8,overlap:true,px:774,dir:1}; };
global.nodeMapModel=function(){ return {ok:true,px:774,kingK:775,levels:[{k:773,isFlr:true,pos:true,taps:0,state:'Building',chg:12}]}; };
global.inPlayZone=function(){ return {k:773,isFlr:true,pos:true,taps:0,state:'Building',chg:12}; };
global.nodeGrade=function(){ return {grade:'A',score:5,tier:'⚖',inputs:{pol:'+',tap:0,rocNow:'Building',rocDay:{pct:20,label:'Acm'},conf:{q:true,v:true,holdDir:1}}}; };
global.decisionCell=function(){ return {cell:'B×A',text:'bounce play',dirGrade:'B',nodeGrade:'A',tier:'⚖',k:773}; };
global.accumCanon=function(){ return {m15:{pct:10,label:'Acm'},session:{pct:20,label:'Acm'}}; };
global.deflAnticipation=function(){ return {fired:true,grade:'A',dist:0.3}; };
global.reactionQuality=function(){ return {q:'confirmed',why:'wick'}; };
global.zoneMeaningful=function(){ return true; };
global.zoneRole=function(){ return 'Flr'; };
global.nodeHoldDir=function(L,px){ return L&&px!=null?(L.k<px?1:-1):0; };
global.actToday=function(){ return [{t:Date.now(),k:773,cell:'B×A',dirGrade:'B',nodeGrade:'A',action:'take'}]; };
global.kingRoll=function(){ return 1; };
global.gatekeeper=function(){ return {ok:true,k:774}; };
global.spineOf=function(){ return {dir:directionGrade(), inPlay:inPlayZone(), node:nodeGrade(), decision:decisionCell()}; };
// (v10.54) the frame the card drew rides on dir/node/decision/act records.
global.tradeFrame=function(sym,L,dir){ return {zone:[772.75,773.25], inval:771, tgt:776, path:'wall', k:L?L.k:null, dir:dir||1}; };
global.RULES_DOC=null;
global.DIR_WEIGHTS={ trend:3, driftAgree:2, driftLean:1, diverge:-2, tentative:1, gradeA:5, gradeB:3 };
global.FEAT_KEEP_BARS=160;
global.render=function(){};
// the two knobs this test actually drives
var RG={tag:'trend', er:0.62};
var CANDLES=[]; for(var i=0;i<20;i++) CANDLES.push({o:774,h:774.2,l:773.8,c:774});
global.regimeTag=function(){ return RG; };
global.closedCandles=function(){ return CANDLES; };
var NOW=new Date(2026,7,21,11,0);          // Friday 2026-08-21 = 3rd Friday = OPEX
global.ctNow=function(){ return NOW; };
global.ctMinutesSinceMidnight=function(){ return 10*60+30; };
global.ctTodayStr=function(){ return '2026-08-21'; };

eval([ 'ruleSeed','rulesSeed','rulesNormalize','rulesLoad','rulesSave','ruleTier','rulePromoted',
       'registerFeature','featureByKey','_fwdHitDir','_fwdHitNum','registerCoreFeatures','seedQuestions',
       'isOpexDay','sessionBucket','eventTagNow','featRegime','featureCtx','featRecordAll','featEnqueue',
       'recorderLoad','recorderSave','recorderDay','_fpct','effN','nTxt','frameRR','_frameRecOf',
       'weightsHash','modelStamp',
       // (v11.91) lifted to top level so __gptsDebug can reach them — the harness must eval them too,
       // or every record() that calls one throws and featRecordAll silently stores null.
       'trendMachineRecord','biasConfirmRecord' ].map(ex).join('\n'));

var F=registerCoreFeatures();

// ================= 1. EVERY record carries regime {tag, opex, event} ==========
var ctx=featureCtx('SPY');
var snap=featRecordAll('SPY', ctx);
var keys=Object.keys(snap);
ok(keys.length===F.length, '1a one record per enrolled feature', keys.length);
ok(keys.length>=18, '1b ...and that is the full registry, not a subset', keys.length);
keys.forEach(function(k){
  var r=snap[k];
  ok(r && r.regime && typeof r.regime==='object', '1·'+k+' carries a regime OBJECT');
  ok(r && r.regime && typeof r.regime.tag==='string' && typeof r.regime.opex==='boolean'
       && typeof r.regime.event==='boolean', '1·'+k+' regime is {tag,opex,event}');
});
ok(keys.every(function(k){ return snap[k].regime.tag==='trend'; }),
   '1c the tag comes from regimeTag() — every record agrees on the day it is in');
// (v10.54, audit 6) WHICH MODEL SCORED THE BAR, on every record, stamped in ONE place.
keys.forEach(function(k){
  var r=snap[k];
  ok(r && r.model && typeof r.model==='object', '1·'+k+' carries a model stamp');
  ok(r && r.model && ('rulesAsOf' in r.model) && ('weightsHash' in r.model),
     '1·'+k+' model is {rulesAsOf, weightsHash}');
});

// ================= 2. THE TAG TRACKS regimeTag(), including "na" ==============
RG={tag:'chop', er:0.18};
ok(featRecordAll('SPY', featureCtx('SPY')).dir.regime.tag==='chop', '2a chop is carried through');
RG={tag:'mixed', er:0.33};
ok(featRecordAll('SPY', featureCtx('SPY')).dir.regime.tag==='mixed', '2b mixed is carried through verbatim, not rounded to a side');
RG=null;                                   // regimeTag() returns null on <12 bars
ok(featRegime('SPY', featureCtx('SPY')).tag==='na', '2c too few bars -> "na", never a guessed regime');
ok(featRecordAll('SPY', featureCtx('SPY')).dir.regime.tag==='na', '2d ...and that is what lands on the record');
RG={tag:'trend', er:0.62};

// ================= 3. OPEX AND EVENT ARE HONEST ==============================
ok(isOpexDay(NOW)===true, '3a 2026-08-21 is the 3rd Friday — an OPEX day');
ok(featRegime('SPY', featureCtx('SPY')).opex===true, '3b ...so opex:true rides on the record');
NOW=new Date(2026,7,20,11,0);              // Thursday
ok(featRegime('SPY', featureCtx('SPY')).opex===false, '3c a non-OPEX day says false', featRegime('SPY').opex);
NOW=new Date(2026,7,21,11,0);
ok(featRegime('SPY', featureCtx('SPY')).event===false, '3d no event calendar is wired, so event is FALSE — never invented');
global.EVENT_TAG='CPI';
ok(eventTagNow()===true && featRegime('SPY', featureCtx('SPY')).event===true,
   '3e ...but an existing event-tag state IS read when one is present');
delete global.EVENT_TAG;
ok(featRegime('SPY', featureCtx('SPY')).event===false, '3f ...and drops back to false when it is not');

// ================= 4. IT SURVIVES INTO THE RECORDER QUEUE ====================
LS={};
var ctx4=featureCtx('SPY');
var snap4=featRecordAll('SPY', ctx4);
featEnqueue('SPY', snap4, ctx4);
var q=recorderDay(recorderLoad()).feat.SPY;
ok(q.length===F.length, '4a the bar was queued', q.length);
ok(q.every(function(r){ return r.rec && r.rec.regime && r.rec.regime.tag==='trend'
                              && typeof r.rec.regime.opex==='boolean'; }),
   '4b EVERY queued record carries its regime — this is what makes outcomes aggregable per regime');
ok(JSON.parse(JSON.stringify(q))[0].rec.regime.tag==='trend', '4c ...and it survives the JSON round trip into the day file');

// ================= 5. ONE PLACE, SO NEW FEATURES GET IT FOR FREE =============
var FRA=ex('featRecordAll');
ok(/out\[f\.key\]=f\.record\(sym, ctx\)/.test(FRA), '5a featRecordAll still iterates the registry (enrollment contract intact)');
ok(/\.regime=\{ tag:rg\.tag, opex:rg\.opex, event:rg\.event \}/.test(FRA),
   '5b ...and stamps the regime on every record in ONE place');
ok(/featRegime/.test(FRA), '5c ...computed once per bar, not once per feature');
var FR=ex('featRegime');
ok(/regimeTag\(closedCandles\(sym\)/.test(FR), '5d the tag is regimeTag() over the closed candles');
ok(/isOpexDay\(\)/.test(FR), '5e opex comes from the session bucket / isOpexDay');
ok(/eventTagNow/.test(FR), '5f event comes from an event-tag state');
ok(/never a guess/.test(ex('eventTagNow')), '5g ...which says out loud that absent one, the answer is false');
ok(FRA.indexOf('registerCoreFeatures()')>=0, '5h enrollment still runs first');

// ================= 6. THE ANALYSIS TABLE SPLITS BY IT ========================
var DFS=ex('dirFactorStats');
ok(/r\.rec && r\.rec\.regime && r\.rec\.regime\.tag/.test(DFS), '6a dirFactorStats reads the regime off the record');
ok(/b\.trend\.n\+\+/.test(DFS) && /b\.chop\.n\+\+/.test(DFS), '6b ...and tallies trend and chop separately');
ok(/trendRate/.test(DFS) && /chopRate/.test(DFS), '6c ...producing a rate per regime');
ok(/regimeSplit=/.test(DFS), '6d ...and flags a row that INVERTS between regimes');
var DFH=ex('dirFactorsHtml');
ok(/trend \/ chop/.test(DFH), '6e the factor table has the regime split column');
ok(/r\.trendRate/.test(DFH) && /r\.chopRate/.test(DFH), '6f ...rendering rate + n for each');
ok(/⚠regime/.test(DFH), '6g ...with the inversion warning beside it');
ok(/⚠1-way/.test(DFH), '6h ...and the 1-way flag is still there');
ok(/dir\.struct/.test(ex('dirFactorGroups')) && /dir\.kingRoll/.test(ex('dirFactorGroups'))
   && /netGamma/.test(ex('dirFactorGroups')) && /dir\.trendFast/.test(ex('dirFactorGroups')),
   '6i (v10.54) the PARKED candidates are grouped too, so a promotion decision has the same columns');

// ================= 7. THE 1-WAY FLAG IS A RATIO, NOT AN ABSOLUTE ZERO ========
// (v10.54, audit 10) The old rule was `up===0 || dn===0`. On 2026-08-11 structure voted
// DOWN on 46 of 49 bars of a down day and "scored" ~94% — the exact artifact this column
// exists to catch — and it was NOT flagged, because three stray up-votes made it
// "two-sided". 90% one way is one way.
function oneSided(up, dn){
  var vn=up+dn;
  var share=(vn>0)?(Math.max(up,dn)/vn):0;
  return (vn>=10 && share>=0.90);
}
ok(oneSided(3,46)===true, '7a THE 2026-08-11 CASE: 46 of 49 votes one way IS flagged 1-way', Math.round(100*46/49)+'%');
ok(oneSided(46,3)===true, '7b ...and the mirror image (46 up of 49) too');
ok(oneSided(19,30)===false, '7c 30 of 49 (61%) is NOT 1-way — a genuinely split factor', Math.round(100*30/49)+'%');
ok(oneSided(0,49)===true, '7d a pure one-way factor is still flagged');
ok(oneSided(1,8)===false, '7e ...but a 9-vote row is too small to judge (vn>=10 required)', 9);
ok(oneSided(1,9)===true, '7f ...and exactly 10 votes at 90% is the boundary, inclusive');
ok(oneSided(2,8)===false, '7g 8 of 10 (80%) is under the bar');
// the SAME arithmetic must be what dirFactorStats actually computes
ok(/b\.oneWayShare=\(vn>0\)\?\(Math\.max\(b\.up,b\.dn\)\/vn\):0/.test(DFS),
   '7h dirFactorStats computes the one-way SHARE');
ok(/b\.oneSided=\(vn>=10 && b\.oneWayShare>=0\.90\)/.test(DFS),
   '7i ...and flags at >= 0.90 on vn >= 10');
ok(!/b\.oneSided=\(vn>=10 && \(b\.up===0 \|\| b\.dn===0\)\)/.test(DFS),
   '7j ...the absolute-zero rule is gone');
ok(/oneWayShare/.test(DFH), '7k the flag hover states the actual share, so the number is inspectable');

// ================= 8. EFFECTIVE N IS WHAT THE TABLE SHOWS ====================
ok(effN(200)===20, '8a effN divides the overlapping bar-records out: 200 bars = 20 eff', effN(200));
ok(effN(10)===1 && effN(0)===0 && effN(null)===0, '8b ...and degrades honestly');
ok(nTxt(200)==='n=200 bars → eff 20', '8c every displayed n reads "n=200 bars → eff 20"', nTxt(200));
ok(/effN\(r\.n\)>=RULE_UNLOCK_N/.test(DFH), '8d the factor table unlocks on EFFECTIVE n, not raw bar-records');
ok(/eff n/.test(DFH), '8e ...and labels the column so nobody reads it as independent observations');

console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
