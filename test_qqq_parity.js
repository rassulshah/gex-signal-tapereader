// (v10.55 PART G) QQQ PARITY. Everything the panel computes for SPY — the spine, the
// deflection zones, the leg engine, the feature records, the histories — must run for
// QQQ when QQQ is the ACTIVE underlying (an NQ/MNQ futures chart, or a QQQ chart), and
// every store must be keyed by symbol so the two can never bleed into each other.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m+(g!==undefined?' -> '+g:''));} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+g:''));} };

global.FUTMODE={ chart:'SPY', fam:null, underlying:'SPY', r:1, live:true, approx:false, ok:true };
eval(['activeSym'].map(ex).join('\n'));

// ================= 1. THE ACTIVE UNDERLYING ======================================
ok(activeSym()==='SPY', '1a a SPY chart -> SPY is active', activeSym());
global.FUTMODE.underlying='QQQ';
ok(activeSym()==='QQQ', '1b a QQQ (or NQ) chart -> QQQ is active', activeSym());
global.FUTMODE.underlying=null;
ok(activeSym()==='SPY', '1c anything unknown falls back to SPY rather than to nothing');
global.FUTMODE=null;
ok(activeSym()==='SPY', '1d ...and a missing mode object cannot break the panel');
global.FUTMODE={ chart:'SPY', fam:null, underlying:'SPY', r:1, ok:true };

// ================= 2. THE DASHBOARD RENDERS THE ACTIVE SYMBOL ====================
var R=ex('render');
ok(/__asym=activeSym\(\)/.test(R), '2a render() resolves the active underlying each pass');
ok(/readBlock44\(__asym\)/.test(R), '2b the READ is built for it');
ok(/driftLineHtml\(__asym\)/.test(R), '2c the drift line too');
ok(/briefBlockHtml\(__asym\)/.test(R), '2d and the pre-open brief');
ok(/var sym=activeSym\(\)/.test(ex('nodeMapBlock')), '2e the node map / deflection zones follow the active symbol');
ok(/var sym=activeSym\(\)/.test(ex('kingHeaderBlock')), '2f so does the King header');
ok(/var __asym=activeSym\(\)/.test(ex('accumBlock')), '2g and the accumulation block');
ok(/futureStructureSummary\(__asym\)/.test(ex('accumBlock')), '2h ...reading the ACTIVE symbol\'s structure, not a hardcoded SPY');
ok(/divLab=dispIsFut\(\)\?FUTMODE\.chart:sym/.test(ex('deflZonesBlock')),
   '2i the price divider names what is actually being displayed');

// ================= 3. THE PER-BAR LOOP RUNS FOR IT ===============================
var T=ex('tick');
ok(/var ASYM=activeSym\(\)/.test(T), '3a the tick resolves the active symbol once');
ok(/if\(ASYM!=='SPY'\) refreshSym\(ASYM\)/.test(T), '3b QQQ is refreshed when it is the active underlying');
ok(/resolveFeatureOutcomes\(sym\)/.test(ex('recordNodeSnapshot')), '3c ...and its feature outcomes resolve on the same loop (inside recordNodeSnapshot, per sym — v11.0 removed the per-tick duplicate)');
ok(/legEngine\(ASYM\)/.test(T), '3d ...and the leg engine runs on it');
ok(/nodeHistSample\('SPY'\);\s*\n\s*nodeHistSample\('QQQ'\);/.test(T),
   '3e the node-cluster history is sampled for BOTH symbols, always');
ok(/fcHistSample\('SPY'\);\s*\n\s*fcHistSample\('QQQ'\);/.test(T), '3f as is the Flr/Ceil history');
ok(/recordNodeSnapshot\('QQQ'\)/.test(T), '3g and the node snapshots (unchanged from v10.54)');

// ================= 4. EVERY STORE IS KEYED BY SYMBOL =============================
ok(/var LEG_STATE=\{ SPY:null, QQQ:null \}/.test(src), '4a the leg state machine is per symbol');
ok(/var LEG_CACHE=\{ SPY:null, QQQ:null \}/.test(src), '4b so is its per-bar cache');
ok(/var LEG_PB_LOG=\{ SPY:\[\], QQQ:\[\] \}/.test(src), '4c so is the detected-pullback log');
ok(/NODEHIST=\{ v:1, sym:\{\} \}/.test(ex('nodeHistLoad')), '4d NODEHIST is stored under sym');
ok(/H\.sym\[sym\]/.test(ex('nodeHistSample')), '4e ...and written per symbol');
ok(/RECORDER_SYMS *= *\['SPY','QQQ'\]/.test(src), '4f the recorder covers both symbols');
ok(/SPINE_CACHE=\{ SPY:null, QQQ:null \}/.test(src), '4g the spine cache is per symbol');
// the leg engine takes a sym and threads it everywhere
var LE=ex('legEngine');
ok(/sym=sym\|\|'SPY'/.test(LE) && /LEG_STATE\[sym\]/.test(LE) && /LEG_CACHE\[sym\]/.test(LE),
   '4h legEngine(sym) never touches another symbol\'s state');
var LC=ex('legCtxOf');
ok(/nodeMapModel\(sym\)/.test(LC) && /trendVerdict\(sym\)/.test(LC) && /closedCandles\(sym\)/.test(LC),
   '4i ...and builds its context entirely from that symbol\'s own tape');

// ================= 5. THE FEATURES ARE RECORDED PER SYMBOL =======================
ok(/f\.record\(sym, ctx\)/.test(ex('featRecordAll')), '5a every feature record takes the symbol');
["leg.dir","leg.pbPredict","leg.pbDetect","leg.roll","leg.magnet","predictors"].forEach(function(k){
  var i=src.indexOf("key:'"+k+"'");
  var blk=src.slice(i, i+2600);
  ok(/record:function\(sym/.test(blk), '5·'+k+' records per symbol');
});
ok(/day\.feat\[sym\]/.test(ex('featEnqueue')), '5z the day queue is keyed by symbol');
ok(/RECORDER_SYMS\.forEach\(function\(s\)\{ o\[s\]=nodeHistOf\(s\); \}\)/.test(ex('buildDayExport')),
   '5y the export carries the node history for both');

// ================= 6. AN NQ CHART IS A QQQ PANEL =================================
// The one end-to-end statement: NQ on the chart means the QQQ tape underneath, QQQ
// strikes recorded, and NQ points on the face.
ok(/NQ:'QQQ', MNQ:'QQQ'/.test(src), '6a NQ and MNQ map to the QQQ underlying');
ok(/QQQ:'QQQ'/.test(src), '6b a QQQ chart maps to itself');
global.FUTMODE={ chart:'NQ', fam:'NQ', underlying:'QQQ', r:41.36, live:true, approx:false, ok:true };
ok(activeSym()==='QQQ', '6c so an NQ chart puts the whole panel on QQQ', activeSym());

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
