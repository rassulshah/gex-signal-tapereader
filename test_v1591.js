// test_v1591.js — (v15.91) THE D ROWS BY THEIR BOOK, DEDUPED — "i'll go with your recommendation" (2026-09-09) + the
// "derived King on the nq": the NDX book's King projected onto NQ (29355.41 / 1.00087 = 29330), a level the file never carried.
// Also the nightly's ORDER: the corpus appends before any judge or table reads it (his first v15.90 run read 284, not 290).
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);if(!m)throw new Error('no fn '+n);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}

ok(/@version\s+15\.91/.test(src) && /var GPTS_VERSION='15\.91';/.test(src), '0a v15.91 in both spots');

// ---------- 1. the source of every row, from Skylit's ratios ----------
{
  global.LASTFUTDER_FRONT={}; global.FUTDER_STALE_MS=300000;
  eval(['futDerFrontList','futDerDedupe'].map(ex).join('\n'));
  // his NQ chart, 2026-09-09 ~10:40 CT: three books (QQQ 41.11874 · NDXP 1.00087 · NDX 1.00087), the merged FRONT slice
  const nq={ expirations:['2026-09-09'], derived:[{source:'QQQ',ratio:41.11874,levels:[]},{source:'NDXP',ratio:1.00087,levels:[]},{source:'NDX',ratio:1.00087,levels:[]}],
    snapshot:{ slices:[{ exp:'2026-09-09', l:[{k:29355.41,v:100,d:1,net:100},{k:29523.25,v:100,d:-1,net:-100},{k:29445.49,v:90,d:1,net:90},{k:29441.02,v:65,d:-1,net:-65},{k:29495.53,v:64,d:-1,net:-64}] }] } };
  LASTFUTDER_FRONT.NQ1={ j:nq, ts:Date.now() };
  const R=futDerFrontList('NQ1').rows;
  ok(R.length===5 && R[0].src==='NDX' && R[0].strike===29330 && R[0].lbl==='D-NDX KING' && R[0].pct===100, '1a 29355.41 / 1.00087 = 29330 — the NDX book\'s King, labelled D-NDX KING (the "derived King on the nq")', R[0]);
  ok(R[1].src==='QQQ' && R[1].strike===718 && R[1].lbl==='D-QQQ KING' && R[1].pct===-100, '1b 29523.25 / 41.119 = 718.00 — the QQQ King, D-QQQ KING −100%', R[1]);
  ok(R[2].lbl==='D-NDX2' && R[2].strike===29420 && R[3].lbl==='D-QQQ2' && R[3].strike===716 && R[4].lbl==='D-NDX3' && R[4].strike===29470, '1c the rest ranked within their book: D-NDX2 (29420) · D-QQQ2 (716) · D-NDX3 (29470)', R.slice(2).map(r=>r.lbl+':'+r.strike));
  // his ES chart the same morning: two books, the FRONT slice = the two Kings and three nodes
  const es={ expirations:['2026-09-09'], derived:[{source:'SPY',ratio:10.02561,levels:[]},{source:'SPXW',ratio:1.00071,levels:[]},{source:'SPX',ratio:1.00071,levels:[]}],
    snapshot:{ slices:[{ exp:'2026-09-09', l:[{k:7665.24,v:100,d:1,net:100},{k:7649.94,v:100,d:-1,net:-100},{k:7655.23,v:84,d:-1,net:-84},{k:7639.92,v:83,d:-1,net:-83},{k:7620.21,v:75,d:1,net:75}] }] } };
  LASTFUTDER_FRONT.ES1={ j:es, ts:Date.now() };
  const E=futDerFrontList('ES1').rows;
  ok(E[0].lbl==='D-SPY KING' && E[0].strike===763 && E[1].lbl==='D-SPX KING' && E[1].strike===7660 && E[2].lbl==='D-SPX2' && E[2].strike===7650 && E[3].lbl==='D-SPY2' && E[3].strike===762 && E[4].lbl==='D-SPX3' && E[4].strike===7615, '1d ES: SPXW/SPX share a ratio → SPX; two Kings at 100% tie to the lower price first: 7649.94 → 763 (D-SPY KING), 7665.24 → 7660 (D-SPX KING), 7655.23 → 7650, 7639.92 → 762, 7620.21 → 7615', E.map(r=>r.lbl+':'+r.strike));
  const none={ expirations:['2026-09-09'], derived:[], snapshot:{ slices:[{ exp:'2026-09-09', l:[{k:7658,v:34,d:-1,net:-34},{k:7650,v:20,d:1,net:20}] }] } };
  LASTFUTDER_FRONT.ES1={ j:none, ts:Date.now() };
  const N=futDerFrontList('ES1').rows;
  ok(N[0].lbl==='D1' && !N[0].src && N[1].lbl==='D2', '1e no ratio resolves a row → plain D1 · D2 (never an invented book)', N.map(r=>r.lbl));
  // the pooled path knows each row's book from derived[].source — SPXW is labelled SPX (one ruler, one name); two 100% rows of ONE book: only the first is its KING
  const pooled={ expirations:['2026-09-09'], derived:[{source:'SPXW',ratio:1.00071,levels:[{t:1,s:7655,l:[{k:7665.24,v:100,d:1,net:100},{k:7655.23,v:99,d:-1,net:-99},{k:7620.21,v:75,d:1,net:75}]}]},{source:'SPY',ratio:10.02561,levels:[{t:1,s:7655,l:[{k:7649.94,v:100,d:-1,net:-100}]}]}] };
  LASTFUTDER_FRONT.ES1={ j:pooled, ts:Date.now() };
  const Q=futDerFrontList('ES1').rows;
  ok(Q.map(r=>r.lbl).join(',')==='D-SPY KING,D-SPX KING,D-SPX2,D-SPX3' && Q[1].src==='SPX' && Q[1].pct===100 && Q[2].pct===-99, '1g the pooled path: SPXW rows labelled SPX; the book\'s 100% row is its KING, the next (99%) D-SPX2', Q.map(r=>r.lbl+':'+r.pct));
  const odd={ expirations:['2026-09-09'], derived:[{source:'SPY',ratio:10.02561,levels:[]},{source:'SPXW',ratio:1.00071,levels:[]}], snapshot:{ slices:[{ exp:'2026-09-09', l:[{k:7658,v:34,d:-1,net:-34}] }] } };
  LASTFUTDER_FRONT.ES1={ j:odd, ts:Date.now() };
  const O=futDerFrontList('ES1').rows;
  ok(O[0].lbl==='D1' && !O[0].src, '1f 7658 resolves to no strike of either book (7652.6 SPX · 763.8 SPY) → D1: the label never claims a book it cannot show', O[0]);
}

// ---------- 2. the dedupe ----------
{
  eval(ex('futDerDedupe'));
  const rows=[{k:29355.41,lbl:'D-NDX KING'},{k:29523.25,lbl:'D-QQQ KING'},{k:29445.49,lbl:'D-NDX2'},{k:29441.02,lbl:'D-QQQ2'},{k:29495.53,lbl:'D-NDX3'}];
  const kept=futDerDedupe(rows, [29524.00, 29441.75, 29506.75, 29400, 29333]);   // QQQ KING 29524 · G2 29441.75 · G3 29506.75 · two IRT levels
  ok(kept.map(r=>r.lbl).join(',')==='D-NDX KING,D-NDX2,D-NDX3', '2a on NQ the QQQ-derived rows (29523.25 vs QQQ KING 29524.00; 29441.02 vs G2 29441.75) are duplicates and go; the NDX rows stay', kept.map(r=>r.lbl+':'+r.k));
  const es=[{k:7665.24,lbl:'D-SPX KING'},{k:7649.94,lbl:'D-SPY KING'},{k:7655.23,lbl:'D-SPX2'},{k:7639.92,lbl:'D-SPY2'},{k:7620.21,lbl:'D-SPX3'}];
  ok(futDerDedupe(es, [7665.25, 7655.25, 7620.25, 7670.25, 7595.25, 7650, 7640]).length===0, '2b on FRONT the ES list is the two books\' own rows — every one already on the chart (SPXW KING 7665.25 · XG2 7655.25 · XG3 7620.25 · SPY KING 7650 · SG2 7640): nothing written twice');
  ok(futDerDedupe(es, [7700]).length===5 && futDerDedupe([{k:7658,lbl:'D1'}], [7655.25, 7660]).length===1, '2c a row with no line within half a point (0.012% of price) survives — the 7658 case');
  ok(futDerDedupe(es, [7665.9]).length===4 && futDerDedupe(es, [7666.3]).length===5, '2d the tolerance: 0.92 on ES (7665.24 vs 7665.9 → duplicate; vs 7666.3 → not)');
}

// ---------- 3. the export blocks ----------
{
  const csv=ex('irtBuildCsv');
  ok(/var have=rows\.map\(function\(r\)\{ return \(typeof r\.es==='number'\)\?r\.es:\(fm\?irtRound\(r\.k\*fm\.mul, fm\.tick\):null\); \}\);/.test(csv) && /var kept=futDerDedupe\(DL\.rows\.slice\(0,5\), have\);/.test(csv), '3a ES: the prices already written (Skylit\'s, or the basis) are what the D rows are deduped against');
  ok(/var haveQ=out\.filter\(function\(l\)\{ return String\(l\)\.indexOf\(nqSym\+','\)===0; \}\)\.map\(function\(l\)\{ return parseFloat\(String\(l\)\.split\(','\)\[1\]\); \}\);/.test(csv) && /var keptQ=futDerDedupe\(DQ\.rows\.slice\(0,5\), haveQ\);/.test(csv), '3b NQ: the rows already written on the NQ symbol (QQQ KING, G2..G5) are what its D rows are deduped against');
  ok(/every row of Atlas.{1,8}s list is already on the chart under its book.{1,8}s label/.test(csv), '3c when nothing survives the preview says so (dWhy / dqWhy), never "no rows"');
  ok(/lbl:n\.lbl, src:n\.src\|\|null/.test(csv) && /var isK=\/ KING\$\/\.test\(g\.lbl\)/.test(csv) && /if\(ds\.length\) irtGLatch/.test(csv) && /if\(dqs\.length\) irtGLatch/.test(csv), '3d the label is the list\'s (D-<BOOK> KING / D-<BOOK>n / Dn); a KING row is width 2; an empty survivor set does not overwrite the hold');
  const fc=ex('futDerCol');
  ok(/var book=\(src==='SPY'\)\?'SPY':\(\(src==='QQQ'\)\?'QQQ':'SPXW'\);/.test(fc) && /return nodeCol\(book, g\.pct\);/.test(fc) && /isKing\?IRT_COLORS\.dking:IRT_COLORS\.dnode/.test(fc), '3e the colours: the book\'s polarity shades (his rule), the derived greys when no book resolved');
  ok(/\^D-\[A-Z\]\+ KING\$\|\^D-\[A-Z\]\+\[2-9\]\$\|\^D\[1-9\]\$/.test(ex('irtGHeld')), '3f the day-scoped hold accepts the new labels');
}

// ---------- 4. the nightly's order ----------
{
  const rp=fs.readFileSync('./tools/nightly/run.py','utf8');
  const iF=rp.indexOf('futures = refresh_futures(days)'), iJ=rp.indexOf("verdicts = [judge(H, eps, defl, null['p95'], since=frm)"), iS=rp.indexOf('if write: refresh_sweeps()'), iC=rp.indexOf('_cv.write(ROOT)');
  ok(iF>0 && iF<iJ && iJ<iS && iS<iC, '4a the corpus appends before any judge reads it; the sweep tables after the append; the coverage count last', [iF,iJ,iS,iC]);
  ok(/THE CORPUS APPENDS FIRST/.test(rp) && /10:25 CT, 2026-09-09/.test(rp), '4b …and the reason is in the file (his first v15.90 run read 284 on a machine that held 290)');
  const { execSync } = require('child_process');
  let o=''; try{ o=execSync('python3 tools/nightly/run.py --selftest',{encoding:'utf8',stdio:['ignore','pipe','pipe']}); }catch(e){ o='FAILED'+(e.stdout||''); }
  ok(/planted effect found: True/.test(o), '4c run.py\'s selftest still finds the planted effect with the new order');
}

// ---------- 5. the records ----------
{
  const P=JSON.parse(fs.readFileSync('./learning/plan.json','utf8'));
  const nx=(P.roadmap||[]).find(r=>r.status==='next'); const r90=(P.roadmap||[]).find(r=>r.v==='15.90');
  ok(nx && nx.v==='15.91' && /D-<BOOK>|by their book|deduped/i.test(nx.title) && r90 && r90.status==='shipped' && (P.roadmap||[]).find(r=>r.v==='15.92' && /TAP RECORD/.test(r.title)), '5a the plan: 15.90 shipped, 15.91 this (next), 15.92 the tap record', nx&&[nx.v, nx.status]);
  ok(/^var PLAN_SEED=/m.test(src) && JSON.stringify(JSON.parse(src.match(/^var PLAN_SEED=(.*);$/m)[1]))===JSON.stringify(P), '5b PLAN_SEED equals the file');
  const cl=fs.readFileSync('./changelog/CHANGELOG.md','utf8'); ok(/^## v15\.91 /m.test(cl) && /NDX/.test(cl.split('## v15.90')[0]), '5c CHANGELOG v15.91 names the NDX book');
  const le=fs.readFileSync('./session-state/LESSONS.md','utf8'); ok(/^### v15\.91 /m.test(le), '5d LESSONS carries v15.91');
  const cfg=JSON.parse(fs.readFileSync('./.gex-config.json','utf8')); ok(!/2026-09-09[abcd]$/.test(cfg.version) && cfg.theWhatAndTheHow.pinnedBy.indexOf('test_v1591.js')>=0, '5e .gex-config.json re-stamped and test_v1591 pinned', cfg.version);
}

console.log('test_v1591: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
