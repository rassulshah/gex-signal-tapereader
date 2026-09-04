// ============================================================================================
// test_v1563.js — (v15.63) THE DASHBOARD CONVERSATION: the node row (NEW · ⇄ ROLL · ▲ GROWTH · SETUP), the King zone,
//   the tally, the SWEPT line, the DAY table off the face. Every decision is his (2026-09-03/04); the reference is
//   design/mockup-king-strip.html v3b. Fixtures are the real book of 2026-09-03 12:48 CT where a number matters.
//   ⚠ (v15.64) the assertions marked (v15.64) were REWRITTEN when he corrected the tells the same day: NEW needs a
//   below-observation and growth, a stack is named once on members ≥ 30%, the rugs take price's side, the SWEPT line
//   is names-only, the tally is off the face. test_v1564.js carries the new rules in full; what stays here is the
//   v15.63 surface that survived.
// ============================================================================================
const fs=require('fs');
const SRC=process.env.GPTS_SRC||'./current/gex-signal-tapereader.user.js';
const src=fs.readFileSync(SRC,'utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,400):''));} };
function ex(n){ const m=new RegExp('function\\s+'+n+'\\s*\\(','g').exec(src); if(!m) throw new Error('not found: '+n);
  let i=src.indexOf('{',m.index),d=0; for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){ d--; if(!d) return src.slice(m.index,k+1); } } }
const exVar=(n)=>{ const i=src.indexOf('var '+n+'='); const j=src.indexOf(';\n', i); return src.slice(i, j+1); };
const build=(g,fns,tail)=>new Function('__g', Object.keys(g).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+exVar('GRID_NEW_BARS')+exVar('GRID_STACK_STEPS')+exVar('GRID_STACK_MAX_PCT')+exVar('GRID_RUG_FLOOR_STEPS')+exVar('GRID_STACK_PTS')+exVar('GRID_LVL_SNAP_PTS')+exVar('GROWTH_WINDOWS')+exVar('NODEBORN_KEY')+exVar('NODE_BORN')+'\n'+fns.map(ex).join('\n')+'\n'+ex('gridStep')+'\n'+ex('gridReplay')+'\n'+ex('gridNow')+'\n'+ex('bornFromSnaps')+'\n'+ex('gridBook')+'\n'+exVar('REPLAY_BORN')+'\n'+(tail||''));
const esc=s=>String(s==null?'':s).replace(/"/g,'&quot;').replace(/</g,'&lt;');
const tip=t=>t?(' title="'+esc(t)+'"'):'';
const two=x=>{ x=''+x; return x.length<2?'0'+x:x; };
const fnum=x=>(typeof x==='number')?(Math.round(x*100)/100).toFixed(x%1?2:0).replace(/(\.\d*?)0+$/,'$1').replace(/\.$/,''):String(x);   // ⚠ strips zeros only AFTER a point — the older form turned 7690 into 769

// ---- 1 · GROWTH: a share of the node, sign-aware, the window a setting ----------------------------------
{
  const VEL={ 7740:{cur:94.4e6,d5:2.6e6,d15:19.5e6,d60:39.9e6}, 7750:{cur:273.2e6,d15:7.0e6}, 7675:{cur:-58e6,d15:-26e6}, 7725:{cur:15.8e6,d15:-4.7e6}, 9999:{cur:1e6,d15:2e6} };
  const g={ VEL, CFG:{growthWin:15} };
  const f=build(g,['growthWin','nodeGrowth','growthHtml'],'return { nodeGrowth, growthHtml, growthWin };')(g);
  ok(f.nodeGrowth(7740)===26,'1a 7740 at 12:48: +$19.5M on a $74.9M base = +26% (the mockup’s number, no dollars)',f.nodeGrowth(7740));
  ok(f.nodeGrowth(7750)===3,'1b the King +$7M on $266M = +3% — growth is a share of the node, so the King’s +$7M reads small',f.nodeGrowth(7750));
  ok(f.nodeGrowth(7675)===81,'1c a −γ node that went −$32M → −$58M GREW +81% (magnitude, sign-aware)',f.nodeGrowth(7675));
  ok(f.nodeGrowth(7725)===-23,'1d a shrinking node reads negative (−$4.7M on $20.5M = −23%)',f.nodeGrowth(7725));
  ok(f.nodeGrowth(9999)===null && f.nodeGrowth(1)===null,'1e a node that did not exist at the window’s start (base ≤ 0), or is unknown, has no growth — never a fake %');
  ok(f.nodeGrowth(7740,5)===3 && f.nodeGrowth(7740,60)===73,'1f the window is a setting: 5 min → +3%, 60 min → +73%',[f.nodeGrowth(7740,5),f.nodeGrowth(7740,60)]);
  g.CFG.growthWin=60; ok(f.growthWin()===60 && f.nodeGrowth(7740)===73,'1g CFG.growthWin picks the window; an unknown value falls back to 15');
  g.CFG.growthWin=30; ok(f.growthWin()===15,'1g2 …30 is not recorded (d5 · d15 · d60), so it falls back');
  ok(/g3gg up">▲ \+26%<\/span>/.test(f.growthHtml(26)) && /g3gg dn">▼ -23%/.test(f.growthHtml(-23)) && /g3gg fl">▸ \+2%/.test(f.growthHtml(2)) && /g3gg fl">—/.test(f.growthHtml(null)),'1h the growth cell: arrow + percent, no dollars anywhere',[f.growthHtml(26),f.growthHtml(-23)]);
  ok(!/\$/.test(f.growthHtml(26)+f.growthHtml(-23)),'1i …literally no $ in the cell');
}

// ---- 2 · ROLL: from / to with the roll’s own direction ---------------------------------------------------
{
  const g={ two, g3tip:tip };
  const f=build(g,['rollTagFor','rollHtml'],'return { rollTagFor, rollHtml };')(g);
  const rolls=[{from:7745,to:7750,dir:'up',lastT:1788464400000,live:true},{from:7790,to:7715,dir:'down',lastT:1788460000000,live:false},{from:7700,to:7705,dir:'up',lastT:1,gone:true}];
  const a=f.rollTagFor(7750,rolls), b=f.rollTagFor(7745,rolls), c=f.rollTagFor(7715,rolls), d=f.rollTagFor(7705,rolls);
  ok(a && a.end==='from' && a.other===7745 && a.dir==='up','2a the node that RECEIVED the roll reads ▲ from 7745',a);
  ok(b && b.end==='to' && b.other===7750 && b.dir==='up','2b the node that LOST it reads ▲ to 7750 — a roll is between any two nodes',b);
  ok(c && c.end==='from' && c.dir==='dn','2c a roll down carries its own direction (▼)',c);
  ok(d===null,'2d a roll marked gone is not drawn');
  const h=f.rollHtml(a);
  ok(/g3chip roll up/.test(h) && /▲ from 7745 · \d\d:\d\d/.test(h) && !/\$/.test(h),'2e the chip: arrow, from, strike, time — no dollars',h);
  ok(/g3chip roll dn/.test(f.rollHtml(c)) && /▼ from 7790/.test(f.rollHtml(c)),'2f …and the down roll is red-classed with ▼');
}

// ---- 3 · SETUP: the patternpedia’s shapes, per book -------------------------------------------------------
{
  const g={ g3tip:tip, g3esc:esc };
  const f=build(g,['gridSetups','setupHtml'],'return { gridSetups, setupHtml };')(g);
  const N=(book,list)=>list.map(x=>({book, k:x[0], pct:x[1], disp:x[2], pos:x[1]>0})).sort((a,b)=>b.disp-a.disp);
  // the SPXW book at 12:48: all +γ, 7740/7735 five points apart
  const spx=f.gridSetups(N('SPX',[[7755,20,7763.4],[7750,100,7758.4],[7745,45,7753.4],[7740,35,7748.4],[7735,26,7743.4]]));
  // (v15.64) members are the nodes ≥ 30% of the King: 7750 · 7745 · 7740; 7755 (20%) and 7735 (26%) are not members
  ok(spx[7750] && spx[7750][0].kind==='pika' && spx[7750][0].txt==='SPX PIKA STACK · 7740–7750' && spx[7745] && /^stk pika$/.test(spx[7745][0].kind) && spx[7740] && /^stk pika$/.test(spx[7740][0].kind) && !spx[7755] && !spx[7735],'3a (v15.64) the three +γ members ≥ 30% = ONE pika cloud 7740–7750, named ONCE on the King, the other two bracketed; the 20% and 26% neighbours are not members',spx);
  const spx2=f.gridSetups(N('SPX',[[7755,20,7763.4],[7745,22,7753.4],[7740,18,7748.4]]));
  ok(!Object.keys(spx2).length,'3a2 a run whose biggest member is under 40% of the King is not a cloud — mass is required');
  ok(!Object.values(spx).flat().some(s=>s.kind==='rug'||s.kind==='rrug'||s.kind==='barney'),'3b an all-yellow book has no rug, no reverse rug, no barney');
  // the SPY book at 12:48: 773 (+γ King) above 772 · 771 (−γ), only a $15M floor 768.5 (below the threshold → not a node)
  const spy=f.gridSetups(N('SPY',[[774,33,7765.8],[773,100,7755.7],[772,-35,7745.7],[771,-74,7735.7]]));
  ok(spy[773] && spy[773].some(s=>s.kind==='rug' && /SPY RUG · 773 over 772/.test(s.txt)),'3c yellow 773 directly above purple 772 with no +γ node within 15 points below → SPY RUG on the yellow row (no price given: no side rule)',spy);
  // (v15.64) named once, on the biggest member (771 at 74%); 772 (35%) is bracketed
  ok(spy[771] && spy[771].some(s=>s.kind==='barney' && s.txt==='SPY BARNEY STACK · 771–772') && spy[772] && spy[772].some(s=>/^stk barney$/.test(s.kind)) && !spy[772].some(s=>s.kind==='barney'),'3d (v15.64) 772 · 771 both −γ within 1, each ≥ 30% → SPY BARNEY STACK named on 771 (74%), 772 bracketed',spy);
  ok(!spy[773].some(s=>s.kind==='rrug'),'3e …and no reverse rug on that side');
  // a floor within 15 points cancels the rug
  const spy2=f.gridSetups(N('SPY',[[773,100,7755.7],[772,-21,7745.7],[770,40,7725.7]]));
  ok(!(spy2[773]||[]).some(s=>s.kind==='rug'),'3f a +γ node within 3 strikes under the purple one is a floor in sight — no rug called');
  const spy3=f.gridSetups(N('SPY',[[773,100,7755.7],[772,-21,7745.7],[768,40,7705.7]]));
  ok((spy3[773]||[]).some(s=>s.kind==='rug'),'3f2 …four strikes down is out of sight: the rug is called');
  // the reverse rug: purple directly above yellow with no purple ceiling within 15 above
  const q=f.gridSetups(N('QQQ',[[719,-30,7770.5],[718,100,7759.7]]));
  ok(q[718] && q[718].some(s=>s.kind==='rrug' && /QQQ REVERSE RUG · 719 over 718/.test(s.txt)),'3g purple 719 directly over yellow 718, no purple ceiling above → QQQ REVERSE RUG on the yellow row',q);
  const q2=f.gridSetups(N('QQQ',[[721,-20,7791],[719,-30,7770.5],[718,100,7759.7]]));
  ok(!(q2[718]||[]).some(s=>s.kind==='rrug'),'3h a purple ceiling within 15 points above cancels the reverse rug (the barney pair stays)');
  const q3=f.gridSetups(N('QQQ',[[718,100,7759.7],[715,-27,7727]]));
  ok(!(q3[718]||[]).some(s=>s.kind==='rug'),'3k a purple node three strikes below is not "stacked" under the yellow — no rug (the docs: directly above)');
  const html=f.setupHtml(spy[773]);
  ok(/g3pb rug/.test(html) && /title="SPY RUG · 773 over 772 — /.test(html) && /<i class="g3pr">773\/772<\/i>/.test(html),'3i (v15.65) the block: RUG with its two strikes; the pattern’s name, the book and the patternpedia’s definition in the hover');
  ok(f.setupHtml([])==='' && f.setupHtml(null)==='','3j nothing present → nothing drawn');
}

// ---- 4 · NEW: the age of a strike, seeded from today’s recorded book ------------------------------------
{
  const now=1788465600000; // 2026-09-03 14:00 CT
  // (v15.64) consecutive 3-minute bars — a 30-minute hole between frames is a RECORDING GAP and the first bar after one is never a birth
  const snaps=[ { bar:now-1980000, vend:{rows:[[7750,200e6,0,0,0,0],[7745,60e6,0,0,0,0],[7730,20e6,0,0,0,0]]} },   // 13:27 — the opening book of this fixture; 7730 at 10% is not a node
                { bar:now-1800000, vend:{rows:[[7750,220e6,0,0,0,0],[7745,70e6,0,0,0,0],[7755,50e6,0,0,0,0]]} } ];  // 13:30 — 7755 crosses (22%), absent from the list before
  const store={};
  // (v15.64) 7755 was ABSENT from the 13:00 top list (= small) and crossed at 13:30; it is $120M now against $50M then
  const VEL={ 7755:{cur:120e6,d15:40e6}, 7750:{cur:220e6,d15:5e6} };
  const g={ VEL, CFG:{nodeThresh:20, growthWin:15}, ctTodayStr:()=>'2026-09-03', recorderLoad:()=>({days:{'2026-09-03':{snaps:{SPY:snaps}}}}), localStorage:{getItem:k=>store[k]||null,setItem:(k,v)=>{store[k]=v;}} };
  const f=build(g,['nodeBornLoad','nodeBornSave','nodeBelowTouch','nodeBornTouch','nodeBornOf','nodeAgeBars','nodeIsNew','nodeGrowth','growthWin'],'return { nodeBornLoad, nodeBornTouch, nodeBelowTouch, nodeAgeBars, nodeIsNew };')(g);
  ok(f.nodeAgeBars(7750,now)===null && f.nodeAgeBars(7755,now)===10 && f.nodeAgeBars(7730,now)===null,'4a (v15.64) seeded from the day’s book: 7750 is the OPENING book (never a birth), 7755 born 13:30 (10 bars), 7730 never a node',[f.nodeAgeBars(7750,now),f.nodeAgeBars(7755,now)]);
  const nw=f.nodeIsNew(7755,now);
  ok(nw && nw.age===10 && nw.x===2.4 && nw.g===50 && f.nodeIsNew(7750,now)===null && f.nodeIsNew(7755,now+40*180000)===null,'4b (v15.64) NEW = born within 20 bars AND grew (×2.4 its size at the crossing, +50% over the window); not the opening book, not after the window',nw);
  f.nodeBornTouch(7760, now, 60e6, 27); ok(f.nodeAgeBars(7760,now+180000)===null && !(store['gpts_nodeborn_v2'] && JSON.parse(store['gpts_nodeborn_v2']).m[7760]),'4c (v15.64) a strike FIRST SEEN live at the threshold is NOT born — first sight is never a birth');
  f.nodeBelowTouch(7760); f.nodeBornTouch(7760, now, 60e6, 27); const b60=JSON.parse(store['gpts_nodeborn_v2']).m[7760];
  ok(f.nodeAgeBars(7760,now+180000)===1 && b60 && b60.t===now && b60.mag===60e6 && b60.pct===27,'4c2 (v15.64) …seen BELOW first, then at the threshold: born now, with its size at the crossing, and persisted (v2 store)',b60);
  f.nodeBornTouch(7755, now, 1, 1); ok(f.nodeAgeBars(7755,now)===10,'4d a second touch never moves the birth');
}

// ---- 5 · the tally: every King tap today, per book, counts until n ≥ 15 -----------------------------------
{
  const mk=(kings,cont,t)=>({kings,cont,t});
  const day={ defl:{ SPY:[mk(['SPX'],1,1),mk(['SPX'],1,2),mk(['SPX','SPY'],0,3),mk([],1,4),mk(['QQQ'],null,5)], QQQ:[mk(['QQQ'],1,6)] } };
  const g={ TODAY:'2026-09-03', recorderLoad:()=>({days:{'2026-09-03':day}}), RATE_MIN_N:15, DEFL_CONT_PTS:2, DEFL_FWD_BARS:10, g3tip:tip, g3esc:esc };
  const f=build(g,['kingTapsToday','tallyLineHtml'],'return { kingTapsToday, tallyLineHtml };')(g);
  const T=f.kingTapsToday();
  ok(T.SPX.held===2 && T.SPX.broke===1 && T.SPY.broke===1 && T.SPY.held===0 && T.QQQ.held===1 && T.QQQ.pending===1,'5a taps counted per book from the ledger’s kings[] — a tap of two Kings counts for both',T);
  const h=f.tallyLineHtml('SPY');
  ok(/SPX King 2 held · 1 broke/.test(h) && /SPY King 0 held · 1 broke/.test(h) && /QQQ King 1 held · 0 broke · 1 pending/.test(h) && !/%/.test(h),'5b the line prints counts and NO rate under n=15',h);
  for(let i=0;i<14;i++) day.defl.SPY.push(mk(['SPX'],1,10+i));
  ok(/SPX King 16 held · 1 broke · 94% \(n=17\)/.test(f.tallyLineHtml('SPY')),'5c …and a rate with its n once a book reaches 15');
  const g2=Object.assign({},g,{ recorderLoad:()=>({days:{}}) });
  ok(/SPX King —/.test(build(g2,['kingTapsToday','tallyLineHtml'],'return tallyLineHtml;')(g2)('SPY')),'5d no taps yet → “—”, never 0%');
}

// ---- 6 · the SWEPT line: key levels only, simple words, the rates in the hover ------------------------------
{
  const ev=[ {level:'ONL-',side:'LOD',px:7743,at:'08:41',atBar:11,status:'reclaimed',speed:11,depth:6}, {level:'VW1L',side:'LOD',px:7738,at:'09:10',atBar:40,status:'reclaimed',speed:3,depth:1}, {level:'PDH+',side:'HOD',px:7702,at:'08:30',atBar:0,status:'accepted',depth:3}, {level:'IBH+',side:'HOD',px:7726,at:'09:31',atBar:61,status:'pending',depth:0.5} ];
  const g={ sweepEventsToday:()=>ev, statsRead:()=>({lines:[{txt:'ONL swept 08:41 · deep · slow — printed the extreme 40% (n=86) vs 24% control'}]}), levelTier:n=>({ONL:1,PDH:1,IBH:2,VW1L:3}[String(n).replace(/[-+]$/,'')]||4), g3tip:tip, g3esc:esc, frameNum:fnum, SWEEP_RECLAIM_MAX:30 };
  const h=build(g,['sweptLineHtml'],'return sweptLineHtml("SPY");')(g);
  // (v15.64) names only, grouped by the side price was working, the latest sweep's side first; time · price · status in each name's hover
  const face=h.replace(/title="[^"]*"/g,'');
  ok(/<em>SWEPT<\/em>/.test(h) && /<em>making HOD<\/em> <b class="open" >PDH<\/b> <i>·<\/i> <b class="tst" >IBH<\/b>/.test(face) && /<em>making LOD<\/em> <b class="rec" >ONL<\/b>/.test(face) && face.indexOf('making HOD')<face.indexOf('making LOD'),'6a (v15.64) SWEPT · making HOD: PDH · IBH · making LOD: ONL — names only, the latest sweep’s side (IBH at bar 61) first',face);
  ok(!/VW1L/.test(face) && !/40%/.test(face) && !/7743|08:41|reclaimed/.test(face),'6b (v15.64) a tier-3 level (VWAP band), the rates, the prices and the times stay OFF the line',face);
  ok(/<b class="rec" title="ONL 7743 — swept 08:41 · reclaimed in 11 bars\."/.test(h) && /title="PDH 7702 — swept 08:30 · opened beyond\."/.test(h) && /title="IBH 7726 — swept 09:31 · being tested\."/.test(h),'6b2 (v15.64) …each name’s hover carries its price, time and status; the ± suffix is stripped from the name',h);
  ok(/title="[^"]*40% \(n=86\)/.test(h),'6c …the rates are in the hover');
  const g2=Object.assign({},g,{ sweepEventsToday:()=>[] });
  ok(/no key level swept yet today/.test(build(g2,['sweptLineHtml'],'return sweptLineHtml("SPY");')(g2)),'6d nothing swept → says so');
}

// ---- 7 · the grid ladder: the mockup’s columns, the King zone, no dollars, no MARK/STATE words --------------
{
  const VEL={ 7755:{cur:55.9e6,d15:11.9e6}, 7750:{cur:273.2e6,d15:7e6}, 7745:{cur:123.6e6,d15:12.3e6}, 7740:{cur:94.4e6,d15:19.5e6}, 7735:{cur:69.9e6,d15:17.2e6}, 7730:{cur:29.4e6,d15:-1.8e6} };
  const now=1788465600000;
  const store={ gpts_nodeborn_v2: JSON.stringify({day:'2026-09-03', m:{7755:{t:now-180000,mag:25e6,pct:10}, 7750:now-4e6, 7745:now-4e6, 7740:now-4e6, 7735:now-4e6}, below:{}}) };   // (v15.64) v2: a birth carries its size; a bare number still reads (a v1 shape)
  const tape={ pct:{'7755':20,'7750':100,'7745':45,'7740':35,'7735':26,'7730':11}, king:7750, count:100 };
  const ladders={ SPY:{ pct:{'774':33,'773':100,'772':-21,'771':-74}, king:773, kingKd:390226 }, QQQ:{ pct:{'720':17,'719':47,'718':100,'715':-27}, king:718, kingKd:257257 } };
  const g={ VEL, CFG:{nodeThresh:20, growthWin:15}, STATE:{ SPY:{price:773.03}, QQQ:{price:717.66} }, TODAY:'2026-09-03', ctTodayStr:()=>'2026-09-03',
    recorderLoad:()=>({days:{'2026-09-03':{snaps:{SPY:[]},defl:{}}}}), localStorage:{getItem:k=>store[k]||null,setItem:(k,v)=>{store[k]=v;}},
    ifLadder:()=>({ px:7756, dispScale:1.00108, undPx:773.03, rows:[{id:'CR0',disp:7766.0},{id:'PS0',disp:7736.0}] }),
    tapeMap:()=>tape, laddersByDollar:()=>ladders, g3tip:tip, g3esc:esc, frameNum:fnum, two, LAD_KING_TEST_PTS:2, Date:class extends Date{ static now(){ return now; } } };   // the clock frozen at 14:00 CT, the constructor real
  const fns=['growthWin','nodeBornLoad','nodeBornSave','nodeBelowTouch','nodeBornTouch','nodeBornOf','nodeAgeBars','nodeIsNew','nodeGrowth','growthHtml','rollTagFor','rollHtml','gridDisp','gridBookNodes','gridSetups','setupHtml','kingPathSeed','kingPathTouch','kingTriTouch','kingGrowth','kingsNow','kingTapsToday','kingCellHtml','kingStripHtml','ladderGridHtml'];
  const PS=[{k:7755,disp:7763.4,pct:20,brake:true},{k:7750,disp:7758.4,pct:100,brake:true,role:'KING'},{k:7745,disp:7753.4,pct:45,brake:true},{k:7740,disp:7748.4,pct:35,brake:true},{k:7735,disp:7743.4,pct:26,brake:true},{k:7730,disp:7738.3,pct:11,brake:true,sub:true}];
  const rolls=[{from:7745,to:7750,dir:'up',lastT:now-1e6,live:true}];
  g.readTrinityHeaders=()=>null; g.RATE_MIN_N=15;   // (v15.64) gridDisp asks the page's Trinity headers first, live
  const R=build(g,fns,exVar('KING_PATH')+' return { grid:ladderGridHtml("SPY", __g.PS, __g.rolls, {pdh:7708.25,pdl:7674.75,pdc:7697.75}, {ok:true,high:7743.4,low:7665.1}), strip:kingStripHtml("SPY", __g.rolls), kings:kingsNow("SPY") };')(Object.assign({PS,rolls},g));
  const grid=R.grid;
  ok(/g3gr hd">.*level.*price · strike.*node · %king.*new.*⇄ roll.*▲ growth 15m.*pattern · SPX.*SPY.*QQQ/.test(grid),'7a the header row: the mockup’s six columns, then PATTERN per book (v15.65)',grid.slice(0,400));
  ok(!/\$/.test(grid) && !/IN PLAY|DEFENDING|BREAKING|ATTRACTING|BUILDING|WEAKENING|SPENT/.test(grid),'7b no dollar amounts, no MARK / STATE words on the grid');
  ok(/♛ SPX KING/.test(grid) && /♛ SPY KING/.test(grid) && /♛ QQQ KING ≈/.test(grid) && /KING ZONE/.test(grid) && /3 layers<\/i>/.test(grid) && /3 layers, 4 points top to bottom/.test(grid),'7c the three Kings are rows of one zone, bracketed with its layer count and span',(grid.match(/layers[^<]*/)||[])[0]);
  const order=['♛ QQQ KING','♛ SPX KING','class="g3now"','♛ SPY KING'].map(t=>grid.indexOf(t));
  ok(order.every(i=>i>=0) && order[0]<order[1] && order[1]<order[2] && order[2]<order[3],'7d …in price order: QQQ K 7759.7 · SPX K 7758.4 · NOW 7756 · SPY K 7755.7 (NOW inside the zone)',order);
  ok(/NEW 1b ×2\.2/.test(grid) && grid.indexOf('NEW 1b')>grid.indexOf('7763.4') && grid.indexOf('NEW 1b')<grid.indexOf('7758.4') && (grid.match(/g3chip new/g)||[]).length===1,'7e (v15.64) 7755, born one bar ago at $25M and $55.9M now, reads NEW 1b ×2.2 on its row — and it is the ONLY NEW chip on the ladder');
  ok(/▲ from 7745/.test(grid) && /▲ to 7750/.test(grid),'7f the roll shows on both ends: the King received it, 7745 lost it');
  ok(/▲ \+27%/.test(grid) && /▲ \+26%/.test(grid) && /▲ \+33%/.test(grid) && /▲ \+3%/.test(grid),'7g growth per row as the mockup’s numbers (the King’s +3% from its own strike)',(grid.match(/g3gg [a-z]+">[^<]*/g)||[]));
  const gface=grid.replace(/title="[^"]*"/g,'');
  ok((gface.match(/g3pb pika">PIKA<\/span><i class="g3pr">7740–7750/g)||[]).length===1 && (gface.match(/g3stk pika/g)||[]).length===2 && /g3gc pat qqq"><span class="g3pat"\s*><span class="g3pb pika">PIKA<\/span><i class="g3pr">718–719/.test(gface) && /g3gc pat spy"><span class="g3pat"\s*><span class="g3pb pika">PIKA<\/span><i class="g3pr">773–774/.test(gface),'7h (v15.65) each stack ONCE, as a block in its book’s column — SPX 7740–7750 on the King with its members bracketed, QQQ 718–719 in the QQQ column on the QQQ King row, SPY 773–774 in the SPY column on the SPY King row');
  ok(!/SPY RUG/.test(grid),'7h2 (v15.64) …and no SPY RUG: price (7756) sits ABOVE the yellow 773 (7755.7), so the yellow is a floor under price, not the rug’s ceiling — the docs’ "spot positioned below the positive node"');
  ok(/g3gr zone king/.test(grid) && /g3rolled|g3kbadges/.test(R.strip) && !/SPX King —|SPY King —|QQQ King —/.test(grid+R.strip),'7h3 (v15.64) the King rows carry the king class (lit and pulsing in CSS); the tally text is off the face');
  ok(/g3gr sub">/.test(grid) && /context row/.test(grid) && !/7738\.3[\s\S]{0,300}NEW/.test(grid),'7i a context row (below the threshold) draws its bar and nothing else');
  ok(/PDH/.test(grid) && /PDL/.test(grid) && /EMH/.test(grid) && /g3gc lv/.test(grid),'7j the levels stay in their own column (PDH/PDL/PDC, the EM edges, the IF book)');
  const strip=R.strip;
  ok((strip.match(/class="g3kc /g)||[]).length===3 && /SPY KING/.test(strip) && /SPX KING · flow/.test(strip) && /QQQ KING ≈/.test(strip),'7k the strip: three cells, SPY · SPX · QQQ');
  ok(/g3pos be">BELOW · 0\.3/.test(strip) && /g3pos ab">ABOVE · 2\.4/.test(strip) && /g3pos ab">ABOVE · 3\.7/.test(strip),'7l each cell’s ABOVE / BELOW price badge with the distance',(strip.match(/g3pos [^<]*/g)||[]));
  ok((strip.match(/<em>growth<\/em>/g)||[]).length===3 && (strip.match(/<em>rolled<\/em>/g)||[]).length===3 && !/<em>size<\/em>|<em>taps<\/em>|<em>price<\/em>/.test(strip),'7m …and only GROWTH and ROLLED under the price — no size, no taps, no dollars');
  ok(/▲<\/span> up · from 7745 at \d\d:\d\d/.test(strip),'7n the SPX King’s ROLLED line carries the arrow head and the roll');
  ok(R.kings.SPX.k===7750 && Math.abs(R.kings.SPY.disp-7755.7)<0.1 && Math.abs(R.kings.QQQ.disp-7759.7)<0.1 && R.kings.QQQ.bearing===true,'7o the Kings on the chart’s scale: SPY 773 → 7755.7, QQQ 718 → ≈7759.7 (a bearing)',R.kings);
}

// ---- 8 · the wiring: the toggles, the ledger’s kings, the DAY table off the face ---------------------------
ok(/ladderGrid: true,/.test(src) && /dayRead: true,/.test(src) && /growthWin: 15,/.test(src),'8a the three settings default on (15 min)');
ok(/if\(CFG\.ladderGrid!==false\)\{[\s\S]{0,600}kingStripHtml\(sym, RAILROLLS\)\+ladderGridHtml\(sym, RAILPS_DRAW, RAILROLLS, SESSL, EB\)/.test(src) && /\} else \{\s*try\{ h\+=ladderHtml\(EB, RB, sym, RAILPS_DRAW/.test(src),'8b the grid replaces the ladder when on; the v15.62 ladder is one toggle away (v15.64: the tally line is off the face)');
ok(/if\(CFG\.dayRead===false\)\{\s*function row\(cls,tag,cells\)/.test(src) && /\}\s*\/\/ \(v15\.63\) end of the statistics table/.test(src),'8c the 22-column table renders only with dayRead off');
ok(/try\{ if\(CFG\.dayRead===false\)\{\s*var _ddb=recorderLoad\(\)/.test(src),'8d the taps list too');
ok(/if\(CFG\.dayRead!==false\)\{ try\{ h\+=sweptLineHtml\(sym\); \}/.test(src) && /else \{ try\{ h\+=statsReadHtml\(sym\); \}/.test(src),'8e the SWEPT line replaces the two-line read when on');
ok(/range ~'\+base\.rngPts\+' pts from the first extreme/.test(src),'8f the READ line’s timing text now says where the other side could go (the range)');
ok(/kings:\(function\(\)\{ try\{ return kingsAtStrike\(sym, L\.k\); \}/.test(src),'8g the ledger records which Kings a tap touched');
ok(/gpts-ladgrid/.test(src) && /gpts-dayread/.test(src) && /gpts-growthwin/.test(src) && /CFG\.growthWin=parseInt\(gwx\.value,10\)/.test(src),'8h the gear has the two toggles and the growth window');
ok(/o\.growthWin===5\|\|o\.growthWin===15\|\|o\.growthWin===60/.test(src),'8i a stored window outside 5 · 15 · 60 is ignored');

console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
