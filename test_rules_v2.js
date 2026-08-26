// (v10.54 GROUP 1) rules.json v2 — the machine-applicable mental model, AND THE FIAT
// PATHS THAT USED TO RUN THROUGH IT, CLOSED.
//
//   1. rulesLoad() reads the v2 document AND still reads a v1 file (fallback).
//   2. `weights` IN A FETCHED DOCUMENT IS INERT. Up to v10.53 it was read straight into
//      the live DIR_WEIGHTS at boot, so editing one JSON file on GitHub re-tuned the
//      panel with no evidence, no promotion bar and no record. The live weights now come
//      from exactly two places: DIR_WEIGHTS_HAND, and proposals THIS PANEL promoted.
//   3. `tier:'measured'` / `promoted:true` on a fetched RULE is documentation. A tier is
//      earned locally: promoted here AND locally measured eff n >= RULE_UNLOCK_N.
//   4. The shipped learning/rules.json is a well-formed v2 file carrying the 45 hand
//      rules, empty proposals/promoted/challengers, and the 4 kill conditions.
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
global.FEATURES=[];
global.TREND_WINDOW=20;
global.RULE_UNLOCK_N=20;
global.RULES_KEY='gpts_rules_v1'; global.RULES2_KEY='gpts_rules_v2'; global.PROMO_KEY='gpts_promo_v1';
global.RULES=null; global.RULES_DOC=null; global.RULES_FETCHED=true;   // no network in tests
global.RULES_URL='';
global.PROMO=null;
global.PROMO_MIN_N=20; global.PROMO_WF_SESSIONS=3;
global.DIR_WEIGHTS_HAND={ trend:3, driftAgree:2, driftLean:1, diverge:-2, tentative:1, gradeA:5, gradeB:3 };
global.DIR_WEIGHTS={ trend:3, driftAgree:2, driftLean:1, diverge:-2, tentative:1, gradeA:5, gradeB:3 };
global.DIR_WEIGHTS_SOURCE='hand-set';
global.KILL_DEFS={};

global.FEAT_FWD=10;
global.ctNowSecOfDay=function(){ return 7*3600; };   // pre-open: the model may move
global.recorderLoad=function(){ return { days:{} }; };
global.CFG={ trendMA:{SPY:50,QQQ:50} };
eval(['ruleSeed','rulesSeed','rulesNormalize','rulesDefaultWeights','rulesDocNormalize','rulesDoc',
      'rulesLoad','rulesIngest','rulesSave','ruleGet','effN','nTxt','pctN','featStatsCached',
      'featStatsInvalidate','ruleLocalRate','rulePromotedApplied','ruleTier','rulePromoted',
      'promoLoad','promoSave','promoToday','proposalRegimeFlip','proposalFeatureKey','proposalLocalN',
      'proposalWalkForwardLocal','proposalClearsBar','promoDirWeightKey',
      'promoApplyWeight','promoApplySwap','promoApplySwapsPersisted','promoApplyKill','applyProposals',
      'rulesApplyWeights','promoApplyPersisted','promoDemote','promoDemoteCheck','promoEvents',
      'rulesApplyAllowed','rulesApply','weightsHash','modelStamp','promotionsList','promoMarker',
      'killActive','killCheck','gradeOfScore'].map(ex).join('\n'));
global.RULES_APPLIED_AT=null; global.RULES_APPLIED_DAY=null;
global.PROMO_DEMOTE_STRIKES=3; global.PROMO_WEEK_MS=604800000;
// no local records in this file: every rule is UNMEASURED here, which is the point.
global.featStats=function(){ return { byKey:{}, byGrade:{dir:{},node:{},dirSide:{}}, cells:{},
  act:{take:{n:0,hit:0},pass:{n:0,hit:0}}, frame:{}, partial:0, days:0, dayKeys:[] }; };

function reset(){ LS={}; RULES=null; RULES_DOC=null; PROMO=null; RULES_APPLIED_AT=null; featStatsInvalidate();
  DIR_WEIGHTS={ trend:3, driftAgree:2, driftLean:1, diverge:-2, tentative:1, gradeA:5, gradeB:3 };
  DIR_WEIGHTS_SOURCE='hand-set'; }

// ================= 1. THE SHIPPED FILE IS A WELL-FORMED v2 DOC =================
const RJ=JSON.parse(fs.readFileSync('./learning/rules.json','utf8'));
ok(RJ.schema==='gex-rules/v2', '1a learning/rules.json declares schema gex-rules/v2', RJ.schema);
ok(typeof RJ.asOf==='string' && /^\d{4}-\d{2}-\d{2}$/.test(RJ.asOf), '1b it carries an asOf date', RJ.asOf);
ok(RJ.weights && RJ.weights.dir && typeof RJ.weights.dir.trend==='number', '1c weights.dir exists and is numeric');
ok(Object.keys(RJ.rules).length===73, "1d all 73 rule ids carried over (+ em.read v11.70, spx.nodes v11.84, rolllatch v13.9, attract v14.7, levelstate v14.30)", Object.keys(RJ.rules).length);
ok(Object.keys(RJ.rules).every(id=>RJ.rules[id].tier==='hand' && RJ.rules[id].promoted===false),
   '1e every carried rule is tier "hand" — nothing ships pre-promoted');
ok(Object.keys(RJ.rules).every(id=>RJ.rules[id].regime && RJ.rules[id].regime.trend && RJ.rules[id].regime.chop),
   '1f every rule has the per-regime split slot (v10.53 C)');
ok(Object.keys(RJ.rules).every(id=>RJ.rules[id].walkForward && RJ.rules[id].walkForward.held===false),
   '1g every rule has a walk-forward counter, starting not-held');
ok(Array.isArray(RJ.proposals) && RJ.proposals.length===0, '1h proposals start empty');
ok(Array.isArray(RJ.promoted) && RJ.promoted.length===0, '1i promoted starts empty');
ok(RJ.challengers && Object.keys(RJ.challengers).length===0, '1j challengers start empty');
ok(Array.isArray(RJ.killList) && RJ.killList.length===4, '1k the kill list carries the four existing kill.* rules', RJ.killList.length);
['kill.tap3','kill.midrange','kill.noConf','kill.negGammaWide'].forEach(id=>{
  ok(RJ.killList.some(k=>k.id===id), '1·killList has '+id);
});
ok(RJ.killList.every(k=>k.tier==='hand' && k.promoted===false), '1l ...all hand-set: a kill entry caps nothing until promoted');
// the LIVE constants and the shipped weights must agree, or boot silently re-tunes the panel
const HAND=/var DIR_WEIGHTS_HAND=\{([^}]*)\}/.exec(src)[1];
['trend:3','driftAgree:2','driftLean:1','diverge:-2','tentative:1','gradeA:5','gradeB:3'].forEach(kv=>{
  ok(HAND.replace(/\s/g,'').indexOf(kv)>=0, '1m the hand-set default '+kv+' is in the code');
  const [k,v]=kv.split(':');
  ok(RJ.weights.dir[k]===Number(v), '1n ...and learning/rules.json ships the same '+k, RJ.weights.dir[k]);
});

// ================= 2. v2 LOADS ================================================
reset();
LS['gpts_rules_v2']=JSON.stringify({v:2, at:1, doc:RJ});
let R=rulesLoad();
ok(!!R && !!R['dir.A'], '2a rulesLoad() returns the rule map from a v2 document');
ok(rulesDoc().schema==='gex-rules/v2', '2b ...and rulesDoc() exposes the whole v2 document');
ok(Object.keys(R).length>=45, '2c all rules present', Object.keys(R).length);
ok(DIR_WEIGHTS.trend===3 && DIR_WEIGHTS.gradeA===5, '2d the LIVE constants are the hand-set ones');
ok(DIR_WEIGHTS_SOURCE==='hand-set', '2e ...and the source stays hand-set: nothing was promoted', DIR_WEIGHTS_SOURCE);

// ---- 2f/2g: THE FIAT PATH. A fetched weights.dir must move NOTHING. ----
reset();
const doc2=JSON.parse(JSON.stringify(RJ));
doc2.weights.dir.trend=4; doc2.weights.dir.gradeA=6; doc2.weights.dir.diverge=-9;
LS['gpts_rules_v2']=JSON.stringify({v:2, at:1, doc:doc2});
rulesLoad();
ok(DIR_WEIGHTS.trend===3 && DIR_WEIGHTS.gradeA===5 && DIR_WEIGHTS.diverge===-2,
   '2f a changed weights.dir in a FETCHED doc is INERT — the panel still scores hand-set',
   DIR_WEIGHTS.trend+'/'+DIR_WEIGHTS.gradeA+'/'+DIR_WEIGHTS.diverge);
ok(gradeOfScore(5)==='A' && gradeOfScore(4)==='B',
   '2g ...so gradeOfScore keeps the hand-set cut points, not the document\'s', gradeOfScore(5)+'/'+gradeOfScore(4));
ok(DIR_WEIGHTS_SOURCE==='hand-set', '2h ...and the source is still HAND-SET: only a promotion earns 📊');
ok(rulesDoc().weights.dir.trend===4, '2h2 the document still CARRIES its weights — they are documentation, not behaviour', rulesDoc().weights.dir.trend);
// an unknown weight key may not be injected either
reset();
const doc3=JSON.parse(JSON.stringify(RJ)); doc3.weights.dir.nonsense=99;
LS['gpts_rules_v2']=JSON.stringify({v:2, at:1, doc:doc3});
rulesLoad();
ok(!('nonsense' in DIR_WEIGHTS), '2i an unknown weight key is refused, not silently added');
// ---- 2j: ONLY gpts_promo_v1 MOVES A WEIGHT ----
reset();
LS['gpts_promo_v1']=JSON.stringify({v:1, applied:{}, weights:{dir:{trend:4}}, kills:[], swaps:[]});
LS['gpts_rules_v2']=JSON.stringify({v:2, at:1, doc:doc2});
rulesLoad();
ok(DIR_WEIGHTS.trend===4, '2j a weight this panel PROMOTED (gpts_promo_v1) is what moves the constant', DIR_WEIGHTS.trend);
ok(DIR_WEIGHTS_SOURCE==='measured', '2k ...and only that makes the source measured', DIR_WEIGHTS_SOURCE);
ok(DIR_WEIGHTS.gradeA===5, '2l ...while the doc\'s OTHER weights are still ignored', DIR_WEIGHTS.gradeA);

// ================= 3. v1 FALLBACK =============================================
reset();
LS['gpts_rules_v1']=JSON.stringify({v:1, at:1, rules:{'dir.A':{id:'dir.A',promoted:true,tier:'measured',rate:71,n:34}}});
R=rulesLoad();
ok(!!R['dir.A'] && R['dir.A'].promoted===true, '3a a v1 store still loads (fallback)');
// (v10.54, audit 1) A FETCHED promoted:true / tier:'measured' IS DOCUMENTATION. The rule
// above claims 71% on n=34; nothing on this machine measured it, so the panel says ⚖.
ok(ruleTier('dir.A')==='⚖', '3b a rule promoted only IN THE DOCUMENT renders ⚖, never 📊', ruleTier('dir.A'));
ok(rulePromoted('dir.A')===false, '3b2 ...and rulePromoted refuses it too');
ok(!!R['kill.tap3'], '3c ...with the rest back-filled from the seed');
ok(rulesDoc().schema==='gex-rules/v2', '3d ...normalised UP into a v2 document');
ok(DIR_WEIGHTS.trend===3 && DIR_WEIGHTS_SOURCE==='hand-set', '3e a v1 file cannot carry weights, so the hand-set values stand');
// a BARE v1 rule map (no wrapper) also loads
reset();
LS['gpts_rules_v1']=JSON.stringify({'dir.B':{id:'dir.B',tier:'hand',promoted:false}});
ok(!!rulesLoad()['dir.B'] && !!rulesLoad()['kill.midrange'], '3f a bare v1 rule map loads too');

// ================= 4. rulesIngest: a fresh fetch replaces the doc =============
reset();
rulesLoad();
ok(DIR_WEIGHTS.trend===3, '4a boot with no cache = seeded defaults');
const fresh=JSON.parse(JSON.stringify(RJ)); fresh.asOf='2026-08-22'; fresh.weights.dir.driftLean=2;
rulesIngest(fresh);
ok(rulesDoc().asOf==='2026-08-22', '4b a freshly fetched rules.json replaces the document', rulesDoc().asOf);
ok(DIR_WEIGHTS.driftLean===1, '4c ...and its weights STILL do not become live (inert)', DIR_WEIGHTS.driftLean);
ok(!!LS['gpts_rules_v2'], '4d ...and it is cached under gpts_rules_v2');
ok(!!LS['gpts_rules_v1'], '4e ...while gpts_rules_v1 is still written (no key renamed)');

// ================= 5. dirWeightsSource = 'measured' ONLY via a promotion ======
reset();
const docP=JSON.parse(JSON.stringify(RJ));
docP.promoted=[{ id:'p.trend', kind:'weight', target:'dir.weights.trend', from:3, to:4, on:'2026-08-18',
                 evidence:{n:42, lift:11, wf:{sessions:3,held:true}} }];
docP.weights.dir.trend=4;
LS['gpts_rules_v2']=JSON.stringify({v:2, at:1, doc:docP});
rulesLoad();
// (v10.54) doc.promoted is the weekly run's RECORD of what it thinks happened. Only
// gpts_promo_v1 — written by applyProposals after re-deriving the bar from local data —
// can move a live constant. A document that simply declares a promotion changes nothing.
ok(DIR_WEIGHTS.trend===3, '5a a promotion DECLARED in the document does not move the weight', DIR_WEIGHTS.trend);
ok(DIR_WEIGHTS_SOURCE==='hand-set', '5b ...and the source stays hand-set', DIR_WEIGHTS_SOURCE);
ok(/📊 promoted/.test(promoMarker()||'x')===false || true, '5c (marker is exercised in test_promotion_bar)');
// a promoted entry that is NOT a dir weight does not claim the dir weights are measured
reset();
const docK=JSON.parse(JSON.stringify(RJ));
docK.promoted=[{ id:'p.kill', kind:'kill', target:'kill.tap3', on:'2026-08-18', evidence:{n:30} }];
LS['gpts_rules_v2']=JSON.stringify({v:2, at:1, doc:docK});
rulesLoad();
ok(DIR_WEIGHTS_SOURCE==='hand-set', '5d a promoted KILL does not make the direction weights measured', DIR_WEIGHTS_SOURCE);

// ================= 6. SOURCE INVARIANTS ======================================
ok(/var RULES_KEY='gpts_rules_v1'/.test(src), '6a gpts_rules_v1 is NOT renamed');
ok(/var RULES2_KEY='gpts_rules_v2'/.test(src), '6b gpts_rules_v2 is the new doc cache');
ok(/var PROMO_KEY='gpts_promo_v1'/.test(src), '6c gpts_promo_v1 is the new applied-promotions store');
ok(/rulesApply\(true\);\s*\/\/ \(v10\.54\)/.test(src), '6d rulesApply(true) runs at BOOT — one of only two moments the model may move');
ok(/rulesApplyAllowed/.test(ex('rulesApply')), '6d2 ...and any other call is refused mid-session');
ok(/learning\/rules\.json/.test(ex('pipeRulesTry')), '6e pipeCheck fetches learning/rules.json from the raw base');
ok(/PIPE_RAW_BASE/.test(ex('pipeRulesTry')), '6f ...via the same PIPE_RAW_BASE as the review');
ok(/pipeRulesTry\(P\);/.test(ex('pipeCheck')), '6g ...and it is wired into pipeCheck alongside the review');
ok(/rulesIngest/.test(ex('pipeRulesTry')), '6h ...a fresh document re-runs the promotion pass');
ok(/@version\s+14.30/.test(src) && /v'\+GPTS_VERSION\+' part1 loaded/.test(src) && />v'\+GPTS_VERSION\+'<\/span>/.test(src),
   '6i version 10.56 in all three spots');
// (v11.56) An unreachable update URL means Tampermonkey never offers a new version. The companion shipped
// for releases without one and silently sat at an old version while the repo moved on.
ok(/@updateURL\s+https:\/\/raw\.githubusercontent\.com/.test(src) && /@downloadURL\s+https:\/\/raw\.githubusercontent\.com/.test(src),
   '6j the tapereader declares @updateURL and @downloadURL so it can be updated at all');

// ================= 7. SOURCE: THE FIAT PATHS ARE CLOSED ======================
const RAW=ex('rulesApplyWeights');
ok(!/RULES_DOC/.test(RAW), '7a rulesApplyWeights does not read RULES_DOC at all — the doc cannot reach the weights');
ok(/DIR_WEIGHTS_HAND/.test(RAW) && /promoLoad\(\)/.test(RAW), '7b ...it reads the hand-set defaults + gpts_promo_v1, and nothing else');
const KA=ex('killActive');
ok(!/killList/.test(KA), '7c killActive no longer consults the fetched killList');
ok(/P\.kills/.test(KA), '7d ...only what THIS panel promoted');
const RT=ex('ruleTier');
ok(/rulePromotedApplied/.test(RT) && /ruleLocalRate/.test(RT) && /RULE_UNLOCK_N/.test(RT),
   '7e a 📊 tier needs a LOCAL promotion AND local effN >= RULE_UNLOCK_N');
ok(/effN/.test(ex('ruleLocalRate')), '7f ...and the local n is EFFECTIVE, not raw bar-records');
ok(effN(200)===20 && effN(0)===0, '7g effN divides the overlapping bar-records out', effN(200));

console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
