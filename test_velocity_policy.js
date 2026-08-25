// v13.1 — SKYLIT VELOCITY CAPTURE, ROLLS, AND THE SOURCE-OF-TRUTH POLICY.
// The policy this file enforces: we STORE the vendor's numbers and we do not invent our own.
// Anything we compute that they already publish is a fork in the truth, and a fork that looks
// identical on the face is the worst kind — nobody can tell which number is being acted on.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function noc(s){ return String(s).replace(/\/\*[\s\S]*?\*\//g,'').replace(/^\s*\/\/.*$/gm,''); }
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);
  if(!m) return '';let i=src.indexOf('{',m.index),d=0,e=-1;
  for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}
  return src.slice(m.index,e+1);}
let pass=0,fail=0;
function ok(c,m,x){ if(c){pass++;} else {fail++; console.log('FAIL '+m+(x!==undefined?'  got: '+x:''));} }

// ---------- CAPTURE, NOT DERIVATION ----------
{
  const vh=ex('velHarvest');
  ok(/__reactFiber\$/.test(ex('velFiberKey')), 'the harvest reads Skylit\u2019s own fiber props');
  ok(/velFiberKey\(el\)/.test(vh), 'and every element is probed through that one accessor');
  ok(/delta1Day/.test(vh) && /delta5Min/.test(vh) && /delta15Min/.test(vh) && /delta1Hour/.test(vh),
     'and captures their published 5m / 15m / 60m / 1d verbatim');
  ok(/ts:now/.test(vh), 'every captured row is timestamped');
  // ⚠ the harvest must never COMPUTE a delta — that is the whole policy
  ok(!/d5\s*[:=]\s*[^,]*[-+]\s*prev/.test(noc(vh)) && !/computeDelta|deriveDelta/.test(noc(vh)),
     'and computes NO delta of its own');
}
// ---------- THE HARVEST OWES THE FEED NOTHING ----------
// ⚠⚠ v13.1 called velHarvest() inside `if(haveFeed){` — the network-feed branch — directly beneath a
// comment saying it must run regardless. It reads the DOM. With the market closed it never ran ONCE:
// VEL_META sat at ts:0 while Skylit's ladder was on screen. The comment made the mistake look considered.
{
  ok(/function velStart\(/.test(src), 'the harvest has its own starter');
  ok(/setInterval\(function\(\)\{ try\{ velHarvest\(\); \}catch\(e\)\{\} \}, VEL_MS\)/.test(src),
     'driven by its own timer, not by a feed tick');
  ok(/velStart\(\); \}catch\(eVS\)/.test(ex('buildPanel')), 'started when the panel is built');
  // --- INVERTED: it must NEVER be reachable only through the feed path again ---
  // ⚠ STRIP COMMENTS FIRST. The comment explaining this very removal contains both `if(haveFeed){`
  // and `velHarvest()`, so a raw slice finds the string it is checking is gone — the removal-comment
  // trap, which has now bitten this project seven times.
  const code=noc(src);
  const fi=code.indexOf('if(haveFeed){');
  const feedPath=(fi>-1)?code.slice(fi, fi+2200):'';
  ok(fi>-1 && !/velHarvest\(\)/.test(feedPath),
     'and is NOT called from inside the haveFeed branch (INVERTED v13.3)');
  ok(/try\{ velHarvest\(\); \}catch\(e2\)\{\}/.test(src),
     'plus one immediate call, so the first render is not blank for a whole interval');
}
// ---------- AN EMPTY LIST SAYS WHICH EMPTY IT IS ----------
{
  const sl=ex('secLoc');
  ok(/no expected-move anchor/.test(sl),
     'an unanchored band is reported as OUR limitation, not as "no nodes in range"');
  ok(/rate of change is unreadable/.test(sl), 'and an unreadable velocity is its own message');
  ok(/none in range/.test(sl), 'while a genuinely empty market still says so');
}
// ---------- STALENESS IS REPORTED, NEVER HIDDEN ----------
{
  const va=ex('velAt');
  ok(/age>VEL_STALE_MS/.test(va), 'a captured row past the staleness window is flagged stale');
  ok(/age:age/.test(va), 'and its actual age is returned, not just a boolean');
  ok(/VEL_STALE_MS\s*=\s*120000/.test(src), 'the window is two minutes');
  // ⚠ virtualisation: a node scrolled out of their ladder unmounts. Keeping the last value is correct;
  // presenting it as live is not.
  ok(/velStale/.test(ex('tradeNodes')), 'nodes carry the stale flag through to the face');
  ok(/aged /.test(ex('secLoc')), 'and the face prints the age rather than a silent stale number');
}
// ---------- A BROKEN HARVEST REFUSES, IT DOES NOT RENDER ZEROES ----------
{
  ok(/function velOk\(\)/.test(src), 'health is a first-class check');
  ok(/if\(!velOk\(\)\)/.test(ex('secLoc')), 'the NODES block asks before it renders');
  ok(/rate of change unavailable/.test(ex('secLoc')),
     'and says so explicitly \u2014 blanks would read as zero, which is a lie');
  ok(/Skylit may have renamed them/.test(src),
     'the fiber-coupling risk is named where it would bite');
}
// ---------- ROLLS: NOT CONSERVATION OF MASS ----------
// Measured live: receivers gained 2.8x, 8.6x, 13.1x and 16.5x what the losers shed. Several strikes
// feed one destination. An equal-and-opposite test would have found almost nothing.
{
  const rs=ex('rollScan');
  ok(/ROLL_MIN_ABS\s*=\s*40000/.test(src), 'the floor is $40K \u2014 measured, not guessed');
  ok(!/ROLL_MIN_ABS\s*=\s*500000/.test(src),
     'and NOT the $500K I first proposed, which would have missed a real 7645\u21927625 roll at \u221235K/+96K');
  ok(/7645 -> 7625/.test(src), 'the measurement that corrected it is recorded in the code');
  ok(/ROLL_MIN_RATIO/.test(rs) && /ratio<ROLL_MIN_RATIO/.test(rs),
     'the receiver must take a material share');
  ok(!/Math\.abs\(dst\.d15\s*\+\s*src\.d15\)/.test(noc(rs)),
     'but there is NO equal-and-opposite balance check (INVERTED: that assumption was wrong)');
  ok(/ROLL_MAX_DIST/.test(rs), 'and proximity is required');
  ok(/function rollBias/.test(src), 'the aggregate exists');
  ok(/migrating /.test(ex('secLoc')), 'and reaches the face as one line, not five badges');
}
// ---------- ONE BOOK: NODES AND THE RAIL ----------
{
  const tn=ex('tradeNodes');
  // ⚠⚠ (v13.1b) NOT "the same book" — THE SAME ARRAY. v13.1 read tapeMap('SPXW') with its own strength
  // floor and reach window, which is a SECOND computation of "which nodes matter". It disagreed with
  // the rail within one session: 7700 was drawn on the rail and missing from the list, and 7665 was
  // yellow on the rail and purple in the list. Two lists that are supposed to agree eventually will not.
  ok(/emPiles\(B, sym\)/.test(tn), 'the NODES list reads emPiles \u2014 the RAIL\u2019S OWN ARRAY');
  ok(!/tapeMap\(/.test(noc(tn)), 'and builds no second node list of its own (INVERTED v13.1b)');
  ok(!/NODE_MIN_PCT/.test(noc(tn)) && !/reach/.test(noc(tn)),
     'with no independent strength floor or reach window to drift out of step');
  ok(/es:\+P\.disp\.toFixed\(2\)/.test(tn), 'it carries the rail\u2019s own display price');
  ok(/cls:\(P\.balanced \? 'bal' : \(P\.accel \? 'acc' : 'brk'\)\)/.test(tn),
     'and the rail\u2019s own role classification, verbatim');
  ok(!/n\.put\?'#a371f7'/.test(src),
     'the list no longer colours by put/call polarity, which is a DIFFERENT question (INVERTED v13.1b)');
  ok(/return b2\.es-a2\.es/.test(tn),
     'sorted DESCENDING so the list order matches the rail left-to-right');
  // the three role colours must be identical in both halves of the panel
  ok(/NODE_COL=\{ acc:'#a371f7', brk:'#e3c341', bal:'#8b98a9' \}/.test(src), 'the role colours live in one map');
  ok(/g3plab\.acc\{color:#a371f7\}/.test(src) && /g3plab\.brk\{color:#e3c341\}/.test(src),
     'and the rail CSS still uses those same hexes \u2014 a colour meaning two things is worse than none');
}
// ---------- CHIPS: SIDE x DIRECTION, TURNING FIRST ----------
{
  const nc=ex('nodeChip');
  const iTurn=nc.indexOf('TURNING'), iBuild=nc.indexOf('BUILDING');
  ok(iTurn>-1 && iBuild>-1 && iTurn<iBuild,
     'TURNING is tested BEFORE building/failing \u2014 it is the early warning, not a late verdict');
  ok(/d5<0 && d15<0 && d60>0/.test(nc), 'turning down = 5m and 15m flipped while 60m has not');
  ok(/d5>0 && d15>0 && d60<0/.test(nc), 'and it works in reverse');
  ok(/RESISTANCE FAILING',\s*cls:'g3cBull'/.test(nc),
     'a dissolving ceiling is BULLISH \u2014 chip colour means what it does to PRICE');
  ok(/SUPPORT BUILDING',\s*cls:'g3cBull'/.test(nc), 'and support building is bullish too');
  ok(/never what the node is/i.test(src), 'the colour rule is written down where it can be checked');
}
// ---------- REACTION IN DOLLARS ----------
{
  const rd=ex('reactDefence');
  ok(/DEFENDING/.test(rd) && /ABANDONING/.test(rd) && /NO REACTION/.test(rd), 'three honest states');
  ok(/v\.d15>0 && v\.d5>=0/.test(rd), 'defending is measured from their 15m and 5m, not inferred');
  ok(/caveat/.test(rd) && /15 minutes old, not an hour old/.test(rd),
     'and a 15m defence against a negative hour is CAVEATED, never presented as settled');
  // ⚠ the string existing is NOT the same as the caveat being DELIVERED. A mutation that returned
  // early with caveat:null left every string in place and this suite passed it. Assert the path.
  ok(!/return \{[^}]*caveat:null/.test(rd),
     'and no path returns a null caveat while the hour still disagrees');
  ok(/caveat:caveat/.test(rd), 'the computed caveat is the one returned');
  ok(/RD\.caveat/.test(ex('secReact')), 'and the face actually renders it');
  ok(/bd>3/.test(rd), 'it refuses when no node is close enough to be the thing being tested');
}
// ---------- THE VERDICT IS A READING, NOT A SECOND MODEL ----------
{
  const nv=ex('nodesVerdict');
  ok(/Same inputs, or no verdict/.test(src),
     'the verdict is built only from the rows above it');
  ok(/velOk\(\)/.test(nv), 'and it refuses outright when the vendor numbers are missing');
  ok(/roll bias is down/.test(nv) && /roll bias is up/.test(nv),
     'and a bias that contradicts the local read is STATED, not hidden');
}
// ---------- THE RECORDER STORES VENDOR RAW, NOT ONLY OUR CONCLUSIONS ----------
// This was the audit finding. Storing `st:'Building'` without the dollars behind it means a flawed
// rule poisons every recorded day and the nightly can never re-derive a better answer.
{
  const rec=ex('recordNodeSnapshot');
  ok(/vend:\(function\(\)/.test(rec), 'the snapshot carries a vendor block');
  ok(/src:'skylit'/.test(rec), 'tagged as Skylit\u2019s own');
  ok(/f:\['k','cur','d5','d15','d60','d1d'\]/.test(rec), 'with its field names, so the nightly need not guess');
  ok(/srcs:\{ vend:'skylit', book:'skylit', nodes:'derived'/.test(rec),
     'and a provenance map marking OUR fields as derived');
  ok(/sig:'derived'/.test(rec) && /deriv:'derived'/.test(rec),
     'including the signal vector, which is entirely ours');
  ok(/training on our own opinions/.test(rec),
     'the reason this exists is recorded, so it is not optimised away later');
}
// ---------- HIDDEN, NOT REMOVED ----------
{
  ok(/var LOC_SHOW_CHART=false;/.test(src), 'the chart is hidden behind a flag');
  ok(/function nodeChartHtml/.test(src), 'but the renderer still exists');
  ok(/if\(LOC_SHOW_CHART\)\{ try\{ h\+=nodeChartHtml\(sym\)/.test(src), 'and is one flag from coming back');
  // ⚠ the sampling behind it must NOT have been switched off with the picture
  ok(/sampleTapeHistory\(sym\);/.test(src), 'node-history sampling still runs \u2014 hiding is presentation only');
}
console.log('\n'+pass+' pass / '+fail+' fail');
