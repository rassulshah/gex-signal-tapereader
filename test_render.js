// Full render smoke test: stub minimal DOM/globals, load 8/11, call analysisBlock().
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
var blob=['_pct','_dirOf','A_day','A_num','A_pct','A_sideOf','A_kingBehavior','A_accumEdge','A_combinedEdge','A_regime','A_tip',
 '_step','_await','_kpi','_accBar','A_edgeRow','A_renderTop','analysisStats','timelineSvg','convergenceSvg','_bodyW','analysisBlock'
].map(grab).join('\n');
var html=eval(blob + '\n; analysisBlock();');
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
A('legacy scorecard still present', /Signal scorecard/.test(html));
console.log(f===0?'RENDER-OK':f+' RENDER FAIL');
// dump a readable slice of the top for eyeballing
console.log('\n--- TOP SNIPPET (tags stripped) ---');
console.log(html.replace(/<[^>]+>/g,' ').replace(/\s+/g,' ').slice(0,900));
process.exit(f?1:0);
