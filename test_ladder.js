// (v14.46) THE LADDER — the horizontal rail, profile and percentages rotated onto one price axis.
// Spec: mockups/mockup-ladder-v10.html, ten operator-reviewed drafts. These tests pin the invariants
// that the drafts had to learn the hard way.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
// ⚠ the LAD_* constants share one multi-var declaration, so only the FIRST is preceded by `var`.
function v(n){const m=new RegExp('(?:var\\s+)?\\b'+n+'\\s*=\\s*([^;,\\n]+)').exec(src); return m?eval('('+m[1]+')'):undefined;}

// ---- ⚠ THE VERSION GUARD. GPTS_VERSION is a SEPARATE constant from the @version header, and its own
// comment calls it "THE ONE VERSION STRING — header, footer, export, logs all read this". From v14.40
// to v14.45 only the header was bumped, so the panel footer told the operator it was running v14.39
// through six installs while every install had in fact landed. The existing tests pinned the header
// and pinned that the footer USES GPTS_VERSION — never that the two AGREE. That is the crack.
{
  const hdr=/@version\s+([0-9.]+)/.exec(src)[1];
  const con=/var GPTS_VERSION='([0-9.]+)'/.exec(src)[1];
  ok(hdr===con, 'v0 GPTS_VERSION EQUALS the @version header — the footer can never lie again', {hdr,con});
}

// ---- geometry lives in one place ----
const W=v('LAD_W'), LVL=v('LAD_LVL'), PXC=v('LAD_PXC'), PXW=v('LAD_PXW'), NODE=v('LAD_NODE'), NMAX=v('LAD_NMAX'),
      KPCT=v('LAD_KPCT'), CH=v('LAD_CH'), CHW=v('LAD_CHW'), PROF=v('LAD_PROF'), PMAX=v('LAD_PMAX'),
      ROLL=v('LAD_ROLL'), ST=v('LAD_ST'), ROC=v('LAD_ROC');
ok([W,LVL,PXC,NODE,NMAX,KPCT,CH,CHW,PROF,PMAX,ROLL,ST,ROC].every(x=>typeof x==='number'),
   'g1 every column offset is a named constant, not a magic number in the markup');

// ⚠⚠ THE CHUTE IS PRICE'S ALONE. This is the invariant the whole layout exists to protect: the
// operator caught "current price in two columns", and the King's %King label landing inside the
// chute. Nothing may be positioned within the chute's x-range except price itself and the target
// marker that deliberately parks at its inner edge.
{
  const chuteL=CH, chuteR=CH+CHW;
  const nodeTip=NODE+NMAX;
  ok(nodeTip<=chuteL, 'g2 a 100% node bar STOPS before the chute — it can never reach into price', {nodeTip,chuteL});
  ok(KPCT+38<=chuteL,  'g3 the %King label ends before the chute too (the King label bug)', {end:KPCT+38,chuteL});
  ok(PROF-PMAX>=chuteR,'g4 the profile grows inward but never enters the chute from the right', {inner:PROF-PMAX,chuteR});
  ok(ROLL>=PROF,       'g5 the roll lane sits outside the profile');
  ok(ST>=ROLL && ROC>=ST,'g6 state then roc, in that order, outside the rolls');
  ok(PXC+PXW<=NODE,    'g7 the price column ends before the node bars begin', {end:PXC+PXW,NODE});
  ok(LVL+4<PXC,        'g8 levels sit left of the price column');
}

// ---- the columns point INWARD (the operator's pinball framing) ----
ok(/left:'\+LAD_NODE\+'px/.test(src)||/left:'+LAD_NODE/.test(src.replace(/\s/g,'')),
   'g9 node bars are anchored on the OUTSIDE and grow toward price');
ok(/LAD_PROF-pf/.test(src) && /LAD_PROF-pp/.test(src),
   'g10 profile bars are anchored on the outside RIGHT and grow inward');

// ---- rolls: the sketch geometry, and the landing that must stay solid ----
eval(ex('ldNum'));
ok(/g3ldup/.test(ldNum(5)) && /\+5%/.test(ldNum(5)), 'r1 a building strike reads green and signed');
ok(/g3lddn/.test(ldNum(-5)), 'r2 ...and a draining one red');
ok(/g3ldfl/.test(ldNum(0)),  'r3 ...and flat is neither');
const RL=ex('ladderRolls');
ok(/H '\+\(xOut-rad\)/.test(RL) && /V '/.test(RL), 'r4 the path steps OUT, then along the ladder');
ok(/xEnd=0/.test(RL), 'r5 ...then back IN past the origin\'s own x, per the sketch');
ok(/THE LANDING IS A SEPARATE SOLID sub-path|landing is a SEPARATE SOLID/.test(src),
   'r6 the landing is documented as a separate solid path');
ok((RL.match(/marker-end/g)||[]).length===1 && /stroke-dasharray/.test(RL),
   'r7 exactly one arrowhead per roll, and it lives on the SOLID sub-path — never on the dashed one');
ok(/r\.live\?0\.98:0\.5/.test(RL) && /r\.live\?' stroke-dasharray/.test(RL),
   'r8 only LIVE rolls animate — a latched roll is structure, not motion');
ok(/circle cx="'\+x0/.test(RL), 'r9 the SOURCE carries a filled circle, so no arrow rises from blank track');
ok(/yA\+9/.test(RL), 'r10 the amount sits at the ORIGIN, below the out-run — two rolls into one destination cannot stack');
ok(/INFERRED from paired changes, never an observed transfer/.test(src),
   'r11 the hover says a roll is inferred, not observed');
ok(/pairing quality|Pairing quality/i.test(src), 'r12 ...and carries its pairing quality');

// ---- honesty invariants ----
const LH=ex('ladderHtml');
ok(/function inFrame/.test(LH) && /OFF\.push/.test(LH),
   'h1 anything outside the frame LEAVES the ladder — a clamped position would be a false one');
ok(/off frame/.test(LH), 'h2 ...and is disclosed with its distance instead');
ok(/LAD_SNAP_PTS/.test(LH), 'h3 a level within a couple of points of a node SHARES its row');
ok(!/Math\.random|Date\.now\(\)\s*\*/.test(LH), 'h4 nothing in the ladder is invented');
ok(/NOTHING HERE COMPUTES A NEW FACT/.test(src),
   'h5 the one-computation rule is stated where the ladder lives');
ok(/RAILTGT/.test(src) && /RAILTGT=frBestP\.disp/.test(src),
   'h6 the destination is the READ\'s own, shared — the two can never disagree');
ok(/0\.80 sigma/.test(LH) && /58%/.test(LH), 'h7 the expected-move caveat survives the rotation');
ok(/A retrace does not hand budget back/.test(LH), 'h8 ...as does the spent-range rule');

// ---- reversibility ----
ok(/CFG\.ladder!==false/.test(src), 'x1 the ladder is behind a setting');
ok(/g3ladon .g3emw/.test(src) && /hidden, not deleted/i.test(src),
   'x2 the old rail is HIDDEN, not deleted — one class reverts the whole change');
ok(/classList\.toggle\('g3ladon'/.test(src), 'x3 ...and the class is actually applied');

// ---- coverage: everything the old surfaces carried still has a home ----
[['levels','g3ldlv'],['price','g3ldpx'],['nodes','g3ldbar'],['day peak','g3ldpk'],['%King','g3ldkp'],
 ['price now','g3ldnow'],['expected move','g3ldem'],['day range','g3ldrange'],['open','g3ldopn'],
 ['profile','g3ldpf'],['rolls','g3ldrolls'],['state','g3ldst'],['roc','g3ldroc'],['destination','g3ldtgt']
].forEach(c=>ok(new RegExp(c[1]).test(src), 'c: '+c[0]+' has a home on the ladder'));


// ---- (v14.47) THE TWO BUGS THE FIRST LIVE RENDER FOUND -------------------------------------
// Both were invisible to the test suite and obvious the moment the panel drew itself.
// 1. THE LADDER MUST FIT, OR SCROLL — never clip. At 646px inside a 486px body with overflow
//    hidden, STATE and ROC were gone with nothing to say so. Silently dropping live data is the
//    worst failure this panel has.
ok(W<=584, 'w1 the ladder stays close to a default-width panel', W);
ok(/g3ladwrap\{overflow-x:auto/.test(src), 'w2 ...and the container SCROLLS rather than clips');
ok(/g3ladwrap"><div class="g3lad"/.test(src), 'w3 ...with the scroller actually wrapping the ladder');
ok(/costing information|instead of costing information/.test(src), 'w4 the reasoning is recorded');
// 2. UNITS. PEAK.m[k] is |velocity.cur|; P.usdK is THOUSANDS of dollars. Dividing one by the other
//    gave ~1000x, clamped to 100, and drew a full-width day-peak outline on EVERY node.
ok(!/Math\.abs\(pkv\)\/Math\.abs\(P\.usdK\)/.test(src),
   'u1 the day peak is NOT divided by usdK — that was a 1000x unit error');
ok(/pct\*Math\.abs\(pkv\)\/Math\.abs\(vvE\.cur\)/.test(src),
   'u2 peak-as-%King scales today\'s %King by peak/now, both in |cur| space');
ok(/UNITS\. PEAK\.m\[k\] stores \|velocity\.cur\|/.test(src), 'u3 the unit trap is documented at the site');
// the Level Engine divides in the same space — the two must never diverge
ok(/Math\.abs\(vv\.cur\)\/pk/.test(src), 'u4 ...the same space levelStateOf divides in');


// ---- (v14.48) THE THREE KINGS ----------------------------------------------------------------
// "I want to see where price is relative to the 3 kings" — SPXW's, SPY's and QQQ's, as pills.
eval(ex('ladderKings'));
ok(/g3ldkingSPXW/.test(src) && /g3ldkingSPY/.test(src) && /g3ldkingQQQ/.test(src),
   'k1 all three books get a king pill with its own colour');
ok(/\\u265b<b>/.test(src) || /♛<b>/.test(src), 'k2 the crown icon sits to the LEFT of the price');
ok(/g3ldking i\{font-style:normal;font-size:5\.6px/.test(src),
   'k3 the book tag is small — a qualifier on the price, not a rival to it');
// ⚠⚠ THE CONVERSION HONESTY. SPXW→ES and SPY→ES are a real basis; QQQ→ES is NOT — different indices,
// no basis between them, so the only honest mapping is proportional and it assumes a correlation of
// one, which is false on exactly the days it matters. It must be marked as a bearing, not a level.
ok(/kind:'basis'/.test(src) && /kind:'proportional'/.test(src),
   'k4 the two KINDS of conversion are distinguished in the data, not just the prose');
ok(/QQQ\s*→\s*ES\s*is NEITHER/.test(src), 'k5 the difference is documented where the conversion happens');
ok(/A bearing, never a level/.test(src), 'k6 ...and stated in the operator-facing hover');
ok(/kind==='proportional'\?'~':''/.test(src), 'k7 the QQQ price carries a tilde');
ok(/g3ldking\.approx\{border-style:dashed\}/.test(src),
   'k8 ...and a dashed pill, so the eye is told before the hover is read');
ok(/now\*\(ewq\.king\/qs\)/.test(src), 'k9 the QQQ bearing is price x (king / QQQ spot) — proportional, as described');
ok(/ew\.king\*EB\.scaleUsed/.test(src), 'k10 the SPY king uses the same live basis the SPY flag always used');
ok(/Math\.abs\(u-t\)<13/.test(src),
   'k11 kings landing on the same line are NUDGED apart — two crowns on one row was the dual-king confusion');
// ⚠ scoped to the LADDER: the old railLevelsLine still carries a 'SPY K' level, and that is correct —
// the previous surface is hidden, not deleted, for one release so the two can be compared live.
ok(!/n:'SPY K'/.test(LH), 'k12 the SPY King left the LADDER\'s levels column — it is a king pill now');
ok(/railLevelsLine/.test(src), 'k13 ...while the old surface stays intact and revertible');

// ---- (v14.48) the expected move as pills, with the old rail's over/under behaviour intact ----
ok(/g3ldempill/.test(src), 'e1 EH and EL are pills in the price column');
ok(/\['EH',EB\.high,RB\.over\]/.test(src) && /\['EL',EB\.low,RB\.under\]/.test(src),
   'e2 ...and they know whether price has run past them');
ok(/g3ldrailend/.test(src) && /where the expected move ENDED, not where the drawing does/.test(src),
   'e3 when price runs past, the RAIL END is marked separately — the band is never redefined as the frame');
ok(/0\.80 sigma/.test(src) && /1\.25/.test(src), 'e4 the sigma caveat and its conversion survive');

console.log('test_ladder: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
