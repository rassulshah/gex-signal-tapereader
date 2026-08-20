// (v11.8) THE LEVEL SET — CR0/CR · PS0/PS · HVL · Mag, computed from our own tape.
// The defects these tests exist to prevent, all of which produce a level that LOOKS right:
//   * pooling %King across expiry columns (each column is normalised to its OWN King — 100 in the front
//     column and 100 in the monthly column are not the same amount of money);
//   * quoting a one-week wall as though it were an all-expiration wall;
//   * copying the structural flip into the 0DTE slot when the front column has no crossing;
//   * a magnet or zero gamma computed off a map whose keys are strings and whose order is insertion order.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
function v(n){ return src.match(new RegExp('^var '+n+'\\s*=.*$','m'))[0]; }
// multi-line object literal: brace-match instead of grabbing one line
function vo(n){ const i=src.search(new RegExp('^var '+n+'\\s*=\\s*\\{','m')); let j=src.indexOf('{',i), d=0;
  for(let k=j;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){ d--; if(!d) return src.slice(i,k+1)+';'; } } return ''; }
global.window={__gptsDebug:{}};
global.localStorage={ _d:{}, getItem(k){return this._d[k]===undefined?null:this._d[k];}, setItem(k,x){this._d[k]=String(x);} };
eval([v('GLEV_CACHE'),v('GLEV_TTL'),v('EXP_NEAR_DAYS'),v('EXP_BREADTH_MIN')].join('\n'));
eval(['glevZeroGamma','glevMagnet','glevLvl','gLevels','deriveFactors','expiryProfile'].map(ex).join('\n'));

// ---------- glevZeroGamma: the crossing must be found on SORTED strikes ----------
{
  // deliberately out of insertion order — a map is not a sorted series
  const m={'775.00':80,'760.00':-50,'770.00':80,'765.00':-50};
  // ascending: 760 -50, 765 -100, 770 -20, 775 +60  -> crossing 770->775 at 20/80 = 771.25
  ok(glevZeroGamma(m)===771.25,'zero gamma sorts the map before accumulating',glevZeroGamma(m));
}
{
  const m={'760.00':-50,'765.00':-40};
  ok(glevZeroGamma(m)===null,'no sign change means no flip, not a guessed one',glevZeroGamma(m));
  ok(glevZeroGamma({})===null,'empty map returns null');
  ok(glevZeroGamma({'760.00':-50})===null,'a single strike cannot define a crossing');
}
{
  const m={'760.00':-50,'765.00':'oops','770.00':80};
  // the junk value is dropped, leaving 760 -50 then 770 +30 -> crossing at 50/80 of the way
  const z=glevZeroGamma(m);
  ok(z!==null && z>760 && z<770,'a non-numeric cell is skipped rather than poisoning the sum',z);
}
// ---------- glevMagnet ----------
{
  const m={'760.00':-90,'765.00':40,'770.00':80};
  const g=glevMagnet(m);
  ok(g.k===760,'the magnet is the largest ABSOLUTE strike, sign ignored',g);
  ok(g.pos===false,'its polarity is carried, not thrown away',g);
  ok(glevMagnet({})===null,'no strikes, no magnet');
}
// ---------- glevLvl ----------
{
  const L=glevLvl(770,55,true,'col0',767.5);
  ok(L.dist===2.5,'distance is signed from spot',L.dist);
  ok(L.distPct===0.33,'percent distance is rounded to 2dp',L.distPct);
  ok(glevLvl(null,1,true,'x',767)===null,'a missing strike yields no level rather than a zero one');
  const N=glevLvl(770,55,true,'col0',null);
  ok(N.dist===null&&N.distPct===null,'without a price the distances are null, never 0',[N.dist,N.distPct]);
}
// ---------- gLevels: end to end on a two-column ladder ----------
function setup(cols, px, nodes){
  global.STATE={ SPY:{ price:px, king:768 } };
  global.laddersByDollar=()=>({ main:{ pct:cols[0].pct, cols:cols, king:768, count:9 } });
  global.ladderFor=()=>({ pct:cols[0].pct, cols:cols, king:768, count:9 });
  global.futureStructureSummary=()=>({ above:nodes.filter(n=>n.k>px), below:nodes.filter(n=>n.k<px) });
  global.recNode=(r)=>r;
  global.atr=()=>0.6;
  global.closedCandles=()=>[];
  global.ifLevels=undefined;
  GLEV_CACHE={t:0,sym:null,data:null};
}
const FRONT={ '763.00':-70, '765.00':-100, '768.00':60, '770.00':45, '772.00':30 };
const MONTH={ '755.00':-40, '760.00':-55, '775.00':100, '780.00':70 };
const COLS=[ {exp:'2026-08-20', pct:FRONT, n:5}, {exp:'2026-09-19', pct:MONTH, n:4} ];
const NODES=[ {k:763,pct:70,abs:700000,pos:false,derived:false}, {k:765,pct:100,abs:1000000,pos:false,derived:false},
              {k:768,pct:60,abs:600000,pos:true,derived:false},  {k:770,pct:45,abs:450000,pos:true,derived:false},
              {k:775,pct:30,abs:300000,pos:true,derived:false} ];
{
  setup(COLS, 767, NODES);
  const L=gLevels('SPY');
  ok(!!L,'a level set is produced from a two-column ladder',!!L);
  ok(L.cr0 && L.cr0.k===768,'CR0 is the heaviest strike ABOVE spot in the FRONT column',L.cr0&&L.cr0.k);
  ok(L.ps0 && L.ps0.k===765,'PS0 is the heaviest strike BELOW spot in the FRONT column',L.ps0&&L.ps0.k);
  ok(L.mag0 && L.mag0.k===765,'the 0DTE magnet is the front column King',L.mag0&&L.mag0.k);
  // the whole point of reading every column: the monthly wall at 775 is invisible to a front-only reader
  ok(L.cr && L.cr.k===775,'CR comes from the LATER bucket, not the front one',L.cr&&L.cr.k);
  ok(L.cr.k!==L.cr0.k,'CR and CR0 are allowed to disagree — that disagreement is the signal',[L.cr.k,L.cr0.k]);
  ok(L.reach==='month','reach reports what our chain actually spans, 30 days here',L.reach);
  ok(L.reachDays===30,'reachDays is measured front-expiry to last-expiry',L.reachDays);
  ok(L.nCols===2,'the column count rides with the read',L.nCols);
}
// ---------- the honesty rule: a one-week chain must NOT claim full-chain reach ----------
{
  const wk=[ {exp:'2026-08-20', pct:FRONT, n:5}, {exp:'2026-08-24', pct:MONTH, n:4} ];
  setup(wk, 767, NODES);
  const L=gLevels('SPY');
  ok(L.reach==='week','a 4-day span is labelled week, never chain',L.reach);
}
{
  const one=[ {exp:'2026-08-20', pct:FRONT, n:5} ];
  setup(one, 767, NODES);
  const L=gLevels('SPY');
  ok(L.reach==='0dte','a single expiry column claims today only',L.reach);
}
// ---------- HVL and regime ----------
{
  // NODES is put-dominated all the way up (cumulative: -700k, -1.7M, -1.1M, -650k, -350k) and never
  // crosses zero. There is no flip in that book, so there must be no HVL — inventing one from the
  // nearest strike is exactly how a regime line ends up drawn where nothing changes.
  setup(COLS, 767, NODES);
  const L=gLevels('SPY');
  ok(L.hvl===null,'a book whose cumulative gamma never changes sign has NO HVL',L.hvl);
  ok(L.regimeSrc==='netGamma','with no flip the regime falls back to net gamma and says so',L.regimeSrc);
  ok(L.regime==='negGamma','and a net-short book reads as amplifying',L.regime);
}
{
  // a book that DOES flip: calls above outweigh puts below by 775
  const CROSS=[ {k:763,pct:40,abs:400000,pos:false,derived:false}, {k:765,pct:50,abs:500000,pos:false,derived:false},
                {k:770,pct:70,abs:700000,pos:true,derived:false},  {k:775,pct:60,abs:600000,pos:true,derived:false} ];
  setup(COLS, 767, CROSS);
  const L=gLevels('SPY');
  // cumulative: 763 -400k, 765 -900k, 770 -200k, 775 +400k -> crossing 770->775 at 200/600
  ok(L.hvl && L.hvl.k===771.67,'HVL interpolates the true crossing bracket',L.hvl&&L.hvl.k);
  ok(String(L.hvl.src).indexOf('native:')===0,'and is labelled with the basis it was computed on',L.hvl.src);
  ok(L.regimeSrc==='hvl','the regime cites the HVL when there is one',L.regimeSrc);
  ok(L.regime==='negGamma','spot 767 below the flip at 771.67 is the amplifying side',L.regime);
  ok(L.basis==='abs','the native read uses the absolute dollar series when the tape carries it',L.basis);
}
// ---------- derived lanes never reach the level set ----------
{
  const withDerived=NODES.concat([{k:999,pct:100,abs:null,pos:true,derived:true,src:'SPXW'}]);
  setup(COLS, 767, withDerived);
  const L=gLevels('SPY');
  ok(L.nSkipped===1,'an SPXW-derived lane is excluded and counted',L.nSkipped);
  ok(!L.mag || L.mag.k!==999,'and it can never become the magnet',L.mag&&L.mag.k);
  // the derived lane carries abs:null, but it is excluded BEFORE the basis is chosen, so it cannot drag
  // the whole read down to %King. Order matters here: filter first, then pick the ruler.
  ok(L.basis==='abs','excluding the derived lane first keeps the absolute basis intact',L.basis);
}
// ---------- refusal beats fabrication ----------
{
  setup(COLS, null, NODES);
  ok(gLevels('SPY')===null,'no price means no level set rather than levels drawn against nothing');
}
{
  global.STATE={SPY:{price:767,king:768}};
  global.laddersByDollar=()=>({}); global.ladderFor=()=>null;
  global.futureStructureSummary=()=>null; global.atr=()=>0.6; global.closedCandles=()=>[];
  global.ifLevels=undefined; GLEV_CACHE={t:0,sym:null,data:null};
  const L=gLevels('SPY');
  ok(L && L.cr0==null && L.hvl==null,'an unreadable tape yields empty slots, never invented ones',L&&[L.cr0,L.hvl]);
  ok(L && L.reach==='unknown','and the reach says so',L&&L.reach);
}
// ---------- render smoke: the card and the chart must produce HTML, never throw ----------
{
  eval(['levelsChartSvg','levelsHtml','lvlRows'].map(ex).join('\n'));
  eval([vo('LVL_COL'),vo('LVL_NAME'),vo('LVL_WHAT'),vo('PAL')].join('\n'));
  global.LVL_UI={open:true,chart:true};
  global.fmtLvl=(x)=>x==null?'–':String(x);
  global.fmtSpan=(x)=>x==null?'–':String(x);
  setup(COLS, 767, NODES);
  global.closedCandles=()=>{ const o=[]; for(let i=0;i<40;i++) o.push({c:765+Math.sin(i/4)*3, h:766+Math.sin(i/4)*3, l:764+Math.sin(i/4)*3}); return o; };
  const L=gLevels('SPY');
  const svg=levelsChartSvg('SPY',L);
  ok(svg.indexOf('<svg')===0,'the chart renders an svg',svg.slice(0,40));
  ok(svg.indexOf('polyline')>0,'with a price line in it');
  ['CR','CR0','PS0','Mag'].forEach(n=>ok(svg.indexOf('>'+n+'<')>0,'the chart labels '+n));
  ok(svg.indexOf('stroke-dasharray')>0,'and draws the 0DTE pair dashed so it reads as today-only');
  const h=levelsHtml('SPY');
  ok(h.indexOf('LEVELS')>0,'the card renders');
  ok(h.indexOf('data-glvl="open"')>0,'with a collapse handle');
  ok(h.indexOf('data-glvl="chart"')>0,'and a chart toggle');
  ok(h.indexOf('≤1 month')>0 || h.indexOf('reach')>0,'and states the reach on its face',h.slice(0,200));
  // the honesty line must actually be present — it is the whole reason CR is safe to show
  ok(h.indexOf('not an all-expiration wall')>0,'the card says our CR is not an all-expiration wall');
  // an unreadable tape renders nothing rather than an empty skeleton
  global.STATE={SPY:{price:null}}; GLEV_CACHE={t:0,sym:null,data:null};
  ok(levelsHtml('SPY')==='','no level set renders no card');
}
console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
