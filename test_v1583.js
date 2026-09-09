// test_v1583.js — (v15.83) THE SPY BOOK'S TOP FIVE IN THE IRT EXPORT (R-24) · THE SIGNED % ON EVERY NODE LINE · THE NODE
// LINES WEAR THEIR POLARITY. Operator, 2026-09-08: "i currently have top 5 nodes for spx. Im thinking of having the top 5
// for spy as well" → "ok lets go with this" · "also add the % to each of the node lines except the king" · "add a + or -
// too so i know the polarity" · "redo the color schemes for the nodes, using yellow and its derivatives for positive
// nodes and purple and its derivatives for negative gamma nodes". NODES = 5 on his Atlas draws the top five of EACH
// borrowed book; the file carried SPXW's five and only the SPY King.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
function v(n){ return src.match(new RegExp('var '+n+'=[\\s\\S]*?;\\n'))[0]; }

// ---------- 0. the version ----------
ok(/@version\s+15\.(8[3-9]|9\d)/.test(src) && /var GPTS_VERSION='15\.(8[3-9]|9\d)';/.test(src), '0a v15.83 or later in both spots');

// ---------- 1. the helpers ----------
{
  global.window={__gptsDebug:{}};
  eval(['irtColor'].map(ex).join('\n')); eval(v('IRT_COLORS'));
  eval(['irtNodeCol','irtPctTag','irtSpyTop'].map(ex).join('\n'));
  ok(IRT_COLORS.gpos===irtColor(245,215,110) && IRT_COLORS.gneg===irtColor(185,150,250) && IRT_COLORS.spos===irtColor(250,238,190) && IRT_COLORS.sneg===irtColor(225,208,252), '1a the four node shades: SPXW/QQQ yellow · purple, SPY the palest yellow · purple');
  ok(irtNodeCol('SPXW', 77)===IRT_COLORS.gpos && irtNodeCol('SPXW', -56)===IRT_COLORS.gneg && irtNodeCol('QQQ', 44)===IRT_COLORS.gpos && irtNodeCol('QQQ', -9)===IRT_COLORS.gneg, '1b irtNodeCol: the SPXW / QQQ node — yellow for +, purple for −');
  ok(irtNodeCol('SPY', 55)===IRT_COLORS.spos && irtNodeCol('SPY', -27)===IRT_COLORS.sneg, '1c …the SPY node the palest pair');
  ok(irtNodeCol('SPXW', 0)===IRT_COLORS.gpos && irtNodeCol('SPY', null)===IRT_COLORS.spos, '1d a zero or unknown polarity reads as + (the brake side), never a third colour');
  ok(irtPctTag(77)===' +77%' && irtPctTag(-56)===' -56%' && irtPctTag(55.4)===' +55%' && irtPctTag(0)===' 0%' && irtPctTag(null)==='' && irtPctTag(NaN)==='', '1e irtPctTag: " +77%" / " -56%", rounded, empty when unknown');
  // the SPY ranker on a book like today's (10:25 CT: 770 King, 767 98%, 766 80%, 765 67%, 769 40%, 768 18%)
  const J={ levels:[{ s:768, l:[{k:770,v:-127712000,d:-1},{k:767,v:125000000,d:1},{k:766,v:102000000,d:1},{k:765,v:85600000,d:1},{k:769,v:51000000,d:1},{k:768,v:23000000,d:-1},{k:760,v:9000000,d:1}] }] };
  const T=irtSpyTop(J, 770);
  ok(T.rows.length===4 && T.rows.map(r=>r.lbl+' '+r.k+' '+r.pct).join('|')==='SG2 767 98|SG3 766 80|SG4 765 67|SG5 769 40', '1f irtSpyTop: the next four by |v| after the King, SG2..SG5 (v15.85), signed pct', T.rows);
  ok(T.kingMag===127712000, '1g …the King = the largest |v| = 100% (a NEGATIVE King counts by its size)');
  const T2=irtSpyTop(J, null);
  ok(T2.rows.length===4 && T2.rows[0].k===767, '1h no King named → the #1 slot is still never an S row');
  const T3=irtSpyTop({ levels:[{ l:[{k:770,v:-100,d:-1},{k:769,v:-40,d:-1},{k:768,v:40,d:1}] }] }, 770);
  ok(T3.rows.map(r=>r.k+':'+r.pct).join('|')==='768:40|769:-40', '1i ties break toward the lower strike; a negative node carries a negative pct');
  ok(irtSpyTop(null, 770).rows.length===0 && irtSpyTop({ levels:[] }, 770).rows.length===0 && irtSpyTop({ levels:[{ l:[{k:770,v:0,d:1}] }] }, 770).rows.length===0, '1j no book / no rows / no magnitude → no rows, never a throw');
  ok(/\/\^\(XG\|SG\|G\)\[2-5\]\$\|\^D-KING\$\|\^D\[2-5\]\$\/\.test/.test(ex('irtGHeld')), '1k the hold accepts SG2..SG5 (under its own key, GS) and XG2..XG5 (v15.85; v15.90: and D-KING / D2..D5)');
}

// ---------- 2. the export ----------
{
  eval(v('IRT_HEADER')); eval(v('IRT_LAST'));
  eval(v('IRT_RATIO_KEY')); eval(v('IRT_NQRATIO_KEY')); eval(v('KING_LATCH_KEY'));
  eval(src.match(/var KING_LATCH_MS=\d+/)[0]+';');
  eval(v('IRT_QQQK_KEY')); eval(v('IRT_KINGS_KEY'));
  eval(['irtRound','irtCsvRow','irtRatio','irtNqRatio','kingLatchTick','irtKingLatch','irtKingHeld','irtGLatch','irtGHeld','irtQqqKing','irtQqqTop','skylitFutPx','irtBuildCsv'].map(ex).join('\n'));
  global.SKY_FUT={ ES:'ES1', NQ:'NQ1' }; global.FUTDER_STALE_MS=300000;
  global.emBand=()=>({ ok:true, now:7700, nowLive:7700, scaleUsed:10.0268 }); global.ladderKings=()=>[];
  global.ES_RATIO=10.05; global.NQ_RATIO=41.36; global.FEED_STALE_MS=12000;
  var LS={}; global.localStorage={ getItem:k=>(k in LS?LS[k]:null), setItem:(k,val)=>{LS[k]=String(val);} };
  global.CFG={ nodeThresh:20, irt:{ on:true, secs:180, futSym:'EPU26', etfSym:'SPY', file:'FlexLevelsExport.csv', nqOn:true, nqSym:'ENQU26', nqRatio:41.9 } };
  global.FUTMODE={ fam:'ES', r:10.0268, live:true }; global.ctTodayStr=()=>'2026-09-08';
  const IFL_ROWS=[{id:'CR0', k:7750, disp:7755.04, und:773.2},{id:'PS0', k:7650, disp:7654.97, und:763.2}];
  global.ifLadder=(sym)=>({ dispScale:1.00065, undScale:0.0998, rows:IFL_ROWS, err:null, srcSym:(sym==='QQQ'?'QQQ':'SPX') });
  global.ifChain=()=>({ dte0:{ gf:{ flip:7695.3 } } });
  const SPXW_TAPE=()=>({ king:7700, pct:{ '7700.00':100, '7705.00':77, '7680.00':-56, '7695.00':31, '7675.00':-27 } });
  global.tapeMap=(s)=>(s==='QQQ'?({ king:722, count:20, fromFeed:false, pct:{ '722.00':100, '721.00':44, '720.00':-9 } }):SPXW_TAPE());
  global.ladderFor=()=>null;
  const SPYJ={ levels:[{ s:768, l:[{k:770,v:-127712000,d:-1},{k:767,v:125000000,d:1},{k:766,v:102000000,d:1},{k:765,v:85600000,d:1},{k:769,v:51000000,d:1},{k:768,v:-23000000,d:-1}] }] };
  global.LASTFEED={ SPY:{ ts:Date.now(), j:SPYJ }, QQQ:{ ts:Date.now(), j:{ levels:[{ s:722 }] } } };
  global.extractWalls=()=>({ king:770, walls:[{k:770, pct:100, pos:false}] });
  global.LASTFUTDER={ ES1:{ ts:Date.now(), j:{ levels:[], derived:[
    { source:'SPY',  ratio:10.026042, levels:[{ t:1, s:7699.95, l:[{k:7720.05,v:1},{k:7689.97,v:1},{k:7679.95,v:1},{k:7669.92,v:1},{k:7710.03,v:1}] }] },
    { source:'SPXW', ratio:1.000578,  levels:[{ t:1, s:7699.94, l:[{k:7704.45,v:1},{k:7709.46,v:1},{k:7684.44,v:1}] }] } ] } },
    NQ1:{ ts:Date.now(), j:{ levels:[], derived:[{ source:'QQQ', ratio:41.196, levels:[{ t:1, s:29600, l:[{k:29743.512,v:1}] }] }] } } };
  const row=(lines,sym,re)=>lines.find(x=>x.startsWith(sym+',') && re.test(x.split(',')[2]));
  const lbl=(l)=>l?l.split(',')[2]:null, px=(l)=>l?+l.split(',')[1]:null, col=(l)=>l?+l.split(',')[3]:null, wid=(l)=>l?+l.split(',')[4]:null;

  const b=irtBuildCsv(); const L=b.csv.trim().split('\r\n');
  const e=(re)=>row(L,'EPU26',re);
  // the S rows
  ok(lbl(e(/^SG2/))==='SG2 +98%' && px(e(/^SG2/))===7690.0 && col(e(/^SG2/))===IRT_COLORS.spos && wid(e(/^SG2/))===1, '2a S2 = SPY 767 (+98%) at Skylit\'s 7689.97 → 7690.00, the palest yellow, width 1', [lbl(e(/^SG2/)), px(e(/^SG2/))]);
  ok(lbl(e(/^SG3/))==='SG3 +80%' && px(e(/^SG3/))===7680.0 && lbl(e(/^SG4/))==='SG4 +67%' && px(e(/^SG4/))===7670.0 && lbl(e(/^SG5/))==='SG5 +40%' && px(e(/^SG5/))===7710.0, '2b S3 766 · S4 765 · S5 769 — the next four by size, at the derived rows\' prices', [lbl(e(/^SG3/)),lbl(e(/^SG4/)),lbl(e(/^SG5/))]);
  ok(!e(/^S6/) && !e(/^S1/) && lbl(e(/SPY KING/))==='SPY KING' && px(e(/SPY KING/))===7720.0, '2c four S rows, no S1 (the King\'s slot is the SPY KING line, its label bare)');
  // the G rows: the signed %, the polarity colours
  ok(lbl(e(/^XG2/))==='XG2 +77%' && col(e(/^XG2/))===IRT_COLORS.gpos && lbl(e(/^XG3/))==='XG3 -56%' && col(e(/^XG3/))===IRT_COLORS.gneg, '2d G2 +77% yellow · G3 -56% purple — the sign is the polarity, the colour follows it', [lbl(e(/^XG2/)), lbl(e(/^XG3/))]);
  ok(lbl(e(/^XG4/))==='XG4 +31%' && lbl(e(/^XG5/))==='XG5 -27%' && col(e(/^XG5/))===IRT_COLORS.gneg, '2e G4 +31% · G5 -27%');
  ok(lbl(e(/SPXW KING/))==='SPXW KING' && wid(e(/SPXW KING/))===3, '2f the SPXW King keeps its bare label and width 3');
  ok(L.filter(x=>x.startsWith('EPU26,')).length===13, '2g EPU26: 2 Kings + 4 G + 4 S + 3 IF = 13 rows', L.filter(x=>x.startsWith('EPU26,')).length);
  ok(L.filter(x=>x.startsWith('EPU26,') && /~/.test(x.split(',')[2])).length===0, '2h no ~ anywhere on EPU26 — every row on Skylit\'s prices');
  // the NQ G rows
  const q=(re)=>row(L,'ENQU26',re);
  ok(lbl(q(/^G2/))==='G2 +44%' && col(q(/^G2/))===IRT_COLORS.gpos && lbl(q(/^G3/))==='G3 -9%' && col(q(/^G3/))===IRT_COLORS.gneg, '2i the NQ G rows carry the signed % and the polarity colour too', [lbl(q(/^G2/)), lbl(q(/^G3/))]);
  ok(/^live \(SG2 767 98%, SG3 766 80%, SG4 765 67%, SG5 769 40%\)$/.test(IRT_LAST.sWhy||''), '2j IRT_LAST.sWhy names the four', IRT_LAST.sWhy);
  ok(/13 of 13 ES rows on Skylit’s ES1 prices/.test(IRT_LAST.skyWhy||''), '2k skyWhy counts 13 of 13', IRT_LAST.skyWhy);
  // the hold: the SPY feed goes stale → the S rows print from the latch, tagged held
  const held=JSON.parse(LS[IRT_KINGS_KEY]);
  ok(held.GS && held.GS.rows.length===4 && held.GS.rows[0].lbl==='SG2' && held.GS.rows[0].pct===98 && held.GS.rows[0].k===767, '2l the S rows are latched under GS with their strike and signed pct', held.GS&&held.GS.rows[0]);
  global.LASTFEED={ SPY:{ ts:Date.now()-FEED_STALE_MS*4, j:SPYJ }, QQQ:{ ts:Date.now(), j:{ levels:[{ s:722 }] } } };
  const b2=irtBuildCsv(); const L2=b2.csv.trim().split('\r\n'); const e2=(re)=>row(L2,'EPU26',re);
  ok(lbl(e2(/^SG2/))==='SG2 +98%' && px(e2(/^SG2/))===7690.0 && /^held \d+m/.test(IRT_LAST.sWhy) && /^held/.test(IRT_LAST.spyWhy), '2m the SPY feed stale → the S rows (and the King) print from the hold at Skylit\'s prices, sWhy says held', [lbl(e2(/^SG2/)), IRT_LAST.sWhy]);
  // no hold and no feed → no S rows, and the file says so
  LS={}; global.localStorage={ getItem:k=>(k in LS?LS[k]:null), setItem:(k,val)=>{LS[k]=String(val);} };
  const b3=irtBuildCsv(); const L3=b3.csv.trim().split('\r\n');
  ok(!row(L3,'EPU26',/^SG2/) && /no fresh SPY feed and nothing latched today/.test(IRT_LAST.sWhy), '2n nothing to hold and no feed → no S rows, no invention; sWhy says why', IRT_LAST.sWhy);
  global.LASTFEED={ SPY:{ ts:Date.now(), j:SPYJ }, QQQ:{ ts:Date.now(), j:{ levels:[{ s:722 }] } } };
  // the degrade path: the helpers absent → white and bare, never missing
  const B=ex('irtBuildCsv');
  ok(/var pctTag=\(typeof irtPctTag==='function'\)\?irtPctTag:function\(\)\{ return ''; \};/.test(B) && /var nodeCol=\(typeof irtNodeCol==='function'\)\?irtNodeCol:function\(\)\{ return IRT_COLORS\.gate; \};/.test(B) && /var spyTop=\(typeof irtSpyTop==='function'\)\?irtSpyTop:function\(\)\{ return \{ rows:\[\] \}; \};/.test(B), '2o the three helpers are typeof-guarded inside irtBuildCsv (degrade, do not depend)');
  ok(!/IRT_COLORS\.gate, w:1/.test(B), '2p no node row is white any more — every one goes through nodeCol');
  // the ETF symbol carries the S rows in SPY row-space
  ok(px(row(L,'SPY',/^SG2/))===767 && px(row(L,'SPY',/^SG5/))===769, '2q the ETF symbol carries S2..S5 at the SPY strikes');
  // the gear line names the S rows
  ok(/\+\(IRT_LAST\.sWhy\?\(' \\u00b7 S(G)? rows '\+g3esc\(IRT_LAST\.sWhy\)\):''\)/.test(src), '2r the gear\'s IRT line says how the S rows were resolved (SG rows from v15.85)');
}

// ---------- 3. the record ----------
{
  const R=JSON.parse(fs.readFileSync('learning/recommendations.json','utf8')); const r24=R.rows.find(r=>r.id==='R-24');
  ok(r24 && r24.status==='implemented' && r24.version==='15.83' && /SPY/.test(r24.text) && /polarity|yellow/i.test(r24.text+r24.changes), '3a R-24 on Rec, implemented in v15.83', r24&&[r24.status,r24.version]);
  const seedR=JSON.parse(/var REC_SEED=(\{.*?\});\n/.exec(src)[1]); ok(JSON.stringify(seedR)===JSON.stringify(R), '3b REC_SEED equals the file');
  const cl=fs.readFileSync('changelog/CHANGELOG.md','utf8'); ok(/## v15\.83/.test(cl) && cl.indexOf('## v15.83')<cl.indexOf('## v15.82'), '3c the CHANGELOG has the v15.83 entry on top');
  const P=JSON.parse(fs.readFileSync('learning/plan.json','utf8')); const r83=P.roadmap.find(r=>r.v==='15.83'); const seas=P.roadmap.find(r=>/SEASONALITY TRACKED/.test(r.title));
  ok(r83 && /SPY/.test(r83.title) && (r83.status==='next'||r83.status==='shipped') && seas && +seas.v>15.83 && P.roadmap.filter(r=>r.status==='next').length===1, '3d the plan: v15.83 is this build; the seasonality moved past it', r83&&[r83.status, seas&&seas.v]);
  const seedP=JSON.parse(/var PLAN_SEED=(\{.*?\});\n/.exec(src)[1]); ok(JSON.stringify(seedP)===JSON.stringify(P), '3e PLAN_SEED equals the file');
  const ls=fs.readFileSync('session-state/LESSONS.md','utf8'); const logAt=ls.indexOf('## 2 · THE LESSON LOG'); ok(/### v15\.83/.test(ls.slice(logAt>=0?logAt:0)), '3f the lesson log carries the v15.83 entry');
  const rn=fs.readFileSync('session-state/latest-resume-note.md','utf8'); ok(/v15\.(8[3-9]|9\d)/.test(rn.slice(0,600)) && /S(G)?2/.test(rn), '3g the resume note is at v15.83 or later and names the S rows');
  const dc=fs.readFileSync('session-state/DECISIONS.md','utf8').replace(/\s+/g,' '); ok(/yellow and its derivatives for positive nodes and purple and its derivatives for negative gamma nodes/.test(dc), '3h DECISIONS records his colour rule in his words');
}

console.log('test_v1583: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
