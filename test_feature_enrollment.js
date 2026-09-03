global.FEAT_ARCHIVE={};   // (v11.0) IDB archive of resolved records (empty in the harness)
// (v10.49 B) FEATURE ENROLLMENT — the ENFORCEMENT test for the standing rule that no
// feature ships un-scrutinised. Every FEATURES entry must carry all seven fields, and
// every key must appear in the recorder export, the analysis iterator and the rules seed.
// The suite FAILS if a verdict-producing block is added without a registry entry.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m+(g!==undefined?' -> '+g:''));} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+g:''));} };

var LS={};
global.localStorage={ getItem:function(k){ return (k in LS)?LS[k]:null; }, setItem:function(k,v){ LS[k]=String(v); } };
global.window={ __gptsDebug:{} };
global.TODAY='2026-08-17';
global.FEATURES=[];
global.RULES=null;
global.RULES_KEY='gpts_rules_v1';
global.RULE_UNLOCK_N=20;
global.RULES_URL='';
global.FEAT_FWD=10; global.DIR_PTS=0.5; global.DRIFT_PTS=0.5;
global.CANDLE_MS=180000;
global.STATE={SPY:{price:774, candles:[], king:773, lastClosedB:1}, QQQ:{price:null,candles:[]}};
global.RESHUFFLE={SPY:false,QQQ:false};
global.LASTFEED={SPY:null}; global.LASTVEX={SPY:null};
// spine stubs (each feature's record() must survive them)
global.directionGrade=function(){ return {grade:'B',dir:'UP',score:3,tier:'⚖',capped:null,inputs:{drift:{verdict:'AGREE-UP'},structAsym:{bias:'support-heavy'},rangePos:{zone:'lower'},regime:{tag:'trend'},session:{bucket:'morning',opex:false}}}; };
global.driftRead=function(){ return {verdict:'AGREE-UP',gvwap:773.9,vvwap:775,gLo:772.6,gHi:775.2,vLo:773.2,vHi:776.8,overlap:true,px:774,dir:1}; };
global.nodeMapModel=function(){ return {ok:true,px:774,kingK:775,levels:[{k:773,isFlr:true,pos:true,taps:0,state:'Building',chg:12}]}; };
global.inPlayZone=function(){ return {k:773,isFlr:true,pos:true,taps:0,state:'Building',chg:12}; };
// (v10.54) the frame the card drew rides on dir/node/decision/act records so the frame
// outcome (tgt before inval) is scoreable after the fact.
global.tradeFrame=function(sym,L,dir){ return {zone:[772.75,773.25], inval:771, tgt:776, path:'wall', k:L?L.k:null, dir:dir||1}; };
global.nodeGrade=function(){ return {grade:'A',score:5,tier:'⚖',inputs:{pol:'+',tap:0,rocNow:'Building',rocDay:{pct:20,label:'Acm'},conf:{q:true,v:true,holdDir:1}}}; };
global.decisionCell=function(){ return {cell:'B×A',text:'bounce play',dirGrade:'B',nodeGrade:'A',tier:'⚖',k:773}; };
global.accumCanon=function(){ return {m15:{pct:10,label:'Acm'},session:{pct:20,label:'Acm'}}; };
global.deflAnticipation=function(){ return {fired:true,grade:'A',dist:0.3}; };
global.reactionQuality=function(){ return {q:'confirmed',why:'wick'}; };
global.zoneMeaningful=function(){ return true; };
global.zoneRole=function(){ return 'Flr'; };
global.nodeHoldDir=function(L,px){ return L&&px!=null?(L.k<px?1:-1):0; };
global.sessionBucket=function(){ return {bucket:'morning',opex:false,capOdds:false,mins:630}; };
global.actToday=function(){ return [{t:Date.now(),k:773,cell:'B×A',dirGrade:'B',nodeGrade:'A',action:'take'}]; };
global.kingRoll=function(){ return 1; };
global.gatekeeper=function(){ return {ok:true,k:774}; };
global.ctNow=function(){ return new Date(2026,7,17,11,0); };
global.render=function(){};

// (v10.54, audit 1) a tier is earned LOCALLY: promoted by this panel AND locally measured.
eval([ 'ruleSeed','rulesSeed','rulesNormalize','rulesLoad','rulesSave','effN','nTxt',
       'featStatsCached','featStatsInvalidate','ruleLocalRate','rulePromotedApplied','promoLoad','ruleTier','rulePromoted',
       'registerFeature','featureByKey','_fwdHitDir','_fwdHitNum','frameRR','_frameRecOf',
       'registerCoreFeatures','seedQuestions',
       // (v11.91) lifted to top level so __gptsDebug can reach them — the harness must eval them too,
       // or every record() that calls one throws and featRecordAll silently stores null.
       'trendMachineRecord','biasConfirmRecord' ].map(ex).join('\n'));

var F=registerCoreFeatures();

// ================= 1. THE CONTRACT: all seven fields on every entry =================
ok(F.length>=8, '1a at least the eight v10.49 features are registered', F.length);
var REQUIRED=['key','label','phase','record','outcome','fwd','questions','rule'];
F.forEach(function(f){
  REQUIRED.forEach(function(k){
    ok(f[k]!==undefined && f[k]!==null, '1·'+f.key+' has "'+k+'"');
  });
  ok(typeof f.record==='function',  '1·'+f.key+' record is a function');
  ok(typeof f.outcome==='function', '1·'+f.key+' outcome is a function');
  ok(typeof f.fwd==='number' && f.fwd>0, '1·'+f.key+' fwd is a positive bar count', f.fwd);
  ok(Array.isArray(f.questions) && f.questions.length>0, '1·'+f.key+' raises at least one question');
  ok(f.rule && f.rule.id && f.rule.mechanism, '1·'+f.key+' seeds a rule with a mechanism note');
  f.questions.forEach(function(q){ ok(q.id && Array.isArray(q.when), '1·'+f.key+' question "'+(q.id||'?')+'" is well-formed'); });
});

// ================= 2. the spec's required keys are all present =================
var keys=F.map(function(f){ return f.key; });
['dir','dir.drift','node','decision','acm','defl_ant','reaction','act'].forEach(function(k){   // (v11.0) drift merged into dir.drift
  ok(keys.indexOf(k)>=0, '2·'+k+' is enrolled');
});
['rshuf','dir.kingRoll','gateHour'].forEach(function(k){   // (v11.0) roll merged into dir.kingRoll
  ok(keys.indexOf(k)>=0, '2·'+k+' Phase-B item enrolled the same way');
});
ok(keys.length===new Set(keys).size, '2z no duplicate keys');
ok(registerFeature({key:'dir',label:'x',phase:'p',record:function(){},outcome:function(){},fwd:1,questions:[],rule:{id:'dir'}}) &&
   FEATURES.filter(function(f){return f.key==='dir';}).length===1, '2y re-registering a key REPLACES rather than duplicating');
FEATURES.length=0; F=registerCoreFeatures();  // restore

// ================= 3. every record() runs and returns an object =================
var ctx={ px:774, t:Date.now(), bar:1, n:5, m:nodeMapModel(), spine:{}, session:sessionBucket() };
F.forEach(function(f){
  var r=null, threw=false;
  try{ r=f.record('SPY', ctx); }catch(e){ threw=true; }
  ok(!threw, '3·'+f.key+' record() does not throw');
  ok(r && typeof r==='object', '3·'+f.key+' record() returns an object');
});

// ================= 4. every outcome() runs and returns {hit,mfe,mae} ==============
var fwdUp={px0:774, pxEnd:775.2, mfe:1.2, mae:-0.1, net:1.2, n:10, first:'up', kingReached:true};
var fwdDn={px0:774, pxEnd:772.8, mfe:0.1, mae:-1.2, net:-1.2, n:10, first:'dn', kingReached:false};
F.forEach(function(f){
  var rec=f.record('SPY', ctx);
  [fwdUp, fwdDn].forEach(function(fw){
    var o=null, threw=false;
    try{ o=f.outcome(rec, fw); }catch(e){ threw=true; }
    ok(!threw, '4·'+f.key+' outcome() does not throw');
    ok(o && ('hit' in o), '4·'+f.key+' outcome() reports hit');
    ok(o && o.mfe!=null && o.mae!=null, '4·'+f.key+' outcome() carries MFE/MAE (required on EVERY outcome)');
    ok(o.hit===null || o.hit===0 || o.hit===1 || o.hit===true || o.hit===false, '4·'+f.key+' hit is boolean-ish or null (pending)');
  });
});
// the direction helper itself
ok(_fwdHitDir(fwdUp,'UP')===1 && _fwdHitDir(fwdDn,'UP')===0, '4x UP hits only on a real up excursion');
ok(_fwdHitDir(fwdDn,'DN')===1 && _fwdHitUpCheck(), '4y DN hits only on a real down excursion');
function _fwdHitUpCheck(){ return _fwdHitDir(fwdUp,'DN')===0; }
ok(_fwdHitDir({mfe:0.1,mae:-0.1},'SIDE')===1, '4z SIDE hits when price stayed inside the band');

// ================= 5. KEYS ⊂ recorder export ∩ analysis ∩ rules ==================
var rules=rulesSeed();
F.forEach(function(f){
  ok(!!rules[f.rule.id], '5·'+f.key+' rule id "'+f.rule.id+'" is in the rules seed');
});
// the recorder writes every key: featRecordAll iterates FEATURES
ok(/FEATURES\.forEach\(function\(f\)\{\s*try\{ out\[f\.key\]=f\.record/.test(src.replace(/\n/g,'')) ||
   /out\[f\.key\]=f\.record\(sym, ctx\)/.test(src), '5a the recorder iterates FEATURES (featRecordAll)');
ok(/feat:\(function\(\)\{ try\{ return featRecordAll/.test(src), '5b snap.feat is written on the bar snapshot');
ok(/feat:day\.feat\|\|\{\}/.test(src), '5c the day export carries the resolved feature queue');
ok(/FEATURES\.forEach\(function\(f\)\{/.test(ex('featureScorecardsHtml')), '5d the Analysis tab iterates FEATURES');
ok(/FEATURES\.forEach/.test(ex('seedQuestions')), '5e the question queue iterates FEATURES');
ok(/FEATURES\.forEach\(function\(f\)\{ if\(f && f\.rule/.test(src), '5f the rules seed iterates FEATURES');

// ================= 6. the seeded ruleset =================
ok(Object.keys(rules).length>=25, '6a the seed is complete', Object.keys(rules).length);
['dir','dir.A','dir.B','dir.C','drift.conf','node.grade.A','node.grade.B','node.grade.C',
 'node.tap.1','node.tap.2','node.tap.3','node.pol.pos','node.pol.neg','node.rocDay.up','node.rocDay.dn',
 'acm.realVsHedge'].forEach(function(id){ ok(!!rules[id], '6·'+id+' seeded'); });
['A×A','A×B','A×C','B×A','B×B','B×C','C×A','C×B','C×C'].forEach(function(c){
  ok(!!rules['decision.'+c], '6·decision.'+c+' seeded'); });
// THE KILL LIST
['kill.tap3','kill.midrange','kill.noConf','kill.negGammaWide'].forEach(function(id){
  ok(!!rules[id], '6·KILL '+id+' seeded');
  ok(!!rules[id].mechanism, '6·KILL '+id+' carries its mechanism note');
});
Object.keys(rules).forEach(function(id){
  var r=rules[id];
  ok(r.tier==='hand' && r.promoted===false, '6·'+id+' starts hand-set ⚖, never pre-promoted');
});

// ================= 7. tiers + promotion =================
LS={}; global.RULES=null;
ok(ruleTier('dir.A')==='⚖', '7a an unmeasured rule renders ⚖');
ok(rulePromoted('dir.A')===false, '7b ...and may not be cited as odds');
// (v10.54, audit 1) A TIER IS EARNED LOCALLY. A rule carrying promoted:true /
// tier:'measured' / rate:71 / n:34 in a FETCHED document is documentation about what the
// weekly run believes — nothing on THIS machine measured it, so the panel says ⚖. Before
// 10.54 one edited JSON file could make the tool claim a measurement it had never taken.
global.RULES=null; LS['gpts_rules_v1']=JSON.stringify({rules:{'dir.A':{id:'dir.A',promoted:true,tier:'measured',rate:71,n:34}}});
ok(ruleTier('dir.A')==='⚖', '7c a rule promoted only IN A FETCHED FILE still renders ⚖', ruleTier('dir.A'));
ok(rulePromoted('dir.A')===false, '7d ...and may NOT be cited as odds');
// it takes BOTH: a local promotion AND locally measured effN
var __realPromoApplied=rulePromotedApplied;
rulePromotedApplied=function(){ return { id:'p.x', rule:'dir.A' }; };
global.featStats=function(){ return { byGrade:{ dir:{ A:{ n:400, hit:280 } }, node:{}, dirSide:{} },
  byKey:{}, cells:{}, frame:{}, act:{take:{n:0},pass:{n:0}}, partial:0, days:3, dayKeys:[] }; };
featStatsInvalidate();
ok(ruleTier('dir.A')==='📊', '7c2 a LOCAL promotion plus local eff n=40 earns the 📊', ruleTier('dir.A'));
global.featStats=function(){ return { byGrade:{ dir:{ A:{ n:60, hit:42 } }, node:{}, dirSide:{} },
  byKey:{}, cells:{}, frame:{}, act:{take:{n:0},pass:{n:0}}, partial:0, days:1, dayKeys:[] }; };
featStatsInvalidate();
ok(ruleTier('dir.A')==='⚖', '7c3 ...drop the local evidence to eff n=6 and it goes straight back to ⚖', ruleTier('dir.A'));
rulePromotedApplied=__realPromoApplied; delete global.featStats; featStatsInvalidate();
ok(ruleTier('kill.tap3')==='⚖', '7e rules missing from a stored file are back-filled from the seed');

// ================= 8. seedQuestions =================
global.RULES=null; LS={};
var qs=seedQuestions();
ok(qs.length>=10, '8a the queue is seeded from every feature', qs.length);
ok(qs.every(function(q){ return q.id && q.feature && q.state==='proposed' && q.tier; }), '8b every question is well-formed and starts "proposed"');
['dir_A_follow','dir_midrange_cap','drift_conf','node_grade_hold','tap_decay','pol_char','roc_day','matrix_cells'].forEach(function(id){
  ok(qs.some(function(q){ return q.id===id; }), '8·'+id+' present (layer spec §3)');
});
var qids=qs.map(function(q){ return q.id; });
ok(qids.length===new Set(qids).size, '8z no duplicate question ids');


// ================= 9. THE THREE CONSUMERS ACTUALLY RUN (round trip) =================
// Enrollment is worthless if the layers do not consume it. Record on a bar, advance past
// the forward window, resolve, and read the scorecards back — proving DATA → ANALYSIS works
// end to end, is FORWARD-ONLY, and is IDEMPOTENT on both halves.
global.RECORDER_KEY='gpts_rec_v1'; global.RECORDER_DAYS=4; global.RECORDER_SYMS=['SPY','QQQ'];
global.ACM_BAND=8;
global.PAL={card:'#12161f',line:'#1e2530',longAccent:'#2ec27e',shortAccent:'#f0616d',ink:'#e6edf3',sub:'#8b98a9',gold:'#e3c341',blue:'#4a90d9'};
global.fmtNum=function(x){ return ''+x; };
global.DECISION_MATRIX={A:{A:'take · follow-thru',B:'take · tight tgt',C:'wait fresher node'},
                        B:{A:'bounce play',B:'scalp',C:'skip'},
                        C:{A:'scalp only',B:'skip',C:'stand aside'}};
global.spineOf=function(){ return {dir:directionGrade(), inPlay:inPlayZone(), node:nodeGrade(), decision:decisionCell()}; };
var cs=[]; for(var ci=0;ci<20;ci++) cs.push({o:774,h:774.2,l:773.8,c:774});
global.STATE.SPY.candles=cs; global.STATE.SPY.lastClosedB=20;
global.ctNowSecOfDay=function(){ return 11*3600; };   // mid-session: no partial resolution
eval([ 'zoneMeaningful','zoneRole','nodeHoldDir','acmLabel','pctN','frameOutcome','yourCallsHtml',
       'recorderLoad','recorderSave','recorderDay','featureCtx','featRecordAll','featEnqueue',
       'resolveFeatureOutcomes','featStats','_fpct','gradeMonotone','featureScorecardsHtml',
       'actRecord','actToday','actCurrent','modelHeat' ].map(ex).join('\n'));
LS={};
global.FEAT_KEEP_BARS=160;   // (v10.54) records are capped by BARS, not by count
var ctx9=featureCtx('SPY');
var snap9=featRecordAll('SPY', ctx9);
ok(Object.keys(snap9).length===FEATURES.length, '9a snap.feat carries one record per enrolled feature', Object.keys(snap9).length);
featEnqueue('SPY', snap9, ctx9);
var day9=recorderDay(recorderLoad());
ok((day9.feat.SPY||[]).length===FEATURES.length, '9b queued into recorderDay(db).feat[sym]', day9.feat.SPY.length);
ok(day9.feat.SPY.every(function(r){ return r.key && r.t && ('rec' in r) && r.hit===null && r.mfe===null && r.mae===null && r.resolved===false; }),
   '9c queue shape is {key,t,rec,hit,mfe,mae,resolved} and starts unresolved');
featEnqueue('SPY', snap9, ctx9);
ok(recorderDay(recorderLoad()).feat.SPY.length===FEATURES.length, '9d IDEMPOTENT: re-enqueueing the same (key, bar) does not duplicate');
ok(resolveFeatureOutcomes('SPY')===0, '9e FORWARD-ONLY: nothing resolves before the window closes');
for(var b9=0;b9<9;b9++) cs.push({o:774,h:774.3,l:773.9,c:774.1});
ok(resolveFeatureOutcomes('SPY')===0, '9f 9 bars is still short of fwd=10');
cs.push({o:774.1,h:775.2,l:774.0,c:775.2});                       // +1.2 up move closes the window
// (v15.51) a toClose feature (lodhod) waits for 15:00 CT by design — it is pending here on purpose.
var long9=FEATURES.filter(function(f){ return (f.fwd||FEAT_FWD)>10 || f.toClose; }).length;   // (v11.1) nextStop.60 has a 20-bar window
ok(resolveFeatureOutcomes('SPY')===FEATURES.length-long9, '9g the window closes and every 10-bar record resolves (longer-window features stay pending)');
ok(resolveFeatureOutcomes('SPY')===0, '9h IDEMPOTENT: a resolved record is never re-scored');
var q9=recorderDay(recorderLoad()).feat.SPY;
var dir9=q9.filter(function(r){ return r.key==='dir'; })[0];
ok(dir9.resolved===true && dir9.hit===1, '9i an UP grade hits on a +1.2 excursion', dir9.hit);
ok(dir9.mfe>=1.19 && dir9.mae<=0, '9j MFE/MAE written on the record', dir9.mfe+'/'+dir9.mae);
ok(q9.filter(function(r){ return r.resolved; }).every(function(r){ return r.mfe!=null && r.mae!=null; }), '9k EVERY resolved record carries MFE/MAE');
// ANALYSIS consumer
var html9=featureScorecardsHtml('SPY');
ok(html9.length>1000, '9l the Analysis scorecards render', html9.length);
ok(FEATURES.every(function(f){ return html9.indexOf(f.label)>=0; }), '9m every enrolled feature appears in the Analysis tab');
// (v10.54, audit 8) the unlock counter is in EFFECTIVE observations: 10 overlapping
// bar-records of one feature are ONE 30-minute outcome, not ten.
ok(/● recording eff 0\/20/.test(html9), '9n features under the unlock show "● recording eff x/20"');
ok(/n=1 bars → eff 0/.test(html9), '9n2 ...and every n is rendered as "n=X bars → eff Y"');
ok(/BY GRADE/.test(html9) && /DECISION MATRIX/.test(html9) && !/YOUR CALLS/.test(html9),
   '9o by-grade and 3x3 present; takes-vs-passes lives ONLY in Analysis ⑤ now (v11.0 dedup)');
ok(/Direction · SIDE/.test(html9), '9o2 ...with SIDE bars kept in their own by-grade row (v10.54)');
// OPERATOR consumer
var act9=actRecord('SPY','take');
ok(act9 && actToday('SPY').length===1, '9p actRecord writes recorderDay(db).act[sym]');
ok(act9.cell && act9.dirGrade && act9.nodeGrade && act9.session, '9q the action carries the full decision context', JSON.stringify(act9));
ok(!('px' in act9) && !('size' in act9) && !('pnl' in act9), '9r no price, no size, no P&L is ever stored');
actRecord('SPY','pass');
ok(actToday('SPY').length===2 && actCurrent('SPY',act9.k).action==='pass', '9s a later tap appends and wins');
ok(modelHeat('SPY').n>=2, '9t modelHeat reads the resolved dir/node grades back', modelHeat('SPY').n);


// ================= 10. the SHIPPED learning/rules.json IS the seeded ruleset =========
// rulesLoad() reads this file (fail-soft) at boot. If it drifts from the in-script seed,
// the panel's tiers and the nightly loop are describing two different mental models.
var onDisk=null;
try{ onDisk=JSON.parse(require('fs').readFileSync('./learning/rules.json','utf8')); }catch(eF){}
ok(!!onDisk, '10a learning/rules.json exists and parses');
if(onDisk){
  var seedKeys=Object.keys(rulesSeed()).sort().join(',');
  var fileKeys=Object.keys(onDisk.rules||{}).sort().join(',');
  ok(fileKeys===seedKeys, '10b the shipped file has EXACTLY the seeded rule ids', Object.keys(onDisk.rules||{}).length+' ids');
  ok(Object.keys(onDisk.rules).every(function(id){ var r=onDisk.rules[id];
       return r.tier==='hand' && r.promoted===false && r.n===0 && r.rate===null; }),
     '10c every shipped rule starts hand-set ⚖ with no measured record');
  ok(Object.keys(onDisk.rules).every(function(id){ return !!onDisk.rules[id].mechanism; }),
     '10d every shipped rule carries a mechanism note (a rate without a WHY is not an answer)');
  ok(onDisk.unlockN===20 && onDisk.walkForwardSessions===3 && onDisk.decayRuns===3,
     '10e promotion + decay policy is stated in the file');
  ok(/rulesLoad|learning\/rules\.json/.test(src), '10f the panel reads it at boot');
  ok(/gpts_rules_v1/.test(src), '10g localStorage key is the new gpts_rules_v1 (no key renamed)');
}
// the nightly-brief contract must exist alongside it
var brief=null; try{ brief=require('fs').readFileSync('./docs/LLM-NIGHTLY-BRIEF.md','utf8'); }catch(eB){}
ok(!!brief, '10h docs/LLM-NIGHTLY-BRIEF.md exists');
if(brief){
  ['day file','rules.json','prior 3 reviews','act log'].forEach(function(inp){
    ok(brief.toLowerCase().indexOf(inp.toLowerCase())>=0, '10·input "'+inp+'" documented'); });
  ['contradiction','calibration','kill list','question','threshold','missing field'].forEach(function(ask){
    ok(brief.toLowerCase().indexOf(ask.toLowerCase())>=0, '10·ask "'+ask+'" documented'); });
  ok(/review\/YYYY-MM-DD\.json/.test(brief), '10i output path documented');
  ok(/Forbidden/i.test(brief) && /never instructs|never instruct/i.test(brief), '10j the forbidden list is stated');
}

console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
