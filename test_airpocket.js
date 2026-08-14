// (v10.32) AIR POCKET / LIQUIDITY VACUUM detector tests.
// Academy (source of truth, air-pockets-velocity): a low-exposure GAP between two
// significant nodes = a fast PATHWAY (not a target). Extended gap = Liquidity Vacuum.
// Gap width judged RELATIVE to the board's median node spacing.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
function ok(c,m){ if(c){pass++; console.log('PASS '+m);} else {fail++; console.log('FAIL '+m);} }
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}

var STATE={SPY:{price:772, walls:[]}};

// mirror shipped constants (sync-guarded below)
var MIN_STRENGTH=20, AIRPOCKET_GAP_MULT=2.5, AIRPOCKET_VACUUM_MULT=4.0, AIRPOCKET_MIN_STRIKES=2.0;
ok(src.indexOf('var AIRPOCKET_GAP_MULT   = 2.5')>=0, 'shipped AIRPOCKET_GAP_MULT=2.5 matches (sync-guard)');
ok(src.indexOf('var AIRPOCKET_VACUUM_MULT= 4.0')>=0, 'shipped AIRPOCKET_VACUUM_MULT=4.0 matches');
ok(src.indexOf('var AIRPOCKET_MIN_STRIKES= 2.0')>=0, 'shipped AIRPOCKET_MIN_STRIKES=2.0 matches');

eval(ex('airPocketDetect'));
// W(list) -> significant walls at given strikes (pct default 50, all significant)
function W(list){ return list.map(function(a){return {k:a[0], pct:(a.length>1?a[1]:50)};}); }

// ---- tight grid, no pocket: SPY 1-strike spacing, all filled ----
STATE.SPY.walls=W([[768],[769],[770],[771],[772],[773]]);
ok(!airPocketDetect('SPY').ok, 'evenly-spaced 1-strike grid -> no air pocket');

// ---- a clear gap: nodes at 770 and 774 on an otherwise 1-strike board -> Air Pocket ----
// baseline (grid step) = 1; pocketThresh=2.5, vacuumThresh=4. A 4-strike gap (770->774)
// is >=2.5 (pocket) but <4 is false at exactly 4 -> use 3-strike to stay in Air-Pocket band.
STATE.SPY.walls=W([[768],[769],[770],[773],[774],[775]]);  // gap 770->773 = 3 strikes
var a1=airPocketDetect('SPY');
ok(a1.ok && a1.pockets.length===1, 'one air pocket found ('+a1.pockets.length+')');
ok(a1.pockets[0].lo===770 && a1.pockets[0].hi===773, 'pocket spans 770-773 (the empty bracket)');
ok(a1.pockets[0].type==='Air Pocket', 'a 3-strike gap on a 1-strike board = Air Pocket (>=2.5, <4)');

// ---- extended gap -> Liquidity Vacuum ----
STATE.SPY.walls=W([[768],[769],[770],[782],[783],[784]]);  // 12-strike gap on 1-strike board
var a2=airPocketDetect('SPY');
ok(a2.ok && a2.pockets[0].type==='Liquidity Vacuum', 'a very wide gap = Liquidity Vacuum');

// ---- spot INSIDE the pocket -> flagged containsSpot + becomes adjacent ----
STATE.SPY.price=773;
STATE.SPY.walls=W([[768],[769],[770],[776],[777],[778]]);  // spot 773 sits inside 770-776 (span 6 = vacuum)
var a3=airPocketDetect('SPY');
ok(a3.adjacent && a3.adjacent.containsSpot===true, 'spot inside the gap -> adjacent pocket flagged containsSpot');
ok(a3.adjacent.targetUp===776 && a3.adjacent.targetDn===770, 'pathway targets = far-side nodes (up 776 / dn 770)');

// ---- relative spacing: a 3-strike gap on a WIDE (5-strike) board is NOT a pocket ----
STATE.SPY.price=772;
STATE.SPY.walls=W([[760],[765],[770],[773],[778],[783]]);  // median spacing 5; the 3-gap (770->773) is BELOW normal
var a4=airPocketDetect('SPY');
var has770to773 = a4.pockets.some(function(p){return p.lo===770&&p.hi===773;});
ok(!has770to773, 'a below-median gap is NOT a pocket (relative to board spacing)');

// ---- insignificant nodes ignored: a weak (<MIN_STRENGTH) node does not fill a gap ----
// Only 2 significant nodes remain (770,776) => board too sparse to infer a grid step =>
// baseline falls back to AIRPOCKET_MIN_STRIKES floor (2.0). Span 6 >= floor -> pocket.
STATE.SPY.price=772;
STATE.SPY.walls=[{k:770,pct:50},{k:773,pct:5},{k:776,pct:50}];  // 773 below MIN_STRENGTH
var a5=airPocketDetect('SPY');
ok(a5.ok && a5.pockets.some(function(p){return p.lo===770&&p.hi===776;}), 'sub-threshold node does not fill the gap; sparse-board floor still flags 770-776');

// ---- degenerate ----
STATE.SPY.walls=[];
ok(!airPocketDetect('SPY').ok, 'no walls -> no pocket');
STATE.SPY.walls=W([[770]]);
ok(!airPocketDetect('SPY').ok, 'single wall -> no pocket');

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
