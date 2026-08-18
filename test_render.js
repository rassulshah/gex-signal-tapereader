// Full render smoke test: stub minimal DOM/globals, load 8/11, call analysisBlock().
// (v10.54 GROUP 5) the Analysis tab was rebuilt into seven collapsible, question-led
// sections. The legacy per-day render (A_renderTop) and the projection scorecard now
// live inside the collapsible DETAIL section, so this smoke test opens it before
// asserting on their content — and additionally asserts the new sections render.
var fs=require('fs');
var src=fs.readFileSync('v10.js','utf8');

// We can't eval the whole IIFE (it touches window/document heavily). Instead extract
// the analysis render chain + its deps by name and run with stubs.
function grab(name){ var i=src.indexOf('function '+name+'('); if(i<0){ console.error('MISSING '+name); return ''; }
  var depth=0,started=false,j=i; for(;j<src.length;j++){var c=src[j]; if(c==='{'){depth++;started=true;} else if(c==='}'){depth--; if(started&&depth===0){j++;break;}}} return src.slice(i,j); }

// globals/stubs
var PAL={bg:'#0b0e14',card:'#12161f',line:'#1e2530',longAccent:'#2ec27e',shortAccent:'#f0616d',ink:'#e6edf3',sub:'#8b98a9',amber:'#f2b45a',gold:'#e3c341',blue:'#4a90d9'};
var LOADED_DAY=require('./day_811.json');
var ANALYSIS_SYM='SPY', ANALYSIS_REVIEW=null, SAVED_TODAY=null;
var elBody={clientWidth:300};
function fmtNum(x){ return ''+x; }
function buildDayExport(){ return LOADED_DAY; }

// pull deps into ONE source blob, then eval once so they share scope
var FEAT_FWD=10, DIR_PTS=0.5, RULE_UNLOCK_N=20, PROMO_MIN_N=20, PROMO_WF_SESSIONS=3;
var DIR_WEIGHTS={ trend:3, driftAgree:2, driftLean:1, diverge:-2, tentative:1, gradeA:5, gradeB:3 };
var DIR_WEIGHTS_SOURCE='hand-set';
var DECISION_MATRIX={ A:{A:'take · follow-thru',B:'take · tight tgt',C:'wait fresher node'},
                      B:{A:'bounce play',B:'scalp',C:'skip'},
                      C:{A:'scalp only',B:'skip',C:'stand aside'} };
var TAB_SECTIONS={}, TAB_GUIDE={}, SELFTEST_LAST=null, FEATURES=[];
function ctTodayStr(){ return '2026-08-11'; }
function featStats(){ return { byKey:{}, byGrade:{dir:{},node:{},dirSide:{}}, cells:{},
  act:{take:{n:0,hit:0,mfe:0,mae:0,mn:0},pass:{n:0,hit:0,mfe:0,mae:0,mn:0}}, frame:{}, partial:0, days:1, dayKeys:['2026-08-11'] }; }
function recorderLoad(){ return { days:{} }; }
function promoEvents(){ return []; }
function promotionsList(){ return []; }
function rulesDoc(){ return { asOf:null, proposals:[], challengers:{}, killList:[], promoted:[] }; }
function learningStripsHtml(){ return '<div>STRIPS</div>'; }
function dirFactorsHtml(){ return '<div>FACTORS</div>'; }
function featureScorecardsHtml(){ return '<div>SCORECARDS</div>'; }
function projScorecardHtml(){ return '<div>PROJ</div>'; }
function pipeStages(){ return [{key:'rec',label:'rec',state:'green',tip:'Is it recording? yes.'}]; }
function pipeColor(){ return PAL.longAccent; }
function pipeLoad(){ return {}; }
function pipeReviewLine(){ return null; }
function registerCoreFeatures(){ return FEATURES; }
var blob=['_pct','_dirOf','A_day','A_num','A_pct','A_sideOf','A_kingBehavior','A_accumEdge','A_combinedEdge','A_regime','A_tip',
 '_step','_await','_kpi','_accBar','A_edgeRow','A_renderTop','analysisStats','timelineSvg','convergenceSvg','_bodyW',
 'effN','nTxt','pctN','_fpct','gradeMonotone','featStatsCached','featStatsInvalidate','ruleLocalRate',
 'secOpen','secToggle','tabSection','tabEmpty','tabTile','tabGuideBtn','tabHeader','tabGuide',
 'deflStats','deflTableHtml','deflectionsSectionHtml','yourCallsHtml','analysisBlock'
].map(grab).join('\n');
// open the collapsible DETAIL section so the legacy per-day render is included
var html=eval(blob + "\n; ['a1','a2','a3','a4','a5','a6','a7','a8'].forEach(function(k){ TAB_SECTIONS[k]=true; }); analysisBlock();");
console.log('render length:', html.length, 'chars');
// checks
var f=0; function A(n,c){ console.log((c?'PASS':'FAIL')+': '+n); if(!c)f++; }
A('renders without throwing', html.length>500);
A('shows regime Whipsaw', /Whipsaw/.test(html));
A('shows King behavior header', /King behavior/.test(html));
A('shows Reach metric', /Reach:/.test(html));
A('shows dissipation edge', /Support fading/.test(html));
A('shows fade-support ~64%', /64%/.test(html));
A('shows trapdoor', /Trapdoor/.test(html));
A('has coherence tooltips', /title="/.test(html));
A('range-day caveat present', /LOW-signal|range\/no-edge/.test(html));
A('legacy per-day render lives in the DETAIL section', /DETAIL · every enrolled feature/.test(html) && /SCORECARDS/.test(html));
// (v10.54 GROUP 5) the seven question-led sections
[['①','HEADLINE'],['②','WHAT CHANGED'],['③','DIRECTION FACTORS'],['④','DEFLECTIONS'],
 ['⑤','YOUR CALLS'],['⑥','REVIEW']]   /* (v11.0) ⑦ PIPELINE lives in the footer; ⑥ = nightly + weekly */.forEach(function(p){
  A('section '+p[0]+' '+p[1]+' renders', html.indexOf(p[0]+' '+p[1])>=0);
});
A('every section is collapsible', (html.match(/data-gsec="/g)||[]).length>=7);
A('the tab carries a "?" guide', /data-gguide="analysis"/.test(html));
A('empty sections print one honest line, never a blank', /no review has come back yet/.test(html));
A('no bare NaN / undefined anywhere', html.indexOf('NaN')<0 && html.indexOf('undefined')<0);
console.log(f===0?'RENDER-OK':f+' RENDER FAIL');
// dump a readable slice of the top for eyeballing
console.log('\n--- TOP SNIPPET (tags stripped) ---');
console.log(html.replace(/<[^>]+>/g,' ').replace(/\s+/g,' ').slice(0,900));
process.exit(f?1:0);
