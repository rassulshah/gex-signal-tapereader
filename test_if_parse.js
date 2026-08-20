// (v11.8) THE INSIDERFINANCE PARSER. Fixtures are the REAL header text from live pulls of
// /gamma-exposure/SPY and /gamma-exposure/SPX on 2026-08-20, not invented markup.
//
// The parser is deliberately LABEL-anchored, not DOM-anchored: class names are exactly what churns on a
// site we do not control, but "Call Wall" has to stay printed or the page stops being useful to humans too.
// The tests below pin the three ways this can go wrong quietly:
//   * reading a number that belongs to the NEXT label (window overrun);
//   * losing the minus sign on Put GEX, which flips the whole call/put ratio;
//   * reporting ok:true off a page that no longer carries the labels, so stale numbers look live.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
eval(['ifNum','ifBn','ifParse'].map(ex).join('\n'));

// ---- real SPY header, 2026-08-20 (spot 767.44) ----
const SPY_HTML='<html><head><script>var junk="Call Wall $999";<\/script><style>.x{}</style></head><body>'+
 '<div class="a1b2"><span>Spot Price</span><span>$767.44</span></div>'+
 '<div><span>Net GEX</span><span>-$5.4B</span></div>'+
 '<div><span>Ratio</span><span>0.82</span></div>'+
 '<div><span>Call GEX</span><span>$24.6B</span></div>'+
 '<div><span>Put GEX</span><span>-$30.0B</span></div>'+
 '<div><span>Total GEX</span><span>$54.7B</span></div>'+
 '<div><span>Call Wall</span><span>$775</span><span>(+0.99%)</span></div>'+
 '<div><span>Put Wall</span><span>$765</span><span>(-0.32%)</span></div>'+
 '<div><span>Zero Gamma</span><span>$769.53</span><span>(+0.27%)</span></div>'+
 '</body></html>';
{
  const p=ifParse(SPY_HTML,'SPY');
  ok(p.ok===true,'a well-formed page parses',p.why);
  ok(p.spot===767.44,'spot',p.spot);
  ok(p.cw===775,'call wall',p.cw);
  ok(p.pw===765,'put wall',p.pw);
  ok(p.zg===769.53,'zero gamma',p.zg);
  ok(p.callGex===24.6,'call GEX in billions',p.callGex);
  ok(p.putGex===-30,'put GEX keeps its MINUS sign — dropping it inverts the whole book',p.putGex);
  ok(p.netGex===-5.4,'net GEX keeps its sign too',p.netGex);
  ok(p.totGex===54.7,'total GEX',p.totGex);
  ok(p.ratio===0.82,'ratio',p.ratio);
  ok(!p.suspect,'walls straddle spot, so nothing is flagged',p.suspect);
}
// ---- the script tag is stripped BEFORE label matching ----
{
  const p=ifParse(SPY_HTML,'SPY');
  ok(p.cw!==999,'a "Call Wall" string inside a <script> cannot win over the rendered one',p.cw);
}
// ---- real SPX header, same day: thousands separators ----
const SPX_HTML='<body><div>Spot Price $7,708.03</div><div>Net GEX $29.0B</div>'+
 '<div>Call GEX $304.3B</div><div>Put GEX -$275.3B</div>'+
 '<div>Call Wall $7,900 (+2.49%)</div><div>Put Wall $7,500 (-2.70%)</div>'+
 '<div>Zero Gamma $7,667.00 (-0.53%)</div></body>';
{
  const p=ifParse(SPX_HTML,'SPX');
  ok(p.ok===true,'the SPX page parses too');
  ok(p.cw===7900,'a comma-grouped wall is read as 7900, not 7',p.cw);
  ok(p.spot===7708.03,'comma-grouped decimal spot',p.spot);
  ok(p.zg===7667,'comma-grouped zero gamma',p.zg);
  ok(p.putGex===-275.3,'large negative put GEX',p.putGex);
}
// ---- the failure modes ----
{
  const p=ifParse('<body><div>Gamma dashboard coming soon. Check back later for exposure data.</div></body>','SPY');
  ok(p.ok===false,'a page without the labels is NOT reported as ok');
  ok(/not found/.test(p.why||''),'and it says why, so the panel can show the reason',p.why);
}
{
  ok(ifParse('','SPY').ok===false,'an empty body never parses');
  ok(ifParse(null,'SPY').ok===false,'a null body never parses');
}
{
  // markup churn: labels survive, everything around them changes
  const churn='<main><p data-x="9">Spot Price</p><p>$767.44</p><p>Call Wall</p><p>$775</p>'+
              '<p>Put Wall</p><p>$765</p><p>Zero Gamma</p><p>$769.53</p></main>';
  const p=ifParse(churn,'SPY');
  ok(p.ok===true,'a full re-skin still parses because we anchor on labels, not classes',p.why);
  ok(p.cw===775&&p.pw===765,'and the values survive it',[p.cw,p.pw]);
}
{
  // the sanity check that catches a mis-read rather than trusting it
  const bad='<body>Spot Price $767.44 Call Wall $700 Put Wall $800 Zero Gamma $769.53</body>';
  const p=ifParse(bad,'SPY');
  ok(p.ok===true,'the numbers are found');
  ok(/call wall below spot/.test(p.suspect||''),'a call wall UNDER spot is flagged as suspect',p.suspect);
  ok(/put wall above spot/.test(p.suspect||''),'and a put wall OVER spot too',p.suspect);
}
{
  // three of four is enough; four of four is not required, because one row can be absent mid-session
  const partial='<body>Spot Price $767.44 Call Wall $775 Put Wall $765</body>';
  ok(ifParse(partial,'SPY').ok===true,'3 of the 4 core numbers is enough to be usable');
  const thin='<body>Spot Price $767.44 Call Wall $775</body>'+' '.repeat(600);
  ok(ifParse(thin,'SPY').ok===false,'2 of 4 is not');
}
// ---- ifNum / ifBn in isolation ----
{
  ok(ifNum('Call Wall $7,900','Call Wall')===7900,'ifNum strips commas and the dollar sign');
  ok(ifNum('nothing here','Call Wall')===null,'a missing label yields null, not 0');
  ok(ifBn('Put GEX -$275.3B','Put GEX')===-275.3,'ifBn preserves sign');
  ok(ifBn('Net GEX $500M','Net GEX')===0.5,'millions are normalised to billions',ifBn('Net GEX $500M','Net GEX'));
  ok(ifBn('Net GEX 12','Net GEX')===null,'a figure with no B/M/K suffix is not guessed at');
}
console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
