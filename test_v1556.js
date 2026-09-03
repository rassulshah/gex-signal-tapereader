// ============================================================================================
// test_v1556.js — (v15.56) THE BOOK'S LEVELS: CW0 / PW0 / CW / PW / KING in the sweep read, the book table
//   (tools/study-sweeps-book.py over the panel's own day files), the honest overnight (a courier stub is the
//   pre-market, not the overnight), companion v1.18 keeps the full Globex day for ES.
//   Every function is EXTRACTED AND RUN with stubs; GPTS_SRC points the harness at a mutated copy.
// ============================================================================================
const fs=require('fs'), cp=require('child_process');
const SRC=process.env.GPTS_SRC||'./current/gex-signal-tapereader.user.js';
const src=fs.readFileSync(SRC,'utf8');
const comp=fs.readFileSync('./current/gex-if-levels.user.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,300):''));} };
function ex(n){ const m=new RegExp('function\\s+'+n+'\\s*\\(','g').exec(src);
  if(!m) throw new Error('not found: '+n);
  let i=src.indexOf('{',m.index),d=0; for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){ d--; if(!d) return src.slice(m.index,k+1); } } }
const build=(g,fns,tail)=>new Function('__g', Object.keys(g).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+fns.map(ex).join('\n')+'\n'+(tail||''));
const RATE_MIN_N=15;
const PAL={ ink:'#e6edf3', sub:'#8b98a9', line:'#1e2530', card:'#12161f', gold:'#e3c341', blue:'#7cc7ff', amber:'#f2b45a', longAccent:'#2ec27e', shortAccent:'#f0616d' };
const esc=s=>String(s==null?'':s).replace(/"/g,'&quot;').replace(/</g,'&lt;');
const W=JSON.parse(fs.readFileSync('data/es-1min/SWEEPS.json','utf8'));
const Wslim={ corpus:W.corpus, lookup:W.lookup, ledger:W.ledger, cells:W.cells };
const WB=JSON.parse(fs.readFileSync('data/es-1min/SWEEPS-BOOK.json','utf8'));
const bareP=s=>{ const out=[]; const re=/(\d+)%/g; let m; const txt=String(s).replace(/<[^>]+>/g,' '); while((m=re.exec(txt))){ const tail=txt.slice(m.index, m.index+60); if(!/n=|\(n |n \d|sessions\)|n\s*\d+\/\d+/.test(tail)) out.push(tail.slice(0,40)); } return out; };

// ---- 1 · the overnight is honest: a stub is the pre-market --------------------------------------
{
  const mk=(nOn)=>{ const on=[]; for(let i=0;i<nOn;i++) on.push([1700000000+60*i, 100, 101+(i%7), 99-(i%5), 100, 1]);
    const g={ futSessionBars:()=>({ rth:[], on:on }) }; return build(g,['overnightHL'],'return overnightHL();')(g); };
  let o=mk(30);
  ok(o && o.full===false && o.n===30 && o.onh===107 && o.onl===95,'1a 30 bars outside RTH (the old courier trim) -> NOT full: this is the 08:00–08:29 stub',o);
  o=mk(900);
  ok(o && o.full===true && o.n===900,'1b ~900 bars -> a full Globex night',{full:o.full,n:o.n});
  ok(mk(0)===null,'1c no bars -> null, not a fake range');
  // the ladder labels follow the flag
  ok(/add\(ONs\.onh,ONs\.full\?'ONH':'PMH'\)/.test(src) && /add\(ON\.onh,ON\.full\?'ONH':'PMH'\)/.test(src),'1d both ladder consumers label a stub PMH/PML, never ONH/ONL');
  // the companion keeps the whole day for ES
  ok(/\{ k:'ES', y:'ES=F', full:true \}/.test(comp) && /if\(!full && \(sod<FUT_WIN_A\|\|sod>FUT_WIN_B\)\) continue;/.test(comp) && /futParse\(res\.responseText\|\|'', !!m\.full\)/.test(comp) && /@version\s+1\.18/.test(comp),'1e companion v1.18: ES is fetched without the UTC trim; the others keep it');
}

// ---- 2 · the book's levels, in ES points --------------------------------------------------------
{
  const mkG=(fam, r, ratio)=>({ FUTMODE:{ fam:fam, r:r }, irtRatio:()=>({ r:ratio }) });
  let g=mkG('ES', 10.02, 10.02);
  let f=build(g,['dispToEs','tapZoneEs'],'return { es:dispToEs(7660.25), z:tapZoneEs() };')(g);
  ok(f.es===7660.25 && f.z===5,'2a on an ES chart the display is ES and the tap zone is ±5 pts',f);
  g=mkG('ES', 1, 10.02); f=build(g,['dispToEs','tapZoneEs'],'return { es:dispToEs(7660.25), z:tapZoneEs() };')(g);
  ok(f.es===7660.25 && f.z===5,'2b …even before the ratio has been measured (fam decides, not r)',f);
  g=mkG(null, 1, 10.02); f=build(g,['dispToEs','tapZoneEs'],'return { es:dispToEs(765.5), z:tapZoneEs() };')(g);
  ok(Math.abs(f.es-7670.31)<0.01 && Math.abs(f.z-5.01)<0.01,'2c on a cash chart the last-good ES/SPY ratio converts (765.5 -> 7670.31), zone 0.5×r',f);
  g=mkG(null, 1, null); f=build(g,['dispToEs'],'return dispToEs(765.5);')(g);
  ok(f===null,'2d with no ratio the level is left out, not guessed');
  // bookLevelsNow: walls from the IF ladder ids, King and top-5 from the rail
  const rows=[{ id:'CR0·x', disp:7700 },{ id:'PS0', disp:7650 },{ id:'CR·y', disp:7720 },{ id:'PS', disp:7630 },{ id:'GEX', disp:7680 }];
  // deliberately NOT in size order: the small 7500 node comes first, so an unsorted slice would wrongly include it
  const nodes=[{ k:7500, es:7500, pct:10 },{ k:7660, es:7660, pct:-100, isKing:true },{ k:7681, es:7681, pct:70 },{ k:7590, es:7590, pct:30 },{ k:7645, es:7645, pct:60 },{ k:7700, es:7700, pct:50 },{ k:7610, es:7610, pct:40 }];
  g={ FUTMODE:{ fam:'ES', r:10 }, irtRatio:()=>({ r:10 }), ifLadder:()=>({ rows:rows }), tradeNodes:()=>nodes };
  const B=build(g,['dispToEs','bookLevelsNow'],'return bookLevelsNow("SPY");')(g);
  ok(B.walls.map(w=>w.name+':'+w.es).join(',')==='CW0:7700,PW0:7650,CW:7720,PW:7630','2e the four walls come from the IF ladder ids (CR0/PS0/CR/PS), an unrelated row is ignored',B.walls);
  ok(B.king && B.king.es===7660 && B.top5.length===5 && B.top5[0].isKing && B.top5.map(n=>n.es).join(',')==='7660,7681,7645,7700,7610','2f the King and the top-5 by |pct| — the 7590 and 7500 nodes are not top-5',B.top5);
  // sweepLevelsToday: book levels named by side against the open, ON labels by coverage
  const rth=[[1700000000,7670,7672,7668,7671,1]];
  g={ FUTMODE:{ fam:'ES', r:10 }, irtRatio:()=>({ r:10 }), ifLadder:()=>({ rows:rows }), tradeNodes:()=>nodes, SWEEP_IB_BARS:60,
      overnightHL:()=>({ onh:7690, onl:7640, n:30, full:false }), futSessionBars:(o)=>(o===0?{ rth:rth, on:[] }:null), priorProfile:()=>null };
  const LV=build(g,['dispToEs','bookLevelsNow','sweepLevelsToday'],'return sweepLevelsToday("SPY");')(g);
  const names=LV.map(l=>l.name);
  ok(names.indexOf('PML')>=0 && names.indexOf('PMH')>=0 && names.indexOf('ONL')<0,'2g a courier stub is named PML/PMH in the read, never ONL/ONH',names);
  ok(names.indexOf('CW0+')>=0 && names.indexOf('PW0-')>=0 && names.indexOf('CW+')>=0 && names.indexOf('PW-')>=0 && names.indexOf('KING-')>=0,'2h the walls and the King join the sweep levels, side by position against the open (7671)',names);
  const k=LV.find(l=>l.name==='KING-'); ok(k && k.low===true && k.px===7660,'2i the King below the open is a low-side level',k);
  g.overnightHL=()=>({ onh:7690, onl:7640, n:900, full:true });
  const LV2=build(g,['dispToEs','bookLevelsNow','sweepLevelsToday'],'return sweepLevelsToday("SPY");')(g);
  ok(LV2.map(l=>l.name).indexOf('ONL')>=0,'2j …and a full overnight is named ONL/ONH');
  // the King ABOVE the open is a high-side level — side is position, not name
  g.futSessionBars=(o)=>(o===0?{ rth:[[1700000000,7650,7652,7648,7651,1]], on:[] }:null);
  const LV3=build(g,['dispToEs','bookLevelsNow','sweepLevelsToday'],'return sweepLevelsToday("SPY");')(g);
  const k3=LV3.find(l=>/^KING/.test(l.name)); ok(k3 && k3.name==='KING+' && k3.low===false,'2k the King above the open (7651) is KING+, a high-side level',k3);
}

// ---- 3 · statsRead: the node clause reads the book right now, and the book table --------------------
{
  const mk=(evs, defl, book, WB_)=>{
    const g={ PAL, RATE_MIN_N, g3esc:esc, sweepsLoad:()=>Wslim, sweepsBookLoad:()=>WB_, sweepEventsToday:()=>evs, bookLevelsNow:()=>book, tapZoneEs:()=>5,
              recorderLoad:()=>({}), recorderDay:()=>({ defl:{ SPY:defl } }), ANALYSIS_NIGHTLY:null };
    g.LEVEL_TIER=g.LEVEL_TIER||new Function(src.slice(src.indexOf('var LEVEL_TIER='), src.indexOf('function levelTier('))+' return LEVEL_TIER;')(); return build(g,['levelTier','rateTxt','pctOf','ctrlTxt','statsRead'],'return statsRead("SPY");')(g);
  };
  const ev={ level:'ONL', side:'LOD', px:7660, at:'08:41', atBar:11, epoch:1700000000, bucket:'08:30-09:00', ext:7650, depth:10, speed:8, status:'reclaimed' };
  const book={ walls:[{ name:'PW0', es:7652 }], king:{ es:7649, k:765 }, top5:[{ es:7649, k:765, rank:1, isKing:true },{ es:7700, k:770, rank:2 }] };
  let R=mk([ev],[],book,WB);
  ok(/^at a NODE: the KING 7649\.00, PW0 7652\.00 inside the tap zone \(±5\.00\)/.test(R.lines[0].node),'3a the extremum inside the zone of the King and a wall -> named',R.lines[0].node);
  ok(/sweeps AT a node printed the extreme thin \(n=10\) vs NOT at a node thin \(n=8\) \(book corpus, 9 sessions\) · H6 reads at 40/.test(R.lines[0].node),'3b …with H6’s own comparison from the book table: under 15 a cell says thin, with its n',R.lines[0].node);
  R=mk([ev],[],{ walls:[], king:{ es:7620 }, top5:[{ es:7700, rank:1 }] },WB);
  ok(/^at NOTHING: no top-5 node, King or wall inside the tap zone/.test(R.lines[0].node),'3c nothing inside the zone and no deflection -> at NOTHING');
  R=mk([ev],[{ t:1700000000*1000+3*60*1000, strike:7650, cont:1 }],{ walls:[], king:null, top5:[] },WB);
  ok(/^at NOTHING on the book now, but a deflection was recorded 3 min from the sweep at 7650 \(CONTINUED\)/.test(R.lines[0].node),'3d the deflection ledger still speaks when the book has moved on');
  R=mk([ev],[],book,null);
  ok(/the node-conditioned rate is UNMEASURED \(H6, register 0\/40\)/.test(R.lines[0].node),'3e with no book table the rate says UNMEASURED');
  // a sweep OF a book level answers by name from the book table, and says thin
  const kev={ level:'KING-', side:'LOD', px:7660, at:'09:20', atBar:50, epoch:1700003000, bucket:'09:00-10:00', ext:7655, depth:5, speed:4, status:'reclaimed' };
  R=mk([kev],[],{ walls:[], king:{ es:7660 }, top5:[] },WB);
  ok(/by name: KING- \(the book\) thin \(n=5\) · book corpus n=9 sessions — a book level; thin until the exports accumulate/.test(R.lines[0].txt),'3f KING- by name comes from the book table with its n and says thin',R.lines[0].txt.slice(-200));
  ok(bareP(R.lines.map(l=>(l.head||'')+' '+l.txt+' '+(l.node||'')).join(' ')).length===0,'3g no bare % in any line',bareP(R.lines.map(l=>l.txt+' '+(l.node||'')).join(' ')));
}

// ---- 4 · the book study and the table -------------------------------------------------------------
{
  const r=cp.spawnSync('python3',['tools/study-sweeps-book.py','--selftest'],{encoding:'utf8'});
  ok(r.status===0 && /PDL sweep at a node found and scored: True/.test(r.stdout) && /KING- sweep found: True/.test(r.stdout),'4a study-sweeps-book --selftest: a planted PDL sweep landing on the King is found, tagged at-node, scored; the King sweep itself is found',(r.stdout||'').trim().split('\n').pop());
  ok(WB.lookup && WB.lookup.node && WB.lookup.node.atNode && typeof WB.lookup.node.atNode.n==='number' && WB.lookup.level['KING-'] && WB.lookup.level['CW0+'] && WB.corpus.unit && /SPY 3-minute/.test(WB.corpus.unit),'4b SWEEPS-BOOK.json carries the node lookup, the book levels by name, and names its unit');
  ok(WB.corpus.sessions>=5 && WB.lookup.node.atNode.n<40,'4c today the book corpus is thin ('+WB.corpus.sessions+' sessions, '+WB.lookup.node.atNode.n+' at-node events) — it says so rather than pooling');
  const rn=cp.spawnSync('python3',['tools/nightly/run.py','--selftest'],{encoding:'utf8'});
  ok(rn.status===0 && /H6\s+THIN\s+n\s+0\/40\s+0 sweep-at-a-node events on sessions from/.test(rn.stdout),'4d the nightly judges H6 from the book table (thin, n counted from the register date), no longer "blocked"',(rn.stdout||'').split('\n').filter(l=>/H6/.test(l)).join(' | '));
  const S=JSON.parse(fs.readFileSync('learning/studies.json','utf8'));
  const flat=[]; S.subjects.forEach(sj=>sj.subsections.forEach(ss=>ss.studies.forEach(x=>flat.push(x))));
  const f=flat.find(x=>x.id==='H2.10f'), g=flat.find(x=>x.id==='H2.10g');
  ok(f && f.status==='THIN' && /BOOK CORPUS 9 sessions/.test(f.result) && /AT a top-5 node \/ the King 30% n=10 vs NOT at a node 38% n=8/.test(f.result),'4e H2.10f carries the book numbers, thin, with n on every rate');
  ok(g && g.status==='REGISTERED' && /H6/.test(g.result),'4f H2.10g is the sweep × node table, registered as H6');
}

// ---- 5 · the Analysis H2 table shows the book block ---------------------------------------------------
{
  const g={ PAL, RATE_MIN_N, g3esc:esc, tabEmpty:t=>'<div>'+t+'</div>', sweepsBookLoad:()=>({ corpus:WB.corpus, lookup:WB.lookup }) };
  const h=build(g,['rateTxt','sweepTableHtml','panSection','panNote','panRow'],'return sweepTableHtml(__g.W);')(Object.assign({W:Wslim},g));
  ok(/the book · 9 sessions of the panel’s own exports · SPY 3-min/.test(h) && /KING ↓ → LOD/.test(h) && /price sweep AT a top-5 node \/ the King/.test(h) && /thin \(n=/.test(h),'5a the H2 table carries the book block, thin cells say thin',h.slice(h.indexOf('the book'), h.indexOf('the book')+200));
  ok(bareP(h).length===0,'5b no bare % in the table',bareP(h));
  const g2={ PAL, RATE_MIN_N, g3esc:esc, tabEmpty:t=>'<div>'+t+'</div>', sweepsBookLoad:()=>null };
  const h2=build(g2,['rateTxt','sweepTableHtml','panSection','panNote','panRow'],'return sweepTableHtml(__g.W);')(Object.assign({W:Wslim},g2));
  ok(/the book table \(SWEEPS-BOOK\.json\) has not been fetched yet/.test(h2),'5c …and says so when the book table is not there');
}

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
