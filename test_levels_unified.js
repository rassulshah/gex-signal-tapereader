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
global.localStorage={_d:{},getItem(k){return this._d[k]===undefined?null:this._d[k];},setItem(k,v){this._d[k]=v;},removeItem(k){delete this._d[k];}};
eval([vo('LVL_COL'),vo('LVL_NAME'),vo('LVL_WHAT'),vo('PAL')].join('\n'));
eval(['cpFromPayload','lvlUnified','levelsHtmlV2','levelsChartV2','lvlFmt','lvlSpanFmt','ifManLevels'].map(ex).join('\n'));
global.LVL_UI={open:true,chart:false}; global.IFMAN=null; global.LASTFEED={};
global.fmtLvl=x=>x==null?'–':String(x); global.fmtSpan=x=>x==null?'–':String(x);
global.dispIsFut=()=>false; global.dispR=()=>1; global.mul=(a,b)=>a/(1/b); global.futMark=()=>'';
global.closedCandles=()=>[]; global.spxRatio=()=>({r:null});
let FULL=null, DTE0=null, PASSIVE=null;
global.expSetLevels=(n)=> n==='full'?FULL:DTE0;
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
  // net == v on every strike carries no information
  ok(cpFromPayload({levels:[{l:[{k:760,v:10,net:10,d:1},{k:765,v:20,net:20,d:1}]}]},762)===null,'net==v everywhere refuses');
}
// ---------- lvlUnified: source precedence ----------
{
  FULL={ cr:775, crGex:9e8, ps:755, psGex:8e8, hvl:766, mag:760, ratio:0.36, n:250, nExps:12, ageMin:3, kMin:600, kMax:900 };
  DTE0=null; PASSIVE=null;
  const U=lvlUnified('SPY');
  ok(U.src==='chain','the self-fetched full-chain set wins when present',U.src);
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
  ok(U.rows.some(r=>r.id==='Mag' && r.k===760),'Mag falls back to the King when the feed cannot supply it',U.rows);
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
console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
