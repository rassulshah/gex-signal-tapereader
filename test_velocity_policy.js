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
  // (v14.55) the same timer also latches the close-of-session book — it needs the SAME clock,
  // because the latch must be written from the last healthy LIVE harvest, not from a feed tick.
  ok(/setInterval\(function\(\)\{ try\{ velHarvest\(\); \}catch\(e\)\{\}[\s\S]{0,120}\}, VEL_MS\)/.test(src),
     'driven by its own timer, not by a feed tick');
  ok(/lastBookSave\(\); \}catch\(e2\)\{\} \}, VEL_MS\)/.test(src),
     '...and the close-of-session latch rides that same harvest clock');
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
  // (v14.4/6) the bias line moved to the GAMMA PROFILE header, operator-directed — one home
  ok(/ROLL BIAS /.test(ex('gammaProfileHtml')), 'and reaches the face as one chip on the profile header');
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


// ---------- (v13.6) UNITS, COMPLETENESS, AND THE THINGS THAT MUST MATCH ----------
// ⚠⚠ THE CLASSIFIER SHIPPED WITH SPY-DOLLAR THRESHOLDS APPLIED TO ES-POINT PRICES.
// Measured live: BREAK 81 / PIN 13 — an 85% break rate against 3-13% offline. ES moves in quarter
// points across twenty-point ranges, so TOL 0.50 is nothing and everything "broke".
{
  ok(/function nevScaleOpts/.test(src), 'outcome thresholds are scaled to the instrument');
  ok(/TOL:0\.50\*rr, THRU:0\.40\*rr, AWAY:0\.30\*rr, PIN:0\.35\*rr/.test(src),
     'by the display ratio — which is exactly doctrine’s ±5 SPX vs ±0.50 SPY');
  ok(/nevClassify\(lvl, side, bars\.slice\(start, start\+H\[0\]\), nevScaleOpts\(\)\)/.test(src),
     'and the backfill passes them (INVERTED: v13.4 used the SPY defaults on ES prices)');
  ok(/BREAK 81 \/ PIN 13/.test(src), 'the measurement that caught it is recorded');
}
// ---------- the list shows EVERY node the rail draws ----------
{
  const sl=ex('secLoc');
  ok(!/TN\.slice\(0,\s*\d+\)/.test(noc(sl)),
     'no cap on the node list — v13.5 sliced to 6 while the rail drew 7 (INVERTED v13.6)');
  // (v13.9) nodes enter ONE display ladder (with the ES row) and every ladder row renders
  ok(/TN\.forEach\(function\(n0\)\{/.test(sl) && /dispRows\.forEach\(function\(row, rowIdx\)\{/.test(sl),
     'every node enters the display ladder and every ladder row is rendered');
  ok(/if the list ever needs limiting, the RAIL must limit too/.test(sl),
     'and the reason a cap here is dangerous is written down');
}
// ---------- units on the face ----------
{
  ok(/function esTick/.test(src) && /Math\.round\(x\*4\)\/4/.test(ex('esTick')),
     'ES prices are rounded to the quarter point the contract actually moves in');
  ok(/function velP/.test(src), 'percent formatter exists');
  const sl=ex('secLoc');
  ok(/velP\(v\.p5\)/.test(sl) && /velP\(v\.p15\)/.test(sl) && /velP\(v\.p60\)/.test(sl) && /velP\(v\.p1d\)/.test(sl),
     'the four columns show Skylit’s PERCENTS, which are comparable across nodes');
  ok(/cls:\(v>0\)\?'g3up':'g3dn'/.test(ex('velP')), 'and they are green or red by sign');
  ok(/in dollars: /.test(sl), 'with the dollar figures kept in the hover, not lost');
  ok(/esTick\(n\.es\)/.test(sl) && /esTick\(pxNow\)/.test(sl), 'every ES price on the face is tick-rounded');
}
// ---------- price belongs in the ladder ----------
{
  const sl=ex('secLoc');
  // ⚠ the class name existing proves nothing — a mutation that wrapped the emission in `if(false)`
  // left every string in place and this suite passed it. Assert the emission is reached.
  ok(/if\(esAt<0 && pxNow!=null && n0\.es<pxNow\)\{ esAt=dispRows\.length; dispRows\.push\(\{ es:1 \}\); \}/.test(sl)
     && /if\(row\.es\)\{/.test(sl) && /class="g3ndes"/.test(sl),
     'the ES price row is emitted directly under its condition, ungated');
  ok(/n0\.es<pxNow/.test(sl), 'inserted where it belongs — between the nodes above and below it');
  ok(/below every node/.test(sl), 'and it still appears when price is under everything');
}
// ---------- (v14.4/6) bias lives on the PROFILE header now ----------
{
  const gp=ex('gammaProfileHtml');
  ok(/g3gpbias/.test(gp) && /ROLL BIAS /.test(gp), 'ROLL BIAS is emitted as the profile-header chip');
  ok(/g3gpbias\{[^}]*color:#e0645f/.test(src), 'and rendered red');
  // (v14.6) and rolls are an RTH story: every display of the latch gates on the session
  ok(/function rollsLive/.test(src), 'the session gate exists');
  ok(/if\(rollsLive\(\)\) RAILROLLS=/.test(src) && /if\(rollsLive\(\)\) ROLLS=/.test(src) &&
     /if\(rollsLive\(\)\)\{ ROLLS=rollLatched\(sym\)/.test(src),
     'and binds on ALL THREE consumers — rail, profile, nodes list');
}

// ---------- (v13.9) THE ROLL IS SHOWN AS A CONNECTOR IN ITS OWN GUTTER ----------
{
  const sl=ex('secLoc');
  // per-roll segments assembled per display row, dot at the source, elbow+head at the destination
  ok(/function addSeg\(i,s\)/.test(sl) && /function vseg\(i,box\)/.test(sl), 'a row-to-row connector is built');
  ok(/g3nddot/.test(sl) && /g3ndhead/.test(sl) && /g3ndstub/.test(sl),
     'source draws the dot, destination the elbow and arrowhead');
  ok(/Per-row segments \(rows are different heights/.test(sl),
     'and the reason it is per-row is recorded — one absolute shape would drift');
  ok(/var segHtml=\(segs\[rowIdx\]\|\|\[\]\)\.join\(''\);/.test(sl), 'every row emits its own segments');
  ok(/border-left:6px solid '\+col/.test(sl), 'the destination gets an arrowhead');
  ok(/g3ndrow\{position:relative\}/.test(src), 'rows are the positioning context for it');
  // (v14.x) the chip NAMES the roll on the source row's sub-line; the connector says where it went
  ok(/ROLL '\+arr\+' /.test(sl), 'the chip names the roll');
  ok(/chipAt\[at\]=chipAt\[at\]\|\|\[\]/.test(sl), 'and lands on the source row (or the destination when the source vacated)');
}


console.log('\n'+pass+' pass / '+fail+' fail');

// (v14.6) exit code added: this file could print FAIL and still exit 0 — the silent-red pattern.
process.exit((typeof fail!=="undefined"&&fail)?1:0);
