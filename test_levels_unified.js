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
eval(['pickSnapshot','gexLevels','cpFromPayload','lvlUnified','ifManLevels','ifChain'].map(ex).join('\n'));
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
// (v15.53) section removed: levelsHtmlV2 (the LEVELS card) archived (A-dead-else); lvlUnified — the live subject — is pinned above
// (v15.53) section removed: levelsHtmlV2 (the LEVELS card) archived (A-dead-else); lvlUnified — the live subject — is pinned above
// ---------- their numbers stay theirs ----------
// (v15.53) section removed: levelsHtmlV2 (the LEVELS card) archived (A-dead-else); lvlUnified — the live subject — is pinned above
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
  ok(L.hvl>755 && L.hvl<766,'the FLIP chosen is the crossing NEAREST SPOT, not the first from the bottom',L.hvl);
  ok(Math.abs(L.hvl-762.57) < Math.abs(L.crossings[0]-762.57),'and it beats the first crossing on distance to spot',[L.hvl,L.crossings]);
}
// (v15.53) section removed: levelsHtmlV2 (the LEVELS card) archived (A-dead-else); lvlUnified — the live subject — is pinned above
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
// (v15.53) section removed: levelsHtmlV2 (the LEVELS card) archived (A-dead-else); lvlUnified — the live subject — is pinned above
// ---------- (v11.19) THE PRICE ACTION OWNS THE CHART SCALE ----------
// (v15.53) section removed: levelsChartV2 archived with the LEVELS card
// ---------- (v11.20) A FLIP NOWHERE NEAR PRICE IS NOT A FLIP ----------
// (v15.53) section removed: levelsHtmlV2 (the LEVELS card) archived (A-dead-else); lvlUnified — the live subject — is pinned above
// ---------- (v11.20) THE 0DTE SET CAN CARRY THE CARD ALONE ----------
// (v15.53) section removed: levelsHtmlV2 (the LEVELS card) archived (A-dead-else); lvlUnified — the live subject — is pinned above
// ---------- (v11.22) NOTHING GOES MISSING SILENTLY ----------
// Live report: "i dont see CR0 and PS". PS was present but buried inside a "Mag·PS" label with the magnet
// first; CR0 was legitimately absent (0DTE book had no call-dominant strike above spot) but the row was
// simply omitted, so it read as broken rather than as a finding.
// (v15.53) section removed: levelsHtmlV2 (the LEVELS card) archived (A-dead-else); lvlUnified — the live subject — is pinned above
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
// (v15.53) section removed: levelsHtmlV2 (the LEVELS card) archived (A-dead-else); lvlUnified — the live subject — is pinned above
// (v15.53) section removed: levelsHtmlV2 (the LEVELS card) archived (A-dead-else); lvlUnified — the live subject — is pinned above
// ---------- (v11.26) THEIR CHAIN LEVELS, from the companion script ----------
// The companion userscript writes finished levels to localStorage on this origin; we only read. Values
// below are the REAL ones it produced against their live page on 2026-08-20, spot 761.14 — verified
// against their published figures: to-Friday CR 775 / PS 760 / Mag 760 / ratio 0.40, and 0DTE CR
// suppressed at a 2.18% call share, which is exactly the case their page prints as N/A.
// (v15.53) section removed: levelsHtmlV2 (the LEVELS card) archived (A-dead-else); lvlUnified — the live subject — is pinned above
// (v15.53) section removed: levelsHtmlV2 (the LEVELS card) archived (A-dead-else); lvlUnified — the live subject — is pinned above
console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
