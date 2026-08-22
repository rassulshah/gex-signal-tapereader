// (v1.9) THEIR PUBLISHED HEADER VALUES.
//
// The mistake this closes, stated plainly: `pick()` walked initialData, every published metric came back
// null, and the conclusion drawn was "their page computes client-side, so we must compute too." That is
// failure pattern #4 — concluding ABSENT from a shallow look — and it drove real decisions, including
// computing our own zero gamma and calling TERM "structurally dead".
//
// The payload is NOT the page. Verified 2026-08-22 by fetching the raw HTML with NO JavaScript running:
//   Zero Gamma 7646.90 · Call Wall 7900 · Put Wall 7500 · ATM IV 6.2 · Put/Call 1.36 · Term Slope 1.3
//
// ⚠ Their header walls are ALL-EXPIRY. Our ladder's CR0/PS0 are 0DTE. Both are "their values" and they
// answer different questions — substituting one for the other is failure pattern #1.
const fs=require('fs');
const src=fs.readFileSync('./current/gex-if-levels.user.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);
  if(!m) throw new Error('not found: '+n);
  let i=src.indexOf('{',m.index),d=0,e=-1;
  for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}
  return src.slice(m.index,e+1);}

eval(ex('hdrText')); eval(ex('hdrNum'));

// A page shaped like theirs: values live in the MARKUP, not in __NEXT_DATA__.
// ⚠ THE FIXTURE MUST MATCH THE PAGE. v1.9 shipped a truncation bug — Zero Gamma reached the face as 764
// beside a spot of 7674 — and THIS TEST PASSED, because its fixture wrote "$7,646.90" WITH a comma while
// their page renders "$7646.90" WITHOUT one. A fixture invented to suit the parser tests the parser
// against itself. Both renderings are now asserted, and the no-comma form is the one they actually use.
const HTML = '<html><head><script id="__NEXT_DATA__" type="application/json">'+
  JSON.stringify({props:{pageProps:{initialData:{ticker:'SPX',spot:7674.1,options:[{strike:7675}]}}}})+
  '</script></head><body>'+
  '<div>Spot Price</div><div>$7674.10</div>'+
  '<div>ATM IV</div><div>6.2%</div>'+
  '<div>Put/Call Ratio</div><div>1.36</div>'+
  '<div>Call Wall</div><div>$7900</div>'+
  '<div>Put Wall</div><div>$7500</div>'+
  '<div>Zero Gamma Level</div><div>$7646.90</div>'+
  '<div>Max Pain</div><div>$7350.00</div>'+
  '<div>Skew Slope</div><div>-0.8 IV pts</div>'+
  '<div>Term Slope</div><div>+1.3 pts</div>'+
  '</body></html>';

const T = hdrText(HTML);
ok(!/__NEXT_DATA__/.test(T) && !/pageProps/.test(T), 'the script block is stripped, so JSON cannot be mistaken for header text');

ok(hdrNum(T,'Zero Gamma')===7646.90, 'Zero Gamma reads 7646.90 — the value we were COMPUTING instead', hdrNum(T,'Zero Gamma'));
ok(hdrNum(T,'Call Wall')===7900,     'Call Wall 7900 (ALL-EXPIRY, not our 0DTE CR0)', hdrNum(T,'Call Wall'));
ok(hdrNum(T,'Put Wall')===7500,      'Put Wall 7500 (ALL-EXPIRY)', hdrNum(T,'Put Wall'));
ok(hdrNum(T,'Spot Price')===7674.10, 'a NO-COMMA dollar value parses whole — the v1.9 truncation bug', hdrNum(T,'Spot Price'));

// the exact regression, both ways round
const COMMA = hdrText('<div>Zero Gamma</div><div>$7,646.90</div>');
ok(hdrNum(COMMA,'Zero Gamma')===7646.90, 'the SAME value WITH commas also parses whole', hdrNum(COMMA,'Zero Gamma'));
ok(hdrNum(hdrText('<div>Call Wall</div><div>7900</div>'),'Call Wall')===7900,
   'a bare 4-digit integer is not truncated to 3 digits', hdrNum(hdrText('<div>Call Wall</div><div>7900</div>'),'Call Wall'));
ok(hdrNum(hdrText('<div>X</div><div>12345678</div>'),'X')===12345678, 'and long integers survive intact');
ok(hdrNum(T,'ATM IV')===6.2,         'ATM IV 6.2', hdrNum(T,'ATM IV'));
ok(hdrNum(T,'Put/Call')===1.36,      'Put/Call 1.36', hdrNum(T,'Put/Call'));
ok(hdrNum(T,'Max Pain')===7350,      'Max Pain 7350', hdrNum(T,'Max Pain'));
ok(hdrNum(T,'Term Slope')===1.3,     'Term Slope +1.3 — the cell we called "structurally dead"', hdrNum(T,'Term Slope'));
ok(hdrNum(T,'Skew Slope')===-0.8,    'a NEGATIVE value keeps its sign', hdrNum(T,'Skew Slope'));

// null-safety: a moved label must yield null, never a wrong number and never a throw
ok(hdrNum(T,'Gamma Flip Point')===null, 'a label that is not there yields null rather than a nearby number');
ok(hdrNum('','Zero Gamma')===null,      'empty text yields null');
ok(hdrNum(hdrText('<div>Zero Gamma</div><div>n/a</div>'),'Zero Gamma')===null,
   'a label present with NO number yields null — a dash is not a value');

// the wiring
ok(/pubSrc/.test(src),                       'the source of every published value is recorded');
ok(/'payload'/.test(src) && /'tree'/.test(src) && /'header'/.test(src),
   'three sources in order: payload field, tree walk, then rendered header');
ok(/wallsAreAllExpiry/.test(src),            'and the all-expiry caveat travels WITH the walls');

// ---- THE SANITY GATE: a price level must look like a price ----
// The regex was the proximate cause; the real defect was that 764 sat beside a spot of 7674 and nothing
// objected. A parser can fail in ways nobody predicted — check the value against the fact we always have.
ok(/function levelSane/.test(src),           'levels are sanity-checked against spot');
ok(/REJECTED:/.test(src),                    'and a rejected value is RECORDED, not silently dropped');
{
  const g=src.slice(src.indexOf('function levelSane'), src.indexOf('function levelSane')+400);
  ok(/spot\*0\.5/.test(g) && /spot\*2/.test(g), 'the gate is a band around spot', g.slice(0,160));
  ok(/isLevel/.test(src),                    'ratios, IV and slopes are NOT gated — they are not prices');
}
ok(/@version\s+1\.10/.test(src),             'companion pinned to 1.10');

console.log((fail?'FAIL ':'')+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
