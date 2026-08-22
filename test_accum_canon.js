// (v10.49 I) ACM CANONICAL — ONE accumulation source with two labelled horizons.
// The v10.48 bug was two different Acm computations: the Node Map sentence could say
// "Acm" while the ladder row said "Dec" for the SAME node. accumCanon is now the only
// source both of them read.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m+(g!==undefined?' -> '+g:''));} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+g:''));} };

// ---- an in-memory localStorage so the day store is really exercised ----
var LS={};
global.localStorage={ getItem:function(k){ return (k in LS)?LS[k]:null; }, setItem:function(k,v){ LS[k]=String(v); }, removeItem:function(k){ delete LS[k]; } };
global.TODAY='2026-08-17';
global.ACMDAY=null;
global.ACMDAY_KEY='gpts_acmday_v1';
global.ACM_BAND=8;
var HIST=null, FEEDPCT=null, LIVEPCT=null;
global.nodeHistory=function(){ return HIST; };
global.feedStructMap=function(){ return FEEDPCT?{pct:FEEDPCT}:null; };
global.livePctAt=function(){ return LIVEPCT; };

// (v11.40) The v11.0 audit made accumCanon FEED-FIRST. It calls feedSeries/feedSampleAt/feedGoneAt,
// none of which this harness stubbed — so every call threw, was swallowed by the function's own
// try/catch, and returned nulls. Thirteen assertions then reported as failures of the LOGIC when the
// logic had never run. Stub the feed as absent so the tape fallback these fixtures describe is what
// actually gets exercised.
global.FEED_M15_SAMPLES=5;
global.feedSeries=function(){ return null; };
global.feedSampleAt=function(){ return null; };
global.feedGoneAt=function(){ return false; };
eval(['acmDayLoad','acmDaySave','acmLabel','accumCanon'].map(ex).join('\n'));

// ================= 1. acmLabel bands =================
ok(acmLabel(20)==='Acm'   && acmLabel(8)==='Acm',   '1a >= +8% -> Acm');
ok(acmLabel(-20)==='Dec'  && acmLabel(-8)==='Dec',  '1b <= −8% -> Dec');
ok(acmLabel(0)==='Steady' && acmLabel(7)==='Steady' && acmLabel(-7)==='Steady', '1c inside the band -> Steady');
ok(acmLabel(null)==='Steady', '1d null -> Steady, never a guess');

// ================= 2. the NOW horizon (~6m / last samples of the %King strip) ======
LS={}; global.ACMDAY=null;
HIST=[50,50,40,50,60];               // two samples back (~6m) = 40 -> now 60 = +50%
FEEDPCT={'773.00':60};
var a=accumCanon('SPY',773);
ok(a.m15.pct===50, '2a now = +50% over the last window', a.m15.pct);
ok(a.m15.label==='Acm', '2b -> Acm');
HIST=[50,50,60,50,40];
LS={}; global.ACMDAY=null; FEEDPCT={'773.00':40};
var b=accumCanon('SPY',773);
ok(b.m15.pct===-33 && b.m15.label==='Dec', '2c a bleeding node reads Dec', b.m15.pct);
HIST=[50,50,50,50,50];
LS={}; global.ACMDAY=null; FEEDPCT={'773.00':50};
ok(accumCanon('SPY',773).m15.label==='Steady', '2d a flat node reads Steady');
HIST=null;
ok(accumCanon('SPY',773).m15.pct===null, '2e no history -> null, not 0');
ok(accumCanon('SPY',773).m15.label==='Steady', '2f ...and the label degrades to Steady');

// ================= 3. the DAY horizon: vs the FIRST reading today =================
LS={}; global.ACMDAY=null; HIST=null;
FEEDPCT={'773.00':40};
var d1=accumCanon('SPY',773);
ok(d1.session.pct===0, '3a the first reading of the day establishes the baseline at 0%', d1.session.pct);
ok(LS['gpts_acmday_v1'] && /"SPY:773.00":40/.test(LS['gpts_acmday_v1']), '3b baseline persisted to localStorage');
FEEDPCT={'773.00':60};
ok(accumCanon('SPY',773).session.pct===50, '3c later in the day: 40 -> 60 = +50%', accumCanon('SPY',773).session.pct);
ok(accumCanon('SPY',773).session.label==='Acm', '3d -> Acm (real positioning, not hedge churn)');
FEEDPCT={'773.00':20};
ok(accumCanon('SPY',773).session.pct===-50 && accumCanon('SPY',773).session.label==='Dec', '3e 40 -> 20 = −50% Dec');
// the baseline SURVIVES a reload (a fresh in-memory module reading the same store)
global.ACMDAY=null;
FEEDPCT={'773.00':60};
ok(accumCanon('SPY',773).session.pct===50, '3f the baseline survives a reload (read back from localStorage)');
// a NEW day resets it
global.TODAY='2026-08-18'; global.ACMDAY=null;
FEEDPCT={'773.00':60};
ok(accumCanon('SPY',773).session.pct===0, '3g a new day key resets the baseline', accumCanon('SPY',773).session.pct);
global.TODAY='2026-08-17'; global.ACMDAY=null; LS={};

// ================= 4. the two horizons are INDEPENDENT and both reported ==========
LS={}; global.ACMDAY=null;
FEEDPCT={'773.00':100};
accumCanon('SPY',773);                       // baseline 100
HIST=[100,100,60,80,90];                     // two samples back = 60 -> now 90 = +50% (rebuilding)
FEEDPCT={'773.00':90};
var both=accumCanon('SPY',773);
ok(both.m15.label==='Acm' && both.session.label==='Dec',
   '4a a node rebuilding NOW but still below its open reads Acm-now / Dec-day', both.m15.label+'/'+both.session.label);
ok(both.m15.pct===50 && both.session.pct===-10, '4b both numbers reported, neither overwrites the other', both.m15.pct+'/'+both.session.pct);

// ================= 5. sources + defensiveness =================
LS={}; global.ACMDAY=null; FEEDPCT=null; LIVEPCT=55; HIST=null;
ok(accumCanon('SPY',773).session.pct===0, '5a falls back to the tape %King when the feed map is unavailable');
LIVEPCT=null;
LS={}; global.ACMDAY=null;
var none=accumCanon('SPY',773);
ok(none.session.pct===null && none.session.label==='Steady', '5b no magnitude anywhere -> null day pct');
ok(accumCanon('SPY',null).session.pct===null, '5c null strike -> empty result, no throw');
global.nodeHistory=function(){ throw new Error('boom'); };
ok(accumCanon('SPY',773).m15.label==='Steady', '5d a throwing history source degrades quietly');
global.nodeHistory=function(){ return HIST; };

// ================= 6. ONE SOURCE: both consumers read accumCanon =================
function grab(name){ var i=src.indexOf('function '+name+'('); var depth=0,st=false,j=i;
  for(;j<src.length;j++){var c=src[j]; if(c==='{'){depth++;st=true;} else if(c==='}'){depth--; if(st&&depth===0){j++;break;}}} return src.slice(i,j); }
ok(/accumCanon\(sym, L\.k\)/.test(grab('nodeMapSentence')), '6a the Node Map sentence reads accumCanon');
ok(/accumCanon\(sym,L\.k\)/.test(grab('deflZonesBlock')),   '6b the zone rows read accumCanon');
ok(/accumCanon\(sym, L\.k\)/.test(grab('nodeGrade')),       '6c the node grade reads accumCanon');
ok(/THE single Acm source|ONE accumulation source/i.test(src), '6d the single-source contract is documented');
ok(/gpts_acmday_v1/.test(src), '6e the day store uses the new key (no existing key renamed)');

console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
