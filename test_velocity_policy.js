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
  // (v15.53) removed: the NODES list under LOC_SHOW_NODES is archived (C-flagged-off)
  // (v15.53) removed: the NODES list under LOC_SHOW_NODES is archived (C-flagged-off)
  // (v15.53) removed: the NODES list under LOC_SHOW_NODES is archived (C-flagged-off)
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
  // (v15.53) removed: the NODES list under LOC_SHOW_NODES is archived (C-flagged-off)
}
// ---------- A BROKEN HARVEST REFUSES, IT DOES NOT RENDER ZEROES ----------
{
  ok(/function velOk\(\)/.test(src), 'health is a first-class check');
  // (v15.53) removed: the NODES list under LOC_SHOW_NODES is archived (C-flagged-off)
  // (v15.53) removed: this text lived in the archived NODES list / nodeChip (C-flagged-off)
  ok(/Skylit may have renamed them/.test(src),
     'the fiber-coupling risk is named where it would bite');
}
// ---------- ROLLS: NOT CONSERVATION OF MASS ----------
// Measured live: receivers gained 2.8x, 8.6x, 13.1x and 16.5x what the losers shed. Several strikes
// feed one destination. An equal-and-opposite test would have found almost nothing.
{
  const rs=ex('velMass15')+'\n'+ex('rollScan');   // (v15.53, D6)
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
  // (v14.4/6 -> v14.81) the bias line lived on the GAMMA PROFILE header by his direction; the
  // profile was cut 2026-08-28 and he chose to keep the chip, so it now rides the ② LOCATION row.
  ok(/ROLL BIAS /.test(ex('secFrame')), 'and reaches the face as one chip on the LOCATION row');

  // ---- (v15.18) A ROLL IS A MOVE OF MASS, AND MASS IS |cur| ----------------------------------
  // ⚠⚠ EXECUTED, with the operator's own book: the five SPXW nodes at 14:12 CT on 2026-08-31,
  // copied verbatim out of data/2026-08-31.json. Until this build rollScan tested the SIGNED delta,
  // so on the NEGATIVE side of the book both halves were inverted — 7675 deepening from -59.6M to
  // -82.0M (d15 -22.4M) was called a SOURCE while it GAINED $22.4M, and 7670 decaying from -40.2M
  // toward -18.2M (d15 +22.0M) was called its RECEIVER while it emptied. The face drew 7675 -> 7670.
  // The direction is the entire content of an arrow, so this was not a near miss.
  const ROWS = [
    { k:7675, cur:-81988795, d15:-22426842 },   // |cur| 59.6M -> 82.0M   GAINED 22.4M
    { k:7700, cur:-51413336, d15:-15326975 },   // |cur| 36.1M -> 51.4M   GAINED 15.3M
    { k:7685, cur: 34840580, d15:  7286541 },   // |cur| 27.6M -> 34.8M   gained  7.3M
    { k:7695, cur: 19423931, d15:  6065799 },   // |cur| 13.4M -> 19.4M   gained  6.1M
    { k:7670, cur:-18218115, d15: 21974932 }    // |cur| 40.2M -> 18.2M   SHED   22.0M
  ];
  const savedVelAt=global.velAt, savedVelOk=global.velOk;
  global.velOk=()=>true;
  global.velAt=k=>{ const v=ROWS.find(r=>r.k===k); return v?{ v:v, age:0, stale:false }:null; };
  global.ROLL_MIN_ABS=40000; global.ROLL_MAX_DIST=25; global.ROLL_MIN_RATIO=0.40;
  eval(rs);
  const got=rollScan(ROWS.map(r=>r.k));
  ok(got.length===1, 'v1 EXECUTED: exactly ONE roll is present in that book', got.length);
  ok(got[0] && got[0].from===7670, 'v2 ...and the SOURCE is the strike that lost mass, 7670', got[0]&&got[0].from);
  ok(got[0] && got[0].to===7675, 'v3 ...pointing AT the King, which gained it — the old rule drew this backwards', got[0]&&got[0].to);
  ok(got[0] && Math.round(got[0].amt)===21974932, 'v4 ...and the amount is the MASS shed, not the signed delta', got[0]&&got[0].amt);
  ok(!got.some(r=>r.from===7675), 'v5 a strike whose |cur| GREW is never a source, however negative its delta');
  ok(!got.some(r=>r.to===7670),   'v6 ...and a strike whose |cur| SHRANK is never a receiver');
  // a positive-side book must behave exactly as it always did — this fix must not move live behaviour
  // where the sign never confused it.
  const POS=[{k:7600,cur:1000000,d15:-500000},{k:7610,cur:900000,d15:400000}];
  global.velAt=k=>{ const v=POS.find(r=>r.k===k); return v?{ v:v, age:0, stale:false }:null; };
  const gp=rollScan([7600,7610]);
  ok(gp.length===1 && gp[0].from===7600 && gp[0].to===7610,
     'v7 on the positive side of the book the answer is unchanged', gp);
  global.velAt=savedVelAt; global.velOk=savedVelOk;
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
  // (v15.53) removed: the NODES list under LOC_SHOW_NODES is archived (C-flagged-off)
  ok(/g3plab\.acc\{color:#a371f7\}/.test(src) && /g3plab\.brk\{color:#e3c341\}/.test(src),
     'and the rail CSS still uses those same hexes \u2014 a colour meaning two things is worse than none');
}
// ---------- CHIPS: SIDE x DIRECTION, TURNING FIRST ----------
{
  const nc=ex('nodeChip');
  const iTurn=nc.indexOf('TURNING'), iBuild=nc.indexOf('BUILDING');
  // (v15.53) removed: the NODES list under LOC_SHOW_NODES is archived (C-flagged-off)
  // (v15.53) removed: the NODES list under LOC_SHOW_NODES is archived (C-flagged-off)
  // (v15.53) removed: the NODES list under LOC_SHOW_NODES is archived (C-flagged-off)
  // (v15.53) removed: this text lived in the archived NODES list / nodeChip (C-flagged-off)
  // (v15.53) removed: the NODES list under LOC_SHOW_NODES is archived (C-flagged-off)
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
  // (v15.53) removed: the NODES list under LOC_SHOW_NODES is archived (C-flagged-off)
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
  // (v15.53) removed: the NODES list under LOC_SHOW_NODES is archived (C-flagged-off)
  // (v15.53) removed: the NODES list under LOC_SHOW_NODES is archived (C-flagged-off)
  // (v15.53) removed: the NODES list under LOC_SHOW_NODES is archived (C-flagged-off)
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
  // (v15.53) removed: the NODES list under LOC_SHOW_NODES is archived (C-flagged-off)
  // (v15.53) removed: the NODES list under LOC_SHOW_NODES is archived (C-flagged-off)
}
// ---------- units on the face ----------
{
  // (v15.53) removed: the NODES list under LOC_SHOW_NODES is archived (C-flagged-off)
  // (v15.53) removed: the NODES list under LOC_SHOW_NODES is archived (C-flagged-off)
  const sl=ex('secLoc');
  // (v15.53) removed: the NODES list under LOC_SHOW_NODES is archived (C-flagged-off)
  // (v15.53) removed: the NODES list under LOC_SHOW_NODES is archived (C-flagged-off)
  // (v15.53) removed: the NODES list under LOC_SHOW_NODES is archived (C-flagged-off)
  // (v15.53) removed: the NODES list under LOC_SHOW_NODES is archived (C-flagged-off)
}
// ---------- price belongs in the ladder ----------
{
  const sl=ex('secLoc');
  // ⚠ the class name existing proves nothing — a mutation that wrapped the emission in `if(false)`
  // left every string in place and this suite passed it. Assert the emission is reached.
  // (v15.53) removed: the NODES list under LOC_SHOW_NODES is archived (C-flagged-off)
  // (v15.53) removed: the NODES list under LOC_SHOW_NODES is archived (C-flagged-off)
  // (v15.53) removed: the NODES list under LOC_SHOW_NODES is archived (C-flagged-off)
}
// ---------- (v14.4/6) bias lives on the PROFILE header now ----------
{
  // ⚠⚠ THIS BLOCK WAS REWRITTEN, NOT DELETED (v14.81). The gamma profile is gone at his request,
  // so "the profile-header chip" no longer names anything. The chip itself survives by his choice.
  const gp=ex('secFrame');
  ok(/g3gpbias/.test(gp) && /ROLL BIAS /.test(gp), 'ROLL BIAS is emitted on the LOCATION row');
  ok(/g3gpbias\{[^}]*color:#e0645f/.test(src), 'and rendered red');
  ok(!/function gammaProfileHtml/.test(src), 'and its old home is actually gone, not merely unused');
  // ⚠⚠ THE REHOMED CHIP MUST DERIVE ITS OWN ROLLS. `ROLLS` is a LOCAL of secLoc(); reading it from
  // secFrame() is a ReferenceError that the surrounding catch would swallow, leaving the chip
  // permanently absent with every test still green. That is exactly what the first draft did.
  ok(/ROLLSf=rollsLive\(\)\?\(rollLatched\(sym\)\|\|\[\]\):\[\]/.test(noc(gp)),
     'and it builds its own latched list rather than inheriting a scope it cannot see');
  ok(!/rollBias\(ROLLS\.filter/.test(gp), 'and never reads secLoc\'s local ROLLS');
  // ⚠⚠ A SOURCE GREP PROVES THE STRING EXISTS, NOT THAT IT RUNS. Mutation caught this: wrapping the
  // emission in `if(0)` left every assertion above green while the chip vanished from the face.
  // Bind the guard DIRECTLY to the emission so nothing can be inserted between them.
  ok(/if\(BIASp && BIASp\.n>1 && BIASp\.dir!=='mixed'\)\s*\n?\s*h\+='<span class="g3gpbias"/.test(noc(gp)),
     'and the bias test is the ONLY thing standing between it and h+= (no dead-coding it)');
  // (v14.6) and rolls are an RTH story: every display of the latch gates on the session
  ok(/function rollsLive/.test(src), 'the session gate exists');
  // (v15.53) removed: the NODES list under LOC_SHOW_NODES is archived (C-flagged-off)
}

// ---------- (v13.9) THE ROLL IS SHOWN AS A CONNECTOR IN ITS OWN GUTTER ----------
{
  const sl=ex('secLoc');
  // per-roll segments assembled per display row, dot at the source, elbow+head at the destination
  // (v15.53) removed: the NODES list under LOC_SHOW_NODES is archived (C-flagged-off)
  // (v15.53) removed: the NODES list under LOC_SHOW_NODES is archived (C-flagged-off)
  // (v15.53) removed: the NODES list under LOC_SHOW_NODES is archived (C-flagged-off)
  // (v15.53) removed: the NODES list under LOC_SHOW_NODES is archived (C-flagged-off)
  // (v15.53) removed: the NODES list under LOC_SHOW_NODES is archived (C-flagged-off)
  // (v15.53) removed: the NODES list under LOC_SHOW_NODES is archived (C-flagged-off)
  // (v14.x) the chip NAMES the roll on the source row's sub-line; the connector says where it went
  // (v15.53) removed: the NODES list under LOC_SHOW_NODES is archived (C-flagged-off)
  // (v15.53) removed: the NODES list under LOC_SHOW_NODES is archived (C-flagged-off)
}


console.log('\n'+pass+' pass / '+fail+' fail');

// (v14.6) exit code added: this file could print FAIL and still exit 0 — the silent-red pattern.
process.exit((typeof fail!=="undefined"&&fail)?1:0);
