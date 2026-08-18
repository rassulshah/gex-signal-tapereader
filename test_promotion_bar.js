// (v10.54 GROUP 1) THE BAR IS LOCAL TRUTH.
//
// v10.53 "re-checked" the bar — but every number it re-checked came out of the SAME
// document that asked for the promotion. A weekly run could assert n, assert the
// walk-forward hold, assert clearsBar, and the panel would dutifully agree with itself.
// That is not a bar, it is a rubber stamp.
//
// v10.54 derives n and the walk-forward from THIS MACHINE'S OWN recorded sessions:
//   n_local  = EFFECTIVE n (bar-records / FEAT_FWD) from featStats, for the proposal's
//              own feature key. RULE_UNLOCK_N / PROMO_MIN_N compare against THAT.
//   wf_local = distinct recorded session-days strictly AFTER p.madeOn on which the
//              locally measured rate stayed over the bar.
// The proposal's own numbers are advisory: they are used only to catch a document that
// disagrees with local reality (|n_local − p.n| > 20% refuses).
//
// The headline case: clearsBar:true, n:12, AND a matching local self-report — still
// refused, because eff n=12 is under 20. This file is the test of that claim, plus:
//   · idempotency (re-running the same document never double-applies)
//   · persistence to gpts_promo_v1 (a network-less boot uses the last-applied weights)
//   · the 📊 promoted marker
//   · a promoted kill condition capping the grade at C, with the reason on the decision line
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m+(g!==undefined?' -> '+g:''));} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+g:''));} };

// ---------------- mocks ----------------
let LS={};
global.localStorage={ getItem:k=>(k in LS?LS[k]:null), setItem:(k,v)=>{LS[k]=String(v);}, removeItem:k=>{delete LS[k];} };
global.window={}; global.window.__gptsDebug={};
global.render=function(){};
global.ctTodayStr=function(){ return '2026-08-18'; };
global.ctNow=function(){ return new Date(2026,7,18,11,0); };
global.ctMinutesSinceMidnight=function(){ return 10*60+30; };
// (v10.57) drift is in SHADOW mode live (DRIFT_LIVE=false). These pins exercise the hierarchy AS IT RUNS
// WHEN DRIFT IS PROMOTED, so the flag is forced on here; test_drift_shadow.js pins the live shadow behaviour.
global.DRIFT_LIVE=true;
global.FEATURES=[];
global.TREND_WINDOW=20;
global.RULE_UNLOCK_N=20;
global.RULES_KEY='gpts_rules_v1'; global.RULES2_KEY='gpts_rules_v2'; global.PROMO_KEY='gpts_promo_v1';
global.RULES=null; global.RULES_DOC=null; global.RULES_FETCHED=true; global.RULES_URL='';
global.PROMO=null;
global.PROMO_MIN_N=20; global.PROMO_WF_SESSIONS=3;
global.PROMO_DEMOTE_STRIKES=3; global.PROMO_WEEK_MS=604800000;
global.DIR_WEIGHTS_HAND={ trend:3, driftAgree:2, driftLean:1, diverge:-2, tentative:1, gradeA:5, gradeB:3 };
global.DIR_WEIGHTS={ trend:3, driftAgree:2, driftLean:1, diverge:-2, tentative:1, gradeA:5, gradeB:3 };
global.DIR_WEIGHTS_SOURCE='hand-set';
global.NODE_WEIGHTS={ gradeA:5, gradeB:3 };
global.FEAT_FWD=10;
global.TREND_DOM=15;
global.CFG={ trendMA:{SPY:50,QQQ:50} };
global.RULES_APPLIED_AT=null; global.RULES_APPLIED_DAY=null;
global.ctNowSecOfDay=function(){ return 7*3600; };      // pre-open: the model may move

// ---- LOCAL TRUTH, under this test's control -------------------------------------
// LOCAL_BARS = how many BAR-records this machine has for the feature a proposal names.
// LOCAL_DAYS = per-day {n,hit} for the same feature, which is what the walk-forward is
// re-derived from. Nothing here comes from the proposal.
var LOCAL_BARS=0, LOCAL_HIT=0;
var LOCAL_DAYS={};                 // {'2026-08-12': {n:40, hit:30}, ...}
global.featStats=function(){
  return { byKey:{ 'dir.trend5':{ n:LOCAL_BARS, hit:LOCAL_HIT, pending:0, mfe:0, mae:0, mn:0, partial:0 } },
           byGrade:{dir:{},node:{},dirSide:{}}, cells:{}, frame:{}, partial:0,
           act:{take:{n:0,hit:0},pass:{n:0,hit:0}}, days:Object.keys(LOCAL_DAYS).length, dayKeys:Object.keys(LOCAL_DAYS).sort() };
};
global.recorderLoad=function(){
  var days={};
  Object.keys(LOCAL_DAYS).forEach(function(dk){
    var o=LOCAL_DAYS[dk]; var arr=[];
    for(var i=0;i<o.n;i++) arr.push({ key:'dir.trend5', resolved:true, hit:(i<o.hit)?1:0, bar:i, n:i+1 });
    days[dk]={ feat:{ SPY:arr } };
  });
  return { days:days };
};
// give the panel 42 EFFECTIVE observations (420 bar-records) and 3 clean sessions
function localGood(){
  LOCAL_BARS=420; LOCAL_HIT=270;                   // 64%
  LOCAL_DAYS={ '2026-08-11':{n:60,hit:40}, '2026-08-12':{n:60,hit:41}, '2026-08-13':{n:60,hit:39} };
  featStatsInvalidate();
}
function localNone(){ LOCAL_BARS=0; LOCAL_HIT=0; LOCAL_DAYS={}; featStatsInvalidate(); }

eval(['ruleSeed','rulesSeed','rulesNormalize','rulesDefaultWeights','rulesDocNormalize','rulesDoc',
      'rulesLoad','rulesIngest','rulesSave','ruleGet','effN','nTxt','pctN','featStatsCached',
      'featStatsInvalidate','ruleLocalRate','rulePromotedApplied','ruleTier','rulePromoted',
      'promoLoad','promoSave','promoToday','proposalRegimeFlip','proposalFeatureKey','proposalLocalN',
      'proposalWalkForwardLocal','proposalClearsBar','promoDirWeightKey',
      'promoApplyWeight','promoApplySwap','promoApplySwapsPersisted','promoApplyKill','applyProposals',
      'rulesApplyWeights','promoApplyPersisted','promoDemote','promoDemoteCheck','promoEvents',
      'rulesApplyAllowed','rulesApply','weightsHash','modelStamp','promotionsList','promoMarker',
      'killActive','killCheck','gradeOfScore','nodeGradeOfScore','gradeDisp','isOpexDay','sessionBucket',
      'rangePosOf','directionGrade','nodeHoldDir','decisionCell'].map(ex).join('\n'));
// KILL_DEFS is a table, not a function — lift it out of the source verbatim.
eval(src.slice(src.indexOf('var KILL_DEFS='), src.indexOf('function killActive')));

function reset(){
  LS={}; RULES=null; RULES_DOC=null; PROMO=null; RULES_APPLIED_AT=null;
  DIR_WEIGHTS={ trend:3, driftAgree:2, driftLean:1, diverge:-2, tentative:1, gradeA:5, gradeB:3 };
  DIR_WEIGHTS_SOURCE='hand-set'; TREND_WINDOW=20; TREND_DOM=15;
  CFG={ trendMA:{SPY:50,QQQ:50} };
  localGood();
}
function docWith(proposals, killList){
  return { schema:'gex-rules/v2', asOf:'2026-08-18',
           weights:{ dir:{ trend:3, driftAgree:2, driftLean:1, diverge:-2, tentative:1, gradeA:5, gradeB:3 } },
           rules:{}, proposals:proposals||[], promoted:[], challengers:{}, killList:killList||[] };
}
// a proposal that legitimately clears the bar
// n:42 here is the EFFECTIVE n the reviewer claims — and localGood() gives the panel
// 420 bar-records = eff 42, so the self-report MATCHES local truth. `rule` names the
// feature whose local record measures it.
function goodProposal(over){
  return Object.assign({ id:'p.trend.up', kind:'weight', target:'dir.weights.trend',
    current:3, proposed:4, n:42, lift:11, rule:'dir.trend5', baseline:50,
    wf:{ sessions:3, held:true },
    regimeSplit:{ trend:{rate:64,n:26}, chop:{rate:58,n:16} },
    clearsBar:true, reason:'', madeOn:'2026-08-08' }, over||{});
}

// ============ 1. THE HEADLINE CASE: clearsBar:true, n:12, AND A MATCHING =========
// ============    LOCAL SELF-REPORT — STILL REFUSED, BECAUSE eff n=12 < 20 ========
reset();
// the panel has 120 bar-records = 12 EFFECTIVE observations, and the reviewer says 12:
// the document and the machine agree exactly. It is still not enough evidence.
LOCAL_BARS=120; LOCAL_HIT=78;
LOCAL_DAYS={ '2026-08-11':{n:40,hit:26}, '2026-08-12':{n:40,hit:26}, '2026-08-13':{n:40,hit:26} };
featStatsInvalidate();
let doc=docWith([ goodProposal({ n:12 }) ]);
RULES_DOC=doc; RULES=rulesNormalize({});
let r=applyProposals(doc);
ok(proposalLocalN(goodProposal({n:12})).effN===12, '1·pre local eff n really is 12 (120 bars / fwd 10)', proposalLocalN(goodProposal({n:12})).effN);
ok(proposalWalkForwardLocal(goodProposal({n:12})).sessions===3, '1·pre the local walk-forward really is 3 of 3 held');
ok(r.applied.length===0, '1a clearsBar:true + n=12 + a MATCHING local self-report does NOT apply', r.applied.length);
ok(DIR_WEIGHTS.trend===3, '1b ...the weight is untouched', DIR_WEIGHTS.trend);
ok(/insufficient local evidence — eff n=12/.test(r.skipped[0].why), '1c ...and the refusal names the LOCAL shortfall', r.skipped[0].why);
ok(/need 20/.test(r.skipped[0].why), '1c2 ...against PROMO_MIN_N', r.skipped[0].why);
ok(DIR_WEIGHTS_SOURCE==='hand-set', '1d ...the weights stay HAND-SET');
ok(!LS['gpts_promo_v1'], '1e ...and nothing is persisted as applied');
// (and the walk-forward being locally perfect changes nothing — n is checked first)
[0,1,19].forEach(n=>{
  reset(); LOCAL_BARS=n*10; LOCAL_HIT=Math.round(n*6.4); featStatsInvalidate();
  doc=docWith([ goodProposal({ n:n }) ]); RULES_DOC=doc;
  ok(applyProposals(doc).applied.length===0, '1·eff n='+n+' is below the bar and never applies');
});
reset(); doc=docWith([ goodProposal({ n:undefined }) ]); RULES_DOC=doc;
ok(applyProposals(doc).applied.length===0, '1f a MISSING n is treated as 0, never as "fine"');

// ============ 1B. THE PROPOSAL\'S OWN NUMBERS CANNOT SUPPLY THE EVIDENCE ==========
reset(); localNone();
doc=docWith([ goodProposal({ n:9999 }) ]); RULES_DOC=doc;
r=applyProposals(doc);
ok(r.applied.length===0, '1g n=9999 asserted with NOTHING recorded locally does not apply', r.applied.length);
ok(/eff n=0/.test(r.skipped[0].why), '1h ...the panel reports its OWN n, not the document\'s', r.skipped[0].why);
// (v11.0 audit G4) the "self-report within 20% of local n" test is GONE: the reviewer's n spans the
// whole repo, local truth spans what this machine kept, so the test made promotion impossible as
// the repo grew. The bar is LOCAL evidence only; a proposal must still carry an n.
reset(); doc=docWith([ goodProposal({ n:200 }) ]); RULES_DOC=doc;   // local eff n is 42
r=applyProposals(doc);
ok(r.applied.length===1, '1i a self-report far above local n is NOT refused for that reason (local evidence decides)', r.applied.length);
reset(); doc=docWith([ goodProposal({ n:0 }) ]); RULES_DOC=doc;
r=applyProposals(doc);
ok(r.applied.length===0 && /carries no n/.test(r.skipped[0].why), '1j a proposal with no n is malformed and refused', r.skipped[0]&&r.skipped[0].why);
reset(); doc=docWith([ goodProposal({ n:44 }) ]); RULES_DOC=doc;
ok(applyProposals(doc).applied.length===1, '1k ...a normal proposal with local eff n>=20, walk-forward held, no flip is accepted');
// a proposal nothing local measures can never clear the bar
reset(); doc=docWith([ goodProposal({ id:'p.unknown', target:'', rule:null, kind:'weight' }) ]); RULES_DOC=doc;
r=applyProposals(doc);
ok(r.applied.length===0 && /nothing local measures/.test(r.skipped[0].why),
   '1l a proposal with no local feature key is refused, not assumed fine', r.skipped[0].why);

// ======= 2. THE WALK-FORWARD IS COUNTED FROM LOCALLY RECORDED SESSIONS ==========
// The reviewer's wf block is not consulted at ALL — it describes days this machine may
// never have seen. Only sessions recorded HERE, strictly after p.madeOn, on which the
// locally measured rate stayed over the bar, count.
reset();
LOCAL_DAYS={ '2026-08-11':{n:60,hit:40}, '2026-08-12':{n:60,hit:41} };   // only 2 local sessions
featStatsInvalidate();
doc=docWith([ goodProposal({ wf:{ sessions:9, held:true } }) ]); RULES_DOC=doc;
r=applyProposals(doc);
ok(r.applied.length===0 && DIR_WEIGHTS.trend===3, '2a 2 LOCAL sessions does not apply, whatever the document claims (it says 9 held)');
ok(/walk-forward not held locally \(2 of 3/.test(r.skipped[0].why), '2b ...and the refusal counts LOCAL sessions', r.skipped[0].why);
// a session where the rate fell UNDER the bar does not count as held
reset();
LOCAL_DAYS={ '2026-08-11':{n:60,hit:40}, '2026-08-12':{n:60,hit:41}, '2026-08-13':{n:60,hit:12} };
featStatsInvalidate();
doc=docWith([ goodProposal() ]); RULES_DOC=doc;
r=applyProposals(doc);
ok(r.applied.length===0 && /2 of 3/.test(r.skipped[0].why), '2c a session under the baseline breaks the hold', r.skipped[0].why);
// sessions BEFORE madeOn are not walk-forward, they are the sample it was fitted on
reset();
LOCAL_DAYS={ '2026-08-05':{n:60,hit:50}, '2026-08-06':{n:60,hit:50}, '2026-08-07':{n:60,hit:50},
             '2026-08-11':{n:60,hit:40} };
featStatsInvalidate();
doc=docWith([ goodProposal() ]); RULES_DOC=doc;   // madeOn 2026-08-08
r=applyProposals(doc);
ok(r.applied.length===0 && /1 of 3/.test(r.skipped[0].why),
   '2d sessions BEFORE madeOn are in-sample and never count as walk-forward', r.skipped[0].why);
ok(proposalWalkForwardLocal(goodProposal()).days.join(',')==='2026-08-11',
   '2e ...only the strictly-after day is counted', proposalWalkForwardLocal(goodProposal()).days.join(','));
// the document's wf block cannot rescue a machine with no recorded sessions at all
reset(); LOCAL_BARS=420; LOCAL_HIT=270; LOCAL_DAYS={}; featStatsInvalidate();
doc=docWith([ goodProposal({ wf:{ sessions:3, held:true } }) ]); RULES_DOC=doc;
ok(applyProposals(doc).applied.length===0, '2f wf:{sessions:3,held:true} with 0 local sessions does not apply');

// ================= 3. A REGIME FLIP NEVER APPLIES ============================
reset(); doc=docWith([ goodProposal({ regimeSplit:{ trend:{rate:71,n:30}, chop:{rate:38,n:22} } }) ]); RULES_DOC=doc;
r=applyProposals(doc);
ok(r.applied.length===0 && DIR_WEIGHTS.trend===3, '3a a rate that inverts between trend and chop does not apply');
ok(/regime flip/.test(r.skipped[0].why), '3b ...and the refusal names the flip', r.skipped[0].why);
reset(); doc=docWith([ goodProposal({ regimeSplit:{ trend:{rate:38,n:30}, chop:{rate:71,n:22} } }) ]); RULES_DOC=doc;
ok(applyProposals(doc).applied.length===0, '3c the mirror image (fails in trend, works in chop) is refused too');
reset(); doc=docWith([ goodProposal({ regimeSplit:{ trend:{rate:64,n:30}, chop:{rate:57,n:22} } }) ]); RULES_DOC=doc;
ok(applyProposals(doc).applied.length===1, '3d ...but the SAME SIDE in both regimes is fine (on local evidence)');
ok(proposalRegimeFlip({ regimeSplit:{ trend:{rate:60,n:9} } })===null, '3e a one-sided split cannot be judged, so it is not called a flip');

// ================= 4. clearsBar:false NEVER APPLIES ==========================
reset(); doc=docWith([ goodProposal({ clearsBar:false, reason:'insufficient — n=8, need 20' }) ]); RULES_DOC=doc;
r=applyProposals(doc);
ok(r.applied.length===0, '4a clearsBar:false is honoured even when the numbers look fine');
ok(/insufficient/.test(r.skipped[0].why), '4b ...and the reviewer\'s own reason is surfaced', r.skipped[0].why);

// ================= 5. A VALID PROPOSAL APPLIES EXACTLY ONCE, IDEMPOTENTLY ====
reset();
doc=docWith([ goodProposal() ]);
RULES_DOC=doc; RULES=rulesNormalize({}); RULES['dir.trend5']=ruleSeed('dir.trend5','',''); doc.rules=RULES;
r=applyProposals(doc);
ok(r.applied.length===1, '5a a proposal that truly clears the bar applies', r.applied.length);
ok(DIR_WEIGHTS.trend===4, '5b ...the live weight moved 3 -> 4', DIR_WEIGHTS.trend);
ok(DIR_WEIGHTS_SOURCE==='measured', '5c ...and dirWeightsSource is now MEASURED', DIR_WEIGHTS_SOURCE);
ok(doc.promoted.length===1 && doc.promoted[0].id==='p.trend.up', '5d ...recorded in the document\'s promoted list');
ok(doc.promoted[0].evidence.n===42 && doc.promoted[0].evidence.wf.sessions===3, '5e ...with the evidence that earned it');
ok(doc.promoted[0].evidence.localEffN===42 && doc.promoted[0].evidence.wfLocal===3,
   '5e2 ...alongside what the PANEL itself measured, so the two can be compared later',
   doc.promoted[0].evidence.localEffN+'/'+doc.promoted[0].evidence.wfLocal);
ok(RULES['dir.trend5'].tier==='measured' && RULES['dir.trend5'].promotedOn==='2026-08-18',
   '5f ...and the rule behind it is marked promoted in the document', RULES['dir.trend5'].tier);
ok(ruleTier('dir.trend5')==='📊', '5f2 ...and NOW it renders 📊, because the local eff n also clears the bar', ruleTier('dir.trend5'));
LOCAL_BARS=60; featStatsInvalidate();
ok(ruleTier('dir.trend5')==='⚖', '5f3 ...but drop the LOCAL evidence and it goes straight back to ⚖', ruleTier('dir.trend5'));
LOCAL_BARS=420; featStatsInvalidate();
ok(/📊 promoted 2026-08-18/.test(promoMarker('p.trend.up')), '5g the promoted MARKER is set', promoMarker('p.trend.up'));
ok(/📊 promoted 2026-08-18/.test(promoMarker()), '5h ...and the direction weights carry it too', promoMarker());
ok(promotionsList().length===1, '5i __gptsDebug.promotions() lists it', promotionsList().length);
// IDEMPOTENT
const before=JSON.stringify(promoLoad());
r=applyProposals(doc);
ok(r.applied.length===0, '5j RE-RUNNING the same document applies NOTHING more', r.applied.length);
ok(DIR_WEIGHTS.trend===4, '5k ...the weight did not move again (no double-apply)', DIR_WEIGHTS.trend);
ok(doc.promoted.length===1, '5l ...and the promoted list did not grow', doc.promoted.length);
ok(/already applied on 2026-08-18/.test(r.skipped[0].why), '5m ...it says it was already applied', r.skipped[0].why);
ok(JSON.stringify(promoLoad())===before, '5n ...gpts_promo_v1 is unchanged');
applyProposals(doc); applyProposals(doc);
ok(promotionsList().length===1 && DIR_WEIGHTS.trend===4, '5o ...still true after four passes');

// ================= 6. PERSISTENCE: a NETWORK-LESS boot keeps the weights =====
ok(!!LS['gpts_promo_v1'], '6a the applied state is persisted to gpts_promo_v1');
const savedLS=JSON.parse(JSON.stringify(LS));
// simulate a fresh boot with the promo store but NO rules document at all
LS={ 'gpts_promo_v1':savedLS['gpts_promo_v1'] };
RULES=null; RULES_DOC=null; PROMO=null;
DIR_WEIGHTS={ trend:3, driftAgree:2, driftLean:1, diverge:-2, tentative:1, gradeA:5, gradeB:3 };
DIR_WEIGHTS_SOURCE='hand-set';
rulesLoad();
ok(DIR_WEIGHTS.trend===4, '6b a boot with NO network and NO cached rules still uses the last-applied weight', DIR_WEIGHTS.trend);
ok(DIR_WEIGHTS_SOURCE==='measured', '6c ...and still says the weights are measured', DIR_WEIGHTS_SOURCE);
ok(promotionsList().length===1, '6d ...and still lists what was promoted');

// ================= 7. THE OTHER PROPOSAL KINDS ===============================
// threshold: a grade cut point
reset(); doc=docWith([ goodProposal({ id:'p.cut', kind:'threshold', target:'dir.weights.gradeA', current:5, proposed:6 }) ]);
RULES_DOC=doc; applyProposals(doc);
ok(DIR_WEIGHTS.gradeA===6, '7a a threshold proposal moves the grade cut point', DIR_WEIGHTS.gradeA);
ok(gradeOfScore(5)==='B' && gradeOfScore(6)==='A', '7b ...and gradeOfScore follows it', gradeOfScore(5)+'/'+gradeOfScore(6));
// swap: the one voting-set constant that exists today
reset(); doc=docWith([ goodProposal({ id:'p.swap', kind:'swap', target:'dir.trend.window', current:20, proposed:10,
                                      incumbent:'dir.trend5', challenger:'dir.trendFast' }) ]);
RULES_DOC=doc; r=applyProposals(doc);
ok(r.applied.length===1 && TREND_WINDOW===10, '7c a swap on the trend window is applied live', TREND_WINDOW);
// (v10.54, audit 8) THE DOMINANCE COUNT MOVES WITH THE WINDOW. Moving TREND_WINDOW alone
// turned "15 of 20" into "15 of 10" — unreachable — so a promoted window swap silently
// flattened every trend on the panel.
ok(TREND_DOM===8, '7c2 ...and TREND_DOM follows it (75% of the new window), or the rule becomes unreachable', TREND_DOM);
ok(promoLoad().swaps.length===1 && promoLoad().swaps[0].challenger==='dir.trendFast', '7d ...and recorded with its challenger');
// the SMA PERIOD swap targets CFG.trendMA — it used to match nothing and be recorded inert
reset(); doc=docWith([ goodProposal({ id:'p.sma', kind:'swap', target:'dir.trendMA', current:50, proposed:20,
                                      incumbent:'dir.trend5', challenger:'dir.trendFast' }) ]);
RULES_DOC=doc; r=applyProposals(doc);
ok(r.applied.length===1 && CFG.trendMA.SPY===20 && CFG.trendMA.QQQ===20,
   '7d2 an SMA-period swap moves CFG.trendMA, the constant that actually sets the SMA', CFG.trendMA.SPY);
ok(promoLoad().swaps[0].live===true, '7d3 ...and is recorded as LIVE, not as an inert note');
// a swap whose target does not exist yet is recorded, not silently "applied" to nothing
reset(); doc=docWith([ goodProposal({ id:'p.swap2', kind:'swap', target:'dir.netGamma.vote', current:0, proposed:1,
                                      incumbent:'dir.drift', challenger:'netGamma' }) ]);
RULES_DOC=doc; applyProposals(doc);
ok(promoLoad().swaps[0].live===false, '7e a swap with no live wiring is marked live:false, never faked');
// an unknown weight target is refused outright
reset(); doc=docWith([ goodProposal({ id:'p.bogus', target:'dir.weights.nonsense' }) ]);
RULES_DOC=doc; r=applyProposals(doc);
ok(r.applied.length===0 && /no applicable target/.test(r.skipped[0].why), '7f an unknown target is refused', r.skipped[0].why);

// ================= 8. THE KILL LIST CAPS THE GRADE ===========================
// live inputs that would otherwise grade A: confirmed uptrend + AGREE-UP drift, lower range.
global.STATE={ SPY:{ price:772.6, candles:[], lastClosedB:1 } };
global.trendVerdict=function(){ return { state:'up', up:16, dn:1, win:20, slope:0.4 }; };
let DRIFT={ verdict:'AGREE-UP', dir:1, gvwap:773.9, vvwap:775.0, overlap:true };
global.driftRead=function(){ return DRIFT; };
let NET={ bias:'support-heavy', dir:1, ratio:2, decisive:true };
global.netPositioning=function(){ return NET; };
global.regimeTag=function(){ return { tag:'trend', er:0.6 }; };
global.nodeMapModel=function(){ return { ok:true, px:772.6, flr:{k:772}, ceil:{k:778}, levels:[], kingK:775 }; };
global.closedCandles=function(){ return []; };
global.PAL={ sub:'#8b98a9' };
global.DECISION_MATRIX={ A:{A:'take · follow-thru',B:'take · tight tgt',C:'wait fresher node'},
                         B:{A:'bounce play',B:'scalp',C:'skip'},
                         C:{A:'scalp only',B:'skip',C:'stand aside'} };

reset(); doc=docWith([], [{ id:'kill.noConf', condition:'driftVerdict=SPLIT & structAsym=opposed', tier:'hand', promoted:false }]);
RULES_DOC=doc; RULES=rulesNormalize({}); doc.rules=RULES;
DRIFT={ verdict:'AGREE-UP', dir:1, gvwap:773.9, vvwap:775.0, overlap:true };
let d=directionGrade('SPY');
ok(d.grade==='A' && d.kill===null, '8a baseline: this tape grades A with no kill condition matching', d.grade);
// the condition now MATCHES but the kill entry is only hand-set: it must change nothing
DRIFT={ verdict:'SPLIT', dir:0, overlap:false }; NET={ bias:'resistance-heavy', dir:-1, ratio:3, decisive:true };
d=directionGrade('SPY');
ok(d.kill===null, '8b a matching but HAND-SET kill entry caps nothing', d.grade+'/'+d.kill);
ok(killActive('kill.noConf')===false, '8c ...killActive says so');
// promote it through the bar and it caps
reset();
doc=docWith([ goodProposal({ id:'p.kill.noConf', kind:'kill', target:'kill.noConf',
                             condition:'driftVerdict=SPLIT & structAsym=opposed', rate:31 }) ],
            [{ id:'kill.noConf', condition:'driftVerdict=SPLIT & structAsym=opposed', tier:'hand', promoted:false }]);
RULES_DOC=doc; RULES=rulesNormalize({}); doc.rules=RULES;
r=applyProposals(doc);
ok(r.applied.length===1 && killActive('kill.noConf')===true, '8d a kill proposal past the bar becomes ACTIVE');
ok(doc.killList[0].promoted===true && doc.killList[0].tier==='measured', '8e ...and is marked promoted in the kill list');
d=directionGrade('SPY');
ok(d.kill && d.kill.id==='kill.noConf', '8f ...the matching bar now reports the kill', d.kill&&d.kill.id);
ok(d.grade==='C', '8g ...and the grade is CAPPED at C', d.grade);
ok(d.noOdds===true, '8h ...with no odds claim allowed');
ok(/no confluence/.test(d.capped||''), '8i ...the cap names the condition', d.capped);
// the decision line says why, descriptively
global.inPlayZone=function(){ return null; };
global.nodeGrade=function(){ return { grade:'B', disp:'B', score:3, tier:'⚖', k:775, kill:null, inputs:{} }; };
let dc=decisionCell('SPY');
ok(/\(kill: no confluence\)/.test(dc.text), '8j the decision line appends the reason, descriptively', dc.text);
ok(!/\b(buy|sell|long|short|stop|size|entry)\b/i.test(dc.text), '8k ...and stays free of order words');
// a NON-matching bar is untouched by the same active kill
DRIFT={ verdict:'AGREE-UP', dir:1, gvwap:773.9, vvwap:775.0, overlap:true }; NET={ bias:'support-heavy', dir:1, ratio:2, decisive:true };
d=directionGrade('SPY');
ok(d.kill===null && d.grade==='A', '8l an active kill only fires on bars that MATCH it', d.grade);
// node-scope kill: the 3rd tap
reset();
doc=docWith([ goodProposal({ id:'p.kill.tap3', kind:'kill', target:'kill.tap3', condition:'tap>=2', rate:33 }) ], []);
RULES_DOC=doc; RULES=rulesNormalize({}); doc.rules=RULES;
applyProposals(doc);
ok(killActive('kill.tap3')===true, '8m the 3rd-tap kill can be promoted the same way');
ok(killCheck('node',{tap:2,pol:'+',path:'wall'}) && killCheck('node',{tap:2}).reason==='kill: 3rd tap',
   '8n ...and a 3rd tap reports "kill: 3rd tap"', killCheck('node',{tap:2}).reason);
ok(killCheck('node',{tap:1})===null, '8o ...while a 2nd tap does not');
ok(killCheck('dir',{rangeZone:'mid'})===null, '8p ...and a node-scope kill never fires on the direction scope');

// ================= 9. SOURCE INVARIANTS ======================================
ok(/var PROMO_MIN_N=20/.test(src), '9a the n bar is 20, in code');
ok(/var PROMO_WF_SESSIONS=3/.test(src), '9b the walk-forward bar is 3 NEW sessions, in code');
const AP=ex('applyProposals');
ok(/proposalClearsBar\(p\)/.test(AP), '9c applyProposals RE-CHECKS the bar itself — it never trusts clearsBar alone');
ok(/P\.applied\[p\.id\]/.test(AP), '9d ...and guards on the applied set for idempotency');
ok(/promoSave\(\)/.test(AP), '9e ...and persists what it applied');
ok(/__gptsDebug\.promotions/.test(src), '9f __gptsDebug.promotions() exists');
ok(/promoMarker/.test(ex('directionGrade')), '9g the READ direction grade carries the promoted marker');
ok(/killCheck\('dir'/.test(ex('directionGrade')), '9h directionGrade consults the kill list');
ok(/killCheck\('node'/.test(ex('nodeGrade')), '9i nodeGrade consults the kill list');
ok(/kk\.reason/.test(ex('decisionCell')), '9j the decision line carries the kill reason');
const PCB=ex('proposalClearsBar');
ok(/proposalLocalN/.test(PCB), '9k the bar is derived from LOCAL n, not from the proposal');
ok(/proposalWalkForwardLocal/.test(PCB), '9l ...and the walk-forward from LOCAL sessions');
ok(!/p\.wf|p\.walkForward/.test(PCB), '9m ...the document\'s own wf block is not consulted at all');
ok(/carries no n/.test(PCB) && !/dev>0\.20/.test(PCB), '9n ...a proposal must carry an n; the 20% self-report test is gone (v11.0 G4)');

// ================= 10. DEMOTION — promotion is no longer one-way ==============
// (v10.54, audit 24) Nothing that cleared the bar could ever be taken back, so a rule
// that stopped working kept moving the weights forever.
reset(); doc=docWith([ goodProposal() ]); RULES_DOC=doc; RULES=rulesNormalize({}); doc.rules=RULES;
applyProposals(doc);
ok(DIR_WEIGHTS.trend===4, '10a start from a promoted weight', DIR_WEIGHTS.trend);
LOCAL_BARS=420; LOCAL_HIT=126; featStatsInvalidate();        // 30% — under the 50% baseline
let dc1=promoDemoteCheck(true);
ok(dc1.struck.length===1 && dc1.demoted.length===0, '10b one weekly check under the bar = one strike, not a demotion');
ok(DIR_WEIGHTS.trend===4, '10c ...the weight is still applied after one strike', DIR_WEIGHTS.trend);
promoDemoteCheck(true);
let dc3=promoDemoteCheck(true);
ok(dc3.demoted.length===1, '10d three consecutive weekly checks under the bar DEMOTE it', dc3.demoted.join(','));
ok(DIR_WEIGHTS.trend===3, '10e ...the weight reverts to the hand-set value', DIR_WEIGHTS.trend);
ok(DIR_WEIGHTS_SOURCE==='hand-set', '10f ...and the source goes back to hand-set', DIR_WEIGHTS_SOURCE);
ok(promotionsList().length===0, '10g ...it is no longer listed as promoted');
ok(promoEvents().some(e=>e.kind==='demotion'), '10h ...and the demotion is LOGGED as an event');
ok(promoEvents().some(e=>e.kind==='promotion'), '10i ...beside the promotion that preceded it');
// a promoted item that keeps WORKING is never demoted
reset(); doc=docWith([ goodProposal() ]); RULES_DOC=doc; RULES=rulesNormalize({}); doc.rules=RULES;
applyProposals(doc);
LOCAL_BARS=420; LOCAL_HIT=270; featStatsInvalidate();        // 64% — over the baseline
promoDemoteCheck(true); promoDemoteCheck(true); promoDemoteCheck(true); promoDemoteCheck(true);
ok(DIR_WEIGHTS.trend===4 && promotionsList().length===1, '10j a promoted item that keeps working is never demoted', DIR_WEIGHTS.trend);

// ================= 11. THE MODEL IS FROZEN MID-SESSION ========================
// (v10.54, audit 6) rulesApply moved live weights the instant a fetch landed, so two
// bars five minutes apart could be scored by two different models with nothing on the
// record to say so.
reset(); RULES_APPLIED_AT=Date.now();
ctNowSecOfDay=function(){ return 11*3600; };                 // 11:00 CT, mid-session
ok(rulesApplyAllowed()===false, '11a mid-session the model may not move');
let mid=rulesApply();
ok(mid.deferred===true && mid.applied.length===0, '11b ...so rulesApply defers instead of applying');
ctNowSecOfDay=function(){ return 15*3600+120; };             // after the close
ok(rulesApplyAllowed()===true, '11c after the close it may move again');
ctNowSecOfDay=function(){ return 7*3600; };
ok(rulesApplyAllowed()===true, '11d ...and before the open');
ok(/rulesAsOf/.test(JSON.stringify(modelStamp())) && modelStamp().weightsHash!=null,
   '11e every record can be stamped with {rulesAsOf, weightsHash}', JSON.stringify(modelStamp()));
const h0=weightsHash();
DIR_WEIGHTS.trend=7;
ok(weightsHash()!==h0, '11f the hash CHANGES when a live weight changes', h0+' -> '+weightsHash());
DIR_WEIGHTS.trend=3;
ok(weightsHash()===h0, '11g ...and comes back when it is restored');

console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
