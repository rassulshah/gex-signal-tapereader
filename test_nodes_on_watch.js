// v10.57 — NODES ON WATCH (user: "only nodes in play and relevant nodes"), DRIFT SHADOW MODE
// (user: "remove it until it is tested and proven"), and the Dashboard-tab fix (from Testing).
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);if(!m)throw new Error('no fn '+n);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
let p=0,f=0;const ok=(n,c,g)=>{if(c){p++;console.log('PASS '+n);}else{f++;console.log('FAIL '+n+(g!==undefined?' -> '+JSON.stringify(g):''));}};
global.PB_REACH=5; global.WATCH_N=4;
eval(['zoneMeaningful','nodesOnWatch'].map(ex).join('\n'));

// board: price 768.3. King 768 (below), 769 ceil, 771 mag, 773 ceil (gatekeeper), 767 flr (rolled off), 765 flr, 762 flr (beyond reach), 770 thin
const LV=[
  {k:773,pct:40,isCeil:true,isGatekeeper:true},
  {k:771,pct:45,isStrongMag:true},
  {k:770,pct:8},                       // thin: no role
  {k:769,pct:45,isCeil:true},
  {k:768,pct:100,isKing:true},
  {k:767,pct:41,isFlr:true},
  {k:765,pct:30,isFlr:true},
  {k:762,pct:60,isFlr:true},
];
const M={levels:LV.map(o=>Object.assign({},o))};
const px=768.3;
const legUp={dir:'up',pbDetected:{k:768,pct:100},magnet:{k:769,pct:45},rolledOff:[767],handoff:null};
const inPlay={k:769};   // 769 is the in-play card
let w=nodesOnWatch(M,px,legUp,inPlay);
const ks=w.map(x=>x.k), why=Object.fromEntries(w.map(x=>[x.k,x.watch]));
ok('1a the in-play node is NOT repeated in the list', ks.indexOf(769)<0, ks);
ok('1b the PB (768, King) is listed once, tagged PB (leg role wins over King)', ks.indexOf(768)>=0 && why[768]==='PB', why);
ok('1c the rolled-off 767 is OUT', ks.indexOf(767)<0, ks);
ok('1d thin 770 is OUT', ks.indexOf(770)<0, ks);
ok('1e beyond PB_REACH (762) is OUT', ks.indexOf(762)<0, ks);
ok('1f nearest meaningful wall above not already listed = 771 (next wall)', why[771]==='next wall', why);
ok('1g nearest meaningful wall below not already listed = 765 (next wall; 767 rolled off is skipped)', why[765]==='next wall', why);
ok('1h capped at WATCH_N and sorted as a price ladder (highest first)', w.length<=4 && ks.every((k,i)=>i===0||ks[i-1]>k), ks);

// TGT and PB both present when the in-play node is something else
let w2=nodesOnWatch(M,px,legUp,{k:771});
const why2=Object.fromEntries(w2.map(x=>[x.k,x.watch]));
ok('2a TGT (769) and PB (768) both listed when neither is in play', why2[769]==='TGT' && why2[768]==='PB', why2);

// handoff → next PB
const legDn={dir:'dn',pbDetected:{k:771,pct:45},magnet:{k:768,pct:100,isKing:true},rolledOff:[],handoff:{active:true,from:{k:771},to:{k:769,pctNow:12}}};
let w3=nodesOnWatch(M,768.3,legDn,null);
const why3=Object.fromEntries(w3.map(x=>[x.k,x.watch]));
ok('3a during a handoff the node building to be the next PB is listed as "next PB"', why3[769]==='next PB', why3);
ok('3b ...with the PB (771) and TGT (768 King) also listed', why3[771]==='PB' && why3[768]==='TGT', why3);

// no leg: King + nearest ceiling + nearest floor
let w4=nodesOnWatch(M,768.3,{dir:'none'},null);
const why4=Object.fromEntries(w4.map(x=>[x.k,x.watch]));
ok('4a no leg → King + nearest wall above (769) + nearest wall below (767)', why4[768]==='King' && why4[769]==='next wall' && why4[767]==='next wall', why4);
ok('4b nothing crashes on an empty board', nodesOnWatch({levels:[]},768,null,null).length===0);

// header + tags in the render
const blk=ex('deflZonesBlock');
ok('5a section header reads "Nodes on watch" with a question-first hover ("Why these nodes?")', /Nodes on watch/.test(blk) && /Why these nodes\?/.test(blk));
ok('5b the list is nodesOnWatch(...) not the old top-N sort', /var pick=nodesOnWatch\(m, px, legR, inPlay\)/.test(blk) && !/others\.slice\(0,DEFLZONES_N\)/.test(blk));
ok('5c "next PB" and "next wall" tags render on rows', /L\.watch==='next PB'/.test(blk) && /L\.watch==='next wall'/.test(blk) && /legTagHtml\(lt, legR\)/.test(blk));

// ---- DRIFT SHADOW MODE ----
ok('6a DRIFT_LIVE=false (shadow) is the shipped default', /^var DRIFT_LIVE=false;/m.test(src));
ok('6b the Drift row is off the face while !DRIFT_LIVE', /if\(DRIFT_LIVE\)\{ try\{ html\+=driftLineHtml\(__asym\); \}catch\(eD49\)\{\} \}/.test(src));
ok('6c the live hierarchy forces driftD to 0 while !DRIFT_LIVE (drift never votes)', /var shadowDriftD=driftD;\s*if\(!DRIFT_LIVE\) driftD=0;/.test(src));
ok('6d the shadow relation/direction is still computed (out.shadow) for the learning loop', /out\.shadow=\{ relation:sRel, dir:/.test(src));
ok('6e dir.relation RECORDS the shadow relation while !DRIFT_LIVE (the claim under test stays measurable)', /var sh=\(d\.shadow&&!d\.driftLive\)\?d\.shadow:null;/.test(src) && /shadow:!!sh, liveDir:d\.dir\|\|null/.test(src));
ok('6f READ tentative wording has no GEX/VEX lean while !DRIFT_LIVE', /!DRIFT_LIVE\) \? 'No trend — no lean; rotation likely\.'/.test(src));
ok('6g direction hover explains SHADOW mode', /GEX\/VEX drift is in SHADOW mode/.test(src));
// live behaviour: with DRIFT_LIVE=false a confirmed up trend + diverging drift is trend-only B, not divergence C
(function(){
  try{
    global.DRIFT_LIVE=false;
    global.STATE={SPY:{price:772.6,candles:[],lastClosedB:1}};
    global.DIR_WEIGHTS={trend:3,driftAgree:2,driftLean:1,diverge:-2,tentative:1,gradeA:5,gradeB:3};
    global.trendVerdict=()=>({state:'up',up:16,dn:1,win:20,slope:0.4});
    global.driftRead=()=>({verdict:'AGREE-DN',dir:-1,gvwap:770,vvwap:769,overlap:true});
    global.nodeMapModel=()=>({ok:true,px:772.6,flr:{k:770},ceil:{k:778},levels:[],kingK:775});
    global.netPositioning=()=>null; global.regimeTag=()=>({tag:'trend',er:0.6}); global.closedCandles=()=>[];
    global.rangePosOf=()=>({pos:0.9,zone:'upper',lo:770,hi:778}); global.sessionBucket=()=>({bucket:'mid',opex:false,capOdds:false});
    global.legEngine=()=>null; global.sessionRoll=()=>null; global.killCheck=()=>null; global.promoMarker=()=>null;
    global.gradeOfScore=(s)=>s>=5?'A':(s>=3?'B':'C'); global.gradeDisp=(g)=>g; global.ruleTier=()=>'⚖';
    eval(ex('directionGrade'));
    var d=directionGrade('SPY');
    ok('7a LIVE: up trend + diverging drift → relation trend-only, grade B (drift cannot cap while in shadow)', d.dir==='UP' && d.relation==='trend-only' && d.grade==='B', {dir:d.dir,rel:d.relation,g:d.grade});
    ok('7b ...but the SHADOW says divergence (recorded for the review)', d.shadow && d.shadow.relation==='divergence' && d.shadow.dir==='UP', d.shadow);
    global.trendVerdict=()=>({state:'flat',up:9,dn:8,win:20,slope:0});
    d=directionGrade('SPY');
    ok('7c LIVE: no trend → SIDE (drift lean not used); shadow keeps the lean', d.dir==='SIDE' && d.shadow && d.shadow.dir==='DN', {dir:d.dir,sh:d.shadow});
    global.DRIFT_LIVE=true; global.trendVerdict=()=>({state:'up',up:16,dn:1,win:20,slope:0.4});
    d=directionGrade('SPY');
    ok('7d when promoted (DRIFT_LIVE=true) the hierarchy is unchanged: divergence hard-caps at C', d.relation==='divergence' && d.grade==='C', {rel:d.relation,g:d.grade});
  }catch(e){ ok('7 live drift shadow threw: '+e, false); }
})();

// ---- Dashboard tab from Testing ----
ok('8a Dashboard tab calls showDashboard(), which clears BOTH view flags', /showDashboard\(\)'/.test(src) && /showDashboard=function\(\)\{ ANALYSIS_VIEW=false; TESTING_VIEW=false;/.test(src));
console.log('test_nodes_on_watch: '+p+' passed, '+f+' failed');
