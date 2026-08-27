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
      CH=v('LAD_CH'), CHW=v('LAD_CHW'), MK=v('LAD_MK'), MKW=v('LAD_MKW'),
      ROLL=v('LAD_ROLL'), ROLLW=v('LAD_ROLLW'), ST=v('LAD_ST'), STW=v('LAD_STW'), TAP=v('LAD_TAP'),
      TAPW=v('LAD_TAPW'), ROC=v('LAD_ROC'), ROCW=v('LAD_ROCW'),
      DAX=v('LAD_DAX'), DMAX=v('LAD_DMAX'), DLAB=v('LAD_DLAB'), DLABW=v('LAD_DLABW'),
      PCTIN=v('LAD_PCT_IN_BAR');
ok([W,LVL,PXC,NODE,NMAX,CH,CHW,MK,MKW,ROLL,ST,ROC,DAX,DMAX,DLAB,PCTIN].every(x=>typeof x==='number'),
   'g1 every column offset is a named constant, not a magic number in the markup');
// ⚠ (v14.54) THE COLUMN THAT WAS DELETED MUST STAY DELETED. %King moved inside its own bar; if a
// later build reinstates LAD_KPCT the 42px comes back and the arithmetic below silently loosens.
ok(v('LAD_KPCT')===undefined && v('LAD_PROF')===undefined,
   'g0 the parked %King column and the mirrored profile are GONE, not merely unused');

// ⚠⚠ THE CHUTE IS PRICE'S ALONE. This is the invariant the whole layout exists to protect: the
// operator caught "current price in two columns", and the King's %King label landing inside the
// chute. Nothing may be positioned within the chute's x-range except price itself and the target
// marker that deliberately parks at its inner edge.
{
  const chuteL=CH, chuteR=CH+CHW;
  const nodeTip=NODE+NMAX;
  ok(nodeTip<=chuteL, 'g2 a 100% node bar STOPS before the chute — it can never reach into price', {nodeTip,chuteL});
  // ⚠ (v14.54) THE %King FALLBACK IS THE NEW WAY INTO THE CHUTE. When a bar is too short to hold its
  // own number the label falls OUT of the bar at NODE+len+4 — and the WORST case is the widest bar
  // that still misses the cut (PCTIN-1). A signed 3-digit percentage is ~30px at 8px.
  ok(NODE+(PCTIN-1)+4+30<=chuteL,
     'g3 the %King fallback label ends before the chute even at its worst offset',
     {end:NODE+(PCTIN-1)+4+30, chuteL});
  ok(DAX-DMAX>=chuteR, 'g4 the delta bars hang left off their axis but never reach the chute', {inner:DAX-DMAX,chuteR});
  // (v14.54) the roll lane moved LEFT of the prices, so it must clear the level names on one side
  // and stop before the price column on the other.
  ok(ROLL-16>=LVL+2,   'g5a the roll amounts start after the level names end', {lab:ROLL-16, lvEnd:LVL});
  ok((ROLL-16)+ROLLW<=PXC, 'g5b ...and end before the price column begins', {end:(ROLL-16)+ROLLW, PXC});
  // ⚠ THE WIDTHS ARE PART OF THE ARITHMETIC. The old test checked only that the OFFSETS were ordered,
  // which they were — while a 44px state cell rendering "WEAKENING" at ~50px printed over the ROC
  // numbers beside it. Ordering offsets proves nothing about what actually gets drawn.
  // (v14.54) the full left-to-right chain, every link asserting offset+width <= the next offset.
  ok(chuteR<=MK,       'g6a the chute ends before the marker column begins', {chuteR,MK});
  ok(MK+MKW<=TAP,      'g6b the marker ends before the tests counter begins', {end:MK+MKW,TAP});
  ok(TAP+TAPW<=DAX-DMAX,'g6c the counter ends before the longest delta bar starts', {end:TAP+TAPW,barL:DAX-DMAX});
  ok(DLAB>=DAX,        'g6d the delta figure sits right of its own zero line', {DLAB,DAX});
  ok(DLAB+DLABW<=ST,   'g6e the delta figure ends before the state cell begins', {end:DLAB+DLABW,ST});
  ok(ST+STW<=ROC,      'g6f the state cell ENDS before the roc column begins', {end:ST+STW,ROC});
  ok(ROC+ROCW<=W,      'g6g the roc column ends inside the ladder', {end:ROC+ROCW,W});
  ok(STW>=50,          'g6h the state cell is wide enough for the longest word (WEAKENING)', STW);
  ok(PXC+PXW<=NODE,    'g7 the price column ends before the node bars begin', {end:PXC+PXW,NODE});
  ok(LVL+4<PXC,        'g8 levels sit left of the price column');
}

// ---- the columns point INWARD (the operator's pinball framing) ----
ok(/left:'\+LAD_NODE\+'px/.test(src)||/left:'+LAD_NODE/.test(src.replace(/\s/g,'')),
   'g9 node bars are anchored on the OUTSIDE and grow toward price');
ok(/LAD_DAX-dMag/.test(src), 'g10 delta bars are anchored on the zero line and hang LEFT of it');

// ---- rolls: the sketch geometry, and the landing that must stay solid ----
eval(ex('ldNum'));
ok(/g3ldup/.test(ldNum(5)) && /\+5%/.test(ldNum(5)), 'r1 a building strike reads green and signed');
ok(/g3lddn/.test(ldNum(-5)), 'r2 ...and a draining one red');
ok(/g3ldfl/.test(ldNum(0)),  'r3 ...and flat is neither');
const RL=ex('ladderRolls');
ok(/H '\+\(xOut\+rad\)/.test(RL) && /V '/.test(RL), 'r4 the path steps OUT (leftward now), then along the ladder');
ok(/xEdge=LAD_PXC-2/.test(RL), 'r5 ...and both ends land on the PRICE column\'s own edge, per the operator sketch');
ok(/THE LANDING IS A SEPARATE SOLID sub-path|landing is a SEPARATE SOLID/.test(src),
   'r6 the landing is documented as a separate solid path');
ok((RL.match(/marker-end/g)||[]).length===1 && /stroke-dasharray/.test(RL),
   'r7 exactly one arrowhead per roll, and it lives on the SOLID sub-path — never on the dashed one');
ok(/r\.live\?0\.98:0\.5/.test(RL) && /r\.live\?' stroke-dasharray/.test(RL),
   'r8 only LIVE rolls animate — a latched roll is structure, not motion');
ok(/circle cx="'\+xEdge/.test(RL), 'r9 the SOURCE carries a filled circle, so no arrow rises from blank track');
ok(/xOut=LAD_ROLL\+2\+n\*8/.test(RL), 'r9b the lane is LEFT of the prices and fans by roll index');
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
 ['delta profile','g3lddbar'],['delta figure','g3lddval'],['delta axis','g3lddbase'],
 ['rolls','g3ldrolls'],['state','g3ldst'],['tests','g3ldtap'],['roc','g3ldroc'],['destination','g3ldtgt'],
 ['king taps','g3ldkt']
].forEach(c=>ok(new RegExp(c[1]).test(src), 'c: '+c[0]+' has a home on the ladder'));


// ---- (v14.54) %KING RIDES ITS OWN BAR --------------------------------------------------------
ok(/roomPct=\(len>=Math\.max\(LAD_PCT_IN_BAR, pctW\+8\)\)/.test(LH),
   'p1 the bar carries its own %King when it is wide enough');
// ⚠ the fit test must MEASURE the strings. Charging the role for a maximum-width %King pushed a
// role out of a bar that could hold it, and the clamp then dropped it back on top of that bar.
ok(/roomTyp=\(role && len>=\(pctW\+roleW\+11\)\)/.test(LH),
   'p1b ...and the role room is measured against the ACTUAL %King beside it, not a constant');
ok(/if\(pxL!=null\) h\+='<span class="g3ldkp/.test(LH),
   'p2 ...and a bar too short to hold the number lets it FALL OUT, rather than suppressing it');
// ⚠⚠ THREE STATES, NOT TWO — the render audit found both wrong answers in turn. A label that fits
// nowhere legal is NOT DRAWN; it is never shoved into the chute and never stacked on its own bar.
ok(/var barTip=LAD_NODE\+len\+2/.test(LH) && /\(max>=barTip\)\?max:null/.test(LH),
   'p2b a fallback label that fits nowhere is DROPPED, not shoved into the chute or onto the bar');
ok(/pxL!=null/.test(LH) && /rlL!=null/.test(LH),
   'p2c ...and BOTH fallbacks honour it — the %King and the role alike');
ok(/justify-content:space-between/.test(src),
   'p3 %King is left-justified in the bar and the type right-justified at its tip');

// ---- (v14.54) THE DELTA PROFILE IS IN DOLLARS, AND ITS UNITS ARE MEASURED ---------------------
// ⚠ kingKd is THOUSANDS and velocity.d15 is DOLLARS. Multiplying is right; dividing is the L-F bug.
ok(/Math\.abs\(TK\.kingKd\)\*1000/.test(LH),
   'd1 the King mass is kingKd x 1000 — kingKd is in THOUSANDS, d15 is in dollars');
ok(!/kingKd\s*\/\s*1000/.test(LH), 'd2 ...and is never divided by 1000, which was the unit trap');
ok(/velocity\.cur -12,680,083/.test(src) || /kingKd 12680/.test(src),
   'd3 the measurement that settles the unit pairing is recorded at the site, not asserted from memory');
ok(/LAD_DELTA_FULL_FRAC/.test(LH) && /LAD_DELTA_MIN_FRAC/.test(LH),
   'd4 full scale is a fraction of the King, so a quiet day stays quiet and the scale self-fits the book');
ok(/MAGNITUDE, not polarity/.test(LH),
   'd5 the hover says the delta is MAGNITUDE — a draining negative node is not becoming more negative');
ok(/vv\.d15/.test(LH), 'd6 the figure is Skylit\'s own delta15, not one we compute');

// ---- (v14.54) THE DAY PEAK ONLY DRAWS WHEN IT MEANS SOMETHING --------------------------------
ok(/giveback>=LAD_PEAK_MIN_GIVEBACK/.test(LH),
   'y1 the day-peak outline needs a real giveback — otherwise it was wallpaper on every row');

// ---- (v14.54) THE CROWN CARRIES ITS TEST COUNT ------------------------------------------------
ok(/nodeTapCount\('SPXW', K\.raw\)/.test(LH),
   'n1 king taps are counted at the king\'s OWN STRIKE, not at its converted chart price');
ok(/kt>0\?\('<b class="g3ldkt/.test(LH),
   'n2 the counter is ABSENT at zero — an untested crown is the strong one, not a weak signal');
ok(/UNTESTED today/.test(LH) && /the ~80% state/.test(LH), 'n3 ...and the hover says so in words');

// ---- (v14.54) PRICE ON A KING SAYS SO ---------------------------------------------------------
ok(/g3ldnow\.kSPXW/.test(src) && /g3ldnow\.kSPY/.test(src) && /g3ldnow\.kQQQ/.test(src),
   'q1 the price pill can take each book\'s colour');
ok(/LAD_KING_TEST_PTS/.test(LH), 'q2 ...within a named tolerance, not a magic number');
ok(/EB\.stretched\?' str':\(onK\?/.test(LH),
   'q3 STRETCHED still wins over testing-a-king — two states cannot share one background');


// ---- (v14.47) THE TWO BUGS THE FIRST LIVE RENDER FOUND -------------------------------------
// Both were invisible to the test suite and obvious the moment the panel drew itself.
// 1. THE LADDER MUST FIT, OR SCROLL — never clip. At 646px inside a 486px body with overflow
//    hidden, STATE and ROC were gone with nothing to say so. Silently dropping live data is the
//    worst failure this panel has.
ok(W<=640, 'w1 the ladder stays within a resizable panel', W);
// ⚠ (v14.54) THE BUILD MUST NOT GET WIDER. 632 -> 588 was the whole point; a later change that adds a
// column and pushes it back up has undone this build without anything else noticing.
// ⚠ 618 IS THE TRUE WIDTH, and it is compared against a TRUE 657 before this build, not the 632 the
// old constants claimed — LAD_ROCW was 56 for a column that needs 84, so the last column overflowed
// and LAD_W was wrong. That 25px is the discrepancy LOCKED-ITEMS recorded and could not explain.
ok(W<=618, 'w1b the v14.54 compaction holds — the ladder is no wider than 618', W);
ok(ROC+ROCW===W, 'w1c ...and LAD_W is exactly where the last column ends, so it cannot lie again', {end:ROC+ROCW,W});
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
// (v14.54) the nudge pitch tracks the PILL HEIGHT: the crowns are 14px tall inside the chute now,
// so 13px of separation would still let two of them touch. 15 is the pill plus a hairline.
ok(/Math\.abs\(u-t\)<15/.test(src),
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
