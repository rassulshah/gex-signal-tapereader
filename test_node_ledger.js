// v11.0 — THE NODE LEDGER (layer 1). User: "a layer that tracks all the nodes like a map to figure
// out how they influence price as they grow, accumulate or dissipate ... and the impact on price."
// Pins: life (born/peak/now/gone) from the feed series; touches + reactions from candles; influence
// (toward while acm / away while dec); pure build; export shape; Analysis ⑦ NODES; ledger.touch enrolled.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);if(!m)throw new Error('no fn '+n);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
let p=0,f=0;const ok=(n,c,g)=>{if(c){p++;console.log('PASS '+n);}else{f++;console.log('FAIL '+n+(g!==undefined?' -> '+JSON.stringify(g):''));}};
function mul(a,b){return a/(1/b);} global.mul=mul;
global.FEED_SERIES_CACHE={}; global.LEDGER_CACHE={}; global.MIN_STRENGTH=20; global.PB_MIN_PCT=20; global.PB_REACH=5; global.MAP_ACM=8; global.MAP_DEC=-8; global.MAP_DROP=25; global.FEED_M15_SAMPLES=15; global.FEED_GONE_N=3; global.DEFLECT_ZONE=0.5; global.LEDGER_INFL_BARS=5;
global.PAL={longAccent:'#2ec27e',shortAccent:'#f0616d',sub:'#8b98a9',ink:'#e6edf3',line:'#21262d'};
global.dispIsFut=()=>false; global.futMark=()=>''; global.dispR=()=>1; global.fmtFut=String; global.window={};
eval(['fmtNum','fmtLvl','feedSeriesAll','feedSeries','feedSampleAt','feedGoneAt','mapNodeState','ledgerStateAt','ledgerStateAtAbs','ledgerBuild','ledgerLifeText','ledgerNode','nodeLedger','ledgerExport'].map(ex).join('\n'));

// ---- a synthetic session: 40 one-minute samples, t = 100..2440; candles every 3 samples
function snap(t,l){ return {t:t,s:768.3,l:l}; }
const nat=[], der=[];
for(let i=0;i<40;i++){
  const t=100+60*i; const l=[{k:768,v:100,d:1}];
  // 769: builds 20→60 by i=15, holds, then bleeds and is GONE from i=30
  if(i<30) l.push({k:769,v:(i<15?20+ (40*i/15):60-(i-15)*2),d:1});
  // 767: born at i=10 and builds to 70
  if(i>=10) l.push({k:767,v:20+(i-10)*1.7,d:1});
  nat.push(snap(t,l));
  // SPXW lane 769.5 holds at 100 (its own King), 767.7 builds
  der.push(snap(t,[{k:769.48,v:100,d:1},{k:767.72,v:10+2*i,d:1}]));
}
global.LASTFEED={SPY:{j:{levels:nat,derived:[{source:'SPXW',levels:der}]}}};
// candles: price walks 768.3 → touches 769 at bar 4 (rejects), touches 767 at bar 9 (goes through), then drifts to 766.8
const cs=[];
for(let b=0;b<13;b++){
  const t=100+60*(b*3+2);
  let o=768.3,h=768.4,l=768.1,c=768.3;
  if(b===4){ o=768.3; h=769.2; l=768.2; c=768.4; }          // opened below the 769 zone, wick into it, closed back below → DEFLECT
  if(b===9){ o=767.6; h=767.7; l=766.3; c=766.4; }          // opened above 767, closed below 767-0.5 → THROUGH
  if(b>9){ o=766.4; h=766.5; l=766.2; c=766.3; }
  cs.push({t:t,o:o,h:h,l:l,c:c});
}
global.STATE={SPY:{price:766.3,candles:cs}};
global.closedCandles=()=>cs; global.legBarKey=()=>'k1';

const all=feedSeriesAll('SPY');
const L=ledgerBuild(all, cs, {});
ok('1a the ledger has an entry for every node that was ever meaningful (768, 769, 767, 769.5, 767.5)', L.n===5 && L.nodes['768.00'] && L.nodes['769.00'] && L.nodes['767.00'] && L.nodes['769.50'] && L.nodes['767.50'], Object.keys(L.nodes));
const n769=L.nodes['769.00'], n767=L.nodes['767.00'], n7695=L.nodes['769.50'];
ok('2a 769 life: born at sample 0, peak 60% around sample 15, GONE now', n769.first===0 && n769.peak===60 && n769.peakAt===15 && n769.gone===true && n769.state==='gone', {first:n769.first,peak:n769.peak,peakAt:n769.peakAt,state:n769.state});
ok('2b 769 life phases: build 15 samples, gone for ~9', n769.life.build===15 && n769.life.goneFor>=8, n769.life);
ok('2c 767 life: born at sample 10, still accumulating (acm) at the end', n767.first===10 && n767.state==='acm' && n767.m15>=8, {first:n767.first,state:n767.state,m15:n767.m15});
ok('2d the SPXW lane 769.5 is in the ledger with src SPXW, holding at its own King', n7695.derived===true && n7695.src==='SPXW' && n7695.cur===100 && n7695.state==='hold', n7695);
// touches
ok('3a 769 was touched once (bar 4) and the reaction is DEFLECT (wick in, close back below the zone)', n769.touches.length===1 && n769.touches[0].react==='deflect' && n769.deflect===1, n769.touches);
ok('3b ...and the touch carries the node state AT THE TOUCH (acm — it was building then)', n769.touches[0].state==='acm', n769.touches[0]);
ok('3c 767 was touched at bar 9 and price went THROUGH', n767.touches.some(t=>t.bar===9 && t.react==='through') && n767.through>=1, n767.touches);
// influence
ok('4a influence counters exist and are non-negative', n767.infl.acmN>=0 && n769.infl.decN>=0, {n767:n767.infl,n769:n769.infl});
ok('4b while 767 was accumulating price came toward it (toward count > 0)', n767.infl.acmToward>0, n767.infl);
// pure + cached
ok('5a nodeLedger caches per bar key', nodeLedger('SPY')===nodeLedger('SPY'));
ok('5b ledgerNode finds by strike', ledgerNode('SPY',769) && ledgerNode('SPY',769).k===769);
// life text
const lt=ledgerLifeText('SPY',769);
ok('6a the life text is one honest line with peak, state, touches', /Life: born/.test(lt) && /peak 60%/.test(lt) && /gone/.test(lt) && /touches 1: deflect 1/.test(lt), lt);
// export
const ex1=ledgerExport('SPY');
ok('7a ledgerExport is compact: counts + last touches + influence, per node', ex1 && ex1.n===5 && ex1.nodes['769.00'] && ex1.nodes['769.00'].deflect===1 && Array.isArray(ex1.nodes['769.00'].touches) && ex1.nodes['769.00'].infl, ex1 && Object.keys(ex1.nodes));
ok('7b the day export carries `ledger` per sym', /ledger:\(function\(\)\{ try\{ var o=\{\}; RECORDER_SYMS\.forEach\(function\(s\)\{ o\[s\]=ledgerExport\(s\); \}\)/.test(src));
// face + enrollment
ok('8a Analysis ⑦ NODES section renders the ledger', /tabSection\('a7','⑦','NODES'/.test(src) && /function ledgerSectionHtml/.test(src));
ok('8b the ledger section answers the two questions with n', /Does accumulation pull price\?/.test(src) && /Do accumulating nodes deflect more than dissipating ones\?/.test(src));
ok('8c the node hovers carry the life line', (src.match(/ledgerLifeText\(sym,L\.k\)/g)||[]).length>=2);
ok('8d ledger.touch is a registered FEATURE with the acm-vs-dec question', /registerFeature\(\{ key:'ledger\.touch'/.test(src) && /acm_deflects_more/.test(src));
ok('8e rules.json seeds ledger.touch (61 ids)', (function(){ var RJ=JSON.parse(fs.readFileSync('./learning/rules.json','utf8')); return !!RJ.rules['ledger.touch'] && Object.keys(RJ.rules).length===61; })());
console.log('test_node_ledger: '+p+' passed, '+f+' failed');
