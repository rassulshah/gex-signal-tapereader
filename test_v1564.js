// ============================================================================================
// test_v1564.js — (v15.64) THE SECOND DASHBOARD CONVERSATION (2026-09-04). His eleven corrections, executed not grepped:
//   NEW re-defined (a below-observation, a crossing within 20 bars, and GROWTH), the stacks re-defined (members ≥ 30%
//   of the King, named ONCE, bracketed), the rugs take price's side, the QQQ King draws LIVE (the Trinity headers), the
//   ROLLED badge above ABOVE/BELOW, the tally off the face, the SWEPT line names-only and grouped by the side price is
//   working, the READ line cut to its two facts, ⓪a at the top and the replay strip at the bottom, the NOW row and the
//   King rows lit, the Kings pulsing (motion-gated). Calibration: tools/study-gridtells.py (S6 for the stacks, NEW rule C).
// ============================================================================================
const fs=require('fs');
const SRC=process.env.GPTS_SRC||'./current/gex-signal-tapereader.user.js';
const src=fs.readFileSync(SRC,'utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,500):''));} };
function ex(n){ const m=new RegExp('function\\s+'+n+'\\s*\\(','g').exec(src); if(!m) throw new Error('not found: '+n);
  let i=src.indexOf('{',m.index),d=0; for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){ d--; if(!d) return src.slice(m.index,k+1); } } }
const exVar=(n)=>{ const i=src.indexOf('var '+n+'='); if(i<0) throw new Error('no var '+n); const j=src.indexOf(';\n', i); return src.slice(i, j+1); };
const decomment=s=>s.replace(/\/\*[\s\S]*?\*\//g,'').replace(/(^|[^:\\'"])\/\/[^\n]*/g,'$1');
const build=(g,fns,tail)=>new Function('__g', Object.keys(g).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+exVar('GRID_NEW_BARS')+exVar('GRID_RUG_FLOOR_STEPS')+exVar('GRID_LVL_SNAP_PTS')+exVar('GROWTH_WINDOWS')+exVar('NODEBORN_KEY')+exVar('NODE_BORN')+exVar('REPLAY_BORN')+'\n'+fns.map(ex).join('\n')+'\n'+ex('gridStep')+'\n'+ex('gridReplay')+'\n'+ex('gridNow')+'\n'+ex('bornFromSnaps')+'\n'+ex('gridBook')+'\n'+(tail||''));
const esc=s=>String(s==null?'':s).replace(/"/g,'&quot;').replace(/</g,'&lt;');
const tip=t=>t?(' title="'+esc(t)+'"'):'';
const two=x=>{ x=''+x; return x.length<2?'0'+x:x; };
const fnum=x=>(typeof x==='number')?(Math.round(x*100)/100).toFixed(x%1?2:0).replace(/(\.\d*?)0+$/,'$1').replace(/\.$/,''):String(x);   // ⚠ strips zeros only AFTER a point — the older form turned 7690 into 769
const face=h=>String(h).replace(/title="[^"]*"/g,'');

// ---- 0 · the version and the constants -------------------------------------------------------------------------
ok(/@version\s+15\.64/.test(src) && /var GPTS_VERSION='15\.64';/.test(src),'0a v15.64 in both spots');
ok(/var GRID_NEW_BARS=20;/.test(src) && /var GRID_NEW_GROW_X=2;/.test(src) && /var GRID_NEW_GROW_PCT=20;/.test(src) && /var GRID_STACK_MIN_PCT=30;/.test(src) && /var GRID_STACK_MAX_PCT=40;/.test(src),'0b the calibrated constants: NEW within 20 bars, ×2 or +20%; a stack member ≥ 30%, the biggest ≥ 40%');
ok(/var NODEBORN_KEY='gpts_nodeborn_v2';/.test(src),'0c the birth store is v2 — his day’s first-sight "births" are discarded on install');
ok(/hand-set until[\s\S]{0,40}the ledger scores them/.test(src) && /7755 \(09-03\), 7705 \(08-28\),\s*\/\/ 7665 \(08-31\)/.test(src),'0d the constants say they are hand-set (⚖) and name the three taps the rule cannot catch — recorded, not hidden');

// ---- 1 · births: the opening book is not a birth, nor the first frame after a gap; absent = small ------------------
{
  const f=build({},[],'return { bornFromSnaps };')({});
  const T0=1788464400000;   // 13:00 CT
  const bar=(t,rows)=>({ bar:t, vend:{rows} });
  const sn=[ bar(T0,           [[7750,200e6,0,0,0,0],[7745,60e6,0,0,0,0],[7730,20e6,0,0,0,0]]),            // the opening book: 7750 100% · 7745 30% · 7730 10%
             bar(T0+180000,    [[7750,210e6,0,0,0,0],[7745,62e6,0,0,0,0],[7730,45e6,0,0,0,0]]),            // 7730 crosses (21%) after being seen at 10%
             bar(T0+360000,    [[7750,220e6,0,0,0,0],[7745,70e6,0,0,0,0],[7755,50e6,0,0,0,0],[7730,30e6,0,0,0,0]]),   // 7755 appears at 22.7% — absent before = small
             bar(T0+360000+30*60000, [[7750,220e6,0,0,0,0],[7790,90e6,0,0,0,0]]),                          // 30 minutes later: a recording gap — 7790 is UNKNOWN, not born
             bar(T0+360000+33*60000, [[7750,220e6,0,0,0,0],[7790,95e6,0,0,0,0],[7795,60e6,0,0,0,0]]) ];    // …but 7795, absent in the post-gap bar, is born here
  const r=f.bornFromSnaps(sn, 0.20);
  ok(!r.m[7750] && !r.m[7745],'1a the opening book is never a birth (7750 · 7745 at 13:00)',Object.keys(r.m));
  ok(r.m[7730] && r.m[7730].t===T0+180000 && r.m[7730].mag===45e6 && r.m[7730].pct===21,'1b 7730, seen at 10% then 21%: born 13:03 with its size at the crossing ($45M, 21%)',r.m[7730]);
  ok(r.m[7755] && r.m[7755].t===T0+360000 && r.m[7755].pct===23,'1c 7755, absent from the top list then 22.7%: absent = small, so it is born 13:06',r.m[7755]);
  ok(!r.m[7790] && r.m[7795] && r.m[7795].t===T0+360000+33*60000,'1d the first frame after a 30-minute hole is unknown (7790 not born); the next frame’s newcomer 7795 is',[r.m[7790],r.m[7795]]);
  ok(r.below[7730]===1 && r.below[7745]===1 && r.below[7755]===1,'1e the below-set remembers every strike seen under the cut or dropped from the list (7745 · 7755 fell out at the gap bar)',r.below);
  const one=f.bornFromSnaps([sn[0]], 0.20);
  ok(!Object.keys(one.m).length && one.below[7730]===1,'1f one frame: no births at all, one below-observation');
}

// ---- 2 · NEW = born within 20 bars AND grew: ×2 its size at the crossing, or +20% over the window ------------------
{
  const now=1788465600000;
  const store={ gpts_nodeborn_v2: JSON.stringify({ day:'2026-09-03', below:{7710:1},
    m:{ 7755:{t:now-9*180000, mag:20e6, pct:8},     // born 9 bars ago at $20M
        7740:{t:now-9*180000, mag:60e6, pct:25},    // born 9 bars ago at $60M, barely moved
        7720:{t:now-25*180000, mag:20e6, pct:8},    // born 25 bars ago — outside the window
        7700:{t:now-4*180000, mag:null, pct:null},  // a v1-shaped birth (no size recorded)
        7690:now-4*180000 } }) };                    // a bare v1 number
  const VEL={ 7755:{cur:46e6,d15:6e6}, 7740:{cur:63e6,d15:2e6}, 7720:{cur:80e6,d15:30e6}, 7700:{cur:50e6,d15:20e6}, 7690:{cur:50e6,d15:1e6} };
  const g={ VEL, CFG:{nodeThresh:20, growthWin:15}, ctTodayStr:()=>'2026-09-03', recorderLoad:()=>({days:{}}), localStorage:{getItem:k=>store[k]||null,setItem:(k,v)=>{store[k]=v;}} };
  const f=build(g,['growthWin','nodeGrowth','nodeBornLoad','nodeBornSave','nodeBelowTouch','nodeBornTouch','nodeBornOf','nodeAgeBars','nodeIsNew'],'return { nodeIsNew, nodeBornTouch, nodeBelowTouch, nodeAgeBars, nodeBornLoad };')(g);
  const a=f.nodeIsNew(7755, now);
  ok(a && a.age===9 && a.x===2.3 && a.g===15,'2a $20M at the crossing, $46M now = ×2.3 → NEW 9b ×2.3 (growth over the window only +15%, the multiple carries it)',a);
  ok(f.nodeIsNew(7740, now)===null,'2b born 9 bars ago but $60M → $63M (×1.05, +3%): a node that appeared and did NOT grow is not NEW — "it grew rapidly for this purpose"');
  ok(f.nodeIsNew(7720, now)===null,'2c ×4 but born 25 bars ago: outside the 20-bar window');
  const d=f.nodeIsNew(7700, now);
  ok(d && d.age===4 && d.x===null && d.g===67,'2d a birth without a recorded size falls back to the window growth alone (+67%)',d);
  ok(f.nodeIsNew(7690, now)===null,'2e …and a bare v1 number with +2% growth is not NEW');
  // the live latch: below first, then the crossing
  f.nodeBornTouch(7725, now, 30e6, 22); ok(f.nodeAgeBars(7725, now)===null,'2f a strike the panel has never seen below the threshold is not born at first sight');
  f.nodeBornTouch(7710, now-180000, 30e6, 22); const b=f.nodeBornLoad().m[7710];
  ok(b && b.t===now-180000 && b.mag===30e6 && b.pct===22 && !f.nodeBornLoad().below[7710],'2g a strike seen below (the stored below-set) and now at the threshold is born, with its size, and leaves the below-set',b);
  f.nodeBelowTouch(7725); f.nodeBornTouch(7725, now, 30e6, 22);
  ok(f.nodeAgeBars(7725, now)===0 && JSON.parse(store.gpts_nodeborn_v2).m[7725].mag===30e6,'2h nodeBelowTouch then nodeBornTouch: born now, persisted');
  f.nodeBornTouch(7710, now, 999, 99); ok(f.nodeBornLoad().m[7710].mag===30e6,'2i a second crossing never moves a birth');
}

// ---- 3 · the stacks: members ≥ 30%, named once on the biggest, bracketed; the rugs take price's side ----------------
{
  const g={ g3tip:tip, g3esc:esc };
  const f=build(g,['gridSetups','setupHtml'],'return { gridSetups, setupHtml };')(g);
  const N=(book,list)=>list.map(x=>({book, k:x[0], pct:x[1], disp:x[2], pos:x[1]>0})).sort((a,b)=>b.disp-a.disp);
  // the 2026-09-03 12:48 book — the v3b mockup's bar: 7755 20% · 7750 100% · 7745 45% · 7740 35% · 7735 26%
  const spx=f.gridSetups(N('SPX',[[7755,20,7763.4],[7750,100,7758.4],[7745,45,7753.4],[7740,35,7748.4],[7735,26,7743.4]]), {px:7756});
  ok(spx[7750] && spx[7750].length===1 && spx[7750][0].kind==='pika' && spx[7750][0].txt==='SPX PIKA STACK · 7740–7750' && spx[7750][0].span.n===3,'3a 12:48: ONE pika stack 7740–7750 (three members ≥ 30%), named on the King',spx[7750]);
  ok(spx[7745] && spx[7745][0].kind==='stk pika' && spx[7745][0].txt==='' && /member/.test(spx[7745][0].tip) && spx[7740] && spx[7740][0].kind==='stk pika','3b 7745 · 7740 are MEMBERS — a marker with the hover, no words');
  ok(!spx[7755] && !spx[7735],'3c 7755 (20%) and 7735 (26%) are not members: under the 30% cut, they break the run rather than join it');
  const thin=f.gridSetups(N('SPX',[[7750,100,7758.4],[7745,26,7753.4],[7740,45,7748.4]]), {px:7756});
  ok(!thin[7750] && !thin[7740],'3d a 26% node BETWEEN two members splits them: 7750 alone and 7740 alone are not stacks');
  const two=f.gridSetups(N('SPX',[[7750,38,7758.4],[7745,35,7753.4]]), {px:7756});
  ok(!two[7750],'3e two members but the biggest under 40%: no stack — the cloud needs mass');
  const b=f.gridSetups(N('SPY',[[773,100,7755.7],[772,-35,7745.7],[771,-74,7735.7],[770,-31,7725.7]]), {px:7760});
  ok(b[771] && b[771].some(s=>s.kind==='barney' && s.txt==='SPY BARNEY STACK · 770–772' && s.span.n===3) && b[772].some(s=>s.kind==='stk barney') && b[770].some(s=>s.kind==='stk barney') && !b[772].some(s=>s.kind==='barney'),'3f three −γ members: SPY BARNEY STACK 770–772 named once on 771 (74%), 772 and 770 bracketed',b);
  // the rugs: RUG = yellow ceiling directly over purple, price UNDER the yellow, no floor in sight
  const under=f.gridSetups(N('SPY',[[773,100,7755.7],[772,-35,7745.7]]), {px:7750});
  ok(under[773] && under[773].some(s=>s.kind==='rug' && /SPY RUG · 773 over 772/.test(s.txt)),'3g yellow 773 over purple 772 with price at 7750, UNDER the yellow: SPY RUG');
  const over=f.gridSetups(N('SPY',[[773,100,7755.7],[772,-35,7745.7]]), {px:7760});
  ok(!(over[773]||[]).some(s=>s.kind==='rug'),'3h …the same book with price at 7760, ABOVE the yellow: NOT a rug — the yellow is a floor under price (the docs: "spot positioned below the positive node")');
  const nopx=f.gridSetups(N('SPY',[[773,100,7755.7],[772,-35,7745.7]]));
  ok((nopx[773]||[]).some(s=>s.kind==='rug'),'3i with no price given (a fixture) the side rule is off');
  // the reverse rug: purple directly over yellow, price ABOVE the yellow, no ceiling in sight
  const rr=f.gridSetups(N('QQQ',[[719,-30,7770.5],[718,100,7759.7]]), {px:7765});
  ok(rr[718] && rr[718].some(s=>s.kind==='rrug'),'3j purple 719 over yellow 718 with price at 7765, above the yellow: QQQ REVERSE RUG');
  const rr2=f.gridSetups(N('QQQ',[[719,-30,7770.5],[718,100,7759.7]]), {px:7750});
  ok(!(rr2[718]||[]).some(s=>s.kind==='rrug'),'3k …price at 7750, under the yellow: not a reverse rug');
  const html=f.setupHtml(spx[7745]);
  ok(/^<span class="g3stk pika" title="[^"]*member[^"]*">┃<\/span>$/.test(html),'3l a member renders as the bracket glyph with its hover',html);
  ok(/g3chip pika/.test(f.setupHtml(spx[7750])) && /SPX PIKA STACK · 7740–7750/.test(f.setupHtml(spx[7750])),'3m the biggest member renders the named chip');
}

// ---- 4 · the QQQ King draws LIVE: the Trinity headers feed gridDisp when there is no replay frame -------------------
{
  const g={ ifLadder:()=>({ px:7710.26, dispScale:1.00108, undPx:769.43 }), STATE:{ SPY:{price:769.43} }, readTrinityHeaders:()=>({ SPY:{px:769.43}, QQQ:{px:717.82}, SPXW:{px:7710.26} }) };
  const f=build(g,['gridDisp'],'return { gridDisp };')(g);
  const D=f.gridDisp('SPY');
  ok(D && D.qqqPx===717.82 && D.spyPx===769.43,'4a live under the SPX pin (STATE.QQQ never set): QQQ 717.82 comes from the Trinity headers — the missing third layer',D);
  const g2=Object.assign({}, g, { readTrinityHeaders:()=>null });
  ok(build(g2,['gridDisp'],'return gridDisp("SPY");')(g2).qqqPx===null,'4b no headers on the page and no STATE: null, never a guess');
  const g3=Object.assign({}, g, { readTrinityHeaders:()=>{ throw new Error('no document'); } });
  ok(build(g3,['gridDisp'],'return gridDisp("SPY");')(g3).spyPx===769.43,'4c a throwing reader is swallowed; SPY still comes from STATE');
  const gs=decomment(ex('gridDisp'));
  ok(/if\(!xm\)\{ try\{ xm=readTrinityHeaders\(\); \}/.test(gs) && gs.indexOf('gridReplay()')<gs.indexOf('readTrinityHeaders()'),'4d …and a replayed frame’s xm still wins over the live page');
}

// ---- 5 · the King cells: ROLLED above ABOVE/BELOW; the tally in the hover, off the face ------------------------------
{
  const g={ g3tip:tip, g3esc:esc, frameNum:fnum, two, growthWin:()=>15, growthHtml:x=>'<span class="g3gg">'+x+'</span>', rollTagFor:(k,rolls)=>(rolls||[]).filter(r=>r.to===k).map(r=>({end:'from',other:r.from,dir:r.dir,t:r.lastT}))[0]||null, RATE_MIN_N:15 };
  const f=build(g,['kingCellHtml'],'return { kingCellHtml };')(g);
  const K={ k:7750, disp:7758.4, above:true, dist:2.4, g:3, moved:null };
  const rolls=[{from:7745,to:7750,dir:'up',lastT:1788464400000}];
  const h=f.kingCellHtml(K,'SPX KING · flow','spx','SPY',rolls,{ SPX:{held:2,broke:1,pending:0} });
  const kb=(h.match(/<span class="g3kbadges">([\s\S]*?)<\/span><\/div>/)||[])[1]||'';
  ok(/^<span class="g3rolled up">▲ ROLLED UP<\/span><span class="g3pos ab">ABOVE · 2\.4<\/span>$/.test(kb),'5a the badge column: ROLLED UP stacked ABOVE the ABOVE/BELOW badge',kb);
  ok(/title="[^"]*TAPS TODAY \(the deflection ledger\): 2 held · 1 broke — a rate prints at n ≥ 15/.test(h),'5b the tally counts live in the cell’s hover, with the n rule',h.match(/TAPS[^"]*/)[0]);
  ok(!/2 held/.test(face(h)) && !/SPX King/.test(face(h)),'5c …and nowhere on the face');
  const h2=f.kingCellHtml(Object.assign({},K,{moved:{from:7745,to:7750,dir:'dn',t:1788464400000}}),'SPY KING','spy','SPY',[],null);
  ok(/g3rolled dn">▼ ROLLED DOWN/.test(h2),'5d a non-SPX book rolls by the King path: ▼ ROLLED DOWN');
  const h3=f.kingCellHtml(K,'QQQ KING ≈','qqq','SPY',[],{ QQQ:{held:16,broke:1,pending:2} });
  ok(!/g3rolled/.test(h3) && /<span class="g3kbadges"><span class="g3pos ab">/.test(h3),'5e no roll today: no ROLLED badge, the position badge alone');
  ok(/16 held · 1 broke · 2 pending · 94% held \(n=17\)/.test(h3),'5f …a rate with its n once n ≥ 15',h3.match(/TAPS[^"]*/)[0]);
  ok(/kingCellHtml\(K\.SPY,'SPY KING','spy',sym,rolls,T\)/.test(ex('kingStripHtml')) && /T=kingTapsToday\(\)/.test(ex('kingStripHtml')),'5g the strip hands every cell the tally');
  const sf=decomment(ex('secFrame'));
  ok(/kingStripHtml\(sym, RAILROLLS\)\+ladderGridHtml\(/.test(sf) && !/tallyLineHtml\(sym\)/.test(sf),'5h secFrame no longer mounts tallyLineHtml — "spx king --, spy king -- and qqq king -- … taking up too much space"');
  ok(/^function tallyLineHtml\(/m.test(src),'5i …the function survives (the Testing tab and the record still read the counts)');
}

// ---- 6 · the SWEPT line: names only, in the context of the side price is working, details in the hover ---------------
{
  const mk=(level,side,px,at,atBar,status,speed)=>({level,side,px,at,atBar,status,speed,depth:1});
  const ev=[ mk('POC-','LOD',7690,'09:12',14,'accepted'), mk('VAL-','LOD',7684,'09:21',17,'pending'), mk('IBL-','LOD',7688,'09:30',20,'reclaimed',4), mk('PDH+','HOD',7702,'08:30',0,'accepted'), mk('VW1L','LOD',7686,'09:22',18,'reclaimed',2) ];
  const g={ sweepEventsToday:()=>ev, statsRead:()=>({lines:[{txt:'POC swept 09:12 — printed the extreme 40% (n=86)'}]}), levelTier:n=>({POC:1,VAL:2,IBL:2,PDH:1,ONL:1,VW1L:3}[String(n).replace(/[-+]$/,'')]||4), g3tip:tip, g3esc:esc, frameNum:fnum, SWEEP_RECLAIM_MAX:30 };
  const h=build(g,['sweptLineHtml'],'return sweptLineHtml("SPY");')(g);
  const fc=face(h);
  ok(/<em>SWEPT<\/em><span><span class="g3swg"><em>making LOD<\/em> <b class="brk" >POC<\/b> <i>·<\/i> <b class="tst" >VAL<\/b> <i>·<\/i> <b class="rec" >IBL<\/b><\/span> <i>·<\/i> <span class="g3swg"><em>making HOD<\/em> <b class="open" >PDH<\/b><\/span><\/span>/.test(fc),'6a SWEPT · making LOD: POC · VAL · IBL · making HOD: PDH — the side of the latest sweep (IBL, bar 20) first, tier order inside',fc);
  ok(!/7690|09:12|reclaimed|broke|VW1L|40%/.test(fc),'6b no prices, no times, no status words, no tier-3 levels, no rates on the face');
  ok(/<b class="brk" title="POC 7690 — swept 09:12 · broke\."/.test(h) && /<b class="rec" title="IBL 7688 — swept 09:30 · reclaimed in 4 bars\."/.test(h) && /<b class="tst" title="VAL 7684 — swept 09:21 · being tested\."/.test(h) && /<b class="open" title="PDH 7702 — swept 08:30 · opened beyond\."/.test(h),'6c each name’s hover: price · time · status');
  ok(/title="THE SWEPT LINE[^"]*Green = reclaimed[^"]*red = broke[^"]*amber = still being tested[^"]*40% \(n=86\)/.test(h),'6d the line’s own hover explains the colours and carries the rates');
  const g2=Object.assign({},g,{ sweepEventsToday:()=>[mk('ONL-','LOD',7743,'08:41',11,'reclaimed',11), mk('PDH+','HOD',7760,'10:02',31,'pending')] });
  const fc2=face(build(g2,['sweptLineHtml'],'return sweptLineHtml("SPY");')(g2));
  ok(fc2.indexOf('making HOD')<fc2.indexOf('making LOD') && /making HOD<\/em> <b class="tst" >PDH/.test(fc2),'6e the latest sweep is on the high side: making HOD leads — what price is attempting NOW reads first',fc2);
  const g3=Object.assign({},g,{ sweepEventsToday:()=>[] });
  ok(/no key level swept yet today/.test(build(g3,['sweptLineHtml'],'return sweptLineHtml("SPY");')(g3)),'6f nothing swept → says so');
  ok(/#gpts-body \.g3swept b\.rec\{color:#2ec27e\}#gpts-body \.g3swept b\.brk\{color:#f0616d\}#gpts-body \.g3swept b\.tst\{color:#f2b45a\}#gpts-body \.g3swept b\.open\{color:#6c7889/.test(src),'6g the CSS: green reclaimed · red broke · amber testing · dim opened-beyond');
}

// ---- 7 · the READ line: two facts on the face, the rest in its hover -----------------------------------------------
{
  const sd=decomment(ex('secDay'));
  ok(/\(\(CFG\.dayRead!==false\) \? '' : hlFarClause\(D, CALL\)\)/.test(sd),'7a the "% of the range" clause leaves the face when the read is on (it stays with the table)');
  ok(/'<div class="g3daysub"'\+\(\(CFG\.dayRead!==false && !NOREAD\)\?' style="display:none"':''\)/.test(sd),'7b the timing prose is hidden with the read on (shown with the table, and when there is no reading)');
  ok(/THE RANGE: '\+Math\.round\(100\*CALL\.far\)\+'% of the range to today(\\u2019|’)s '\+D\.second/.test(sd) && /THE TIMING: when an extreme of this age held, the other side printed later (\\u2014|—) median gap '\+hlDur\(base\.gapMin\)/.test(sd),'7c …both live in the READ line’s hover instead');
  ok(/#gpts-body \.g3dayhd\{display:none\}/.test(src),'7d the ⓪a header line stays off the face');
}

// ---- 8 · the order of the face: ⓪a at the top, the replay strip at the bottom, the warning still at the top ---------------
{
  const pv=decomment(ex('panelV3'));
  ok(/var _dayTop=\(CFG\.dayRead!==false\);\s*if\(_dayTop && _dayHtml\) h\+=_dayHtml;\s*for\(var j=0/.test(pv),'8a panelV3 mounts ⓪a BEFORE the ladder when it is the read');
  ok(/if\(j===secs\.length-1 && _dayHtml && !_dayTop\) h\+=_dayHtml;/.test(pv),'8b …and after it, as before, when the statistics table is on');
  const rnd=decomment(ex('render'));
  const iWarn=rnd.indexOf("replayBarHtml('warn')"), iSync=rnd.indexOf('tapeSync('), iV3=rnd.indexOf('panelV3(__asym)'), iClose=rnd.indexOf("html+='</div>';               // close single column")>=0?rnd.indexOf("// close single column"):rnd.lastIndexOf("html+='</div>';"), iStrip=rnd.lastIndexOf("replayBarHtml('strip')"), iFeed=rnd.lastIndexOf('feedStatusHtml()');
  ok(iWarn>0 && iWarn<iSync && iSync<iV3,'8c render(): the NOT RECORDING warning is emitted at the top, before the sync banner and the dashboard');
  ok(iStrip>iV3 && iStrip<iFeed,'8d …the strip itself after the dashboard and before the footer');
  ok(!/html\+=replayBarHtml\(\)/.test(rnd),'8e no bare replayBarHtml() call remains in render');
  ok((rnd.match(/replayBarHtml\('strip'\)/g)||[]).length===2,'8f the unmapped-instrument branch gets the strip at its bottom too');
  // replayBarHtml(part), executed
  const REPLAY={ frames:[{t:1}], days:['2026-09-03'], day:'2026-09-03', on:true, idx:0, loading:false, err:null };
  const g={ CFG:{}, REPLAY, PAL:{amber:'#f2b45a',ink:'#fff',sub:'#888',card:'#111',line:'#222',time:'#ccc',longAccent:'#2ec27e'}, replayEnsure(){}, replayOn:()=>true, replayFrame:()=>REPLAY.frames[0], replayDayLabel:d=>d, replaySec:()=>9*3600, replaySecOf:()=>9*3600, hlClock:s=>String(s), RP_OPEN_SEC:8.5*3600, RP_CLOSE_SEC:15*3600, g3tip:tip, g3esc:esc, liveSessionPhase:()=>({rth:true,label:'RTH',leftMin:120}), RP_STALEMSG:null, swallow(){}, activeSym:()=>'SPY', skPiles:()=>null, emBand:()=>null };
  const f=build(g,['replayBarHtml'],'return { replayBarHtml };')(g);
  const w=f.replayBarHtml('warn'), s=f.replayBarHtml('strip'), both=f.replayBarHtml();
  ok(/NOT RECORDING/.test(w) && /data-grp="exit"/.test(w) && !/g3rptrack/.test(w),'8g replayBarHtml("warn") is the banner alone — no track');
  ok(/g3rptrack/.test(s) && !/NOT RECORDING/.test(s) && /↺ REPLAY/.test(s),'8h replayBarHtml("strip") is the strip without the banner');
  ok(/NOT RECORDING/.test(both) && /g3rptrack/.test(both) && both.indexOf('g3rptrack')<both.indexOf('NOT RECORDING'),'8i no argument: both, the banner under the strip as before (the pop-out and the older tests)');
  const g2=Object.assign({}, g, { liveSessionPhase:()=>({rth:false,label:'CLOSED',leftMin:null}) });
  const f2=build(g2,['replayBarHtml'],'return { replayBarHtml };')(g2);
  ok(f2.replayBarHtml('warn')==='' && /g3rptrack/.test(f2.replayBarHtml('strip')) && !/NOT RECORDING/.test(f2.replayBarHtml()),'8j outside RTH the warning is empty and the strip is whole');
}

// ---- 9 · the CSS: the NOW row and the King rows lit, the Kings pulsing under two gates, the bracket, the badge ---------
{
  ok(/#gpts-body \.g3gr\.now,#gpts-body \.g3gr\.now\.zone\{background:rgba\(255,255,255,\.11\);box-shadow:inset 3px 0 0 #fff/.test(src),'9a the NOW row: a brighter ground and a white left bar — "the current price row is not obvious enough" — and it wins inside the King zone (the zone tint used to override it: Chromium, 2026-09-04)');
  ok(/#gpts-body \.g3gr\.king\{background:rgba\(227,195,65,\.10\);box-shadow:inset 3px 0 0 #e3c341\}/.test(src),'9b the King rows: a gold ground and a gold left bar');
  ok(/@keyframes g3kingpulse\{/.test(src) && /#gpts-body \.g3gr\.king\{animation:g3kingpulse 2\.4s ease-in-out infinite\}/.test(src) && /#gpts-body \.g3gr\.king \.g3kchip\{animation:g3kchippulse/.test(src),'9c the King rows and their chips pulse');
  ok(/@media \(prefers-reduced-motion: reduce\)\{#gpts-body \.g3gr\.king,#gpts-body \.g3gr\.king \.g3kchip\{animation:none !important\}\}/.test(src) && /#gpts-body\.g3nomo \.g3gr\.king,#gpts-body\.g3nomo \.g3gr\.king \.g3kchip\{animation:none !important\}/.test(src),'9d …gated by the panel’s motion setting (g3nomo) and the OS reduced-motion preference');
  ok(/#gpts-body \.g3gr\.stk\{box-shadow:inset 3px 0 0 rgba\(227,195,65,\.7\)\}#gpts-body \.g3gr\.stk\.barney\{box-shadow:inset 3px 0 0 rgba\(163,113,247,\.7\)\}/.test(src) && /#gpts-body \.g3stk\{/.test(src),'9e the stack bracket: a gold or purple left bar down the member rows, the glyph in the SETUP cell');
  ok(/#gpts-body \.g3kbadges\{margin-left:auto;display:flex;flex-direction:column/.test(src) && /#gpts-body \.g3rolled\.up\{/.test(src) && /#gpts-body \.g3rolled\.dn\{/.test(src),'9f the badge column stacks vertically; ROLLED UP green, ROLLED DOWN red');
  ok(/#gpts-body \.g3kz \.g3kx\{animation/.test(src)===false,'9g the King strip’s price does not pulse — the motion is on the ladder, where he asked for it');
}

// ---- 10 · the ladder as a whole: one NEW chip, one stack name, the members bracketed, the King rows classed ------------
{
  const now=1788465600000;
  const VEL={ 7755:{cur:55.9e6,d15:11.9e6}, 7750:{cur:273.2e6,d15:7e6}, 7745:{cur:123.6e6,d15:12.3e6}, 7740:{cur:94.4e6,d15:19.5e6}, 7735:{cur:69.9e6,d15:17.2e6}, 7730:{cur:29.4e6,d15:-1.8e6} };
  const store={ gpts_nodeborn_v2: JSON.stringify({day:'2026-09-03', m:{ 7755:{t:now-9*180000,mag:22e6,pct:9}, 7745:{t:now-6*180000,mag:120e6,pct:44}, 7740:{t:now-4e6,mag:90e6,pct:34} }, below:{}}) };
  const tape={ pct:{'7755':20,'7750':100,'7745':45,'7740':35,'7735':26,'7730':11}, king:7750, count:100 };
  const ladders={ SPY:{ pct:{'774':33,'773':100,'772':-21,'771':-74}, king:773, kingKd:390226 }, QQQ:{ pct:{'720':17,'719':47,'718':100,'715':-27}, king:718, kingKd:257257 } };
  const g={ VEL, CFG:{nodeThresh:20, growthWin:15}, STATE:{ SPY:{price:773.03} }, TODAY:'2026-09-03', ctTodayStr:()=>'2026-09-03',
    recorderLoad:()=>({days:{'2026-09-03':{snaps:{SPY:[]},defl:{}}}}), localStorage:{getItem:k=>store[k]||null,setItem:(k,v)=>{store[k]=v;}},
    ifLadder:()=>({ px:7756, dispScale:1.00108, undPx:773.03, rows:[] }), readTrinityHeaders:()=>({ SPY:{px:773.03}, QQQ:{px:717.66} }),
    tapeMap:()=>tape, laddersByDollar:()=>ladders, g3tip:tip, g3esc:esc, frameNum:fnum, two, LAD_KING_TEST_PTS:2, RATE_MIN_N:15, Date:class extends Date{ static now(){ return now; } } };
  const fns=['growthWin','nodeBornLoad','nodeBornSave','nodeBelowTouch','nodeBornTouch','nodeBornOf','nodeAgeBars','nodeIsNew','nodeGrowth','growthHtml','rollTagFor','rollHtml','gridDisp','gridBookNodes','gridSetups','setupHtml','kingPathSeed','kingPathTouch','kingTriTouch','kingGrowth','kingsNow','kingTapsToday','kingCellHtml','kingStripHtml','ladderGridHtml'];
  // 7725 is a context row the tape's top list does not carry — only the row path can observe it below
  const PS=[{k:7755,disp:7763.4,pct:20,brake:true},{k:7750,disp:7758.4,pct:100,brake:true,role:'KING'},{k:7745,disp:7753.4,pct:45,brake:true},{k:7740,disp:7748.4,pct:35,brake:true},{k:7735,disp:7743.4,pct:26,brake:true},{k:7730,disp:7738.3,pct:11,brake:true,sub:true},{k:7725,disp:7733.3,pct:9,brake:true,sub:true}];
  const R=build(g,fns,exVar('KING_PATH')+' return { grid:ladderGridHtml("SPY", __g.PS, [], {pdh:7708.25,pdl:7674.75,pdc:7697.75}, {ok:true,high:7743.4,low:7665.1}), strip:kingStripHtml("SPY", []) };')(Object.assign({PS},g));
  const grid=R.grid, gf=face(grid);
  ok(/♛ QQQ KING ≈/.test(grid),'10a the QQQ King row draws with STATE.QQQ unset — the Trinity headers carry it');
  ok((gf.match(/g3chip new/g)||[]).length===1 && /NEW 9b ×2\.5/.test(gf),'10b ONE NEW chip: 7755 ($22M at the crossing, $55.9M now = ×2.5); 7745 (×1.0, +11%) and 7740 (born 22 bars ago) are not NEW',gf.match(/NEW[^<]*/g));
  // the chip prints the figure that CARRIED it: a node under ×2 that qualified on window growth shows the growth, never "×1"
  const store2={ gpts_nodeborn_v2: JSON.stringify({day:'2026-09-03', m:{ 7740:{t:now-6*180000,mag:80e6,pct:30}, 7745:{t:now-6*180000,mag:110e6,pct:40} }, below:{}}) };
  const g2=Object.assign({}, g, { localStorage:{getItem:k=>store2[k]||null,setItem:(k,v)=>{store2[k]=v;}} });
  const grid2=face(build(g2,fns,exVar('KING_PATH')+' return ladderGridHtml("SPY", __g.PS, [], null, null);')(Object.assign({PS},g2)));
  ok(/NEW 6b \+26%/.test(grid2) && !/×1\b/.test(grid2) && (grid2.match(/g3chip new/g)||[]).length===1,'10b2 7740 born 6 bars ago at $80M ($94.4M now = ×1.2, but +26% over the window) is NEW on growth and prints the growth, never "×1"; 7745 (×1.1, +11%) clears neither gate',grid2.match(/NEW[^<]*/g)||'no chip');
  ok((gf.match(/SPX PIKA STACK/g)||[]).length===1 && /SPX PIKA STACK · 7740–7750/.test(gf) && (gf.match(/g3gr zone king stk pika|g3gr zone stk pika|g3gr stk pika/g)||[]).length===2,'10c ONE stack name on the King row; the two member rows carry the stk class (the bracket)',gf.match(/class="g3gr[^"]*"/g));
  ok(/class="g3gr now/.test(gf) && /class="g3gr zone king"/.test(gf),'10d the NOW row and the King rows carry the classes the CSS lights and pulses');
  ok(!/SPX King —|SPY King —|QQQ King —|held ·/.test(gf+face(R.strip)),'10e no tally text anywhere on the face');
  const bl=JSON.parse(store.gpts_nodeborn_v2).below;
  ok(bl[7730]===1 && bl[7725]===1 && bl[7735]===undefined,'10f the render recorded the below-observations: the context rows 7730 (11%, on the tape) and 7725 (9%, rows only) are in the below-set; 7735 (26%) is a node and is not',bl);
}

// ---- 11 · the study behind the numbers, and the records ------------------------------------------------------------
{
  const st=fs.existsSync('tools/study-gridtells.py')?fs.readFileSync('tools/study-gridtells.py','utf8'):'';
  ok(/S6_members>=30%\+max>=40%/.test(st) && /S6 IS WHAT v15\.64 SHIPS/.test(st),'11a tools/study-gridtells.py carries the shipped stack rule (S6) beside the four it beat');
  ok(/def births\(bars\)/.test(st) && /bar 0\) is never a birth/.test(st),'11b …and the birth rule the panel implements (bornFromSnaps mirrors it)');
  const cl=fs.existsSync('changelog/CHANGELOG.md')?fs.readFileSync('changelog/CHANGELOG.md','utf8'):'';
  ok(/## v15\.64/.test(cl) && cl.indexOf('## v15.64')<cl.indexOf('## v15.63'),'11c the CHANGELOG has the v15.64 entry on top');
  const ls=fs.existsSync('session-state/LESSONS.md')?fs.readFileSync('session-state/LESSONS.md','utf8'):'';
  const logAt=ls.indexOf('## 2 · THE LESSON LOG'); const firstEntry=(ls.slice(logAt).match(/### v[\d.]+/)||[])[0];
  ok(firstEntry==='### v15.64','11d the lesson log’s first entry is v15.64 (newest first)',firstEntry);
}

console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
