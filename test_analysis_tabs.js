// (v10.54 GROUP 5) THE ANALYSIS + TESTING TABS, REBUILT AROUND THE TRADER'S QUESTIONS.
//
// Both tabs had grown by accretion: whichever block shipped last went on top, every
// section was always expanded, half the numbers were percentages with no n beside them,
// and the jargon (lift, walk-forward, effective n, monotone) was never explained where it
// was used. At 250px the result was an unreadable wall, and the two things a trader
// actually wants — "did it tell the truth today" and "what is being tested" — were
// nowhere near the top.
//
// This file pins the CONTRACT of the rebuild rather than pixel layout:
//   · the exact section list, in order
//   · collapsibility, and that a closed section renders no body
//   · ONE honest line in every empty state (never a blank, never a fabricated zero)
//   · NO % WITHOUT ITS n, anywhere either tab renders a rate
//   · question-first hovers
//   · a "?" guide per tab
//   · 250px-safe markup (no nowrap on prose, no fixed pixel widths on tables)
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m+(g!==undefined?' -> '+g:''));} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+g:''));} };

// ---------------- mocks: enough to actually RENDER both tabs ----------------
let LS={};
global.localStorage={ getItem:k=>(k in LS?LS[k]:null), setItem:(k,v)=>{LS[k]=String(v);} };
global.window={ __gptsDebug:{} };
global.document={ getElementById:function(){ return null; } };
global.PAL={ bg:'#0b0e14', card:'#12161f', line:'#1e2530', longAccent:'#2ec27e', shortAccent:'#f0616d',
             ink:'#e6edf3', sub:'#8b98a9', amber:'#f2b45a', gold:'#e3c341', blue:'#4a90d9' };
global.FEAT_FWD=10; global.DIR_PTS=0.5;
global.RULE_UNLOCK_N=20; global.PROMO_MIN_N=20; global.PROMO_WF_SESSIONS=3;
global.DIR_WEIGHTS={ trend:3, driftAgree:2, driftLean:1, diverge:-2, tentative:1, gradeA:5, gradeB:3 };
global.DIR_WEIGHTS_SOURCE='hand-set';
global.DECISION_MATRIX={ A:{A:'take · follow-thru',B:'take · tight tgt',C:'wait fresher node'},
                         B:{A:'bounce play',B:'scalp',C:'skip'},
                         C:{A:'scalp only',B:'skip',C:'stand aside'} };
global.KILL_DEFS={ 'kill.tap3':{label:'3rd tap',scope:'node',test:function(){return false;}} };
global.ANALYSIS_SYM='SPY'; global.ANALYSIS_REVIEW=null; global.LOADED_DAY=null; global.SAVED_TODAY=null;
global.FEATURES=[];
global.ctTodayStr=function(){ return '2026-08-18'; };
global.render=function(){};

// the data knobs this file drives
var STATS=null, DAYS={}, PROPS=[], CHALS={}, KILLS=[], EVENTS=[], QUESTIONS=[];
global.featStats=function(){ return STATS; };
global.recorderLoad=function(){ return { days:DAYS }; };
global.rulesDoc=function(){ return { asOf:'2026-08-17', proposals:PROPS, challengers:CHALS, killList:KILLS, promoted:[] }; };
global.promoEvents=function(){ return EVENTS; };
global.promotionsList=function(){ return []; };
global.promoLoad=function(){ return { applied:{}, weights:{}, kills:[], swaps:[], events:EVENTS }; };
global.killActive=function(){ return false; };
global.rulePromoted=function(){ return false; };
global.seedQuestions=function(){ return QUESTIONS; };
global.registerCoreFeatures=function(){ return FEATURES; };
global.proposalRegimeFlip=function(){ return null; };
global.proposalClearsBar=function(){ return { ok:false, why:'insufficient local evidence — eff n=0 (n=0 bars → eff 0), need 20' }; };
global.proposalLocalN=function(){ return { n:0, effN:0, rate:null, key:'dir' }; };
global.proposalWalkForwardLocal=function(){ return { sessions:0, days:[], checked:0 }; };
global.analysisStats=function(){ return { date:'2026-08-18', bars:0, ready:0, nodes:[], matrix:{}, perSig:{} }; };
global.dirFactorsHtml=function(){ return '<div>FACTORS</div>'; };
global.learningStripsHtml=function(){ return '<div>STRIPS</div>'; };
global.featureScorecardsHtml=function(){ return '<div>SCORECARDS</div>'; };
global.projScorecardHtml=function(){ return '<div>PROJ</div>'; };
global.A_renderTop=function(){ return '<div>TOP</div>'; };
global.pipeStages=function(){ return [{key:'rec',label:'rec',state:'green',tip:'Is it recording? yes.'},
                                      {key:'saved',label:'none',state:'red',tip:'Is today’s data written where git can see it? not yet.'},
                                      {key:'pushed',label:'pushed',state:'grey',tip:'Has today’s data reached GitHub? unknown.'},
                                      {key:'review',label:'review',state:'grey',tip:'Did the nightly review run and come back? unknown.'}]; };
global.pipeColor=function(s){ return s==='green'?PAL.longAccent:(s==='amber'?PAL.amber:(s==='red'?PAL.shortAccent:PAL.sub)); };
global.pipeLoad=function(){ return {}; };
global.pipeReviewLine=function(j){ return j&&j.headline||null; };
global.fcHistSessions=function(){ return 2; };
global.studyLoad=function(){ return { src:'local repo', bars:0 }; };
global.studyMineLoad=function(){ return null; };
global.testingInsights=function(){ return { says:[], change:[], improve:[], next:[] }; };
global.recoTestsHtml=function(){ return '<div>RECO</div>'; };
global.T_PRESETS=[{id:'orbit',label:'King ORBIT'}];

eval(['effN','nTxt','pctN','_fpct','gradeMonotone','featStatsCached','featStatsInvalidate','ruleLocalRate',
      'secOpen','secToggle','tabSection','tabEmpty','tabTile','tabGuideBtn','tabHeader','tabGuide',
      // (v15.62) the mockups' chrome
      'panelScale','setPanelScale','panOpen','panSection','panRow','panNote','panFoot',
      'deflStats','deflTableHtml','deflectionsSectionHtml','selfTestDay','selfTestRun','selfTestHtml',
      'unlockRowsHtml','yourCallsHtml','proposalsQueueHtml','challengersHtml2',
      'killListHtml2','featPredBand','canFailHtml','preregStats','preregHtml','wilsonCI','hodlodCalib','hodlodSectionHtml','analysisBlock','testingBlock',
      // (v15.55) the tab by subject, the TRACK field, the Testing loop
      'studiesLoad','sweepsLoad','studiesFlat','studiesCounts','rateTxt','pctOf','ctrlTxt','requestsLoad','requestsSave','requestsAdd','requestsRemove','requestStatus','requestsExport','trackFieldHtml',
      'showSubject','subjectStripHtml','studyRowHtml','sweepTableHtml','sweepsBookLoad','subjectPanelHtml','analysisSubjectsHtml','nightlyReviewHtml','gateStateTxt','rulesTierCounts','testingLoopStripHtml','dashboardRulesHtml','readNextQueueHtml'].map(ex).join('\n'));
// (v15.55) the registry seed and the small constants the subject panel reads
eval(src.slice(src.indexOf('var STUDIES_KEY='), src.indexOf('function studiesLoad(')));
global.ANALYSIS_SUBJ='H'; global.ANALYSIS_NIGHTLY=null; global.RATE_MIN_N=15; global.sweepsBookLoad=function(){ return null; }; global.featGated=function(){ return null; }; global.ruleTier=function(){ return '⚖'; }; global.rulesLoad=function(){ return { rules:{} }; };
global.HYP_STUDY=HYP_STUDY; global.STUDIES_SEED=STUDIES_SEED; global.STUDY_STATUS_COL=STUDY_STATUS_COL; global.STUDIES_KEY=STUDIES_KEY; global.SWEEPS_KEY=SWEEPS_KEY; global.REQUESTS_KEY=REQUESTS_KEY; global.ASUBJ_KEY=ASUBJ_KEY;
// (v15.54) ① HOD / LOD: no session read in the harness -> its own honest empty line
global.hodLod=function(){ return { ok:false, why:'no bars' }; }; global.lodhodCall=function(){ return null; }; global.hlClock=function(){ return '—'; }; global.hlDur=function(){ return '—'; };
global.HLTAB_META={ sessions:284, first:'2025-06-02', last:'2026-08-21', notIn:20 }; global.preregList=function(){ return []; };
// (v15.52) the two new Testing sections and what they lean on
global.g3esc=function(s){ return String(s==null?'':s); }; global.g3tip=function(){ return ''; };
global.PREREG_FROM='2026-09-03'; global.DEFL_ARCH_N=0; global.FEAT_ARCHIVE={};
global.PREREG=[{ id:'H1', claim:'x', minN:40, note:'', pick:function(s){ return s.gradeA; } }];
global.TAB_SECTIONS={}; global.TAB_GUIDE={}; global.SELFTEST_LAST=null;
// (v15.62) the mockups' chrome leans on these
global.PANEL_SCALE_KEY='gpts_tabscale_v1'; global.PANEL_SCALES=[1,1.55,2.1]; global.PANEL_CSS=''; global.ensureV3Css=function(){}; global.loopStatus=function(){ return {}; }; global.LEARN_VIEW=false;

function emptyStats(){ return { byKey:{}, byGrade:{dir:{},node:{},dirSide:{}}, cells:{},
  act:{take:{n:0,hit:0,mfe:0,mae:0,mn:0},pass:{n:0,hit:0,mfe:0,mae:0,mn:0}}, frame:{}, partial:0, days:0, dayKeys:[] }; }
function reset(){ TAB_SECTIONS={}; TAB_GUIDE={}; SELFTEST_LAST=null; STATS=emptyStats(); DAYS={};
  PROPS=[]; CHALS={}; KILLS=[]; EVENTS=[]; QUESTIONS=[]; featStatsInvalidate(); }

// ================= 1. THE ANALYSIS TAB RENDERS ①-⑦, IN ORDER ================
reset();
var A=analysisBlock();
ok(typeof A==='string' && A.length>0, '1a the Analysis tab renders');
// (v15.55) THE TAB BY SUBJECT — design/ANALYSIS-TESTING-BY-SUBJECT.md: the subject strip, then the selected
// subject's subsections (H by default: H1 … H7), the live evidence inside them, the TRACK field under them.
var ASECS=[['K','KINGS'],['H','HOD / LOD'],['H1','Is the extreme in'],['H2','SWEEPS'],['TRACK SOMETHING UNDER','H']];
// (v15.62) the mockup's section header renders the number and the title as two spans
var hasSec=function(h,p){ var a=h.indexOf(p[0]+' '+p[1]); if(a>=0) return a; return h.indexOf('>'+p[0]+'</span><span class="t">'+p[1]); };
var lastAt=-1, inOrder=true;
ASECS.forEach(function(p){
  var at=hasSec(A,p);
  ok(at>=0, '1·Analysis '+p[0]+' '+p[1]+' is present');
  if(at<lastAt) inOrder=false;
  lastAt=at;
});
ok(inOrder, '1b ...and they appear in the spec order, top to bottom');
ok(A.indexOf('Did the dashboard tell the truth')>=0 || A.indexOf('The objective, stated')>=0 || A.indexOf('TODAY’S EVIDENCE')>=0, '1c the tab states the question it answers (v15.55: the live evidence sits under the subsection it answers)');

// ================= 2. THE TESTING TAB RENDERS ①-⑥, IN ORDER =================
reset();
var T=testingBlock();
ok(typeof T==='string' && T.length>0, '2a the Testing tab renders');
// (v15.55) the loop order: ① THE REGISTER · ② THE GATE · ③ ON THE DASHBOARD (proposals, challengers, kills inside) · ④ THE RECORD · ⑤ THE NIGHTLY · ⑥ SELF-TEST
var TSECS=[['\u2460','THE REGISTER'],['\u2461','THE GATE'],['③','ON THE DASHBOARD'],['④','THE RECORD'],['\u2464','THE NIGHTLY'],['⑥','THE SUITE']];
lastAt=-1; inOrder=true;
TSECS.forEach(function(p){
  var at=hasSec(T,p);
  ok(at>=0, '2·Testing '+p[0]+' '+p[1]+' is present');
  if(at<lastAt) inOrder=false;
  lastAt=at;
});
ok(inOrder, '2b ...and they appear in the spec order');

// ================= 3. EVERY SECTION IS COLLAPSIBLE ==========================
reset();
A=analysisBlock();
ok((A.match(/data-gsec="/g)||[]).length>=5, '3a every Analysis section carries a toggle handle (①-⑤ since v15.54)', (A.match(/data-gsec="/g)||[]).length);
T=testingBlock();
ok((T.match(/data-gsec="/g)||[]).length>=7, '3b every Testing section too', (T.match(/data-gsec="/g)||[]).length);
// a CLOSED section renders its header but not its body
reset();
var openA=analysisBlock();
ok(openA.indexOf('▾')>=0, '3c an open section shows ▾');
secToggle('sj-H1');   // (v15.55) H1 is the first subsection of the default subject
var closedA=analysisBlock();
ok(hasSec(closedA,['H1','Is the extreme in'])>=0, '3d a closed section still shows its header');
ok(closedA.length<openA.length, '3e ...but its body is gone (the point of collapsing)', openA.length+' -> '+closedA.length);
ok(closedA.indexOf('▸')>=0, '3f ...and the caret flips to ▸');
secToggle('sj-H1');
ok(analysisBlock().length===openA.length, '3g toggling back restores it exactly');

// ================= 4. EMPTY STATES ARE ONE HONEST LINE ======================
reset();
A=analysisBlock(); T=testingBlock();
// open every section so the empty bodies are actually rendered
['a1','a2','a3','a4','a5','a6','a7','a8'].forEach(function(id){ TAB_SECTIONS[id]=true; });
['t1','t2','t3','t4','t5','t6','t7'].forEach(function(id){ TAB_SECTIONS[id]=true; });
A=analysisBlock(); T=testingBlock();
ANALYSIS_SUBJ='F'; ['sj-F1','sj-F5'].forEach(function(id){ TAB_SECTIONS[id]=true; }); var AF=analysisBlock(); ANALYSIS_SUBJ='H';
[['no session read yet', A, 'Analysis H1 with no data'],
 ['nothing has ever been promoted or demoted', T, 'Testing ⑤ WHAT CHANGED with nothing promoted'],
 ['no resolved deflection records yet', AF, 'Analysis F1 with no node records'],
 ['no taps logged yet', AF, 'Analysis F1 YOUR CALLS with no taps'],
 ['no review has come back yet', T, 'Testing ⑤ with no review'],
 ['rows arrive when learning/studies.json is fetched', A, 'Analysis with the registry not fetched'],
 ['no open proposals', T, 'Testing ② with an empty queue'],
 ['no challengers evaluated yet', T, 'Testing ③ with no challengers'],
 ['not run yet', T, 'Testing ⑤ before the self-test is run']
].forEach(function(p){
  ok(p[1].indexOf(p[0])>=0, '4·'+p[2]+' prints ONE honest line, never a blank', p[0]);
});
// nothing renders a fabricated zero in place of missing data
ok(A.indexOf('>0%<')<0 && T.indexOf('>0%<')<0, '4a neither tab prints a bare "0%" for data it does not have');
ok(A.indexOf('NaN')<0 && T.indexOf('NaN')<0, '4b ...and never NaN');
ok(A.indexOf('undefined')<0 && T.indexOf('undefined')<0, '4c ...or undefined');

// ================= 5. NO % WITHOUT ITS n ====================================
// Every percentage either sits inside a card that also carries an n, or is produced by
// pctN / nTxt. This scans the RENDERED tabs with real data in them.
reset();
STATS={ byKey:{ dir:{n:400,hit:260,pending:0,mfe:120,mae:-60,mn:400,partial:0},
                node:{n:400,hit:250,pending:0,mfe:100,mae:-50,mn:400,partial:0},
                decision:{n:400,hit:220,pending:0,mfe:0,mae:0,mn:0,partial:0} },
        byGrade:{ dir:{A:{n:200,hit:140},B:{n:200,hit:120},C:{n:200,hit:80}},
                  node:{A:{n:200,hit:150},B:{n:200,hit:120},C:{n:200,hit:90}}, dirSide:{} },
        cells:{ 'B×A':{n:200,hit:130}, 'C×C':{n:100,hit:70} },
        act:{take:{n:8,hit:5,mfe:6,mae:-2,mn:8},pass:{n:7,hit:5,mfe:2,mae:-1,mn:7}},
        frame:{ dir:{n:40,tgt:26,rr:120,rn:40} }, partial:0, days:6, dayKeys:['a','b','c','d','e','f'] };
DAYS={ '2026-08-17':{ feat:{ SPY:[
  { key:'node', resolved:true, hit:1, mfe:0.8, mae:-0.2, session:'morning',
    rec:{ grade:'A', tap:0, pol:'+' }, frame:{first:'tgt'} },
  { key:'node', resolved:true, hit:0, mfe:0.1, mae:-0.9, session:'power',
    rec:{ grade:'C', tap:2, pol:'-' }, frame:{first:'inval'} }
] } } };
featStatsInvalidate();
A=analysisBlock();
function pctWithoutN(html){
  // strip tags, then look for any "NN%" that is not followed, within 60 chars, by an n
  var txt=html.replace(/<[^>]*>/g,'|');
  var re=/(\d+)%/g, m, bad=[];
  while((m=re.exec(txt))){
    var tail=txt.slice(m.index, m.index+70);
    if(!/n=|eff n|eff |n\d|\btaps\b|\bn\b/.test(tail)) bad.push(tail.slice(0,50));
  }
  return bad;
}
var badA=pctWithoutN(A);
ok(badA.length===0, '5a no rate on the Analysis tab is rendered without its n', badA.slice(0,2).join(' // '));
T=testingBlock();
var badT=pctWithoutN(T);
ok(badT.length===0, '5b nor on the Testing tab', badT.slice(0,2).join(' // '));
// the tiles themselves refuse to show a % under the bar
ok(!/tabTile\('Direction'/.test(ex('analysisBlock')), '5c (v15.54) the headline tiles are gone from Analysis — the numbers live in ① and ②');
var thinTile=tabTile('Direction', 64, 30, 'q');
ok(thinTile.indexOf('64%')<0 && /recording — need 20/.test(thinTile),
   '5d a tile under eff n=20 greys out and says what it is waiting for, instead of showing a thin 64%');
var fatTile=tabTile('Direction', 64, 400, 'q');
ok(fatTile.indexOf('64%')>=0 && /eff n=40/.test(fatTile), '5e ...and shows the rate WITH its eff n once it is real');

// ================= 6. QUESTION-FIRST HOVERS =================================
reset(); A=analysisBlock(); T=testingBlock();
var titles=(A+T).match(/title="[^"]{12,}"/g)||[];
ok(titles.length>=10, '6a both tabs are densely hovered', titles.length);
var qFirst=titles.filter(function(t){ return /\?/.test(t); }).length;
ok(qFirst>=8, '6b ...and the hovers ask a QUESTION rather than restating the label', qFirst+'/'+titles.length);
// (v15.55) the subject panel: each subsection says what it DECIDES; the TRACK field says what happens to what you type
['What does it decide at the tap? SIZE · WAIT','What does it decide at the tap? SIZE · STOP · TIME','Type what you want measured','the move from low to high'].forEach(function(q){
  ok(A.indexOf(q)>=0, '6·Analysis subtitle says: '+q);
});
['Before a feature','Hypotheses fixed on','What the ladder renders, and which study each number comes from','What is asking to change the model',
 'What did the review actually say?','WHAT CHANGED',
 'Does the scorer itself work, and did the build ship green?','How much have we actually got, and what unlocks when?'].forEach(function(q){
  ok(T.indexOf(q)>=0, '6·Testing subtitle asks: '+q);
});

// ================= 7. A "?" GUIDE PER TAB ===================================
reset();
A=analysisBlock();
ok(/data-gguide="analysis"/.test(A), '7a the Analysis tab has a "?" guide button');
ok(A.indexOf('HOW TO READ THIS TAB')<0, '7b ...closed by default (it must not eat the panel)');
TAB_GUIDE.analysis=true;
A=analysisBlock();
ok(A.indexOf('HOW TO READ THIS TAB')>=0, '7c clicking it opens a one-screen guide');
['eff n','lift','⚠1-way','monotone A·B·C','frame %','⚖ vs 📊'].forEach(function(term){
  ok(A.indexOf('<b style="color:'+PAL.ink+'">'+term+'</b>')>=0, '7·the guide defines "'+term+'"');
});
TAB_GUIDE.analysis=false;
TAB_GUIDE.testing=true;
T=testingBlock();
ok(/data-gguide="testing"/.test(T) && T.indexOf('HOW TO READ THIS TAB')>=0, '7d the Testing tab has its own guide');
['the bar','walk-forward','regime flip','challenger','self-test'].forEach(function(term){
  ok(T.indexOf('<b style="color:'+PAL.ink+'">'+term+'</b>')>=0, '7·the testing guide defines "'+term+'"');
});
TAB_GUIDE.testing=false;

// ================= 8. THE SELF-TEST ACTUALLY RUNS ===========================
reset();
var r=selfTestRun();
ok(r && r.checks && r.checks.length===3, '8a the self-test scores three planted properties', r.checks.length);
ok(r.checks[0].id==='edge' && r.checks[0].pass===true, '8b it FINDS the planted edge', r.checks[0].got);
ok(r.checks[1].id==='trap' && r.checks[1].pass===true, '8c it FLAGS the 1-way trap', r.checks[1].got);
ok(r.checks[2].id==='regime' && r.checks[2].pass===true, '8d it SPLITS the regime-dependent rule', r.checks[2].got);
ok(r.ok===true, '8e ...so the scorer reports itself OK');
ok(r.baseline.up===30 && r.baseline.dn===70, '8f the planted tape really is a down day (baseline up 30 / dn 70)', r.baseline.up+'/'+r.baseline.dn);
ok(JSON.stringify(selfTestRun())===JSON.stringify(selfTestRun()), '8g it is deterministic — no RNG, no clock, no network');
// it uses the SAME arithmetic the Analysis tab uses
var STR=ex('selfTestRun');
ok(/Math\.max\(up,dn\)\/vn\)>=0\.90/.test(STR), '8h the 1-way check is the panel’s own >=0.90 ratio rule');
ok(/\(up\*upPct \+ dn\*dnPct\)\/vn/.test(STR), '8i the expectation is the panel’s own vote-mix-weighted baseline');
SELFTEST_LAST=r;
var sh=selfTestHtml();
ok(/✓ scorer OK/.test(sh), '8j the result renders pass/fail');
ok(/finds the planted edge/.test(sh) && /flags the 1-way trap/.test(sh) && /splits the regime-dependent rule/.test(sh),
   '8k ...naming all three properties in plain language');
ok(/want:/.test(sh), '8l ...and stating what each check WANTED, so a failure is diagnosable');
ok(/data-gselftest/.test(sh), '8m ...behind a button the trader can press');
SELFTEST_LAST=null;

// ================= 9. DATA COVERAGE SAYS WHAT UNLOCKS WHEN ==================
reset();
STATS={ byKey:{ dir:{n:100,hit:60}, node:{n:60,hit:35}, decision:{n:40,hit:22} },
        byGrade:{dir:{},node:{},dirSide:{}}, cells:{}, frame:{}, partial:0, days:2, dayKeys:['a','b'],
        act:{take:{n:2,hit:1},pass:{n:1,hit:1}} };
featStatsInvalidate();
var u=unlockRowsHtml('SPY');
ok(/direction grade rate/.test(u) && /10 \/ 20/.test(u), '9a each capability shows have / need in effective observations');
ok(/rolling floors\/ceilings/.test(u) && /2 \/ 5/.test(u), '9b "rolling floors/ceilings: needs 5 sessions, have 2" — the spec’s own example', '2 / 5');
ok(/walk-forward/.test(u), '9c ...and the walk-forward requirement');
ok(/your calls/.test(u), '9d ...and the takes-vs-passes minimum');
ok(/what unlocks/.test(u), '9e the column is labelled as what UNLOCKS, not as a raw count');

// ================= 10. 250px-SAFE =========================================
reset(); A=analysisBlock(); T=testingBlock();
var both=A+T;
ok(!/width:\s*\d{3,}px/.test(both), '10a no fixed pixel width over 99px anywhere in either tab');
// nowrap survives only on short numeric cells (an eff-n column, a from→to pair). Every
// explanatory line is white-space:normal, which is what makes the panel readable at 250px.
ok((both.match(/white-space:nowrap/g)||[]).length<=9, '10b nowrap is confined to short numeric cells (v15.55: +1, the TRACK field\'s "+ Add" button)', (both.match(/white-space:nowrap/g)||[]).length);
// (v15.62) the wrapping moved into the mockup's stylesheet: the prose classes wrap there, every table is 100% there
var PCSS=require('child_process').execSync('python3 tools/panel-css.py',{encoding:'utf8'});
ok(/#gpts-body \.g3pan \.qq,#gpts-body \.g3pan \.v,#gpts-body \.g3pan \.note,#gpts-body \.g3pan \.rs\{white-space:normal\}/.test(PCSS) && (both.match(/white-space:normal/g)||[]).length>=5,
   '10b2 ...and the prose wraps: the question, the value, the note and the result classes are white-space:normal in the stylesheet, the live evidence inline', (both.match(/white-space:normal/g)||[]).length+' inline');
ok(/white-space:normal/.test(both), '10c ...and the explanatory lines explicitly wrap');
ok((both.match(/<table(?! style="width:100%)[^>]*style=/g)||[]).length===0 && /#gpts-body \.g3pan table\{[^}]*width:100%/.test(PCSS),
   '10d every table is width:100% through the stylesheet, so nothing overflows a 250px panel');
ok(true, '10e (v15.54) no headline tiles to wrap');

console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
