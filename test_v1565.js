// ============================================================================================
// test_v1565.js — (v15.65) THE PATTERN COLUMNS. Operator, 2026-09-04: "split the setup column into 3 columns so 1 is for
//   spx, spy and qqq … a purple block with 769-772 to keep it simple and abbreviated" · "rename setup to Pattern" · "the
//   purple and yellow should also indicate pika or barney and have the prices below the text" · "write barney and pika in
//   black and have the prices in yellow" · "the current price row to be highlighted with a white hue … the other kings can
//   have a separate hue … since qqq is cyan … the spy and spx king, use unique colors … different than the color of the
//   nodes". Executed, not grepped; the reference render is mockups/mockup-pattern-columns.png (the 2026-08-28 13:12 book).
// ============================================================================================
const fs=require('fs');
const SRC=process.env.GPTS_SRC||'./current/gex-signal-tapereader.user.js';
const src=fs.readFileSync(SRC,'utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,500):''));} };
function ex(n){ const m=new RegExp('function\\s+'+n+'\\s*\\(','g').exec(src); if(!m) throw new Error('not found: '+n);
  let i=src.indexOf('{',m.index),d=0; for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){ d--; if(!d) return src.slice(m.index,k+1); } } }
const exVar=(n)=>{ const i=src.indexOf('var '+n+'='); if(i<0) throw new Error('no var '+n); const j=src.indexOf(';\n', i); return src.slice(i, j+1); };
const build=(g,fns,tail)=>new Function('__g', Object.keys(g).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+exVar('GRID_NEW_BARS')+exVar('GRID_RUG_FLOOR_STEPS')+exVar('GRID_LVL_SNAP_PTS')+exVar('GROWTH_WINDOWS')+exVar('NODEBORN_KEY')+exVar('NODE_BORN')+exVar('REPLAY_BORN')+'\n'+fns.map(ex).join('\n')+'\n'+ex('gridStep')+'\n'+ex('gridReplay')+'\n'+ex('gridNow')+'\n'+ex('bornFromSnaps')+'\n'+ex('gridBook')+'\n'+ex('gridPatternHost')+'\n'+(tail||''));
const esc=s=>String(s==null?'':s).replace(/"/g,'&quot;').replace(/</g,'&lt;');
const tip=t=>t?(' title="'+esc(t)+'"'):'';
const two=x=>{ x=''+x; return x.length<2?'0'+x:x; };
const fnum=x=>(typeof x==='number')?(Math.round(x*100)/100).toFixed(x%1?2:0).replace(/(\.\d*?)0+$/,'$1').replace(/\.$/,''):String(x);
const face=h=>String(h).replace(/title="[^"]*"/g,'');
const cellsOf=(row)=>(row.match(/<span class="g3gc[^"]*">[\s\S]*?<\/span>(?=<span class="g3gc|<\/div>)/g)||[]);

ok(/@version\s+15\.(6[5-9]|[7-9]\d)/.test(src) && /var GPTS_VERSION='15\.(6[5-9]|[7-9]\d)';/.test(src),'0a v15.65+ in both spots');

// ---- 1 · the blocks --------------------------------------------------------------------------------------------
{
  const g={ g3tip:tip, g3esc:esc };
  const f=build(g,['gridSetups','setupHtml'],'return { gridSetups, setupHtml };')(g);
  const N=(book,list)=>list.map(x=>({book, k:x[0], pct:x[1], disp:x[2], pos:x[1]>0})).sort((a,b)=>b.disp-a.disp);
  const spy=f.gridSetups(N('SPY',[[773,100,7755.7],[772,-35,7745.7],[771,-74,7735.7],[770,-31,7725.7]]), {px:7750});
  const bl=f.setupHtml(spy[771]);
  ok(/^<span class="g3pat" title="SPY BARNEY STACK · 770–772 — 3 −γ \(purple\) nodes[^"]*"><span class="g3pb barney">BARNEY<\/span><i class="g3pr">770–772<\/i><\/span>$/.test(bl),'1a a barney stack is a purple BARNEY block with the book’s own strikes under it — the full name and the definition in the hover',bl);
  const rug=f.setupHtml(spy[773]);
  ok(/<span class="g3pb rug">RUG<\/span><i class="g3pr">773\/772<\/i>/.test(rug) && /title="SPY RUG · 773 over 772 — The rug setup/.test(rug),'1b a rug is a red RUG block with its two strikes',rug);
  const q=f.gridSetups(N('QQQ',[[719,-30,7770.5],[718,100,7759.7]]), {px:7765});
  ok(/<span class="g3pb rrug">RRUG<\/span><i class="g3pr">719\/718<\/i>/.test(f.setupHtml(q[718])),'1c the reverse rug is a green RRUG block');
  const spx=f.gridSetups(N('SPX',[[7750,100,7758.4],[7745,45,7753.4],[7740,35,7748.4]]), {px:7756});
  ok(/<span class="g3pb pika">PIKA<\/span><i class="g3pr">7740–7750<\/i>/.test(f.setupHtml(spx[7750])) && f.setupHtml(spx[7745])==='<span class="g3stk pika"'+tip(spx[7745][0].tip)+'>┃</span>','1d a pika stack: a yellow PIKA block on the biggest member; the members keep the bracket glyph');
  ok(!/SPX PIKA STACK/.test(face(f.setupHtml(spx[7750]))) && !/BARNEY STACK/.test(face(bl)),'1e the words STACK and the book are off the face — the block and the strikes only');
  ok(/#gpts-body \.g3pb\{display:block;[^}]*color:#0b0e14\}/.test(src) && /#gpts-body \.g3pb\.pika\{background:#e3c341\}#gpts-body \.g3pb\.barney\{background:#a371f7\}/.test(src) && /#gpts-body \.g3pr\{display:block;[^}]*color:#e3c341/.test(src),'1f CSS: the word in black on the coloured block, the strikes in yellow under it');
}

// ---- 2 · the grid: nine columns, a pattern in its book’s column, a SPY pattern off its King placed on the nearest row ----
{
  const now=1788465600000;
  const VEL={ 7755:{cur:55.9e6,d15:11.9e6}, 7750:{cur:273.2e6,d15:7e6}, 7745:{cur:123.6e6,d15:12.3e6}, 7740:{cur:94.4e6,d15:19.5e6}, 7735:{cur:69.9e6,d15:17.2e6}, 7730:{cur:29.4e6,d15:-1.8e6} };
  const store={ gpts_nodeborn_v2: JSON.stringify({day:'2026-09-03', m:{}, below:{}}) };
  const tape={ pct:{'7755':20,'7750':100,'7745':45,'7740':35,'7735':26,'7730':11}, king:7750, count:100 };
  // SPY: the King 773 (+γ, price above it: no rug); a −γ pair 772 · 771 (both ≥ 30%) — a barney stack whose biggest member
  // 772 (→ 7745.7 on the chart) is NOT the King and has no row of its own
  const ladders={ SPY:{ pct:{'774':22,'773':100,'772':-45,'771':-38}, king:773, kingKd:390226 }, QQQ:{ pct:{'720':17,'719':47,'718':100,'715':-27}, king:718, kingKd:257257 } };
  const g={ VEL, CFG:{nodeThresh:20, growthWin:15}, STATE:{ SPY:{price:773.03} }, TODAY:'2026-09-03', ctTodayStr:()=>'2026-09-03',
    recorderLoad:()=>({days:{'2026-09-03':{snaps:{SPY:[]},defl:{}}}}), localStorage:{getItem:k=>store[k]||null,setItem:(k,v)=>{store[k]=v;}},
    ifLadder:()=>({ px:7756, dispScale:1.00108, undPx:773.03, rows:[] }), readTrinityHeaders:()=>({ SPY:{px:773.03}, QQQ:{px:717.66} }),
    tapeMap:()=>tape, laddersByDollar:()=>ladders, g3tip:tip, g3esc:esc, frameNum:fnum, two, LAD_KING_TEST_PTS:2, RATE_MIN_N:15, Date:class extends Date{ static now(){ return now; } } };
  const fns=['growthWin','nodeBornLoad','nodeBornSave','nodeBelowTouch','nodeBornTouch','nodeBornOf','nodeAgeBars','nodeIsNew','nodeGrowth','growthHtml','rollTagFor','rollHtml','gridDisp','gridBookNodes','gridSetups','setupHtml','kingPathSeed','kingPathTouch','kingTriTouch','kingGrowth','kingsNow','kingTapsToday','kingCellHtml','kingStripHtml','ladderGridHtml'];
  const PS=[{k:7755,disp:7763.4,pct:20,brake:true},{k:7750,disp:7758.4,pct:100,brake:true,role:'KING'},{k:7745,disp:7753.4,pct:45,brake:true},{k:7740,disp:7748.4,pct:35,brake:true},{k:7735,disp:7743.4,pct:26,brake:true},{k:7730,disp:7738.3,pct:11,brake:true,sub:true}];
  const R=build(g,fns,exVar('KING_PATH')+' return { grid:ladderGridHtml("SPY", __g.PS, [], {pdh:7708.25,pdl:7674.75,pdc:7697.75}, null), strip:kingStripHtml("SPY", []) };')(Object.assign({PS},g));
  const grid=R.grid, gf=face(grid);
  const hd=(gf.match(/<div class="g3gr hd">[\s\S]*?<\/div>/)||[''])[0];
  ok(/pattern · SPX<\/span><span class="g3gc pat spy">SPY<\/span><span class="g3gc pat qqq">QQQ<\/span><\/div>$/.test(hd) && !/setup/.test(hd),'2a the header ends with PATTERN · SPX | SPY | QQQ — no SETUP');
  const rows=(gf.match(/<div class="g3gr[^"]*">[\s\S]*?<\/div>(?=<div class="g3gr|<\/div>$)/g)||[]);
  ok(rows.length>6 && rows.every(r=>cellsOf(r).length===9),'2b every row has nine cells',rows.map(r=>cellsOf(r).length));
  const spxKing=rows.find(r=>/♛ SPX KING/.test(r)), spyKing=rows.find(r=>/♛ SPY KING/.test(r)), qqqKing=rows.find(r=>/♛ QQQ KING/.test(r));
  ok(/class="g3gr zone king kspx"/.test(spxKing) && /class="g3gr zone king kspy"/.test(spyKing) && /class="g3gr zone king kqqq"/.test(qqqKing),'2c each King row carries its book’s class (kspx · kspy · kqqq) for its own hue');
  const c=cellsOf(spxKing);
  ok(/g3pb pika">PIKA<\/span><i class="g3pr">7740–7750/.test(c[6]) && !/g3pb/.test(c[7]) && !/g3pb/.test(c[8]),'2d the SPX stack sits in the SPX column of the SPX King row, and nowhere else',c.slice(6));
  const cq=cellsOf(qqqKing);
  ok(/g3pb pika">PIKA<\/span><i class="g3pr">718–719/.test(cq[8]) && !/g3pb/.test(cq[6]) && !/g3pb/.test(cq[7]),'2e the QQQ stack sits in the QQQ column of the QQQ King row',cq.slice(6));
  // the SPY barney pair 771–772 is not on the SPY King (773): its biggest member 772 → 7745.7 on the chart → the nearest ladder row is 7735 (7743.4)
  const host=rows.find(r=>/g3pb barney">BARNEY<\/span><i class="g3pr">771–772/.test(r));
  ok(!!host && !/♛ SPY KING/.test(host) && /7743\.4/.test(host) && /g3gc pat spy"><span class="g3pat"\s*><span class="g3pb barney">BARNEY<\/span><i class="g3pr">771–772/.test(host),'2f a SPY pattern that is NOT on its King lands in the SPY column of the ladder row nearest its converted price (772 → 7745.7 → the 7735 row at 7743.4)',host&&host.replace(/<[^>]+>/g,'|').slice(0,160));
  // …and one whose converted price is off the ladder is not drawn at all
  const ladders2=JSON.parse(JSON.stringify(ladders)); ladders2.SPY.pct={'774':22,'773':100,'769':-45,'768':-38};
  const g2=Object.assign({}, g, { laddersByDollar:()=>ladders2 });
  const gf2=face(build(g2,fns,exVar('KING_PATH')+' return ladderGridHtml("SPY", __g.PS, [], null, null);')(Object.assign({PS},g2)));
  ok(!/768–769/.test(gf2),'2f2 a SPY stack at 768–769 (→ 7716 on the chart, 22 points below the lowest ladder row) is off the ladder and not drawn — the host must be within one SPXW strike');
  const cs=cellsOf(spyKing);
  ok(!/g3pb/.test(cs[7]) && !/g3pb/.test(cs[6]),'2g …and the SPY King row (773 alone, no stack) carries no block');
  ok(!/SPX PIKA STACK|BARNEY STACK|SPY RUG/.test(gf),'2h no pattern name words on the face — blocks and strikes only');
  ok(/class="g3gr now/.test(gf) && (gf.match(/<span class="g3gc pat spx"><span class="mut">context row<\/span>/g)||[]).length===1,'2i the NOW row and the context row still render, the context word in the SPX pattern column');
  // gridPatternHost: within one SPXW strike, never a level row, else nothing
  const hostFn=build({}, [], 'return gridPatternHost;')({});
  const rws=[{kind:'node',disp:7758.4},{kind:'level',disp:7752},{kind:'node',disp:7748.4},{kind:'now',disp:7756}];
  ok(hostFn(rws,7752.5,5).disp===7756 && hostFn(rws,7766,5)===null && hostFn(rws,7749,5).disp===7748.4,'2j gridPatternHost: the nearest non-level row within one strike, else none',[hostFn(rws,7752.5,5),hostFn(rws,7766,5)]);
  // the King strip in the Kings' own colours
  ok(/#gpts-body \.g3kchip\.spx\{background:#ff9a3c/.test(src) && /#gpts-body \.g3kchip\.spy\{background:#5ea4ff/.test(src) && /#gpts-body \.g3kchip\.qqq\{background:transparent;color:#5fd3bc/.test(src),'2k the King chips: SPX orange · SPY blue · QQQ cyan');
  ok(/#gpts-body \.g3kc\.spx\{border-color:rgba\(255,154,60,\.6\)\}#gpts-body \.g3kc\.spx \.g3bk,#gpts-body \.g3kc\.spx \.g3kx\{color:#ff9a3c\}/.test(src) && /#gpts-body \.g3kc\.spy\{border-color:rgba\(94,164,255,\.6\)\}/.test(src),'2l …and the strip’s cells match');
  ok(!/#gpts-body \.g3kchip\.spx\{background:#e3c341/.test(src) && !/#gpts-body \.g3kc\.spy\{border-color:rgba\(205,180,250/.test(src),'2m the old yellow SPX chip and lavender SPY cell are gone — no King shares a node colour');
  ok(/#gpts-body \.g3gr\{display:grid;grid-template-columns:48px 58px 104px 58px 82px 60px 62px 62px 62px;/.test(src) && /Math\.round\(pctv\/100\*98\)/.test(src),'2n nine columns as measured in Chromium; the node bar scaled to the narrower NODE column');
}

// ---- 3 · the records ---------------------------------------------------------------------------------------------
{
  const cl=fs.existsSync('changelog/CHANGELOG.md')?fs.readFileSync('changelog/CHANGELOG.md','utf8'):'';
  ok(/## v15\.65/.test(cl) && cl.indexOf('## v15.65')<cl.indexOf('## v15.64'),'3a the CHANGELOG has the v15.65 entry on top');
  const ls=fs.existsSync('session-state/LESSONS.md')?fs.readFileSync('session-state/LESSONS.md','utf8'):'';
  const logAt=ls.indexOf('## 2 · THE LESSON LOG'); const firstEntry=(ls.slice(logAt).match(/### v[\d.]+/)||[])[0];
  ok(/### v15\.65/.test(ls.slice(logAt)) && /invisible whenever the King is not in it/.test(ls),'3b the lesson log carries the v15.65 entry (the off-King pattern that never drew)',firstEntry);
  ok(fs.existsSync('mockups/mockup-pattern-columns.png'),'3c the approved mockup is in mockups/ (the installer carries it)');
}

console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
