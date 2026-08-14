// (v10.38) TAPE <-> TAPEREADER SYNC — regression lock.
//
// THE BUG THIS LOCKS OUT (observed live, SPY, 2026-08-14 09:30 CT):
//   The Skylit tape rendered the King row as "$1,252,620K" (Skylit prints the
//   dollar figure instead of "100%", because King == largest ABSOLUTE exposure
//   == 100% by definition). The old parser called firstStrengthPct() on that
//   cell, which accepts only UNSIGNED percentages, got null, and then FELL BACK
//   TO cells[2] -- a DIFFERENT EXPIRATION COLUMN -- assigning the King a bogus
//   3-4%. kingResolve() then saw 775 at 33-45% beating the "4%" King and
//   invoked 'maxpct-override', crowning the WRONG strike. Every downstream read
//   (node roles, %King normalisation, target ladder, gatekeeper geometry,
//   regime, READ narrative) inherited a wrong structural anchor, and it was
//   being RECORDED that way.
//
//   Second defect in the same function: %King is SIGNED (it carries gamma
//   polarity). firstStrengthPct() rejected every signed value as a "change
//   chip", so EVERY NEGATIVE-GAMMA STRIKE was silently dropped from the map.
//
// FIXTURES BELOW ARE REAL — captured verbatim from the live Skylit DOM.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
function ok(c,m){ if(c){pass++; console.log('PASS '+m);} else {fail++; console.log('FAIL '+m);} }
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);
  let i=src.indexOf('{',m.index),d=0,e=-1;
  for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}
  return src.slice(m.index,e+1);}

var TAPE_KING_DOLLAR_IN = /\$[\d,]+K/;
eval([ 'tapeCellPct', 'kingResolve' ].map(ex).join('\n'));

// ---------------------------------------------------------------- SYNC GUARDS
// These fail loudly if anyone reintroduces the defect.
ok(src.indexOf('firstStrengthPct(cells[2]') === -1,
   'GUARD: no cross-expiry fallback to cells[2] anywhere in the source');
ok(!/out\.kingSrc\s*=\s*'maxpct-override'/.test(src),
   'GUARD: maxpct-override branch is gone (the $K tag may never be demoted)');
ok(/function tapeCellPct/.test(src),
   'GUARD: tapeCellPct exists');
ok(/var TAPE_KING_DOLLAR_IN/.test(src),
   'GUARD: TAPE_KING_DOLLAR_IN constant defined');

// ------------------------------------------------- tapeCellPct: REAL fixtures
// Captured from live Skylit DOM 2026-08-14. Cell shape VARIES: the growth chip
// is only rendered sometimes, so the parser must tolerate both forms.
ok(tapeCellPct('45%')            === 45,  'plain %King "45%" -> 45');
ok(tapeCellPct('10%')            === 10,  'plain %King "10%" -> 10');
ok(tapeCellPct('1%')             === 1,   'plain %King "1%" -> 1');
ok(tapeCellPct('0%')             === 0,   'zero "0%" -> 0 (not null)');
ok(tapeCellPct('37% -1%')        === 37,  'FIRST pct is %King, growth chip ignored');
ok(tapeCellPct('8% +12%')        === 8,   'positive growth chip ignored');
ok(tapeCellPct('3% -1%')         === 3,   'negative growth chip ignored');

// R2 — negative %King == negative-gamma node. OLD PARSER DROPPED ALL OF THESE.
ok(tapeCellPct('-1%')            === -1,  'NEGATIVE %King "-1%" -> -1 (was dropped)');
ok(tapeCellPct('-2%')            === -2,  'NEGATIVE %King "-2%" -> -2 (was dropped)');
ok(tapeCellPct('-2% +1%')        === -2,  'negative %King with growth chip -> -2');
ok(tapeCellPct('−5%')       === -5,  'unicode minus sign handled');

// R3 — King cell prints DOLLARS, not "100%".
ok(tapeCellPct('$1,252,620K')      === 100, 'King cell "$1,252,620K" -> 100');
ok(tapeCellPct('$1,397,016K +4%')  === 100, 'King cell WITH growth chip -> 100');
ok(tapeCellPct('$849,000K')        === 100, 'any $K cell -> 100');

// degenerate
ok(tapeCellPct('')    === null, 'empty cell -> null');
ok(tapeCellPct(null)  === null, 'null cell -> null');
ok(tapeCellPct('abc') === null, 'non-numeric cell -> null');

// ------------------------------------------------------- kingResolve: THE BUG
// Exact live board, SPY 2026-08-14. 780 carries the $K tag; 775 reads 45%.
// The OLD code crowned 775. The tag MUST win.
var live = {
  '784.00':1, '780.00':100, '779.00':6, '778.00':10, '777.00':5,
  '776.00':9, '775.00':45, '774.00':-1, '773.00':3, '772.00':3, '771.00':1
};
var r = kingResolve(live, 780, Object.keys(live).length);
ok(r.king === 780,               'REGRESSION: $K-tagged 780 is King, NOT the 45% strike');
ok(r.kingSrc === 'dollar',       'kingSrc is "dollar" (tag authoritative)');
ok(r.kingConflict === false,     'no conflict flagged when parse is healthy');
ok(r.kingTagged === 780,         'kingTagged always recorded for diagnostics');
ok(live['777.00'] === 5,         'price-marker row 777 present in map (was missing)');
ok(live['774.00'] === -1,        'negative-gamma strike 774 retained with its sign');

// INVARIANT 1 — a tagged King that did not parse to 100 means the parse broke.
var broken = { '780.00':3, '775.00':33 };
var rb = kingResolve(broken, 780, 2);
ok(rb.king === 780,                          'INVARIANT: tag still wins even when parse looks wrong');
ok(rb.kingConflict === true,                 'INVARIANT: conflict flagged when King != 100');
ok(rb.parseSuspect === 'king-not-100',       'INVARIANT: parseSuspect names the failure');
ok(rb.taggedPct === 3,                       'INVARIANT: the bad parsed value is retained for diagnosis');

// INVARIANT 2 — nothing may meet or exceed the King.
var rival = { '780.00':100, '775.00':100 };
var rr = kingResolve(rival, 780, 2);
ok(rr.king === 780,                                'INVARIANT: tag wins over a rival at 100');
ok(rr.kingConflict === true,                       'INVARIANT: rival at/above King flagged');
ok(rr.parseSuspect === 'rival-at-or-above-king',   'INVARIANT: rival failure named');
ok(rr.kingRival === 775,                           'INVARIANT: rival strike identified');

// no tag at all -> data fallback is still allowed
var nt = kingResolve({ '775.00':45, '780.00':10 }, null, 2);
ok(nt.king === 775 && nt.kingSrc === 'maxpct', 'no $K tag -> maxpct fallback still works');

// absolute value, not signed value, picks the max
var neg = kingResolve({ '770.00':-90, '780.00':100 }, 780, 2);
ok(neg.king === 780, 'ABSOLUTE VALUE RULE: sign never decides the King');

// --------------------------------------------- v10.38: the flag must be SURFACED
// The 2026-08-14 desync ran live for hours with kingConflict===true because the
// flag was written and never read. A silent invariant is not protection.
ok(/tpH\.kingConflict/.test(src),
   'SURFACED: feedStatusHtml reads kingConflict (flag is no longer dead)');
ok(/⚠ tape/.test(src),
   'SURFACED: a visible "⚠ tape" warning is rendered on parse failure');
ok(/parseSuspect==='king-not-100'/.test(src),
   'SURFACED: warning distinguishes which invariant failed');

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
