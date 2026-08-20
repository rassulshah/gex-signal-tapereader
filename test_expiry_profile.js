// (v11.6) THE EXPIRATION PROFILE — read every expiry column, not just the front one. Fixtures mirror the
// live 2026-08-19/20 ladder (Strike + 5 ISO expiry columns, share-of-book values, yellow King in column 1).
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
function v(n){ return src.match(new RegExp('^var '+n+'\\s*=.*$','m'))[0]; }
global.window={__gptsDebug:{}};
eval([v('KING_DOLLAR_RE'),v('TAPE_REJECT_RE'),v('ISO_DATE_RE'),v('TAPE_TOK_RE'),v('TAPE_KING_DOLLAR_IN'),v('LADDER_SYM_RE'),v('LADDER_CACHE'),v('EXP_NEAR_DAYS'),v('EXP_BREADTH_MIN')].join('\n'));
eval(['ladderStrikeOf','ladderCellParse','ladderRowsFromDollar','ladderSymbolOf','readLaddersByDollar','laddersByDollar','expiryProfile','nodeBreadth'].map(ex).join('\n'));
global.STATE={SPY:{price:766.4}};

// ---- a main <table>: Strike + 2026-08-20 (front) · 08-21 · 08-24 (weekly) · 09-19 (monthly, far)
let ALL=[];
function El(tag,kids,text,style){ const e={tagName:tag,children:kids||[],style:style||{},parentElement:null,_text:text||null};
  e.children.forEach(k=>k.parentElement=e);
  e.closest=function(sel){ let n=this; while(n){ if(sel==='table'&&n.tagName==='TABLE') return n; n=n.parentElement; } return null; };
  e.contains=function(x){ let n=x; while(n){ if(n===this) return true; n=n.parentElement; } return false; };
  Object.defineProperty(e,'textContent',{get(){ return this._text!=null?this._text:this.children.map(c=>c.textContent).join(''); }});
  e.querySelector=function(sel){ if(sel==='thead') return this.children.find(c=>c.tagName==='THEAD')||null; return null; };
  e.querySelectorAll=function(sel){ const out=[]; (function walk(n){ n.children.forEach(c=>{ if(sel==='tr'?c.tagName==='TR':(sel==='table'?c.tagName==='TABLE':sel.split(',').map(x=>x.trim().toUpperCase()).includes(c.tagName))) out.push(c); walk(c); }); })(this); return out; };
  ALL.push(e); return e; }
const HDR=['Strike','2026-08-20','2026-08-21','2026-08-24','2026-09-19'];
// [strike, front, bg, [08-21, 08-24, 09-19]]
const rows=[
 [790,'1%','',   ['0%','1%','$412,900K62%']],   // a MONTHLY wall far above, carrying the book's $K — invisible to a front-only reader
 [775,'4%','',   ['3%','2%','8%']],
 [770,'22%','',  ['30%','9%','12%']],
 [769,'18%','',  ['26%','6%','5%']],
 [768,'35%','',  ['40%','31%','9%']],     // carried by 3 columns = structural
 [767,'44%','',  ['12%','4%','3%']],
 [766,'55%','',  ['9%','2%','1%']],
 [765,'80%','rgb(253, 231, 37)',['35%','28%','14%']],
 [764,'48%','',  ['8%','3%','2%']],
 [763,'30%','',  ['6%','2%','1%']],
 [762,'12%','',  ['4%','1%','0%']],
 [761,'6%','',   ['2%','0%','0%']],
 [760,'4%','',   ['1%','0%','0%']],
 [755,'2%','',   ['1%','0%','9%']],
 [750,'1%','',   ['0%','0%','58%']],      // a MONTHLY put wall far below
 [749,'0%','',   ['0%','0%','3%']],
 [748,'0%','',   ['0%','0%','2%']],
 [747,'0%','',   ['0%','0%','1%']],
 [746,'0%','',   ['0%','0%','1%']],
 [745,'0%','',   ['0%','0%','1%']]];
function tr(r){ return El('TR',[El('TD',[],r[0].toFixed(1)), El('TD',[],r[1],{backgroundColor:r[2]||''}), ...r[3].map(t=>El('TD',[],t))]); }
const thead=El('THEAD',[El('TR',HDR.map(h=>El('TH',[],h)))]);
const table=El('TABLE',[thead, El('TBODY',rows.map(tr))]);
const page=El('DIV',[table]);
global.document={ querySelectorAll:(sel)=>page.querySelectorAll(sel) };
LADDER_CACHE={t:0,data:null};

const L=(laddersByDollar()||{}).main;
ok(!!L, '0 the main ladder is found without any $K cell');
ok(L.cols && L.cols.length===4, '1a all FOUR expiry columns are read (the reader used one until now)', L.cols&&L.cols.length);
ok(L.cols.map(c=>c.exp).join(',')==='2026-08-20,2026-08-21,2026-08-24,2026-09-19', '1b each column keeps its ISO expiry', L.cols&&L.cols.map(c=>c.exp));
ok(L.cols[0].pct['765.00']===100 && L.cols[3].pct['790.00']===100, '1c every column is rescaled to its OWN strongest strike', [L.cols[0].pct['765.00'],L.cols[3].pct['790.00']]);
ok(L.bookKing && L.bookKing.k===790 && L.bookKing.kd===412900, '1d the $K in the far column is the BOOK King, and column 1 still drives %King', L.bookKing);
ok(L.king===765 && L.kingSrc==='highlight', '1e the yellow front-expiry cell is still today\'s King', [L.king,L.kingSrc]);

const P=expiryProfile('SPY');
ok(!!P && P.nCols===4 && P.front==='2026-08-20', '2a profile built, front expiry named', P&&[P.nCols,P.front]);
ok(P.buckets.dte0.n===1 && P.buckets.near.n===2 && P.buckets.far.n===1, '2b buckets: front / within a week / beyond', P&&[P.buckets.dte0.n,P.buckets.near.n,P.buckets.far.n]);

// today's walls vs the structural walls
const d0=P.buckets.dte0.walls, aw=P.all.walls;
ok(d0.ceil.k===767 && d0.flr.k===765, '3a FRONT expiry walls are the intraday ones — the strongest strike each side of price (767 / 765)', [d0.ceil&&d0.ceil.k, d0.flr&&d0.flr.k]);
ok(aw.ceil.k===790 && aw.flr.k===750 && P.structuralFrom==='far', '3b the STRUCTURAL walls come from the far bucket alone (790 / 750) — the outer cage, invisible to a front-only read, and read inside ONE column set so the percentages are comparable', [aw.ceil&&aw.ceil.k, aw.flr&&aw.flr.k, P.structuralFrom]);
ok(P.buckets.near.walls.ceil.k===768, '3b2 the near bucket has its own walls (768), never mixed with the others', P.buckets.near.walls.ceil&&P.buckets.near.walls.ceil.k);
ok(P.structuralDiffers===true, '3c the panel knows today\'s walls are not the structural ones', P.structuralDiffers);

// breadth
ok(P.breadth['768.00']===3, '4a 768 is carried by 3 expirations — structure', P.breadth['768.00']);
ok(P.breadth['766.00']===1, '4b 766 is a front-expiry-only level — it dies at the close', P.breadth['766.00']);
ok(nodeBreadth('SPY',765)===3 && nodeBreadth('SPY',790)===1, '4c nodeBreadth(k) answers per strike — 765 clears the bar in 3 of 4 columns (14% in the monthly does not count)', [nodeBreadth('SPY',765),nodeBreadth('SPY',790)]);
ok(nodeBreadth('SPY',999)===0, '4d a strike nobody carries is 0, not null');

// a single-column ladder (EXPIRATIONS toggle off) must degrade to null, never guess
const t2=El('TABLE',[El('THEAD',[El('TR',[El('TH',[],'Strike'),El('TH',[],'2026-08-20')])]),
  El('TBODY',rows.map(r=>El('TR',[El('TD',[],r[0].toFixed(1)),El('TD',[],r[1],{backgroundColor:r[2]||''})])))]);
const page2=El('DIV',[t2]); global.document={ querySelectorAll:(sel)=>page2.querySelectorAll(sel) }; LADDER_CACHE={t:0,data:null};
ok(expiryProfile('SPY')===null, '5a one column = no profile (null), never an invented structural wall');
ok(nodeBreadth('SPY',765)===null, '5b ...and breadth is unknown, not 0');

console.log('test_expiry_profile: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
