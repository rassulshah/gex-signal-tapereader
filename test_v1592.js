// test_v1592.js — (v15.92) KINGS + WALLS. Operator, 2026-09-09: "i realize you are tracking the kings, but there are way
// too many levels on my irt charts. i want to reduce it to kings along with the put and call wall only for both the ES
// and NQ. you can continue your study." → the exact list, agreed ("ok"): SPXW KING · SPY KING · CW0 · PW0 on the ES
// symbol; QQQ KING · NDX KING · CW0 · PW0 on the NQ symbol — eight lines. The node rows, FLIP0 and the D rows stay on the
// panel and in the record; the ⚙ IRT "Lines" selector brings them back by name ('all'). NDX KING is the NDX options
// book's King ("why not just have NDX King as the label … i dont see why any king needs to have 100%"); the NQ walls are
// InsiderFinance's QQQ 0DTE walls at Skylit's QQQ ratio; the shade says the book on both charts (index book full, width 3;
// ETF book lighter, width 2 — so QQQ KING takes the lighter pair, as SPY KING has on ES).
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);if(!m)throw new Error('no fn '+n);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
function v(n){ return src.match(new RegExp('var '+n+'=[\\s\\S]*?;\\n'))[0]; }

// ---------- 0. the version ----------
ok(/@version\s+15\.9[2-9]/.test(src) && /var GPTS_VERSION='15\.9[2-9]';/.test(src), '0a v15.92 or later in both spots');

// ---------- 1. futDerBookKing — one book's own King at Skylit's futures price ----------
{
  global.LASTFUTDER={}; global.LASTFUTDER_FRONT={}; global.FUTDER_STALE_MS=300000;
  eval(ex('futDerBookKing'));
  // his NQ1 payload, 2026-09-09: QQQ (41.11874) · NDXP + NDX (1.00087); the NDX book's King 29355.41 = strike 29330, a put node
  const NQJ=(t)=>({ levels:[], expirations:['2026-09-09'], derived:[
    { source:'QQQ',  ratio:41.11874, levels:[{ t:t, s:29520, l:[{k:29523.25,v:94200000,d:1,net:94200000},{k:29441.02,v:61000000,d:-1,net:-61000000},{k:29358.78,v:20000000,d:1,net:20000000}] }] },
    { source:'NDXP', ratio:1.00087,  levels:[{ t:t, s:29520, l:[{k:29445.49,v:290000000,d:1,net:290000000},{k:29355.41,v:320000000,d:-1,net:-320000000},{k:29495.53,v:200000000,d:-1,net:-200000000}] }] },
    { source:'NDX',  ratio:1.00087,  levels:[{ t:t, s:29520, l:[{k:29355.41,v:100000000,d:1,net:100000000}] }] } ] });
  LASTFUTDER_FRONT.NQ1={ j:NQJ(1788974000), ts:Date.now() };
  const K=futDerBookKing('NQ1', ['NDXP','NDX']);
  ok(K.k===29355.41 && K.pct===-100 && K.strike===29330 && K.src==='NDXP' && K.window==='front', '1a the NDXP book\'s largest |dollars| row: 29355.41 = strike 29330, a −γ King, from the FRONT payload', K);
  ok(/^live · NDXP 29330 at 29355\.41 \(front payload, \d+ s old\)$/.test(K.why), '1b …and says so', K.why);
  const Q=futDerBookKing('NQ1', ['QQQ']);
  ok(Q.k===29523.25 && Q.strike===718 && Q.pct===100 && Q.src==='QQQ', '1c the QQQ book by the same rule: 29523.25 / 41.11874 = 718 (the ETF grid is whole numbers)', Q);
  ok(futDerBookKing('NQ1', ['NDX']).src==='NDX' && futDerBookKing('NQ1', ['NDX']).pct===100, '1d asked for NDX alone it reads the NDX book (a +γ 100M row there)');
  ok(futDerBookKing('NQ1', ['NDXW','NDXP']).src==='NDXP', '1e a source the payload lacks is skipped, the next taken');
  LASTFUTDER_FRONT.NQ1={ j:NQJ(1788974000), ts:Date.now()-301000 }; LASTFUTDER.NQ1={ j:NQJ(1788974060), ts:Date.now() };
  const A=futDerBookKing('NQ1', ['NDXP','NDX']);
  ok(A.k===29355.41 && A.window==='any', '1f the front payload stale → the freshest-any payload, said as such', A.window);
  LASTFUTDER.NQ1={ j:NQJ(1788974060), ts:Date.now()-301000 };
  const S=futDerBookKing('NQ1', ['NDXP','NDX']);
  ok(S.k===null && /older than five minutes/.test(S.why), '1g both stale → k null, the why says why (the caller holds the day\'s last)', S.why);
  LASTFUTDER={}; LASTFUTDER_FRONT={};
  ok(futDerBookKing('NQ1', ['NDXP']).k===null && /no NQ1 payload yet/.test(futDerBookKing('NQ1', ['NDXP']).why), '1h no payload → "no NQ1 payload yet"');
  LASTFUTDER_FRONT.NQ1={ j:{ derived:[{ source:'QQQ', ratio:41.1, levels:[{ l:[{k:29523.25,v:1,d:1}] }] }] }, ts:Date.now() };
  ok(/no NDXP \/ NDX book/.test(futDerBookKing('NQ1', ['NDXP','NDX']).why), '1i a payload without the book → "no NDXP / NDX book"');
  const tie={ derived:[{ source:'NDXP', ratio:1.00087, levels:[{ l:[{k:29495.53,v:5,d:1},{k:29355.41,v:5,d:-1}] }] }] };
  LASTFUTDER_FRONT.NQ1={ j:tie, ts:Date.now() };
  ok(futDerBookKing('NQ1', ['NDXP']).k===29355.41, '1j a tie in dollars goes to the lower price (stable tick to tick)');
  ok(futDerBookKing('NQ1', ['NDXP']).k!==undefined && futDerBookKing('ES1', ['SPXW']).k===null, '1k never throws; a symbol with no payload answers null');
}

// ---------- 2. the file: eight lines ----------
{
  eval(v('IRT_HEADER')); eval(v('IRT_LAST'));
  eval(v('IRT_RATIO_KEY')); eval(v('IRT_NQRATIO_KEY')); eval(v('KING_LATCH_KEY'));
  eval(src.match(/var KING_LATCH_MS=\d+/)[0]+';');
  eval(v('IRT_QQQK_KEY')); eval(v('IRT_KINGS_KEY'));
  eval(v('IF_STALE_MIN'));
  eval(['irtColor'].map(ex).join('\n')); eval(v('IRT_COLORS'));
  eval(['irtNodeCol','irtPctTag','irtSpyTop','irtRound','irtCsvRow','irtRatio','irtNqRatio','kingLatchTick','irtKingLatch','irtKingHeld','irtGLatch','irtGHeld','irtQqqKing','irtQqqTop','skylitFutPx','futDerIsFront','futDerFrontList','futDerCol','futDerDedupe','futDerBookKing','irtBuildCsv'].map(ex).join('\n'));
  global.SKY_FUT={ ES:'ES1', NQ:'NQ1' }; global.FUTDER_STALE_MS=300000;
  global.emBand=()=>({ ok:true, now:7650, nowLive:7650, scaleUsed:10.0262 }); global.ladderKings=()=>[];
  global.ES_RATIO=10.05; global.NQ_RATIO=41.36; global.FEED_STALE_MS=12000;
  var LS={}; global.localStorage={ getItem:k=>(k in LS?LS[k]:null), setItem:(k,val)=>{LS[k]=String(val);} };
  // his config as the panel holds it after the upgrade: no `lines` key stored → the literal's default, 'kw'
  global.CFG={ nodeThresh:20, irt:{ on:true, secs:180, futSym:'EPU26', etfSym:'', file:'FlexLevelsExport.csv', nqOn:true, nqSym:'ENQU26', nqRatio:41.1963, lines:'kw' } };
  global.FUTMODE={ fam:'ES', r:10.0262, live:true }; global.ctTodayStr=()=>'2026-09-09';
  // InsiderFinance, 2026-09-09 ~11:00 CT: SPX 0DTE walls 7925 / 7630 (the call wall had moved up that hour), QQQ 0DTE 721 / 714
  const IFL_ROWS=[{id:'CR0', k:7925, disp:7936.39, und:790.2},{id:'PS·PS0·Mag', k:7630, disp:7640.96, und:760.9}];
  global.ifLadder=(sym)=>({ dispScale:1.001437, undScale:0.09988, rows:IFL_ROWS, err:null, srcSym:(sym==='QQQ'?'QQQ':'SPX') });
  var QCHAIN={ dte0:{ lv:{ cr:721, ps:714, mag:714 } }, toFri:{ lv:{ cr:722, ps:714 } }, all:{ lv:{ cr:730, ps:700 } }, spot:715.775, ageMin:9 };
  global.ifChain=(sym)=>(sym==='QQQ'?QCHAIN:{ dte0:{ gf:{ flip:7640 }, lv:{ cr:7925, ps:7630 } }, spot:7635.03, ageMin:12 });
  // the SPXW tape: King 7660 (+γ), then 7650 −90 · 7665 +64 · 7640 −47 · 7620 +40 (his chart that hour)
  const SPXW_TAPE=()=>({ king:7660, pct:{ '7660.00':100, '7650.00':-90, '7665.00':64, '7640.00':-47, '7620.00':40 } });
  global.tapeMap=(s)=>(s==='QQQ'?({ king:718, count:20, fromFeed:false, pct:{ '718.00':100, '716.00':-65, '714.00':21 } }):SPXW_TAPE());
  global.ladderFor=()=>null;
  // the SPY book: King 764 (+γ), 763 / 762 / 761 / 765 behind it
  const SPYJ={ levels:[{ s:763.6, l:[{k:764,v:133000000,d:1},{k:763,v:110000000,d:-1},{k:762,v:90000000,d:1},{k:761,v:70000000,d:1},{k:765,v:50000000,d:-1}] }] };
  global.LASTFEED={ SPY:{ ts:Date.now(), j:SPYJ }, QQQ:{ ts:Date.now(), j:{ levels:[{ s:718 }] } } };
  global.extractWalls=()=>({ king:764, walls:[{k:764, pct:100, pos:true}] });
  // Skylit's futures books: ES1 (SPY 10.026228 · SPXW 1.0006963), NQ1 (QQQ 41.11874 · NDXP + NDX 1.00087)
  const ESJ=(t)=>({ levels:[], expirations:['2026-09-09'], derived:[
    { source:'SPY',  ratio:10.026228, levels:[{ t:t, s:7649.0, l:[{k:7660.04,v:133000000,d:1,net:133000000},{k:7650.01,v:110000000,d:-1,net:-110000000},{k:7639.99,v:90000000,d:1,net:90000000},{k:7629.96,v:70000000,d:1,net:70000000},{k:7670.06,v:50000000,d:-1,net:-50000000}] }] },
    { source:'SPXW', ratio:1.0006963, levels:[{ t:t, s:7648.85, l:[{k:7665.33,v:258000000,d:1,net:258000000},{k:7655.33,v:232000000,d:-1,net:-232000000},{k:7670.34,v:165000000,d:1,net:165000000},{k:7645.32,v:121000000,d:-1,net:-121000000},{k:7625.31,v:103000000,d:1,net:103000000}] }] },
    { source:'SPX',  ratio:1.0006963, levels:[{ t:t, s:7648.85, l:[{k:7665.33,v:258000000,d:1,net:258000000}] }] } ],
    snapshot:{ t:t, s:7649.0, slices:[{ exp:'2026-09-09', l:[{k:7665.33,v:100,d:1,net:100},{k:7660.04,v:100,d:1,net:100},{k:7655.33,v:90,d:-1,net:-90},{k:7650.01,v:83,d:-1,net:-83},{k:7670.34,v:64,d:1,net:64}] }] } });
  const NQJ=(t)=>({ levels:[], expirations:['2026-09-09'], derived:[
    { source:'QQQ',  ratio:41.11874, levels:[{ t:t, s:29520, l:[{k:29523.25,v:94200000,d:1,net:94200000},{k:29441.02,v:61000000,d:-1,net:-61000000},{k:29358.78,v:20000000,d:1,net:20000000}] }] },
    { source:'NDXP', ratio:1.00087,  levels:[{ t:t, s:29520, l:[{k:29445.49,v:290000000,d:1,net:290000000},{k:29355.41,v:320000000,d:-1,net:-320000000},{k:29495.53,v:200000000,d:-1,net:-200000000}] }] },
    { source:'NDX',  ratio:1.00087,  levels:[{ t:t, s:29520, l:[{k:29355.41,v:100000000,d:1,net:100000000}] }] } ],
    snapshot:{ t:t, s:29520, slices:[{ exp:'2026-09-09', l:[{k:29355.41,v:100,d:-1,net:-100},{k:29523.25,v:100,d:1,net:100},{k:29445.49,v:90,d:1,net:90},{k:29441.02,v:65,d:-1,net:-65},{k:29495.53,v:64,d:-1,net:-64}] }] } });
  global.LASTFUTDER={ ES1:{ ts:Date.now(), j:ESJ(1788974000) }, NQ1:{ ts:Date.now(), j:NQJ(1788974000) } };
  global.LASTFUTDER_FRONT={ ES1:{ ts:Date.now(), j:ESJ(1788974000) }, NQ1:{ ts:Date.now(), j:NQJ(1788974000) } };
  const row=(lines,sym,re)=>lines.find(x=>x.startsWith(sym+',') && re.test(x.split(',')[2]));
  const lbl=(l)=>l?l.split(',')[2]:null, px=(l)=>l?+l.split(',')[1]:null, col=(l)=>l?+l.split(',')[3]:null, wid=(l)=>l?+l.split(',')[4]:null;

  const b=irtBuildCsv(); const L=b.csv.trim().split('\r\n');
  const E=L.filter(x=>x.startsWith('EPU26,')), Q=L.filter(x=>x.startsWith('ENQU26,'));
  const e=(re)=>row(L,'EPU26',re), q=(re)=>row(L,'ENQU26',re);
  ok(L.length===9 && E.length===4 && Q.length===4 && b.n===8, '2a EIGHT LINES: the header, four on EPU26, four on ENQU26 — n counts the lines written', [L.length, E.length, Q.length, b.n]);
  ok(E.map(lbl).sort().join('|')==='CW0|PW0|SPXW KING|SPY KING', '2b EPU26: SPXW KING · SPY KING · CW0 · PW0, nothing else', E.map(lbl));
  ok(Q.map(lbl).sort().join('|')==='CW0|NDX KING|PW0|QQQ KING', '2c ENQU26: QQQ KING · NDX KING · CW0 · PW0, nothing else', Q.map(lbl));
  ok(!L.some(x=>/XG\d|SG\d|,G\d|FLIP0|D-|,D\d/.test(x.split(',')[2])), '2d no node row, no flip, no D row anywhere in the file');
  ok(px(e(/SPXW KING/))===7665.25 && wid(e(/SPXW KING/))===3 && col(e(/SPXW KING/))===IRT_COLORS.brk, '2e SPXW KING 7660 at Skylit\'s 7665.33 → 7665.25, gold, width 3 (unchanged)', [px(e(/SPXW KING/)), wid(e(/SPXW KING/))]);
  ok(px(e(/SPY KING/))===7660.0 && wid(e(/SPY KING/))===2 && col(e(/SPY KING/))===IRT_COLORS.sply, '2f SPY KING 764 at 7660.04 → 7660.00, the lighter yellow, width 2 (unchanged)', [px(e(/SPY KING/)), wid(e(/SPY KING/))]);
  ok(px(e(/^CW0$/))===7930.5 && col(e(/^CW0$/))===IRT_COLORS.ceil && wid(e(/^CW0$/))===2 && px(e(/^PW0$/))===7635.25 && col(e(/^PW0$/))===IRT_COLORS.flr, '2g the SPX 0DTE walls 7925 / 7630 at Skylit\'s SPXW ratio: 7930.50 red · 7635.25 green, width 2 — the 7635 he asked about IS the 0DTE wall converted, not an error', [px(e(/^CW0$/)), px(e(/^PW0$/))]);
  ok(E.every(x=>!/~/.test(x.split(',')[2])), '2h no ~ on EPU26 — every line on Skylit\'s ES1 prices');
  // the NQ symbol
  ok(px(q(/QQQ KING/))===29523.25 && wid(q(/QQQ KING/))===2 && col(q(/QQQ KING/))===IRT_COLORS.sply && lbl(q(/QQQ KING/))==='QQQ KING', '2i QQQ KING 718 at Skylit\'s 29523.25 — now the LIGHTER yellow, width 2: the ETF book\'s King, as SPY KING is on ES', [px(q(/QQQ KING/)), wid(q(/QQQ KING/)), col(q(/QQQ KING/))]);
  ok(lbl(q(/NDX KING/))==='NDX KING' && px(q(/NDX KING/))===29355.5 && wid(q(/NDX KING/))===3 && col(q(/NDX KING/))===IRT_COLORS.accp, '2j NDX KING: the NDXP book\'s King 29330 at Skylit\'s 29355.41 → 29355.50, bare label (no %), the full purple (−γ), width 3 — the index book\'s King', [lbl(q(/NDX KING/)), px(q(/NDX KING/)), wid(q(/NDX KING/)), col(q(/NDX KING/))]);
  ok(px(q(/^CW0$/))===29646.5 && col(q(/^CW0$/))===IRT_COLORS.ceil && wid(q(/^CW0$/))===2 && px(q(/^PW0$/))===29358.75 && col(q(/^PW0$/))===IRT_COLORS.flr, '2k the QQQ 0DTE walls 721 / 714 at Skylit\'s QQQ ratio (× 41.11874): 29646.50 red · 29358.75 green, width 2', [px(q(/^CW0$/)), px(q(/^PW0$/))]);
  ok(Q.every(x=>!/~/.test(x.split(',')[2])), '2l no ~ on ENQU26 — the NQ1 payload prices every line');
  ok(IRT_LAST.lines==='Kings + walls' && /^live · NDXP 29330 at 29355\.41/.test(IRT_LAST.ndxWhy||'') && /^0DTE from the QQQ chain \(CW0 721, PW0 714, 9m old\)$/.test(IRT_LAST.nqIfWhy||''), '2m IRT_LAST says the mode, the NDX King\'s source and the NQ walls\' source', [IRT_LAST.lines, IRT_LAST.ndxWhy, IRT_LAST.nqIfWhy]);
  ok(/^on the panel, not in the file \(Kings \+ walls\) — live \(XG2 7650 -90%/.test(IRT_LAST.gWhy||'') && /^on the panel, not in the file \(Kings \+ walls\) — /.test(IRT_LAST.sWhy||'') && /^on the panel, not in the file \(Kings \+ walls\) — /.test(IRT_LAST.gqWhy||'') && /^on the panel, not in the file \(Kings \+ walls\) — /.test(IRT_LAST.dqWhy||'') && /the flip stays on the panel \(Kings \+ walls\)/.test(IRT_LAST.ifWhy||''), '2n the node families, the flip and the D rows are still computed and say "on the panel, not in the file"', [IRT_LAST.gWhy, IRT_LAST.ifWhy]);
  ok(IRT_LAST.esKept==='SPXW KING, SPY KING, CW0, PW0' && IRT_LAST.esLines===4, '2o esKept names the four ES lines in the file', [IRT_LAST.esKept, IRT_LAST.esLines]);
  // the latches keep running while the file is short — "you can continue your study"
  const held=JSON.parse(LS[IRT_KINGS_KEY]);
  ok(held.G && held.G.rows.length===4 && held.GS && held.GS.rows.length===4 && held.GQ && held.GQ.rows.length===2 && held.NDX && held.NDX.k===29355.5 && held.NDX.pct===-100 && held.NDX.src===29330, '2p the G / GS / GQ holds still fill (a switch to everything mid-day has its rows); NDX KING latched under NDX with its strike', Object.keys(held));

  // ---- the hold: the NQ1 payload goes stale → NDX KING from the day's latch, said held
  global.LASTFUTDER.NQ1.ts=Date.now()-301000; global.LASTFUTDER_FRONT.NQ1.ts=Date.now()-301000;
  const b2=irtBuildCsv(); const L2=b2.csv.trim().split('\r\n'); const q2=(re)=>row(L2,'ENQU26',re);
  ok(px(q2(/NDX KING/))===29355.5 && wid(q2(/NDX KING/))===3 && col(q2(/NDX KING/))===IRT_COLORS.accp && /^held \d+m — the NQ1 payload is older than five minutes/.test(IRT_LAST.ndxWhy||''), '2q NDX KING held day-scoped (v14.74: a blind tick must not delete a level he trades against), the why says held', [px(q2(/NDX KING/)), IRT_LAST.ndxWhy]);
  ok(/~/.test(lbl(q2(/QQQ KING/))) && px(q2(/QQQ KING/))===irtRound(718*41.1963,0.25), '2r …while the QQQ rows fall back to the ratio chain with ~ (unchanged)', lbl(q2(/QQQ KING/)));
  global.LASTFUTDER.NQ1.ts=Date.now(); global.LASTFUTDER_FRONT.NQ1.ts=Date.now();
  // ---- no hold, no payload → no NDX KING, no invention
  LS={}; global.localStorage={ getItem:k=>(k in LS?LS[k]:null), setItem:(k,val)=>{LS[k]=String(val);} };
  const savedNQ=global.LASTFUTDER.NQ1, savedNQF=global.LASTFUTDER_FRONT.NQ1;
  delete global.LASTFUTDER.NQ1; delete global.LASTFUTDER_FRONT.NQ1;
  const b3=irtBuildCsv(); const L3=b3.csv.trim().split('\r\n');
  ok(!row(L3,'ENQU26',/NDX KING/) && /no NQ1 payload yet/.test(IRT_LAST.ndxWhy||''), '2s nothing to hold and no payload → no NDX KING line; ndxWhy says why', IRT_LAST.ndxWhy);
  global.LASTFUTDER.NQ1=savedNQ; global.LASTFUTDER_FRONT.NQ1=savedNQF;

  // ---- the NQ walls: stale chain refused, scale checked, absent chain said
  QCHAIN.ageMin=30;
  const b4=irtBuildCsv(); const L4=b4.csv.trim().split('\r\n');
  ok(!row(L4,'ENQU26',/^CW0/) && !row(L4,'ENQU26',/^PW0/) && /QQQ chain 30m old — refused/.test(IRT_LAST.nqIfWhy||'') && row(L4,'EPU26',/^CW0$/), '2t a QQQ chain older than IF_STALE_MIN → no NQ walls (the ES walls unaffected), nqIfWhy says refused', IRT_LAST.nqIfWhy);
  QCHAIN.ageMin=9; QCHAIN.dte0.lv={ cr:7210, ps:714, mag:714 };
  const b5=irtBuildCsv(); const L5=b5.csv.trim().split('\r\n');
  ok(!row(L5,'ENQU26',/^CW0/) && row(L5,'ENQU26',/^PW0/) && /0DTE from the QQQ chain \(PW0 714, 9m old\)/.test(IRT_LAST.nqIfWhy||''), '2u a wall on the wrong scale (7210 beside a 715 spot — v14.97\'s lesson) is dropped alone; the other wall stays', IRT_LAST.nqIfWhy);
  QCHAIN.dte0.lv={ cr:721, ps:714, mag:714 };
  const savedChain=global.ifChain; global.ifChain=(sym)=>(sym==='QQQ'?null:savedChain(sym));
  const b6=irtBuildCsv(); const L6=b6.csv.trim().split('\r\n');
  ok(!row(L6,'ENQU26',/^CW0/) && /no QQQ chain/.test(IRT_LAST.nqIfWhy||'') && row(L6,'EPU26',/^PW0$/), '2v no QQQ chain at all → no NQ walls, said; the ES walls stand', IRT_LAST.nqIfWhy);
  global.ifChain=savedChain;

  // ---- everything: the ⚙ selector brings every family back exactly as v15.91 wrote it
  CFG.irt.lines='all';
  const b7=irtBuildCsv(); const L7=b7.csv.trim().split('\r\n');
  const E7=L7.filter(x=>x.startsWith('EPU26,')), Q7=L7.filter(x=>x.startsWith('ENQU26,'));
  const e7=(re)=>row(L7,'EPU26',re), q7=(re)=>row(L7,'ENQU26',re);
  ok(E7.length===13 && lbl(e7(/^XG2/))==='XG2 -90%' && lbl(e7(/^SG2/))==='SG2 -83%' && lbl(e7(/^FLIP0/))==='FLIP0' && e7(/SPXW KING/) && e7(/^CW0$/), '2w everything, EPU26: 2 Kings + XG2..5 + SG2..5 + CW0 · PW0 · FLIP0 = 13 (the D rows all dedupe on FRONT)', [E7.length, E7.map(lbl)]);
  ok(Q7.length===8 && lbl(q7(/^G2/))==='G2 -65%' && lbl(q7(/^G3/))==='G3 +21%' && q7(/NDX KING/) && q7(/QQQ KING/) && q7(/^CW0$/) && q7(/^PW0$/), '2x everything, ENQU26: QQQ KING · NDX KING · CW0 · PW0 · G2 · G3 + the D rows that survive', [Q7.length, Q7.map(lbl)]);
  ok(!q7(/D-NDX KING/) && q7(/^D-NDX2/) && q7(/^D-NDX3/) && !q7(/D-QQQ/), '2y the D list dedupes against NDX KING (29355.41 vs 29355.50) and the QQQ rows; D-NDX2 · D-NDX3 remain', Q7.map(lbl));
  ok(IRT_LAST.lines==='every family' && /^live \(XG2/.test(IRT_LAST.gWhy||'') && b7.n===21, '2z IRT_LAST.lines says every family; the whys are bare again; n = 21', [IRT_LAST.lines, b7.n]);
  CFG.irt.lines='kw';
  ok(irtBuildCsv().n===8, '2aa …and back to eight');
  delete CFG.irt.lines;
  ok(irtBuildCsv().n===8 && IRT_LAST.lines==='Kings + walls', '2ab a config with no `lines` key (his, after the upgrade) is Kings + walls');
}

// ---------- 3. the source pins ----------
{
  const B=ex('irtBuildCsv');
  ok(/var lean=\(\(cfgI\.lines\|\|'kw'\)!=='all'\);/.test(B) && /var famOn=function\(fam\)\{ return !lean \|\| fam==='king' \|\| fam==='wall'; \};/.test(B), '3a one flag, one predicate: lean unless the selector says all; king and wall are the families that pass');
  ok(/if\(!famOn\(R2\.fam\)\) return;/.test(B), '3b the targets loop drops the other families before the ETF / futures split');
  const fams=(B.match(/fam:'(king|wall|node|flip|d)'/g)||[]);
  ok(fams.length===12 && fams.filter(f=>/king/.test(f)).length===4 && fams.filter(f=>/wall/.test(f)).length===1 && fams.filter(f=>/node/.test(f)).length===4 && fams.filter(f=>/flip/.test(f)).length===1 && fams.filter(f=>/'d'/.test(f)).length===2, '3c every rows[] push carries its family: 4 King pushes (live + held × 2 books), 1 wall, 4 node (live + held × 2 books), 1 flip, 2 D (live + held)', fams.length);
  ok(/if\(!lean\) QT\.rows\.forEach/.test(B) && /if\(!lean\) HGQ\.rows\.forEach/.test(B) && /if\(!lean\) dqs\.forEach/.test(B) && /if\(!lean\) HDQ\.rows\.forEach/.test(B), '3d the NQ node and D rows are written only with every family on — computed and latched either way');
  ok(/var NK=futDerBookKing\(SKY_NQ, \['NDXP','NDX'\]\);/.test(B) && /out\.push\(irtCsvRow\(nqSym, nkPx, 'NDX KING', \(NK\.pct<0\)\?IRT_COLORS\.accp:IRT_COLORS\.brk, 3, 0\)\)/.test(B) && /irtKingLatch\('NDX', nkPx, NK\.pct, NK\.strike\)/.test(B) && /irtKingHeld\('NDX'\)/.test(B), '3e NDX KING: NDXP before NDX, bare label, full colours by polarity, width 3, latched and held under NDX');
  ok(/'QQQ KING'\+QP\.tag,\n\s+\(QK\.pct<0\)\?IRT_COLORS\.splp:IRT_COLORS\.sply, 2, 0\)\)/.test(B), '3f QQQ KING: the lighter pair, width 2');
  ok(/var cq=null; try\{ cq=ifChain\('QQQ'\); \}catch\(eCQ\)\{\}/.test(B) && /cq\.ageMin>IF_STALE_MIN/.test(B) && /Math\.abs\(v\/qs-1\)<0\.15/.test(B) && /'CW0'\+CP\.tag, IRT_COLORS\.ceil, 2, 0/.test(B) && /'PW0'\+PP\.tag, IRT_COLORS\.flr, 2, 0/.test(B), '3g the NQ walls: ifChain(QQQ) dte0, the stale rule, the scale check, red / green width 2, priced by nqPx');
  const iN=B.indexOf("'NDX KING'"), iW=B.indexOf("ifChain('QQQ')"), iG=B.indexOf('var QT=irtQqqTop'), iD=B.indexOf('var DQ=futDerFrontList(SKY_NQ)');
  ok(iN>0 && iN<iW && iW<iG && iG<iD, '3h order on the NQ symbol: NDX KING, the walls, then the G rows, then the D rows — so the D dedupe sees every line already written', [iN,iW,iG,iD]);
  ok(/n:esN\+nqN/.test(B) && /if\(ti===0\) esN\+\+;/.test(B), '3i n counts the lines written on the first target plus the NQ lines');
  ok(/lines:'kw' \}/.test(src) && /if\(o\.irt\.lines==='kw' \|\| o\.irt\.lines==='all'\) CFG\.irt\.lines=o\.irt\.lines;/.test(src), '3j the default is kw in the CFG literal and the persisted value is merged back (v14.12: a field not merged is a field that resets)');
  const cfg=ex('cfgHtml');
  ok(/class="gpts-irt-lines" data-lines="'\+p\[0\]\+'"/.test(cfg) && /\[\['kw','Kings \+ walls'\],\['all','everything'\]\]/.test(cfg) && /IRT_LAST\.lines\?\(', '\+g3esc\(IRT_LAST\.lines\)\):''/.test(cfg), '3k ⚙ IRT: the Lines selector (Kings + walls / everything) and the status line says the mode');
  ok(/CFG\.irt\.lines=\(v==='all'\)\?'all':'kw'; saveCfg\(\); IRT_TICK_LAST=0; try\{ if\(CFG\.irt\.on\) irtExportNow\(false\); \}catch\(eX\)\{\} renderCfg\(\);/.test(src), '3l a click saves the mode and rewrites the file at once');
  ok(/NDX KING '\+g3esc\(IRT_LAST\.ndxWhy\)/.test(cfg) && /NQ walls '\+g3esc\(IRT_LAST\.nqIfWhy\)/.test(cfg), '3m the status line carries the NDX King\'s and the NQ walls\' whys');
}

// ---------- 4. the records ----------
{
  const P=JSON.parse(fs.readFileSync('./learning/plan.json','utf8'));
  const nx=(P.roadmap||[]).find(r=>r.status==='next'); const r91=(P.roadmap||[]).find(r=>r.v==='15.91');
  const r92=(P.roadmap||[]).find(r=>r.v==='15.92'); ok(r92 && /KINGS \+ WALLS/.test(r92.title) && r91 && r91.status==='shipped' && (P.roadmap||[]).find(r=>/TAP RECORD/.test(r.title)) && (P.roadmap||[]).find(r=>/SEASONALITY/.test(r.title)), '4a the plan: 15.91 shipped, 15.92 Kings + walls, the tap record and seasonality still on the roadmap after it (a later build may re-sequence which is next)', r92&&[r92.v, r92.status]);
  ok(/^var PLAN_SEED=/m.test(src) && JSON.stringify(JSON.parse(src.match(/^var PLAN_SEED=(.*);$/m)[1]))===JSON.stringify(P), '4b PLAN_SEED equals the file');
  ok(/the QQQ chain.s 0DTE walls are the IRT file.s CW0 \/ PW0 on the NQ symbol/.test(JSON.stringify(P)), '4c the IF integration line names the NQ walls');
  const cl=fs.readFileSync('./changelog/CHANGELOG.md','utf8'); ok(/^## v15\.92 — KINGS \+ WALLS/m.test(cl) && /eight lines/.test(cl.split('## v15.91')[0]) && /NDX KING/.test(cl.split('## v15.91')[0]) && /7635/.test(cl.split('## v15.91')[0]), '4d CHANGELOG v15.92: eight lines, NDX KING, the 7635 answer');
  const le=fs.readFileSync('./session-state/LESSONS.md','utf8'); ok(/^### v15\.92 /m.test(le) && le.indexOf('### v15.92')<le.indexOf('### v15.91'), '4e LESSONS carries v15.92 at the top of the log');
  const de=fs.readFileSync('./session-state/DECISIONS.md','utf8'); ok(/## 2026-09-09 · v15\.92 — Kings \+ walls/.test(de) && /The tap record is v15\.93/.test(de) && /QQQ 0DTE walls at Skylit.s QQQ ratio/.test(de), '4f DECISIONS: the ruling, the NQ walls\' book, the tap record → v15.93');
  const inv=fs.readFileSync('./design/DASHBOARD-INVENTORY.md','utf8'); ok(/## 0z · v15\.92 — the IRT file is eight lines/.test(inv), '4g INVENTORY §0z');
  const ifm=fs.readFileSync('./session-state/INSIDERFINANCE.md','utf8'); ok(/THE QQQ CHAIN FEEDS THE NQ WALLS/.test(ifm) && /never compare the NQ wall \(QQQ book\) with NDX KING \(NDX book\)/.test(ifm), '4h INSIDERFINANCE: the QQQ chain feeds the NQ walls, and the two-book warning');
  const sf=fs.readFileSync('./session-state/SKYLIT-FEEDS.md','utf8'); ok(/A BOOK.S OWN KING FROM ITS derived\[\] ENTRY/.test(sf), '4i SKYLIT-FEEDS: the book-King read');
  const rn=fs.readFileSync('./session-state/latest-resume-note.md','utf8'); ok(/v15\.92: KINGS \+ WALLS/.test(rn) && /TAP RECORD/.test(rn) && fs.existsSync('./session-state/2026-09-09_resume-v15.91.md'), '4j the resume note carries the Kings + walls block and the tap record, with the v15.91 snapshot kept');
  const cfg=JSON.parse(fs.readFileSync('./.gex-config.json','utf8')); ok(JSON.stringify(cfg).indexOf('test_v1592.js')>0 && /KINGS \+ WALLS/.test(cfg.theWhatAndTheHow.irtExport), '4k .gex-config.json lists test_v1592 and the irtExport note says Kings + walls');
  const it=fs.readFileSync('./learning/items.json','utf8'); const lk=fs.readFileSync('./session-state/LOCKED-ITEMS.md','utf8'); ok(/roadmap v15\.9[3-9]/.test(it) && /v15\.9[3-9], next/.test(lk), '4l items.json and LOCKED-ITEMS carry the tap record at its roadmap slot (v15.93 at ship, re-sequenced later)');
}

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
