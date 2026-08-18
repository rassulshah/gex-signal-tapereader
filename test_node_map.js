// v10.58 — THE MAP (node flow). User 2026-08-18: "you must build the ability to detect this rolling
// feature to see how as nodes dissipate, other nodes accumulate and start influencing price."
// Pins: derived book normalised to its own King (the SPXW lanes become visible); the feed as
// history with drop-out = dissipation; mapNodeState acm/dec/gone/hold; transfers on both sides
// independent of the SMA; widening; lean; the Map line words (green acm / red dec, "Map:");
// structure leads the leg when the SMA has no trend; enrollment.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);if(!m)throw new Error('no fn '+n);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
let p=0,f=0;const ok=(n,c,g)=>{if(c){p++;console.log('PASS '+n);}else{f++;console.log('FAIL '+n+(g!==undefined?' -> '+JSON.stringify(g):''));}};
function mul(a,b){return a/(1/b);} global.mul=mul;
global.FEED_SERIES_CACHE={}; global.MIN_STRENGTH=20; global.PB_MIN_PCT=20; global.PB_REACH=5; global.MAP_ACM=8; global.MAP_DEC=-8; global.MAP_DROP=25; global.FEED_M15_SAMPLES=15;
global.PAL={longAccent:'#2ec27e',shortAccent:'#f0616d',sub:'#8b98a9',ink:'#e6edf3',line:'#21262d'};
global.dispIsFut=()=>false; global.futMark=()=>''; global.dispR=()=>1; global.fmtFut=String;
global.FEED_GONE_N=3;
eval(['fmtNum','fmtLvl','extractWalls','synthDerived','feedSeriesAll','feedSeries','feedSampleAt','feedGoneAt','mapNodeState','mapTransfersOf','mapWord'].map(ex).join('\n'));

// ---- 1. the derived book is normalised to ITS OWN King ----
const J={ levels:[{t:1,s:768.3,l:[{k:768,v:883e6,d:1,net:1},{k:769,v:400e6,d:1,net:1},{k:767,v:371e6,d:1,net:1}]}],
          derived:[{source:'SPXW',ratio:0.0997,levels:[{t:1,s:768.3,l:[{k:769.48,v:5.9e6,d:1,net:1},{k:767.02,v:5.1e6,d:1,net:1},{k:770.2,v:1.0e6,d:-1,net:-1}]}]}] };
const W=extractWalls(J);
const byK=Object.fromEntries(W.walls.map(w=>[w.k,w]));
ok('1a SPY King is 768 (native only decides the King)', W.king===768, W.king);
ok('1b the SPXW lane 769.5 is on the board at 100% of ITS book (was 0.7% of SPY King → invisible)', byK[769.5] && byK[769.5].pct===100 && byK[769.5].derived===true, byK[769.5]);
ok('1c 767 stays NATIVE (integer strikes belong to SPY; SPXW 767.02 snaps onto it and is dropped)', byK[767] && !byK[767].derived, byK[767]);
ok('1d derived rows carry src SPXW', byK[769.5].src==='SPXW');
ok('1e a thin derived lane (770.2 = 17%) is still filtered by MIN_STRENGTH', !byK[770] && !byK[770.5]);

// ---- 2. the feed as history: per-book %King series, drop-out = 0 ----
function snap(t, l){ return {t:t, s:768.3, l:l}; }
const nat=[], der=[];
for(let i=0;i<30;i++){
  // 768.5-ish native 769 bleeds from 60% to gone at i>=20; 767 builds from 20% to 60%
  const l=[{k:768,v:100,d:1}]; if(i<20) l.push({k:769,v:60-2*i,d:1}); l.push({k:767,v:20+1.5*i,d:1}); nat.push(snap(i,l));
  // SPXW: 769.5 lane bleeding, 769.0 lane building — but 769 is native-owned so it is dropped
  der.push(snap(i,[{k:769.48,v:i<25?(100-3*i):10,d:1},{k:767.72,v:10+3*i,d:1}]));
}
global.LASTFEED={SPY:{j:{levels:nat, derived:[{source:'SPXW',levels:der}]}}};
const S769=feedSeries('SPY',769), S767=feedSeries('SPY',767), S7695=feedSeries('SPY',769.5), S7677=feedSeries('SPY',767.5);
ok('2a native series exist for 769 and 767', Array.isArray(S769) && Array.isArray(S767));
ok('2b 769 reads 0 after it DROPS OUT (dissipation, not missing data)', S769[29]===0 && S769[10]>0, [S769[10],S769[29]]);
ok('2c 767 builds 20% → 64% of the SPY King over the session', S767[29]===64 && S767[0]===20, [S767[0],S767[29]]);
ok('2d SPXW 769.5 lane has its own series (normalised to the SPXW King per snapshot)', Array.isArray(S7695) && S7695[0]===100, S7695&&S7695.slice(0,3));
ok('2e SPXW 767.7 snaps to 767.5 and reads 100% at the end (its book King)', Array.isArray(S7677) && S7677[29]===100, S7677&&S7677.slice(-2));

// ---- 2x. hiccup tolerance + gone rule + time alignment ----
ok('2f a single missing minute is not a death: feedSampleAt reads the neighbour', feedSampleAt([50,0,52],1)===50 && feedSampleAt([50,0,0],2)===0);
ok('2g gone needs 3 absent samples after being meaningful in the window', feedGoneAt([60,60,60,60,0,0,0],6)===true && feedGoneAt([60,60,60,60,60,0,0],6)===false && feedGoneAt([5,5,5,5,0,0,0],6)===false);
(function(){
  // derived book skips one minute: samples must align by TIME, not index
  const nat2=[], der2=[];
  for(let i=0;i<10;i++){ nat2.push({t:100+60*i,s:768,l:[{k:768,v:100,d:1}]}); if(i!==4) der2.push({t:100+60*i,s:768,l:[{k:769.48,v:10*(i+1),d:1},{k:766.5,v:100,d:1}]}); }
  const save=global.LASTFEED; global.LASTFEED={SPY:{j:{levels:nat2,derived:[{source:'SPXW',levels:der2}]}}};
  const v=feedSeries('SPY',769.5);
  ok('2h derived samples align by timestamp across a skipped minute (index 5 = t 400 → 60%)', v && v[5]===60 && v[9]===100 && v[4]===0, v);
  global.LASTFEED=save;
})();

// ---- 3. node state words ----
ok('3a m15 +12 → acm', mapNodeState({m15:{pct:12},session:{fromPeak:0}})==='acm');
ok('3b m15 −12 → dec', mapNodeState({m15:{pct:-12},session:{fromPeak:5}})==='dec');
ok('3c near peak, flat → hold; 30% off peak → dec even with flat m15', mapNodeState({m15:{pct:1},session:{fromPeak:3}})==='hold' && mapNodeState({m15:{pct:0},session:{fromPeak:30}})==='dec');
ok('3d gone flag → gone', mapNodeState({gone:true})==='gone');

// ---- 4. transfers, widening, lean — PURE on states, no SMA anywhere ----
const px=768.3;
const N1=[ {k:769.5,side:'above',state:'dec'}, {k:769,side:'above',state:'acm'}, {k:768,side:'below',state:'hold'}, {k:767.5,side:'below',state:'dec'}, {k:767,side:'below',state:'acm'} ];
const T1=mapTransfersOf(N1,px);
ok('4a ceiling 769.5 dec → 769 acm is a transfer rolling DOWN', T1.transfers.some(t=>t.side==='ceil'&&t.from===769.5&&t.to===769&&t.dir==='dn'), T1.transfers);
ok('4b floor 767.5 dec → 767 acm is a transfer rolling DOWN', T1.transfers.some(t=>t.side==='flr'&&t.from===767.5&&t.to===767&&t.dir==='dn'), T1.transfers);
ok('4c both sides rolling down → lean dn ("both sides rolling down")', T1.lean==='dn' && /both sides/.test(T1.leanWhy), [T1.lean,T1.leanWhy]);
// widening: 768.5 (nearest above) dies while 769.5 above and 767 below build
const N2=[ {k:769.5,side:'above',state:'acm'}, {k:768.5,side:'above',state:'dec'}, {k:768,side:'below',state:'hold'}, {k:767,side:'below',state:'acm'} ];
const T2=mapTransfersOf(N2,px);
ok('4d 768.5 dec between 769.5 acm and 767 acm → range widening 767–769.5', T2.widening && T2.widening.dead[0]===768.5 && T2.widening.hi===769.5 && T2.widening.lo===767, T2.widening);
ok('4e ...and no lean (nothing rolling)', T2.lean==='none' && T2.transfers.every(t=>t.side!=='flr'), T2);
// mirror: uptrend rolling up
const N3=[ {k:770,side:'above',state:'acm'}, {k:769,side:'above',state:'dec'}, {k:768,side:'below',state:'acm'}, {k:767,side:'below',state:'dec'} ];
const T3=mapTransfersOf(N3,768.3);
ok('4f mirror: 769→770 and 767→768 both UP → lean up', T3.lean==='up' && T3.transfers.length===2, T3);
// a lone ceiling roll with a silent floor still leans dn (the user\'s 12:05 case)
const N4=[ {k:769.5,side:'above',state:'dec'}, {k:769,side:'above',state:'acm'}, {k:768,side:'below',state:'hold'}, {k:767,side:'below',state:'hold'} ];
ok('4g ceiling rolling down, floor silent → lean dn ("ceiling rolling down")', mapTransfersOf(N4,px).lean==='dn' && /ceiling/.test(mapTransfersOf(N4,px).leanWhy));
// sides apart: ceiling down, floor... up = no lean
const N5=[ {k:769.5,side:'above',state:'dec'}, {k:769,side:'above',state:'acm'}, {k:768,side:'below',state:'acm'}, {k:767,side:'below',state:'dec'} ];
ok('4h ceiling rolling down + floor rolling up (compression) → no lean', mapTransfersOf(N5,px).lean==='none');

// ---- 5. words ----
ok('5a acm is GREEN, dec is RED, plain words (no arrows)', /#2ec27e/.test(mapWord('acm')) && />acm</.test(mapWord('acm')) && /#f0616d/.test(mapWord('dec')) && />dec</.test(mapWord('dec')) && !/[▲▼↑↓]/.test(mapWord('acm')+mapWord('dec')));
ok('5b the READ line is labelled "Map:"', /<b style="color:'\+PAL\.sub\+'">Map:<\/b>/.test(src));
ok('5c the Map line hover is question-first ("What is the structure doing?") and says it is independent of the SMA', /What is the structure doing\?[\s\S]{0,900}independent of the SMA/.test(src));
ok('5d rows carry the acm/dec/holding chip (in-play card AND other rows); the SPXW tag is OFF the rows (v11.0.1, hover only)', (src.match(/mapChipHtml\(mapStateOf\(sym,L\)\)/g)||[]).length===2 && (src.match(/mapSrcHtml\(L\)\+/g)||[]).length===0);
ok('5e the structure sentence is part of the ONE read (mapSentence inside read3Beat), no separate Map line', /mapSentence\(READ_SYM\|\|'SPY', leg, trendConf/.test(src) && !/return mapLineHtml\(sym, legR/.test(src));

// ---- 6. structure leads when the SMA has no trend ----
ok('6a legCtxOf: dirIn falls back to the Map lean when the five-state has no trend (dirSrc map)', /if\(dirIn==='none'\)\{\s*try\{ var mf=nodeFlow\(sym\); if\(mf && mf\.ok && \(mf\.lean==='dn'\|\|mf\.lean==='up'\)\)\{ dirIn=mf\.lean; dirSrc='map'; \}/.test(src));
ok('6b the direction spine: no trend + map lean → relation structure-leads, still capped C', /out\.relation='structure-leads'/.test(src) && /hardC=true;\s*\/\/ tentative is capped at C, always/.test(src));
ok('6c READ wording treats structure-leads like trend-only (no GEX/VEX beat)', /relation==='trend-only'\|\|relation==='structure-leads'/.test(src));
ok('6d the Map caveat: "structure leads, trend unconfirmed" when the SMA has nothing', /SMA-50 has no trend: structure leads, trend unconfirmed\./.test(src));

// ---- 7. enrollment ----
ok('7a map.transfer and map.lean are registered FEATURES', /registerFeature\(\{ key:'map\.transfer'/.test(src) && /registerFeature\(\{ key:'map\.lean'/.test(src));
const RJ=JSON.parse(fs.readFileSync('./learning/rules.json','utf8'));
ok('7b rules.json seeds map.transfer + map.lean (55 ids)', RJ.rules['map.transfer'] && RJ.rules['map.lean'] && Object.keys(RJ.rules).length>=54, Object.keys(RJ.rules).length);
ok('7c LLM brief evaluates the Map', /map\.transfer/.test(fs.readFileSync('./docs/LLM-NIGHTLY-BRIEF.md','utf8')));
console.log('test_node_map: '+p+' passed, '+f+' failed');
