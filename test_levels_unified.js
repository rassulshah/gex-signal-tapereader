// (v11.17) ONE BLOCK, ONE SCALE, NO DUPLICATES — plus the self-fetched expiry sets.
//
// What went wrong in v11.16, from the user's live card: the same levels printed THREE times (a %King
// block, a call/put block, and a broken SPX block), every row carried a "SPY 765" tail while the chart
// was on ES, CR and CR0 printed the identical number twice, and the SPX block showed SPY values with
// -6879 distances. The instruction was exact: one set of levels, on the chart's instrument.
//
// Also pinned here: NO Skylit UI setting widens the feed we read (tested live — the overlay EXPIRATIONS
// dropdown and the Heatmap Wide preset both left `nodes=60 & exp_mode=next_n & exp_count=4` untouched),
// which is why the expiry sets have to be self-fetched.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
function vo(n){ const i=src.search(new RegExp('^var '+n+'\\s*=\\s*\\{','m')); let j=src.indexOf('{',i), d=0;
  for(let k=j;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){ d--; if(!d) return src.slice(i,k+1)+';'; } } return ''; }
global.window={__gptsDebug:{}};
global.SNAP_MAX_STEPBACK=8;
global.localStorage={_d:{},getItem(k){return this._d[k]===undefined?null:this._d[k];},setItem(k,v){this._d[k]=v;},removeItem(k){delete this._d[k];}};
eval([vo('LVL_COL'),vo('LVL_NAME'),vo('LVL_WHAT'),vo('PAL')].join('\n'));
eval(['pickSnapshot','gexLevels','cpFromPayload','lvlUnified','levelsHtmlV2','levelsChartV2','lvlFmt','lvlSpanFmt','ifManLevels','ifChain','ifChainRows'].map(ex).join('\n'));
global.LVL_UI={open:true,chart:false}; global.IFMAN=null; global.LASTFEED={};
global.IFC_KEY='gpts_if_chain_v1'; global.IFC_STALE_MIN=20;
global.fmtLvl=x=>x==null?'–':String(x); global.fmtSpan=x=>x==null?'–':String(x);
global.dispIsFut=()=>false; global.dispR=()=>1; global.mul=(a,b)=>a/(1/b); global.futMark=()=>'';
global.closedCandles=()=>[]; global.spxRatio=()=>({r:null});
let FULL=null, DTE0=null, PASSIVE=null;
global.SIDE_MIN_SHARE=0.05; global.HVL_MAX_DIST=0.03;
// (v11.19) expSetLevels is now (sym, setName) — the v11.17/11.18 signature was (setName) with SPY
// hardcoded inside, which is exactly the defect this release fixes.
// (v11.21) the structural set is now 'week' — every expiry through this Friday — not the full chain.
global.expSetLevels=(sym,n)=> n==='week'?FULL:(n==='dte0'?DTE0:null);
global.activeSym=()=>'SPY';
global.cpLevels=()=>PASSIVE;
global.STATE={SPY:{price:762.57, king:760}};

// ---------- cpFromPayload: decomposition on an arbitrary payload ----------
{
  const mk=(k,call,put)=>({k:k, v:call+put, net:put-call, d:1});
  const j={ levels:[{t:1,l:[]},{t:2,l:[mk(755,10,900),mk(760,20,800),mk(765,700,30),mk(770,400,10)]}],
            expirations:['2026-08-20'] };
  const L=cpFromPayload(j, 762.57);
  ok(L.ps===755,'put wall = heaviest PUT below spot',L.ps);
  ok(L.cr===765,'call wall = heaviest CALL above spot',L.cr);
  // v per strike: 755->910, 760->820, 765->730, 770->410. The magnet is TOTAL gamma, so it is 755 —
  // the same strike as the put wall. That coincidence is normal in a put-heavy book and is exactly why
  // the row de-duplication matters: 755 must print once, not as both PS and Mag.
  ok(L.mag===755,'magnet = heaviest TOTAL strike, which here is also the put wall',L.mag);
  // calls 10+20+700+400 = 1130; puts 900+800+30+10 = 1740 -> 0.65, put-heavy
  ok(L.ratio===0.65,'the ratio reports the book put-heavy, matching the fixture',L.ratio);
  ok(L.n===4 && L.kMin===755 && L.kMax===770,'range and count ride along',[L.n,L.kMin,L.kMax]);
  ok(cpFromPayload(j,null)===null,'no price, no levels');
  ok(cpFromPayload({levels:[]},762)===null,'no rows, no levels');
  // |net| > v anywhere means the fields are not total-and-net; refuse rather than invent a split
  ok(cpFromPayload({levels:[{l:[{k:760,v:10,net:-900,d:1}]}]},762)===null,'|net| > v refuses');
  // (v11.17.1) An ALL-PURE set is valid, not a refusal. On expiry day most strikes are entirely call or
  // entirely put, so |net| == v on nearly all of them — and requiring a mixed strike is exactly why the
  // 0DTE set came back null on the live panel. The convention is already established from the wider books.
  const pure=cpFromPayload({levels:[{l:[{k:760,v:10,net:10,d:1},{k:765,v:20,net:-20,d:1}]}]},762);
  ok(pure!==null,'an all-pure book decomposes rather than refusing',pure);
  ok(pure.mixed===0 && pure.pure===2,'and reports that NO strike was mixed, so the caller can see it',[pure&&pure.mixed,pure&&pure.pure]);
  ok(pure.ps===760,'760 has net=+v so it is all put -> the put wall',pure.ps);
  ok(pure.cr===765,'765 has net=-v so it is all call -> the call wall',pure.cr);
}
// ---------- lvlUnified: source precedence ----------
{
  FULL={ cr:775, crGex:9e8, ps:755, psGex:8e8, hvl:766, mag:760, ratio:0.36, n:250, nExps:12, ageMin:3, kMin:600, kMax:900 };
  DTE0=null; PASSIVE=null;
  const U=lvlUnified('SPY');
  ok(U.src==='week','the self-fetched weekly set wins when present',U.src);
  ok(U.rows.length===4,'four levels, once each',U.rows.map(r=>r.id));
  ok(U.rows[0].k===775 && U.rows[U.rows.length-1].k===755,'rows are ordered by price, high to low',U.rows.map(r=>r.k));
  ok(U.regime==='negGamma','spot 762.57 below the flip at 766 is the amplifying side',U.regime);
  ok(U.nExps===12,'the expiration count is reported so "full chain" is checkable',U.nExps);
}
{
  FULL=null; DTE0=null;
  PASSIVE={ callWall:765, callWallGex:1.7e8, putWall:760, putWallGex:2.8e8, zg:null, n:60, kMin:440, kMax:800, ratio:0.7 };
  const U=lvlUnified('SPY');
  ok(U.src==='feed','falls back to the passive feed when no set has been fetched',U.src);
  ok(U.hvlMissing===true,'and reports the missing flip rather than dropping the row silently');
  // the King (760) is also the put wall here, so the row MERGES rather than one label vanishing
  ok(U.rows.some(r=>r.k===760 && r.id.indexOf('Mag')>=0 && r.id.indexOf('PS')>=0),
     'Mag falls back to the King, and merges with PS when they share a strike',U.rows.map(r=>r.id+':'+r.k));
}
{
  FULL=null; DTE0=null; PASSIVE=null;
  ok(lvlUnified('SPY')===null,'with no source at all there is no card rather than an empty one');
  FULL={ cr:775, ps:755, hvl:766, mag:760, n:250, nExps:12 };
  global.STATE={SPY:{price:null}};
  ok(lvlUnified('SPY')===null,'no price, no card');
  global.STATE={SPY:{price:762.57, king:760}};
}
// ---------- THE DUPLICATE BUG: a level may appear only once ----------
{
  FULL={ cr:775, crGex:1, ps:755, psGex:1, hvl:766, mag:760, n:250, nExps:12 };
  DTE0={ cr:775, crGex:1, ps:755, psGex:1, n:80, nExps:1 };     // 0DTE agrees with structural
  PASSIVE=null;
  const U=lvlUnified('SPY');
  ok(U.rows.length===4,'a 0DTE wall identical to the structural one adds NO row',U.rows.map(r=>r.id));
  ok(!U.rows.some(r=>r.id==='CR0'),'no CR0 row when it would just repeat CR');
  const ks=U.rows.map(r=>r.k);
  ok(new Set(ks).size===ks.length,'no value is ever printed twice',ks);
}
{
  DTE0={ cr:768, crGex:1, ps:758, psGex:1, n:80, nExps:1 };     // 0DTE genuinely differs
  const U=lvlUnified('SPY');
  ok(U.rows.some(r=>r.id==='CR0' && r.k===768),'a 0DTE wall that DIFFERS earns its own row',U.rows.map(r=>r.id+':'+r.k));
  ok(U.rows.some(r=>r.id==='PS0' && r.k===758),'same on the put side');
  ok(U.rows.length===6,'six rows: four structural plus two that disagree',U.rows.length);
  ok(U.rows[0].k>=U.rows[1].k && U.rows[4].k>=U.rows[5].k,'still ordered by price',U.rows.map(r=>r.k));
}
// ---------- THE SCALE BUG: one instrument, no second-scale tail ----------
{
  FULL={ cr:775, crGex:9e8, ps:755, psGex:8e8, hvl:766, mag:760, ratio:0.36, n:250, nExps:12, ageMin:3 };
  DTE0=null; PASSIVE=null;
  global.dispIsFut=()=>true; global.dispR=()=>10.05;
  const h=levelsHtmlV2('SPY');
  ok(h.indexOf('LEVELS')>0,'the card renders');
  ok(h.indexOf('SPY 7')<0 && h.indexOf('>SPY <')<0,'NO "SPY nnn" tail anywhere — the chart is on ES',h.indexOf('SPY'));
  ok(h.indexOf('SPX ')<0,'and no SPX tail either');
  ok(h.indexOf('7789')>0,'levels are converted to the chart instrument (775 x 10.05)',h.indexOf('7789'));
  ok((h.match(/>CR</g)||[]).length===1,'CR appears exactly once',(h.match(/>CR</g)||[]).length);
  ok((h.match(/>PS</g)||[]).length===1,'PS appears exactly once');
  ok(h.indexOf('12 exps')>0,'the source is stated on the face',h.indexOf('12 exps'));
  global.dispIsFut=()=>false; global.dispR=()=>1;
}
{
  // and only ONE block is emitted — the v11.16 card stacked three
  FULL={ cr:775, ps:755, hvl:766, mag:760, n:250, nExps:12 };
  const h=levelsHtmlV2('SPY');
  ok((h.match(/data-glvl="open"/g)||[]).length===1,'exactly one LEVELS header');
  ok(h.indexOf('OURS · SPX')<0,'the broken SPX block is gone');
  ok(h.indexOf('CALL\/PUT · derived')<0,'and the separate call/put block is gone — it IS the levels now');
}
// ---------- their numbers stay theirs ----------
{
  global.IFMAN={ cw:7900, pw:7640, zg:7660.22, mag:7645, scale:'SPY', t:Date.now() };
  const h=levelsHtmlV2('SPY');
  ok(h.indexOf('INSIDERFINANCE')>0,'an entered IF set still renders, separately');
  ok(h.indexOf('font-style:italic')>0,'and is styled apart from ours');
  global.IFMAN=null;
  ok(levelsHtmlV2('SPY').indexOf('INSIDERFINANCE')<0,'and vanishes when cleared');
}
// ---------- (v11.17.1) the two bugs the live card exposed ----------
{
  // ZERO GAMMA must be the crossing NEAREST SPOT. Over the real 241-strike / 345-1180 chain the cumulative
  // crosses several times, and the first crossing walking up from the deep OTM puts came out at 479.7
  // against a spot of 762.57 — a tail artefact presented as the gamma flip.
  const R=[ {k:400,v:200,net:100}, {k:450,v:400,net:-300}, {k:700,v:150,net:50},
            {k:758,v:200,net:100}, {k:765,v:250,net:100}, {k:800,v:60,net:-20} ];
  // cumulative: +100, -200, -150, -50, +50, +30  -> crossings near 4xx and between 758 and 765
  const j={ levels:[{l:R.map(r=>({k:r.k, v:r.v, net:r.net, d:1}))}] };
  const L=cpFromPayload(j, 762.57);
  ok(L.nCross>=2,'the chain genuinely crosses zero more than once',L.nCross);
  ok(L.hvl>755 && L.hvl<766,'the HVL chosen is the crossing NEAREST SPOT, not the first from the bottom',L.hvl);
  ok(Math.abs(L.hvl-762.57) < Math.abs(L.crossings[0]-762.57),'and it beats the first crossing on distance to spot',[L.hvl,L.crossings]);
}
{
  // LABEL MERGE: two levels on one strike print as one row carrying both names, never one silently dropped.
  // The live card showed CR / Mag / HVL with NO put support at all, because PS and Mag were both 760.
  FULL={ cr:765, crGex:1.7e8, ps:760, psGex:2.8e8, hvl:766, mag:760, n:241, nExps:34 };
  DTE0=null; PASSIVE=null;
  const U=lvlUnified('SPY');
  const merged=U.rows.filter(r=>r.k===760)[0];
  ok(!!merged,'the shared strike still has a row',U.rows.map(r=>r.id+':'+r.k));
  ok(merged.id.indexOf('Mag')>=0 && merged.id.indexOf('PS')>=0,'and it carries BOTH labels',merged.id);
  ok(U.rows.filter(r=>r.k===760).length===1,'exactly one row for that strike',U.rows.length);
  ok(U.rows.some(r=>r.id==='CR'),'the other levels are untouched');
  ok(merged.gex===2.8e8,'the merged row keeps the gamma it was given',merged.gex);
  const h=levelsHtmlV2('SPY');
  ok(h.indexOf('Mag·PS')>0 || h.indexOf('PS·Mag')>0,'and the merged label reaches the card',h.indexOf('·'));
}
// ---------- (v11.19) A WALL NEEDS A SIDE WITH REAL GAMMA IN IT ----------
{
  // Live 0DTE book: 169 strikes, call/put ratio 0.00 — essentially no call gamma on expiry day — and we
  // still named a call wall at 792, which then sorted ABOVE the full-chain CR at 765. A 0DTE wall further
  // out than the all-expiration wall is backwards on its face. Their page reports N/A here.
  const mk=(k,call,put)=>({k:k, v:call+put, net:put-call, d:1});
  const j={ levels:[{l:[mk(755,1,900), mk(760,1,3000), mk(765,2,800), mk(792,6,10)].map(r=>r)}] };
  const L=cpFromPayload(j, 762.57);
  ok(L.cr===null,'no call wall is named when the call side is a rounding artefact',L.cr);
  ok(L.crSuppressed && L.crSuppressed.k===792,'the strike it WOULD have named is kept, for the explanation',L.crSuppressed);
  ok(L.crSuppressed.share<5,'and the share that disqualified it',L.crSuppressed.share);
  ok(L.ps===760,'the side that DOES hold gamma still gets its wall',L.ps);
  ok(L.psSuppressed===null,'and is not suppressed');
  ok(L.crGex===null,'a suppressed wall carries no gamma figure either',L.crGex);
}
{
  // symmetry: a call-dominated book must lose its PUT wall by the same rule, not just the put case
  const mk=(k,call,put)=>({k:k, v:call+put, net:put-call, d:1});
  const j={ levels:[{l:[mk(755,900,1), mk(760,3000,1), mk(765,800,2), mk(792,10,6)]}] };
  const L=cpFromPayload(j, 762.57);
  ok(L.ps===null,'a book with no put gamma names no put wall',L.ps);
  ok(L.psSuppressed!==null,'and records why',L.psSuppressed);
  ok(L.cr!==null,'while the call wall stands',L.cr);
}
{
  // a balanced book keeps both — the rule must not fire on normal days
  const mk=(k,call,put)=>({k:k, v:call+put, net:put-call, d:1});
  const j={ levels:[{l:[mk(755,100,400), mk(760,150,600), mk(765,700,200), mk(770,500,150)]}] };
  const L=cpFromPayload(j, 762.57);
  ok(L.cr!==null && L.ps!==null,'a two-sided book keeps both walls',[L.cr,L.ps]);
  ok(L.crSuppressed===null && L.psSuppressed===null,'and nothing is suppressed');
}
{
  // and the card explains the gap rather than just omitting a row
  FULL={ cr:null, crSuppressed:{k:792, share:0.4}, callShare:0.4, putShare:99.6,
         ps:760, psGex:3e8, hvl:766, mag:760, n:169, nExps:1 };
  DTE0=null; PASSIVE=null;
  const U=lvlUnified('SPY');
  ok(U.crSuppressed!==null,'the suppression reaches the unified read',U.crSuppressed);
  ok(!U.rows.some(r=>r.id.indexOf('CR')>=0),'no CR row is printed',U.rows.map(r=>r.id));
  const h=levelsHtmlV2('SPY');
  ok(h.indexOf('% of book')>0,'and the card states WHY the level is absent',h.indexOf('% of book'));
}
// ---------- (v11.19) THE PRICE ACTION OWNS THE CHART SCALE ----------
{
  // The live chart had one wall 30 points above everything else; the range stretched to reach it and the
  // price line plus every real level collapsed into the bottom fifth.
  global.closedCandles=()=>{ const out=[]; for(let i=0;i<40;i++) out.push({c:762+Math.sin(i/5)*1.2, h:763.5, l:761}); return out; };
  FULL={ cr:765, crGex:1, ps:760, psGex:1, hvl:766, mag:760, n:241, nExps:34 };
  DTE0={ cr:792, crGex:1, ps:762, psGex:1, n:169, nExps:1 };
  PASSIVE=null;
  const U=lvlUnified('SPY');
  ok(U.rows.some(r=>r.k===792),'the far level is still IN the read',U.rows.map(r=>r.id+':'+r.k));
  const svg=levelsChartV2('SPY',U);
  ok(!!svg,'the chart renders');
  ok(svg.indexOf('▲')>0,'the far level is drawn as an EDGE MARKER, not by stretching the axis',svg.indexOf('▲'));
  ok(svg.indexOf('CR0')>0,'and it is still labelled so it is not lost');
  ok(svg.indexOf('polyline')>0,'the price line is still drawn');
  // the near levels must occupy real vertical space rather than being squashed together
  const ys=(svg.match(/<line x1="0" y1="([\d.]+)"/g)||[]).map(m=>parseFloat(m.match(/y1="([\d.]+)"/)[1]));
  ok(ys.length>=2,'several level lines are drawn',ys);
  ok(Math.max.apply(null,ys)-Math.min.apply(null,ys) > 20,'and they are spread apart, not collapsed',ys);
  global.closedCandles=()=>[];
}
// ---------- (v11.20) AN HVL NOWHERE NEAR PRICE IS NOT A FLIP ----------
{
  // The live 0DTE set is essentially all put, so its cumulative only crosses out in the tail — 687.09
  // against a spot of 762.57, ten percent away. A gamma flip is a statement about where price IS.
  const R=[ {k:500,v:100,net:100}, {k:700,v:300,net:-300}, {k:900,v:50,net:50} ];
  const L=cpFromPayload({levels:[{l:R.map(r=>({k:r.k,v:r.v,net:r.net,d:1}))}]}, 762.57);
  ok(L.hvl===null,'a crossing far from price is NOT reported as the flip',L.hvl);
  ok(L.hvlFar && L.hvlFar.k>500 && L.hvlFar.k<700,'the strike it would have named is kept for the explanation',L.hvlFar);
  ok(L.hvlFar.pct>3,'along with how far away it was',L.hvlFar&&L.hvlFar.pct);
  FULL={ cr:765, crGex:1, ps:760, psGex:1, hvl:null, hvlFar:{k:687.09,pct:9.9}, mag:760, n:169, nExps:1 };
  DTE0=null; PASSIVE=null;
  const h=levelsHtmlV2('SPY');
  ok(h.indexOf('tail, not a flip')>0,'and the card says so rather than just omitting the row',h.indexOf('tail'));
}
// ---------- (v11.20) THE 0DTE SET CAN CARRY THE CARD ALONE ----------
{
  // Right after a reload the full-chain set is still in flight and the passive feed has not arrived. The
  // 0DTE set had already landed — and the card rendered BLANK, because dte0 was only ever an extra row.
  FULL=null; PASSIVE=null;
  DTE0={ cr:768, crGex:1, ps:760, psGex:1, hvl:763, mag:762, n:169, nExps:1, ratio:0.4 };
  const U=lvlUnified('SPY');
  ok(U!==null,'the card is NOT blank when only the 0DTE set has arrived',U);
  ok(U.src==='0dte','and it names 0DTE as the source',U.src);
  ok(U.rows.length===4,'all four levels come from it',U.rows.map(r=>r.id));
  const ks=U.rows.map(r=>r.k);
  ok(new Set(ks).size===ks.length,'and nothing is added to itself',ks);
  const h=levelsHtmlV2('SPY');
  ok(h.indexOf('0DTE only')>0,'the face says the read is today-only, never passing it off as structural',h.indexOf('0DTE only'));
  // and the full chain still wins the moment it lands
  FULL={ cr:775, crGex:1, ps:755, psGex:1, hvl:766, mag:760, n:241, nExps:34 };
  const U2=lvlUnified('SPY');
  ok(U2.src==='week','the weekly set takes precedence as soon as it exists',U2.src);
  ok(U2.rows.some(r=>r.id.indexOf('CR0')>=0),'and the 0DTE walls come back as their own rows',U2.rows.map(r=>r.id));
}
// ---------- (v11.22) NOTHING GOES MISSING SILENTLY ----------
// Live report: "i dont see CR0 and PS". PS was present but buried inside a "Mag·PS" label with the magnet
// first; CR0 was legitimately absent (0DTE book had no call-dominant strike above spot) but the row was
// simply omitted, so it read as broken rather than as a finding.
{
  FULL={ cr:765, crGex:1, ps:760, psGex:2, hvl:null, mag:760, n:208, nExps:2, ratio:0.52 };
  DTE0={ cr:null, crSuppressed:{k:792,share:0.2}, ps:762, psGex:1, n:169, nExps:1, ratio:0 };
  PASSIVE=null;
  const U=lvlUnified('SPY');
  const merged=U.rows.filter(r=>r.k===760)[0];
  ok(merged.id==='PS·Mag','the WALL is named first — "PS·Mag", not "Mag·PS"',merged.id);
  ok(U.rows.some(r=>r.id==='PS0' && r.k===762),'PS0 keeps its own row when it differs',U.rows.map(r=>r.id));
  ok(Array.isArray(U.absent),'an absent roster exists');
  ok(U.absent.some(a=>a.id==='CR0'),'CR0 is ACCOUNTED FOR rather than omitted',U.absent);
  ok(/all put/.test(U.absent.filter(a=>a.id==='CR0')[0].why),'with the reason it is missing',U.absent);
  ok(!U.absent.some(a=>a.id==='PS'),'PS is NOT listed absent — it is present inside the merged row',U.absent);
  ok(!U.absent.some(a=>a.id==='CR'),'nor CR');
  ok(!U.absent.some(a=>a.id==='Mag'),'nor Mag');
  const h=levelsHtmlV2('SPY');
  ok(h.indexOf('PS·Mag')>0,'the merged label reaches the card wall-first',h.indexOf('·'));
  ok(h.indexOf('all put')>0,'and the absent CR0 is explained on the face');
}
{
  // when the 0DTE wall lands on the SAME strike as the weekly one, say so rather than printing it twice
  FULL={ cr:765, crGex:1, ps:760, psGex:1, hvl:766, mag:758, n:208, nExps:2 };
  DTE0={ cr:765, crGex:1, ps:760, psGex:1, n:169, nExps:1 };
  const U=lvlUnified('SPY');
  // Better than omitting it: the strike carries BOTH labels, so the user can see the 0DTE wall agreed
  // with the weekly one rather than wondering where CR0 went. One row, one price, two facts.
  ok(U.rows.some(r=>r.id==='CR·CR0'),'a coinciding 0DTE wall is shown IN the label, not dropped',U.rows.map(r=>r.id));
  ok(U.rows.some(r=>r.id==='PS·PS0'),'same on the put side',U.rows.map(r=>r.id));
  ok(U.rows.filter(r=>r.id.indexOf('CR')>=0).length===1,'and it is still ONE row, not two at the same price');
  ok(!U.absent.some(a=>a.id==='CR0'),'so CR0 is not listed absent — it is visible',U.absent);
}
{
  // and when the 0DTE set has not loaded at all, that is a different reason and must not be conflated
  FULL={ cr:765, crGex:1, ps:760, psGex:1, hvl:766, mag:758, n:208, nExps:2 };
  DTE0=null;
  const U=lvlUnified('SPY');
  ok(U.absent.some(a=>a.id==='CR0' && /not loaded/.test(a.why)),'"not loaded" is distinguished from "none today"',U.absent);
}
// ---------- (v11.24) A DEGENERATE PAYLOAD MUST NOT LOOK AUTHORITATIVE ----------
{
  // After the close every frame reads net = +/-v, so there is no call/put split to read. The magnet still
  // works (it only needs |net|), but the walls are magnitude-only and must be flagged as provisional.
  const pure=(k,v)=>({k:k, v:v, d:1, net:v});
  const j={ levels:[{t:1,s:762,l:[pure(755,1e8),pure(760,4e8),pure(765,2e8),pure(775,3e8)]}], expirations:['2026-08-20'] };
  const L=cpFromPayload(j, 762.57);
  ok(L!==null,'a degenerate payload still yields something — the magnet survives',!!L);
  ok(L.degenerate===true,'but it is FLAGGED',L.degenerate);
  ok(L.mag===760,'and the magnet is still the largest |net|',L.mag);
  FULL=Object.assign({}, L, {nExps:1, n:4});
  DTE0=null; PASSIVE=null;
  const U=lvlUnified('SPY');
  ok(U.degenerate===true,'the flag reaches the unified read',U.degenerate);
  const h=levelsHtmlV2('SPY');
  ok(h.indexOf('no split')>0,'and the CARD says so — a provisional level must never look measured',h.indexOf('no split'));
}
{
  // a healthy payload carries no warning
  const mixed=(k,v,n)=>({k:k, v:v, d:1, net:n});
  const j={ levels:[{t:1,s:762,l:[mixed(755,1e8,6e7),mixed(760,4e8,3e8),mixed(765,2e8,-1e8),mixed(775,3e8,-2e8)]}], expirations:['2026-08-20'] };
  const L=cpFromPayload(j, 762.57);
  ok(L.degenerate===false,'a payload with real decomposition is not flagged',L.degenerate);
  FULL=Object.assign({}, L, {nExps:1, n:4});
  ok(levelsHtmlV2('SPY').indexOf('no split')<0,'and the card carries no warning');
}
// ---------- (v11.26) THEIR CHAIN LEVELS, from the companion script ----------
// The companion userscript writes finished levels to localStorage on this origin; we only read. Values
// below are the REAL ones it produced against their live page on 2026-08-20, spot 761.14 — verified
// against their published figures: to-Friday CR 775 / PS 760 / Mag 760 / ratio 0.40, and 0DTE CR
// suppressed at a 2.18% call share, which is exactly the case their page prints as N/A.
{
  const chain={ SPY:{ spot:761.14, asOf:Date.now(), today:20260820, friday:20260821,
    dte0:{ exps:[20260820], lv:{cr:null, ps:760, mag:760, maxPain:763, ratio:0.02, pcOI:55.49,
                                crSuppressed:{k:763, share:2.18}} },
    toFri:{ exps:[20260820,20260821], lv:{cr:775, ps:760, mag:760, maxPain:758, ratio:0.4, pcOI:5.18,
                                          crSuppressed:null} } } };
  global.localStorage.setItem('gpts_if_chain_v1', JSON.stringify(chain));
  const C=ifChainRows('SPY','toFri');
  ok(C!==null,'the panel reads the companion script\'s output',!!C);
  ok(C.rows.length===4,'CR, Mag, PS and Max Pain',C.rows.map(r=>r.id));
  ok(C.rows[0].id==='CR' && C.rows[0].k===775,'their call wall, ordered by price',C.rows[0]);
  ok(C.rows.some(r=>r.id==='MP' && r.k===758),'Max Pain rides along — impossible from the Skylit feed',C.rows);
  ok(C.lv.pcOI===5.18,'as does the put/call OI ratio',C.lv.pcOI);
  const C0=ifChainRows('SPY','dte0');
  ok(C0.rows.every(r=>r.id!=='CR'),'the 0DTE window names no call wall',C0.rows.map(r=>r.id));
  ok(C0.lv.crSuppressed.share===2.18,'and reports the share that disqualified it',C0.lv.crSuppressed);

  FULL={ cr:765, crGex:1, ps:760, psGex:1, hvl:766, mag:760, n:208, nExps:2, ratio:0.52 };
  DTE0=null; PASSIVE=null;
  const h=levelsHtmlV2('SPY');
  ok(h.indexOf('CHAIN · IF')>0,'the card shows their lane',h.indexOf('CHAIN'));
  ok(h.indexOf('font-style:italic')>0,'styled apart from ours — a DIFFERENT measurement, never a check on ours');
  // freshness must be visible: a stale lane silently showing yesterday's walls is the worst case
  const old=JSON.parse(JSON.stringify(chain)); old.SPY.asOf=Date.now()-45*60000;
  global.localStorage.setItem('gpts_if_chain_v1', JSON.stringify(old));
  ok(ifChainRows('SPY','toFri').stale===true,'a lane older than 20 minutes is flagged stale');
  ok(levelsHtmlV2('SPY').indexOf('⚠')>0,'and the card warns',levelsHtmlV2('SPY').indexOf('⚠'));
  // an error from the companion is surfaced, not silently treated as "no data"
  global.localStorage.setItem('gpts_if_chain_v1', JSON.stringify({SPY:{err:'no __NEXT_DATA__ block', asOf:Date.now()}}));
  ok(ifChain('SPY').err!=null,'a companion error is readable',ifChain('SPY').err);
  ok(ifChainRows('SPY','toFri')===null,'and yields no rows rather than stale ones');
  global.localStorage.removeItem('gpts_if_chain_v1');
  ok(ifChain('SPY')===null,'no companion installed, no lane');
}
{
  // AUTO WINS, HAND ENTRY IS THE FALLBACK — never both, or the card shows their levels twice
  global.IFMAN={ cw:800, pw:760, zg:766, mag:null, scale:'SPY', t:Date.now() };
  FULL={ cr:765, crGex:1, ps:760, psGex:1, hvl:766, mag:760, n:208, nExps:2 };
  let h=levelsHtmlV2('SPY');
  ok(h.indexOf('INSIDERFINANCE')>0,'with no companion, the hand-entered lane renders');
  const chain2={ SPY:{ spot:761.14, asOf:Date.now(),
    toFri:{ exps:[20260820], lv:{cr:775, ps:760, mag:760, maxPain:758, ratio:0.4, crSuppressed:null} } } };
  global.localStorage.setItem('gpts_if_chain_v1', JSON.stringify(chain2));
  h=levelsHtmlV2('SPY');
  ok(h.indexOf('CHAIN · IF')>0,'with a companion, the AUTO lane renders');
  ok(h.indexOf('INSIDERFINANCE')<0,'and the hand-entered lane steps aside — never both');
  global.localStorage.removeItem('gpts_if_chain_v1'); global.IFMAN=null;
}
console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
