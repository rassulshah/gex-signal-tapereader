// (v11.1.3) TAPE READER vs the 2026-08-19 ladder: EXPIRATIONS view, share-of-book percentages, no permanent
// $K King cell (yellow highlight instead; $ only under the mouse), multi-minute feed payload, SPXW-derived lanes.
// Locks out the three faults that kept the sync gate red all morning on 2026-08-19.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
function v(n){ return src.match(new RegExp('var '+n+'\\s*=.*'))[0]; }
eval([v('KING_DOLLAR_RE'),v('TAPE_REJECT_RE'),v('ISO_DATE_RE'),v('TAPE_TOK_RE'),v('TAPE_KING_DOLLAR_IN')].join('\n'));
eval(['tapeStrikeRowCount','findTapeTable','leadTok','tapeCells','readTapeFromDOM','kingResolve','tapeCellPct','leadSignedPct','parseKingDollarsK','kingFromFeed','feedStructMap'].map(ex).join('\n'));

// ---- a tiny DOM: one <table> shaped like the 2026-08-19 ladder (Strike | 5 expiry columns)
function cell(t, bg){ return { tagName:'TD', textContent:t, style:{backgroundColor:bg||''}, children:[], querySelectorAll:()=>[] }; }
function row(cells){ return { tagName:'TR', children:cells, textContent:cells.map(c=>c.textContent).join(' '), querySelectorAll:(s)=>s==='tr'?[]:[] }; }
const HDR='Strike 2026-08-19 2026-08-20 2026-08-21 2026-08-24 2026-08-25';
const rowsSpec=[ // strike, col1 text, bg, other cols
  [780,'1%','',['1%','17%','0%','-1%']],[779,'-1%','',['1%','-2%','0%','0%']],[778,'0%','',['0%','-2%','0%','0%']],[777,'4%+23%','',['2%','1%','0%','0%']],
  [776,'3%','',['0%','2%','0%','0%']],[775,'6%+23%','',['-4%','-2%','-1%','-2%']],[774,'31%-7%','rgb(253, 231, 37)',['2%','-3%','0%','0%']],
  [773,'10%+2%','',['-1%','+32%','-3%','0%']],[772,'1%','rgb(110, 202, 85)',['6%','-3%','21%','-1%']],[771,'-7%-6%','',['0%','-1%','-1%','-1%']],
  [770,'12%-4%','',['1%','5%','1%','0%']],[769,'15%-3%','',['7%','3%','0%','-1%']],[768,'12%+7%','',['2%','-4%','-2%','0%']],[767,'5%-22%','',['3%','1%','0%','0%']],
  [766,'5%-5%','',['0%','-11%','-1%','0%']],[765,'5%','',['0%','0%','0%','0%']],[764,'5%','',['0%','0%','0%','0%']],[763,'2%','',['0%','0%','0%','0%']],[762,'1%','',['0%','0%','0%','0%']],[761,'0%','',['0%','0%','0%','0%']]];
function makeTable(spec){
  const trs=spec.map(r=>row([cell(r[0].toFixed(1)), cell(r[1], r[2]), ...r[3].map(t=>cell(t))]));
  const thead={ textContent:HDR };
  const tb={ tagName:'TABLE', textContent:HDR+' '+trs.map(r=>r.textContent).join(' '), children:[], querySelector:(s)=>s==='thead'?thead:null, querySelectorAll:(s)=>s==='tr'?trs:[] };
  return tb;
}
let TABLE=makeTable(rowsSpec);
global.document={ querySelectorAll:(s)=> s==='table'?[TABLE]:[] };
global.LASTDISP={SPY:'gamma'};

// ---- 1. the ladder without any $K cell is found and read; King = yellow cell; scale = value/King
var r=readTapeFromDOM('SPY');
ok(!!r, '1a the expirations ladder is found without a $K cell');
ok(r && r.king===774 && r.kingSrc==='highlight', '1b King = the yellow-highlighted cell (774), kingSrc highlight', r&&[r.king,r.kingSrc]);
ok(r && r.pct['774.00']===100, '1c King reads 100 after rescale (was 31 = share of book)', r&&r.pct['774.00']);
ok(r && r.pct['769.00']===48 && r.pct['771.00']===-23, '1d others rescaled relative to the King, sign kept (769→48, 771→−23)', r&&[r.pct['769.00'],r.pct['771.00']]);
ok(r && !r.kingConflict, '1e no parse-invariant flag', r&&r.parseSuspect);
ok(r && r.count===20, '1f all 20 rows read from column 1 only (never a later expiry column)', r&&r.count);

// ---- 2. a HOVER $K read-out on another row must not crown that row
const spec2=rowsSpec.map(r=>r.slice()); spec2[8]=[772,'$92,114K','rgb(110, 202, 85)',['6%','-3%','21%','-1%']];   // mouse over 772
TABLE=makeTable(spec2);
var r2=readTapeFromDOM('SPY');
ok(r2 && r2.king===774 && r2.kingSrc==='highlight', '2a hovered $K cell on 772 is ignored; King stays the yellow 774', r2&&[r2.king,r2.kingSrc]);
ok(r2 && r2.pct['772.00']===undefined && r2.count===19, '2b the hovered strike is unknown for this tick, not 100', r2&&[r2.pct['772.00'],r2.count]);

// ---- 3. the legacy layout (permanent $K on the King row) still works exactly as before
const spec3=rowsSpec.map(r=>r.slice()); spec3[6]=[774,'$113,000K','',['2%','-3%','0%','0%']]; spec3.forEach((r,i)=>{ if(i!==6){ r[1]=r[1].replace(/^(-?)(\d+)%/,(m,s,n)=>s+Math.round(parseInt(n,10)*100/31)+'%'); } });
TABLE=makeTable(spec3);
var r3=readTapeFromDOM('SPY');
ok(r3 && r3.king===774 && r3.kingSrc==='dollar' && r3.pct['774.00']===100 && !r3.kingConflict, '3a legacy $K King row: unchanged behaviour', r3&&[r3.king,r3.kingSrc,r3.parseSuspect]);

// ---- 4. kingFromFeed must use the LATEST minute of the session payload, not the open
global.LASTFEED={ SPY:{ j:{ levels:[ {t:100, s:770, l:[{k:772,v:48e6},{k:774,v:38e6}]}, {t:160, s:771, l:[{k:772,v:45e6},{k:774,v:40e6}]}, {t:220, s:772, l:[{k:774,v:113e6},{k:769,v:56e6}]} ] } } };
ok(kingFromFeed('SPY')===774, '4a feed vote = King of the latest level (774), not levels[0] (772)', kingFromFeed('SPY'));
global.LASTFEED={ SPY:{ j:{ levels:[ {t:220, s:772, l:[{k:774,v:113e6}]}, {t:100, s:770, l:[{k:772,v:48e6}]} ] } } };
ok(kingFromFeed('SPY')===774, '4b ...by max t, whatever the array order', kingFromFeed('SPY'));

// ---- 5. feedStructMap (the tape stand-in) excludes SPXW-derived lanes
global.extractWalls=function(){ return { king:774, walls:[ {k:774,pct:100,pos:true}, {k:769,pct:50,pos:true}, {k:770.5,pct:100,pos:true,src:'SPXW'}, {k:771,pct:30,pos:false} ] }; };
global.LASTFEED={ SPY:{ j:{ levels:[{t:1,l:[{k:774,v:1}]}] } } };
var fm=feedStructMap('SPY');
ok(fm && fm.pct['770.50']===undefined && fm.count===3, '5a derived 770.5 lane (normalised to its own King) is not in the tape map', fm&&fm.pct);
ok(fm && fm.pct['774.00']===100 && fm.pct['771.00']===-30, '5b native strikes intact, sign kept');

console.log('test_tape_v1113: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
