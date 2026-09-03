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
ok(/@version\s+1\.18/.test(src),             'companion pinned to 1.18');

console.log((fail?'FAIL ':'')+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);

// ---------- (v1.11) THE SCRIPT MUST BE UPDATABLE ----------
// Without @updateURL/@downloadURL Tampermonkey never offers an update for a script — not on a schedule,
// not on a page reload, not ever. The tapereader has carried them for releases; the companion did not,
// so it sat silently at v1.9 while the repo moved to v1.10 and the truncated Zero Gamma stayed on the
// face through a reload. The repo was right, the browser was wrong, and NOTHING on either side said so.
{
  const hdr = src.slice(0, src.indexOf('==/UserScript=='));
  ok(/@updateURL\s+https:\/\/raw\.githubusercontent\.com\S+gex-if-levels\.user\.js/.test(hdr),
     'the companion declares @updateURL, so Tampermonkey can see a new version');
  ok(/@downloadURL\s+https:\/\/raw\.githubusercontent\.com\S+gex-if-levels\.user\.js/.test(hdr),
     'and @downloadURL, so it can fetch one');
  ok(/@grant\s+GM_xmlhttpRequest/.test(hdr), 'it still holds the grant the tapereader must not have');
  ok(/@connect\s+insiderfinance\.io/.test(hdr), 'and still declares the host it reaches');
}
console.log('  (update-header block: +4)');

// ---------- (v1.12) THE PER-STRIKE GAMMA PROFILE ----------
// Added because the Tapereader was drawing its gamma piles from SKYLIT's book while every other number on
// that band came from HERE. Measured on the same nominal window the two differ ~113x (Skylit gross $0.58B
// vs this book's $65.8B) and carry OPPOSITE sign conventions. Two books on one rail cannot compose.
{
  eval(src.match(/var SIDE_MIN\s*=\s*[^;]+;/)[0]);
  eval(ex('levelsFor'));
  const spot=100, gg=(gam,oi)=>gam*oi*100*spot*spot*0.01;
  const R=levelsFor([{strike:100,cp:'C',gamma:0.05,openInterest:1000},
                     {strike:105,cp:'C',gamma:0.02,openInterest:500},
                     {strike:100,cp:'P',gamma:0.04,openInterest:2000},
                     {strike:95, cp:'P',gamma:0.03,openInterest:800}], spot, ()=>true);
  ok(Array.isArray(R.gexProf) && R.gexProf.length===3, 'a profile row per strike carrying weight', R.gexProf);
  const sc=R.gexProf.reduce((a,r)=>a+r[1],0), sp=R.gexProf.reduce((a,r)=>a+r[2],0);
  ok(Math.abs(sc-R.callGEX/1e6)<0.05, 'the CALL legs sum EXACTLY to the book callGEX', [sc,R.callGEX/1e6]);
  ok(Math.abs(sp-R.putGEX/1e6)<0.05,  'the PUT legs sum EXACTLY to the book putGEX',  [sp,R.putGEX/1e6]);
  ok(R.gexProf.every(r=>r[2]<=0),     'puts are NEGATIVE — their convention, verified against their page');
  ok(Math.abs(R.gexProf.find(r=>r[0]===100)[1]-gg(.05,1000)/1e6)<0.05,
     'one strike equals gamma x OI x 100 x spot^2 x 0.01');
  // the tail trim must not break the sum
  const many=[]; for(let k=50;k<150;k++) many.push({strike:k,cp:'C',gamma:(k===100?0.05:0.000001),openInterest:100});
  const R2=levelsFor(many, spot, ()=>true);
  ok(R2.gexProf.length < many.length, 'the near-zero tail is trimmed rather than shipped', R2.gexProf.length);
  ok(Math.abs(R2.gexProf.reduce((a,r)=>a+r[1],0) - R2.callGEX/1e6) < 0.05,
     '...and what survives still sums to the book, so the piles never lie about the whole');
}
console.log('  (gexProf block: +7)');
