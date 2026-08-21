// (v11.21) THE VERIFIED WALL RULES — pinned against InsiderFinance's own published table.
//
// On 2026-08-20 I extracted their full 585-strike SPX table (1W filter, spot 7642.21) and tested every
// candidate definition against the rows THEY tag. These are the results, and they are what this file
// exists to lock down:
//
//     rule                                   strike    their tag
//     most POSITIVE net GEX above spot        7775     CALL WALL   <- match
//     most NEGATIVE net GEX below spot        7640     PUT WALL    <- match
//     largest |net GEX| anywhere              7645     Peak GEX    <- match
//     max CALL gamma above spot               7650     (nothing)   <- NOT their call wall
//     max call OPEN INTEREST above spot       8800     (nothing)   <- NOT their call wall
//
// Their own prose describes the call wall in terms of open interest. The tagged row does not follow that
// description. The table is the ground truth; the prose is not.
//
// ⚠ SIGN: their net is call-put (negative on a put-heavy strike). OURS IS THE OPPOSITE — our 760 reads
// +278M where their equivalent reads negative. So our rules are mirrored, and the fixtures below use OUR
// convention. If this ever inverts, the walls swap sides and the card is confidently wrong all day.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
global.window={__gptsDebug:{}};
eval(ex('gexLevels'));

// Their table, rebuilt in OUR sign (ours = -theirs), $M. Real rows from the 1W view.
//   strike   their net    ours
const T=[[7710,-256.9],[7715,-499.3],[7720,28.1],[7725,-374.1],[7730,66.6],[7735,-208.9],
         [7740,210.4],[7745,198.5],[7750,315.1],[7755,442.1],[7760,460.2],[7765,174.0],
         [7770,316.2],[7775,580.0],[7780,225.6],[7640,-14600],[7645,-20500],[7650,-8200]];
const rows=T.map(r=>({k:r[0], net:-r[1]*1e6, v:Math.abs(r[1])*1e6*1.4})).sort((a,b)=>a.k-b.k);
const px=7642.21;

{
  const G=gexLevels(rows, px);
  ok(G.cr===7775,'CALL WALL = most call-dominant strike above spot -> their 7775',G.cr);
  ok(G.ps===7640,'PUT WALL = most put-dominant strike below spot -> their 7640',G.ps);
  ok(G.mag===7645,'PEAK GEX = largest |net| anywhere -> their 7645',G.mag);
  // the rules that must NOT be used, kept as assertions so nobody re-derives them
  const maxCallAbove=rows.filter(r=>r.k>px).reduce((b,r)=>{const c=(Math.abs(r.v)-r.net)/2; return (!b||c>b.c)?{k:r.k,c:c}:b;},null);
  ok(maxCallAbove.k!==7775,'max CALL gamma above spot is NOT the call wall — it lands elsewhere',maxCallAbove.k);
  ok(G.crNet<0,'in OUR sign the call wall has NEGATIVE net (call-dominant)',Math.round(G.crNet/1e6));
  ok(G.psNet>0,'and the put wall POSITIVE net (put-dominant)',Math.round(G.psNet/1e6));
}
{
  // the LEAN test: a side that does not lean that way gets no wall, however big its strikes are
  const allPut=[{k:7600,net:5e9,v:5e9},{k:7700,net:3e9,v:3e9},{k:7800,net:1e9,v:1e9}];
  const G=gexLevels(allPut, px);
  ok(G.cr===null,'no call wall when every strike above spot is still put-dominant',G.cr);
  ok(G.crWeak && G.crWeak.k===7800,'the strike it would have named (least put-dominant above spot) is kept for the explanation',G.crWeak);
  ok(G.ps===7600,'the put side still gets its wall',G.ps);
  const allCall=[{k:7600,net:-5e9,v:5e9},{k:7700,net:-3e9,v:3e9}];
  const G2=gexLevels(allCall, px);
  ok(G2.ps===null,'and symmetrically: no put wall on an all-call book',G2.ps);
  ok(G2.cr===7700,'while the call wall stands',G2.cr);
}
{
  // walls must be chosen by LEAN, not by size — the defect that produced a 792 "call wall"
  const r=[{k:7600,net:9e9,v:9e9},{k:7700,net:-1e6,v:8e9},{k:7800,net:2e9,v:2e9}];
  const G=gexLevels(r, px);
  ok(G.cr===7700,'a small but genuinely call-dominant strike beats a huge put-dominant one above spot',G.cr);
  ok(G.mag===7600,'while the magnet still follows raw size',G.mag);
}
{
  // totals and ratio
  const G=gexLevels([{k:7600,net:1e9,v:3e9},{k:7700,net:-1e9,v:3e9}], px);
  ok(Math.round(G.totalCall/1e9)===3,'call total = sum (v-net)/2',G.totalCall/1e9);
  ok(Math.round(G.totalPut/1e9)===3,'put total = sum (v+net)/2',G.totalPut/1e9);
  ok(G.ratio===1,'ratio is call over put',G.ratio);
  ok(G.netTotal===0,'and the net total sums the signed column',G.netTotal);
}
{
  ok(gexLevels([],px)===null,'no rows, no levels');
  ok(gexLevels(rows,null)===null,'no price, no levels');
  ok(gexLevels(null,px)===null,'null input is handled, not thrown');
  const dirty=gexLevels([{k:7700,net:null,v:1},{k:7710,net:-1e9,v:1e9}],px);
  ok(dirty.cr===7710,'rows with a missing net are skipped rather than poisoning the pick',dirty.cr);
}
// ---------- (v11.21) THE SANITY CHECK ----------
// The user asked for a periodic automated check against a third-party page. It cannot run inside the
// panel: their server sends no CORS header (verified live — "BLOCKED Failed to fetch") and the @grant that
// would bypass it sandboxes the script and kills the feed hook. So the check is INTERNAL, and every
// assertion below corresponds to a failure that actually happened during this build.
{
  eval(ex('gexSanity'));
  global.PAL={sub:'#888'};
  global.ifManLevels=()=>null;
  let U=null; global.lvlUnified=()=>U;

  U={ px:762.57, kMin:700, kMax:800, ageMin:2,
      rows:[{id:'CR',k:770},{id:'Mag·PS',k:755}] };
  let S=gexSanity('SPY');
  ok(S.failed===0,'a healthy set passes every check',S.checks.filter(c=>!c.pass));

  // THE dangerous one: if the sign convention flips, both walls still look like plausible levels but
  // they sit on the wrong sides of price. Nothing else catches this.
  U={ px:762.57, kMin:700, kMax:800, ageMin:2, rows:[{id:'CR',k:755},{id:'PS',k:770}] };
  S=gexSanity('SPY');
  ok(S.checks.filter(c=>!c.pass && /AboveSpot|BelowSpot/.test(c.id)).length===2,'a flipped sign convention is caught on BOTH walls',S.checks.filter(c=>!c.pass).map(c=>c.id));
  ok(S.checks.some(c=>c.id==='crAboveSpot' && !c.pass),'CR below spot is flagged');
  ok(S.checks.some(c=>c.id==='psBelowSpot' && !c.pass),'PS above spot is flagged');

  // a level outside the strikes we actually read
  U={ px:762.57, kMin:700, kMax:800, ageMin:2, rows:[{id:'CR',k:900},{id:'PS',k:755}] };
  ok(gexSanity('SPY').checks.some(c=>c.id==='levelsInRange' && !c.pass),'a level outside the read range is flagged');

  // the duplicate-row bug
  U={ px:762.57, kMin:700, kMax:800, ageMin:2, rows:[{id:'CR',k:770},{id:'Mag',k:755},{id:'PS',k:755}] };
  ok(gexSanity('SPY').checks.some(c=>c.id==='noDuplicateStrikes' && !c.pass),'two rows on one strike are flagged');

  // the tail-artefact level (the 479.7 / 687 class of bug)
  U={ px:762.57, kMin:400, kMax:900, ageMin:2, rows:[{id:'CR',k:770},{id:'HVL',k:480},{id:'PS',k:755}] };
  ok(gexSanity('SPY').checks.some(c=>c.id==='levelsNearPrice' && !c.pass),'a level 37% from price is flagged as a tail artefact');

  // a stale set showing yesterday's walls
  U={ px:762.57, kMin:700, kMax:800, ageMin:45, rows:[{id:'CR',k:770},{id:'PS',k:755}] };
  ok(gexSanity('SPY').checks.some(c=>c.id==='setFresh' && !c.pass),'a stale set is flagged');

  // their hand-entered numbers: the PUT wall is the one that should agree
  U={ px:762.57, kMin:700, kMax:800, ageMin:2, rows:[{id:'CR',k:770},{id:'PS',k:760},{id:'Mag',k:758}] };
  global.ifManLevels=()=>({ cw:800, pw:760, zg:766, mag:null, ageMin:5, scale:'SPY' });
  S=gexSanity('SPY');
  ok(S.psDelta===0,'a matching put wall reports a zero delta',S.psDelta);
  ok(S.checks.some(c=>c.id==='putWallMatchesTheirs' && c.pass),'and passes');
  ok(S.crDelta===-30,'the CR delta is REPORTED',S.crDelta);
  ok(!S.checks.some(c=>c.id.indexOf('callWall')>=0),'but the CR gap is NOT failed — our window is through-Friday, theirs is a rolling 7 days');
  ok(/rolling 7 days/.test(S.note||''),'and the card says why',S.note);
  global.ifManLevels=()=>({ cw:800, pw:757, zg:766, mag:null, ageMin:5, scale:'SPY' });
  ok(gexSanity('SPY').checks.some(c=>c.id==='putWallMatchesTheirs' && !c.pass),'a put wall 3 points off THEIRS is flagged — that one has no window excuse');

  global.lvlUnified=()=>null;
  ok(gexSanity('SPY').failed===1,'no levels at all is itself a failure, not a silent pass');
}
console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
