// (v11.2) $K-ANCHORED LADDER READER — the user's principle: find the King by its dollar cell first, then read the
// ladder outward from it, whatever the markup. Fixtures mirror the live DOM of 2026-08-19 (Trinity panes = div rows
// "[strike][value]" with "+8%$92,931K" on the King row and "-1%65%" elsewhere; main <table> = Strike + 5 expiry columns,
// share-of-book values, yellow King cell, $K (book King) in a later column).
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
function v(n){ return src.match(new RegExp('var '+n+'\\s*=.*'))[0]; }
eval([v('KING_DOLLAR_RE'),v('TAPE_REJECT_RE'),v('ISO_DATE_RE'),v('TAPE_TOK_RE'),v('TAPE_KING_DOLLAR_IN'),v('LADDER_SYM_RE'),v('LADDER_CACHE'),v('TAPE_CACHE')].join('\n'));
eval(['ladderStrikeOf','ladderCellParse','ladderRowsFromDollar','ladderSymbolOf','readLaddersByDollar','laddersByDollar','ladderFor','kingResolve','tapeCellPct','leadSignedPct','parseKingDollarsK','tapeStrikeRowCount','findTapeTable','leadTok','tapeCells','readTapeFromDOM'].map(ex).join('\n'));
global.window={__gptsDebug:{}}; global.LASTDISP={SPY:'gamma',QQQ:'gamma'};

// ---------- a tiny DOM
let ALL=[];
function El(tag, kids, text, style){ const e={tagName:tag, children:kids||[], style:style||{}, parentElement:null, _text:text||null};
  e.children.forEach(k=>k.parentElement=e); e.closest=function(sel){ let n=this; while(n){ if(sel==='table'&&n.tagName==='TABLE') return n; n=n.parentElement; } return null; };
  e.contains=function(x){ let n=x; while(n){ if(n===this) return true; n=n.parentElement; } return false; };
  Object.defineProperty(e,'textContent',{get(){ return this._text!=null?this._text:this.children.map(c=>c.textContent).join(''); }});
  e.querySelector=function(sel){ if(sel==='thead') return this.children.find(c=>c.tagName==='THEAD')||null; return null; };
  e.querySelectorAll=function(sel){ const out=[]; (function walk(n){ n.children.forEach(c=>{ if(sel==='tr'?c.tagName==='TR':(sel==='table'?c.tagName==='TABLE':sel.split(',').map(s=>s.trim().toUpperCase()).includes(c.tagName))) out.push(c); walk(c); }); })(this); return out; };
  ALL.push(e); return e; }
const span=(t)=>El('SPAN',[],t); const div=(kids,text,style)=>El('DIV',kids,text,style);
// Trinity pane: header + ladder rows + footer
function pane(sym, price, rows){ // rows: [strike, cellText, bg]
  const hdr=div([span(sym),span('$'+price),span('+0.4%'),span('King0.4% ↑')]);
  const ladder=div(rows.map(r=>div([div([],String(r[0]),{}), div([span(r[1].slice(0,0))],r[1],{backgroundColor:r[2]||''})])));
  const foot=div([span('−3%'),span(''),span('$92,931K')]);
  return div([hdr,div([ladder]),foot]);
}
const spyRows=[[784,'1%','rgb(68, 14, 95)'],[783,'1%','rgb(68, 14, 95)'],[782,'1%',''],[781,'−1%','rgb(68, 9, 91)'],[780,'2%',''],[779,'−3%','rgb(68, 1, 84)'],[778,'0%',''],[777,'+2%15%','rgb(65, 61, 131)'],[776,'-7%14%','rgb(67, 57, 130)'],[775,'+7%59%','rgb(46, 167, 127)'],[774,'+8%$92,931K','rgb(253, 231, 37)'],[773,'-1%65%','rgb(53, 180, 120)'],[772,'-2%66%','rgb(55, 182, 118)'],[771,'−10%','rgb(68, 1, 84)'],[770,'41%','rgb(80,190,100)'],[769,'59%','rgb(46,167,127)'],[768,'55%',''],[767,'17%',''],[766,'20%',''],[765,'5%','']];
const qqqRows=[[731,'1%',''],[729,'−1%',''],[723,'33%',''],[720,'11%',''],[719,'16%',''],[718,'10%',''],[717,'+1%−$247,657K','rgb(68, 1, 84)'],[716,'34%',''],[715,'56%',''],[714,'5%',''],[713,'2%',''],[712,'1%',''],[711,'0%',''],[710,'0%',''],[709,'0%',''],[708,'0%','']];
// main table (expirations view)
const HDR=['Strike','2026-08-19','2026-08-20','2026-08-21','2026-08-24','2026-08-25'];
function tableRow(k, c1, bg, rest){ return El('TR',[El('TD',[],k.toFixed(1)), El('TD',[],c1,{backgroundColor:bg||''}), ...rest.map(t=>El('TD',[],t))]); }
const mainRows=[[780,'1%','',['1%','17%','0%','-1%']],[779,'-1%','',['1%','-2%','0%','0%']],[778,'0%','',['0%','-2%','0%','0%']],[777,'4%+23%','',['2%','1%','0%','0%']],[776,'3%','',['0%','2%','0%','0%']],[775,'6%+23%','',['-4%','-$341,276K-1%','-1%','-2%']],[774,'31%-7%','rgb(253, 231, 37)',['2%','-3%','0%','0%']],[773,'10%+2%','',['-1%','+32%','-3%','0%']],[772,'1%','rgb(110, 202, 85)',['6%','-3%','21%','-1%']],[771,'-7%-6%','',['0%','-1%','-1%','-1%']],[770,'12%-4%','',['1%','5%','1%','0%']],[769,'15%-3%','',['7%','3%','0%','-1%']],[768,'12%+7%','',['2%','-4%','-2%','0%']],[767,'5%-22%','',['3%','1%','0%','0%']],[766,'5%-5%','',['0%','-11%','-1%','0%']],[765,'5%','',['0%','0%','0%','0%']],[764,'5%','',['0%','0%','0%','0%']],[763,'2%','',['0%','0%','0%','0%']],[762,'1%','',['0%','0%','0%','0%']],[761,'0%','',['0%','0%','0%','0%']]];
const thead=El('THEAD',[El('TR',HDR.map(h=>El('TH',[],h)))]); const tbody=El('TBODY',mainRows.map(r=>tableRow(r[0],r[1],r[2],r[3])));
const table=El('TABLE',[thead,tbody]);
const page=div([div([table]), div([pane('SPY','771.10',spyRows), pane('QQQ','718.23',qqqRows)])]);
global.document={ querySelectorAll:(sel)=>page.querySelectorAll(sel) };

// ---------- 1. all ladders found from their $K, whatever the markup
const all=readLaddersByDollar();
ok(Object.keys(all).sort().join(',')==='QQQ,SPY,main', '1a three ladders found by their $K cell: Trinity SPY, Trinity QQQ, main table', Object.keys(all));
ok(all.SPY && all.SPY.src==='trinity' && all.SPY.king===774 && all.SPY.kingSrc==='dollar' && all.SPY.kingKd===92931, '1b Trinity SPY: King 774 by $K ($92,931K)', all.SPY&&[all.SPY.king,all.SPY.kingSrc,all.SPY.kingKd]);
ok(all.SPY && all.SPY.pct['773.00']===65 && all.SPY.pct['775.00']===59 && all.SPY.vel['773.00']===-1 && all.SPY.vel['775.00']===7, '1c "-1%65%" → %King 65, velocity −1; "+7%59%" → 59 / +7', all.SPY&&[all.SPY.pct['773.00'],all.SPY.vel['773.00']]);
ok(all.SPY && all.SPY.pct['777.00']===15 && all.SPY.pct['776.00']===14 && all.SPY.pct['781.00']===-1, '1d the sign comes from the TEXT: "+2%15%"→+15, "-7%14%"→+14 (velocity −7), "−1%"→−1', all.SPY&&[all.SPY.pct['777.00'],all.SPY.pct['776.00'],all.SPY.pct['781.00']]);
ok(all.SPY && all.SPY.pct['774.00']===100 && all.SPY.count===20, '1e King row = 100, all 20 rows read', all.SPY&&all.SPY.count);
ok(all.QQQ && all.QQQ.king===717 && all.QQQ.kingKd===247657 && all.QQQ.pct['717.00']===-100, '1f Trinity QQQ: King 717 by $K, NEGATIVE King (−$247,657K, purple) reads −100', all.QQQ&&[all.QQQ.king,all.QQQ.kingKd,all.QQQ.pct['717.00']]);
ok(all.main && all.main.src==='main' && all.main.king===774 && all.main.kingSrc==='highlight' && all.main.pct['774.00']===100 && all.main.pct['769.00']===48, '1g main table (expirations view): yellow King 774, share-of-book rescaled (769→48)', all.main&&[all.main.king,all.main.kingSrc,all.main.pct['769.00']]);
ok(all.main && all.main.bookKing && all.main.bookKing.k===775 && all.main.bookKing.kd===341276 && all.main.bookKing.neg===true && all.main.bookKing.col===3, '1h main table: the $K in the 08-21 column is the BOOK King (775, −$341,276K), not crowned', all.main&&all.main.bookKing);

// ---------- 2. the symbol reader prefers the Trinity pane (true %King) and falls back to the main table
LADDER_CACHE={t:0,data:null};
const rS=readTapeFromDOM('SPY');
ok(rS && rS.king===774 && rS.kingSrc==='dollar' && rS.kingKd===92931 && rS.ladderSrc==='trinity', '2a readTapeFromDOM(SPY) = Trinity SPY pane (King by $K)', rS&&[rS.king,rS.kingSrc,rS.ladderSrc]);
const rQ=readTapeFromDOM('QQQ');
ok(rQ && rQ.king===717 && rQ.kingSrc==='dollar' && rQ.ladderSrc==='trinity', '2b readTapeFromDOM(QQQ) works for the first time — the QQQ pane is not a <table>', rQ&&[rQ.king,rQ.ladderSrc]);
// remove the Trinity sidebar → SPY falls back to the main table, QQQ unreadable
page.children[1].children.length=0; LADDER_CACHE={t:0,data:null};
const rS2=readTapeFromDOM('SPY'); const rQ2=readTapeFromDOM('QQQ');
ok(rS2 && rS2.king===774 && rS2.ladderSrc==='main' && rS2.bookKing && rS2.bookKing.k===775, '2c no Trinity sidebar → SPY read from the main table (yellow King, bookKing kept)', rS2&&[rS2.king,rS2.ladderSrc]);
ok(rQ2==null || rQ2.ladderSrc!=='trinity', '2d ...and QQQ has no ladder to read (null), never a wrong one', rQ2&&rQ2.ladderSrc);

// ---------- 3. robustness: markup changes that must NOT matter
// 3a the ladder rows are <li> inside <ul>, strike in the SECOND cell, value first
ALL=[]; const rowsLi=spyRows.map(r=>El('LI',[El('SPAN',[],r[1],{backgroundColor:r[2]||''}), El('SPAN',[],String(r[0]))]));
const ul=El('UL',rowsLi); const pane2=div([div([span('SPY'),span('$771.10')]), div([ul])]);
const page2=div([pane2]); global.document={ querySelectorAll:(sel)=>page2.querySelectorAll(sel) }; LADDER_CACHE={t:0,data:null};
const a2=readLaddersByDollar();
ok(a2.SPY && a2.SPY.king===774 && a2.SPY.pct['773.00']===65 && a2.SPY.count===20, '3a <ul>/<li> rows, strike in the 2nd cell, no table, no header: still read from the $K', a2.SPY&&[a2.SPY.king,a2.SPY.count]);
// 3b a hover $K on a non-King row in the Trinity pane → two $ cells → flagged, not silently crowned
ALL=[]; const spyHover=spyRows.map(r=>r.slice()); spyHover[15]=[769,'+3%$54,000K','rgb(46,167,127)'];
const page3=div([pane('SPY','771.10',spyHover)]); global.document={ querySelectorAll:(sel)=>page3.querySelectorAll(sel) }; LADDER_CACHE={t:0,data:null};
const a3=readLaddersByDollar();
ok(a3.SPY && a3.SPY.kingConflict===true && a3.SPY.parseSuspect==='two-dollar-cells', '3b two $K cells in one ladder → kingConflict flagged (the sync gate sees it)', a3.SPY&&[a3.SPY.kingConflict,a3.SPY.parseSuspect]);

// ---- 4. (v11.4.4) NEGATIVE-gamma rows in the Trinity pane: Skylit prints the value with a UNICODE minus
// while the velocity chip uses an ASCII hyphen, so BOTH tokens look signed. The value is still the SECOND
// token — taking the first (the velocity) put a 117% "strike" above a $K-tagged King and broke the sync.
// Fixtures are the live 2026-08-20 10:18 CT SPY pane, verbatim.
ALL=[];
const liveRows=[[770,'-12%19%','rgb(49, 101, 141)'],[769,'-13%\u221221%','rgb(68, 1, 84)'],[768,'-16%36%','rgb(36, 136, 139)'],
  [767,'-76%\u22123%','rgb(67, 52, 126)'],[766,'+15%62%','rgb(63, 185, 114)'],[765,'+2%$124,074K','rgb(253, 231, 37)'],
  [764,'+2%71%','rgb(102, 199, 90)'],[763,'+11%32%','rgb(39, 128, 140)'],[762,'0%5%',''],[761,'+1%3%',''],[760,'-2%8%',''],
  [759,'0%2%',''],[758,'-1%\u22124%',''],[757,'0%1%',''],[756,'0%0%',''],[755,'0%0%',''],[754,'0%0%',''],[753,'0%0%',''],
  [752,'0%0%',''],[751,'0%0%','']];
const page4=div([pane('SPY','766.86',liveRows)]);
global.document={ querySelectorAll:(sel)=>page4.querySelectorAll(sel) }; LADDER_CACHE={t:0,data:null};
const a4=readLaddersByDollar();
ok(a4.SPY && a4.SPY.king===765 && a4.SPY.kingKd===124074, '4a King = the $K row (765, $124,074K)', a4.SPY&&[a4.SPY.king,a4.SPY.kingKd]);
ok(a4.SPY.pct['765.00']===100, '4a2 the SPY King\u2019s own $K has no minus \u2192 +100');
ok(a4.SPY.pct['767.00']===-3 && a4.SPY.vel['767.00']===-76, '4b "-76%−3%" → %King −3 (unicode minus), velocity −76 — NOT 117-style velocity-as-value', a4.SPY&&[a4.SPY.pct['767.00'],a4.SPY.vel['767.00']]);
ok(a4.SPY.pct['769.00']===-21 && a4.SPY.vel['769.00']===-13, '4c "-13%−21%" → −21 / −13', [a4.SPY.pct['769.00'],a4.SPY.vel['769.00']]);
ok(a4.SPY.pct['766.00']===62 && a4.SPY.pct['764.00']===71 && a4.SPY.pct['770.00']===19, '4d positive rows are POSITIVE — a small +19% painted blue is not flipped negative by the palette', [a4.SPY.pct['766.00'],a4.SPY.pct['764.00'],a4.SPY.pct['770.00']]);
ok(a4.SPY.pct['758.00']===-4, '4e2 an explicit unicode minus is still negative');
ok(!a4.SPY.kingConflict, '4e no strike now exceeds the King → the parse invariant holds and the panel stays IN SYNC', a4.SPY&&a4.SPY.parseSuspect);
let mx=0, mk=null; Object.keys(a4.SPY.pct).forEach(k=>{ if(Math.abs(a4.SPY.pct[k])>mx){ mx=Math.abs(a4.SPY.pct[k]); mk=parseFloat(k); } });
ok(mk===765 && mx===100, '4f tapemax agrees with the $K tag — all three King votes line up', [mk,mx]);
// the main table keeps its own order: value first, velocity second
global.document={ querySelectorAll:(sel)=>page.querySelectorAll(sel) }; LADDER_CACHE={t:0,data:null};
const a5=readLaddersByDollar();
ok(a5.main && a5.main.pct['771.00']===-23, '4g main table "-7%-6%" style rows still read value-first (771 → −23 after rescale)', a5.main&&a5.main.pct['771.00']);
console.log('test_ladder_dollar: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
