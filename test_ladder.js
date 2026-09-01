// (v14.46) THE LADDER — the horizontal rail, profile and percentages rotated onto one price axis.
// Spec: mockups/mockup-ladder-v10.html, ten operator-reviewed drafts. These tests pin the invariants
// that the drafts had to learn the hard way.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
// ⚠ the LAD_* constants share one multi-var declaration, so only the FIRST is preceded by `var`.
// ⚠⚠ COMMENTS ARE STRIPPED FIRST, AND THAT IS NOT COSMETIC (v15.09). A prose line reading
// "LAD_NODE=150 - a 4px overlap on every node row" sits EARLIER in the file than the declaration, so
// the raw regex matched the COMMENT, eval'd "150 - a 4px overlap..." and the whole suite file threw
// before a single assertion ran. A geometry reader that can be steered by a comment is a reader that
// silently tests the wrong numbers the day someone writes a helpful note.
const srcNC=src.replace(/\/\*[\s\S]*?\*\//g,'').replace(/^[ \t]*\/\/.*$/gm,'');
function v(n){const m=new RegExp('(?:var\\s+)?\\b'+n+'\\s*=\\s*([^;,\\n]+)').exec(srcNC); return m?eval('('+m[1]+')'):undefined;}

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
      KS=v('LAD_KS'), KSW=v('LAD_KSW'), KY=v('LAD_KY'), KYW=v('LAD_KYW'), LVLW=v('LAD_LVLW'),
      PILLW=v('LAD_PILLW'),
      ST=v('LAD_ST'), STW=v('LAD_STW'), TAP=v('LAD_TAP'),
      TAPW=v('LAD_TAPW'), ROC=v('LAD_ROC'), ROCW=v('LAD_ROCW'),
      DAX=v('LAD_DAX'), DMAX=v('LAD_DMAX'), DLAB=v('LAD_DLAB'), DLABW=v('LAD_DLABW'),
      ROLL=v('LAD_ROLL'), ROLLW=v('LAD_ROLLW'),   // (v15.09) the roll lane, the new last column
      PCTIN=v('LAD_PCT_IN_BAR');
ok([W,LVL,LVLW,PXC,NODE,NMAX,CH,CHW,MK,MKW,KS,KSW,KY,KYW,ST,ROC,DAX,DMAX,DLAB,PCTIN].every(x=>typeof x==='number'),
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
  // ⚠⚠ (v15.09) g5a/g5b ARE REWRITTEN, NOT DELETED. They guarded the ROLL LANE's gap between the
  // level names and the prices. The operator repurposed that lane into the two King migration
  // columns ("repurpose the arrows that we have today to show how the spx and the spy kings
  // movement during the day"), so the lane still exists — it just holds something else, and it
  // still has to clear what is beside it.
  ok(KS>=0 && KS+KSW<=KY, 'g5a the SPXW column ends before the SPY column begins', {sEnd:KS+KSW, KY});
  ok(KY+KYW<=LVL, 'g5b ...and the SPY column ends before the level names begin', {yEnd:KY+KYW, LVL});
  // ⚠ AND THE NAME MUST TOUCH ITS PRICE. That is the whole point of v15.09: "PDC 7741" as one
  // token. A gap wider than a space between them and they read as two separate columns again.
  ok(LVL+LVLW<=PXC && PXC-(LVL+LVLW)<=4,
     'g5c the level name ends immediately before its price — they read as one token',
     {nameEnd:LVL+LVLW, PXC, gap:PXC-(LVL+LVLW)});
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
// ⚠⚠ (v15.09) r4-r7 PINNED `ladderRolls`, WHICH IS GONE. The operator repurposed its lane:
// "repurpose the arrows that we have today to show how the spx and the spy kings movement during
// the day." The drawing was a DUPLICATE — secLoc() renders the same rolls in its own gutter (v13.9)
// and ROLL BIAS states the whole-book direction on the ② LOCATION row — so removing it removed no
// information. These assertions now pin the REMOVAL and the replacement, so a future context cannot
// quietly reinstate a second roll drawing in a lane that is spoken for.
ok(!/function ladderRolls/.test(src), 'r4 ladderRolls is gone, not merely uncalled');
ok(!/g3ldramt/.test(src), 'r5 ...and so is its CSS — a retired drawer leaves no styling behind');
ok(/function ladderKingCols/.test(src) && /h\+=ladderKingCols\(/.test(src),
   'r6 the King columns took the lane, and are actually called');
// ⚠ ROLLS THEMSELVES MUST SURVIVE THE LOSS OF THIS DRAWING (v11.95: a badge went, not a measurement)
ok(/function rollLatched/.test(src) && /function rollBias/.test(src) && /ROLL BIAS /.test(src),
   'r7 the roll MEASUREMENT and its whole-book chip are untouched');
// r8-r10 described ladderRolls' own SVG and went with it. r11/r12 are about the roll HOVER, which
// lives in secLoc() and survives — they stay, unchanged, because the claim they guard is unchanged.
ok(/INFERRED from paired changes, never an observed transfer/.test(src),
   'r11 the hover says a roll is inferred, not observed');
ok(/pairing quality|Pairing quality/i.test(src), 'r12 ...and carries its pairing quality');

// ============================================================================================
// (v15.09) THE KING MIGRATION COLUMNS
// ============================================================================================
const KC=ex('ladderKingCols'), KT=ex('ktTick');

// ---- ONE SOURCE, OR THE COLUMN CONTRADICTS THE CROWN BESIDE IT -------------------------------
// ⚠⚠ MEASURED 2026-08-28: the recorder's `snap.king` and `snap.tri.SPY.king` disagreed at 13:24 and
// 13:36 (769 vs 771) — one has hysteresis, one does not — so "how many times did the King move" had
// two different answers. The track reads ladderKings(), the same call the chute pills read.
ok(/KG=ladderKings\(EB, sym\)/.test(KT),
   'k1 the track reads ladderKings() — the SAME source the crown pills draw from');
ok(/ktTick\(EB, sym\)/.test(ex('ladderHtml')),
   'k2 ...and it is ticked from ladderHtml with that same EB, so the two cannot diverge');

// ---- TRACK THE STRIKE, NOT THE CHART PRICE ---------------------------------------------------
// ⚠⚠ `at` is chart space and moves whenever the ES/SPX basis drifts. Tracking it would record a
// migration every few minutes and never a real one. `raw` only changes when the crown changes.
ok(/K\.raw/.test(KT) && !/pd\.k!==K\.at|k:K\.at/.test(KT),
   'k3 the track stores the book\'s own strike (raw), never the drifting chart price');
ok(/conv:function\(k\)\{ return dsc>0\?k\*dsc:null; \}/.test(KC),
   'k4 ...and converts at RENDER time, so a run stays level with its row as the basis moves');

// ---- A FLICKER IS NOT A MIGRATION ------------------------------------------------------------
// ⚠⚠ Friday held THREE single-observation excursions that all reverted (SPY 769 at 13:24 and 13:36,
// SPXW 7690 at 13:51). Drawing them would have put four steps on a day that had one real move.
// ⚠⚠ (v15.23) DWELL IS A DURATION NOW, NOT A COUNT. `KT_DWELL=2` meant two OBSERVATIONS, and the
// live latch observes once per render (seconds) while the replay rebuild walks 3-minute frames — so
// one constant produced ~6 seconds of probation live and 6 minutes in replay. The live lane was
// effectively unfiltered, which is what the operator saw: "too erratic ... there should only be a
// couple of movements in a day."
ok(/KT_DWELL_MIN=\d+/.test(src), 'k5 a dwell threshold exists, expressed in MINUTES');
ok(/\(Date\.now\(\)-pd\.t0\)\/60000 < KT_DWELL_MIN/.test(KT),
   'k6 ...and the live latch holds probation by the CLOCK, not by how often it happened to render');
ok(!/pd\.n/.test(KT), 'k6b ...with no observation counter left to disagree with it');
ok(/if\(last===K\.raw\)\{ delete KT_PEND\[bk\]; return; \}/.test(KT),
   'k7 ...and a candidate that reverts is discarded, leaving no trace');
ok(/KT_PEND/.test(KT) && !/KTRACK\.b\[bk\]\.push\(\{ t:Date\.now\(\), k:K\.raw \}\);[\s\S]{0,40}pd/.test(KT),
   'k8 probation is held OUTSIDE the persisted track — a flicker never reaches storage');

// ---- THE EXPIRY ROLL IS NOT A MIGRATION ------------------------------------------------------
// At the close Skylit rolls the front expiry and EVERY crown "moves" (7715->7710, 771->760 on
// 2026-08-28). That is bookkeeping.
ok(/if\(!P \|\| !P\.rth\) return;/.test(KT), 'k9 RTH only — which excludes the close roll outright');
ok(/e:exp/.test(KT), 'k10 ...and each point carries the expiry it was seen under, so a roll is visible');

// ---- WRITES ASK recorderBlind, NOT inReplay --------------------------------------------------
// ⚠ Caught by test_lastbook r3 on the first draft. ktTick WRITES, so it must ask the one predicate
// that means "the face is not showing live truth" — a replay OR a frozen book.
ok(/recorderBlind\(\)\) return;/.test(KT) && !/inReplay\(\)\) return;/.test(KT),
   'k11 the write path asks recorderBlind(), never inReplay() alone');

// ---- THE COLUMNS ARE ROW-ALIGNED, WHICH IS WHY HE CHOSE THEM ---------------------------------
// He drew time running DOWN the column; he then chose price-vertical instead, because vertical
// position means PRICE everywhere else in this band and two meanings for one axis misleads.
ok(/Y\(disp\)/.test(KC), 'k12 vertical position is the ladder\'s own price scale');
ok(/if\(disp<lo \|\| disp>hi\) return;/.test(KC),
   'k13 ...and a strike off the frame draws NO run rather than clamping to an edge it never sat on');
ok(/QQQ/.test(KC.slice(0, KC.indexOf('COLS.forEach'))) === false && /'SPXW'/.test(KC) && /'SPY'/.test(KC),
   'k14 two columns, SPXW and SPY — QQQ is a proportional BEARING and gets no migration line');
// an empty column must say WHY, because "no arrow" has two very different meanings
ok(/g3ldkcw/.test(KC) && /panel was not running/.test(KC),
   'k15 an empty column says whether nothing moved or nothing was watched');

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
ok(/roomPct=\(showPct && len>=Math\.max\(LAD_PCT_IN_BAR, pctW\+8\)\)/.test(LH),
   'p1 the bar carries its own %King when it is wide enough');
// ---- (v15.09) ...AND THE KING DOES NOT CARRY ITS OWN 100% -----------------------------------
// Operator: "you dont need to put 100% for the king." Every %King on this rail is a ratio TO the
// King, so its own number is the denominator announcing itself.
ok(/kingIs100=\(role==='KING' && pct>=100\)/.test(LH),
   'p1b the suppression is tied to the VALUE being 100, not to the role');
ok(/if\(!roomPct && showPct\)/.test(LH),
   'p1c ...and the OUTSIDE-the-bar fallback is suppressed too, or it would print what the bar hid');
// ⚠ a King that is NOT 100 can only happen mid-roll, when the crown has just changed hands — and
// that is exactly when you need the number. Binding to the role alone would hide it.
ok(!/kingIs100=\(role==='KING'\)/.test(LH),
   'p1d ...so a non-100 King still prints its number');
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
// ⚠⚠ (v15.09) AMENDED ONCE, IN THE OPEN. 618 -> 640, because the ROLL LANE was added at the
// operator's request ("arrows that showed where the gamma was flowing out of and into"). I first
// added 48px and this guard caught it — which is exactly what it is for: "a later change that adds
// a column and pushes it back up has undone this build without anything else noticing." I noticed.
// The lane was squeezed 44 -> 20px so the total lands on w1's 640 ceiling rather than sailing past
// it. ⚠ 640 IS NOW THE CAP. The next column that wants width argues for it here, in these words.
ok(W<=640, 'w1b the compaction holds — the ladder is no wider than 640 (was 618 + the 20px roll lane)', W);
// ⚠ the last column is the ROLL LANE now, not ROC. LAD_W must still equal where it ends, so the
// constant cannot drift away from the layout again — that 25px discrepancy took a build to explain.
ok(ROLL+ROLLW===W, 'w1c ...and LAD_W is exactly where the last column ends, so it cannot lie again',
   {end:ROLL+ROLLW, W});
ok(/g3ladwrap\{overflow-x:auto/.test(src), 'w2 ...and the container SCROLLS rather than clips');
// ⚠ (v15.21) NOT AN ADJACENCY TEST ANY MORE. This asserted the two tags were literally touching,
// which broke the moment the column HEADER ROW was added between them — a true structural change
// that the assertion could not distinguish from the scroller being unhooked. What matters is that
// the frame is INSIDE the scroller, so both are asserted, in order, within one wrapper.
ok(/g3ladwrap">(?:(?!<\/div>)[\s\S])*?<div class="g3lad"/.test(src),
   'w3 ...with the scroller actually wrapping the ladder');
ok(/g3ladwrap">'\+hd\+'<div class="g3lad"/.test(src) || /g3ladwrap"><div class="g3lad"/.test(src),
   'w3b ...and nothing but the header row sits between them');
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
// ⚠⚠ (v14.56) THE CHUTE HAS THREE KINDS OF OCCUPANT AND THE EM LABEL IS THE ONE THAT MOVES.
// This is the operator's own design, mockup-ladder-v11.html:343-354, which v14.54 shipped without
// because I stopped reading the mockup at line 339. The band edges are drawn LAST and step around
// the crowns AND the price pill; the amber LINE stays at the true price, so a nudged label loses
// nothing. Nudging the CROWN instead (my first fix) is backwards — a crown has no line behind it.
ok(/var EMQ=\[\]/.test(src) && /EMQ\.push/.test(src) && /EMQ\.forEach/.test(src),
   'k11b the EM pills are QUEUED and emitted last, after the crowns and price');
// ⚠⚠ (v15.25) THE LINE IS TWO SEGMENTS NOW, AND THE REASON IS THE OPERATOR'S: "why is there an
// amber/yellow line that crosses some of the pills. sometimes it crosses the expected high and
// sometimes it crosses the king nodes." It was one 78px stub drawn INSIDE the pill chute at the
// band edge's true price — while the LABEL is nudged 15px at a time to clear the crowns and the
// price pill, or dropped when there is nowhere clear. So an unlabelled amber stub sat in the middle
// of the chute, reading as a line struck THROUGH whatever pill was at that height.
// ⚠ The line was never in the wrong place; it was in the wrong COLUMN. Two segments that stop 6px
// short of the chute on each side cannot cross a pill, and a rule that reads across the ladder
// looks like a band edge instead of a strike-through.
ok(/h\+='<i class="g3ldem g3ldemL" style="top:'\+t\.toFixed\(1\)\+'px"><\/i>'\+/.test(src),
   'k11c ...while the amber LINE is drawn immediately, at the true price');
ok(/g3ldemL\{left:'\+LAD_LVL/.test(src) && /g3ldemR\{left:'\+\(LAD_CH\+LAD_CHW\+6\)/.test(src),
   'k11d ...in two segments that stop either side of the pill chute, so it cannot cross a pill');
ok(/CHUTEY\.push\(tn\)/.test(src),
   'k11d PRICE is a chute occupant the EM label must clear — the two-columns defect');
ok(/var KG=ladderKings\(EB, sym\), used=\[\];/.test(src),
   'k11e the crowns nudge among THEMSELVES; they do not move for an EM pill');
{
  const emBlock = /EMQ\.forEach\(function\(e\)\{[\s\S]{0,2000}?\n    \}\);/.exec(src);
  ok(emBlock && /guard\+\+<4/.test(emBlock[0]) && /Math\.abs\(u-t\)<15/.test(emBlock[0]),
     'k11f ...and the EM nudge uses the mockup\'s own 15px pitch and 4-step guard');
  // ⚠ AND AFTER THE FOUR STEPS, IT DROPS. With three crowns and the price pill in one chute the
  // guard can be exhausted and the label lands back on a crown — the audit reproduced it. The amber
  // line is already on the true row, so an absent pill loses nothing; a pill on a crown claims a row
  // that belongs to something else. Same rule as the off-frame levels and the v14.54 node labels.
  ok(emBlock && /if\(CHUTEY\.some\(function\(u\)\{ return Math\.abs\(u-t\)<15; \}\)\) return;/.test(emBlock[0]),
     'k11g ...and a label with nowhere clear to sit is DROPPED, never drawn over a crown');
}
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


// ============================================================================================
// (v15.09) THE FIVE THINGS HE ASKED FOR ON 2026-08-28, EACH PINNED
// ============================================================================================

// ---- 1 · THE BAR NO LONGER SITS ON THE PRICE ------------------------------------------------
// ⚠⚠ THE DEFECT WAS A HARDCODED WIDTH BESIDE A CONSTANT THAT SAID SOMETHING ELSE. `.g3ldpx` was
// emitted as `width:40px` while LAD_PXW read 30, so the price spanned PXC..PXC+40 and the bars
// started at LAD_NODE — measured 114..154 against a bar at 150, a 4px overlap on EVERY node row.
// The price is RIGHT-aligned, so those 4px land on the last digit. He circled it.
// This is landmine L-D verbatim: assert WIDTHS, not just offsets.
ok(/\.g3ldpx\{position:absolute;left:'\+LAD_PXC\+'px;width:'\+LAD_PXW\+'px/.test(src),
   'x1 the price column takes its width from LAD_PXW — no second, disagreeing number');
ok(!/\.g3ldpx\{[^}]*width:40px/.test(src), 'x1b ...and the hardcoded 40 is gone, not shadowed');
ok(PXC+PXW <= NODE-1,
   'x2 the price column ENDS before the bars begin — the whole point', {pxEnd:PXC+PXW, NODE});

// ---- 2 · THE NAME SITS BESIDE ITS PRICE, AND THE CHUTE IS PILLS ONLY -------------------------
// ⚠⚠ v15.09 PUT THE NAMES INSIDE THE CHUTE AND HE REJECTED IT: "move the level name like PDC next
// to the price levels instead so it will look like 'PDC 7741'". He was right — 100px separated a
// price from the name that belonged to it, and the pair could only be matched by tracking a row
// across the bars. The chute's own "price's alone" invariant, which v15.09 deliberately reversed,
// is therefore RESTORED: the names left, the strip is gone, and nothing but pills sits in there.
ok(v('LAD_LVSW')===undefined, 'x3 the chute sub-column strip is gone with the names it held');
ok(/\.g3ldlv\{position:absolute;left:'\+LAD_LVL\+'px;width:'\+LAD_LVLW\+'px;text-align:right/.test(src),
   'x4 the name is right-aligned in its own column, hard against the price');
ok(LVL+LVLW+4 >= PXC, 'x5 ...with no gap wide enough to read as a separate column',
   {nameEnd:LVL+LVLW, PXC});
ok(CH+CHW-PILLW-2 >= CH, 'x6b the pill zone is inside the chute', {pillStart:CH+CHW-PILLW-2, CH});
// ⚠⚠ (v15.09) x6/x7 FLIPPED FROM RIGHT-JUSTIFIED TO CENTRED, and the reason is the point: the
// pills were pushed right in v14.82 ONLY to clear the level names that then lived in the chute's
// left strip. v14.83 moved the names back out beside their prices and nobody moved the pills back —
// they spent two builds hugging a wall for a neighbour that no longer existed. Operator caught it:
// "justify center the price and kings in the column".
const CENTRE = "left:'+(LAD_CH+Math.round((LAD_CHW-LAD_PILLW)/2))+'px;width:'+LAD_PILLW+'px";
ok(src.indexOf('.g3ldnow{position:absolute;'+CENTRE)>-1,
   'x6 the price pill is CENTRED in the chute, not pinned to a wall');
ok(src.indexOf('.g3ldking{position:absolute;'+CENTRE)>-1,
   'x7 ...and the Kings share that exact zone, so the two can never disagree');
// ⚠ the EM pill is the third occupant and was the one left behind last time this zone moved.
ok(src.indexOf('.g3ldempill{position:absolute;'+CENTRE)>-1,
   'x7b ...and so does the EM pill — all three move together or one of them is visibly off-axis');
// centred means EQUAL air on both sides, which only holds if the arithmetic is symmetric
ok((CHW-PILLW)%2===0,
   'x7c the chute and pill widths leave an even gap, so "centred" is exact not off-by-one',
   {CHW, PILLW, gap:CHW-PILLW});
// ⚠ CENTRING THE BOX IS HALF THE JOB. The king pill is a flex row (crown + price + book letter); if
// its own content is not centred the pill sits centred while the text inside it hugs the left edge,
// which looks exactly like the bug we just fixed. Mutation caught this one surviving.
ok(/\.g3ldking\{[^}]*justify-content:center/.test(src),
   'x7d ...and the king pill centres its CONTENT too, not just its box');
// ⚠ THE CHUTE IS PRICE'S AGAIN. Nothing may be positioned inside its x-range but the pills.
ok(!/left:'\+\(LAD_CH\+2\)\+'px/.test(src),
   'x8 nothing is positioned at the chute\'s left wall any more — the invariant is restored');
ok(/text-overflow:ellipsis/.test(/\.g3ldlv\{[^}]*\}/.exec(src)[0]),
   'x9 a name too long for its column CLIPS rather than running over its price');
ok(NODE+NMAX <= CH-2, 'x10 the bars still stop before the chute', {barEnd:NODE+NMAX, CH});

// ---- 3 · ONE LETTER FOR THE BOOK -------------------------------------------------------------
// ⚠ SPXW and SPY BOTH START WITH S. A charAt(0) shortcut would print the same letter for two books,
// which is worse than no letter at all — the map is explicit for exactly that reason.
ok(/\{SPXW:'S', SPY:'Y', QQQ:'Q'\}/.test(LH), 'x11 the book tag is an EXPLICIT map, not charAt(0)');
ok(!/<i>'\+g3esc\(K\.book\)\+'<\/i>/.test(LH), 'x12 ...and the full book name no longer rides the pill');

// ---- 4 · TWO LEVELS ON ONE LINE BECOME ONE LINE ----------------------------------------------
// ⚠⚠ MERGE IN PIXELS, NOT POINTS. The old byRow keyed on price, so it merged only levels at the
// SAME price; IBH 7758 x PDH 7756 stayed two rows and drew on top of each other (measured 66x4px on
// his live panel). The same 2-point gap is 12px on a tight frame and 3px on a wide one, so points
// cannot answer "do these two texts touch".
ok(/MERGE_PX=\d+/.test(LH), 'x13 the merge threshold exists');
ok(/Math\.abs\(Y\(cur\.at\)-Y\(prv\.at\)\)>=MERGE_PX/.test(LH),
   'x14 ...and it measures RENDERED Y, not a price gap');
ok(/if\(cur\.host \|\| prv\.host\) continue;/.test(LH),
   'x15 a row snapped to a node is never absorbed — that would restate where the level is');
ok(/prv\.names=prv\.names\.concat\(cur\.names\)/.test(LH) && /prv\.tips=prv\.tips\.concat\(cur\.tips\)/.test(LH),
   'x16 both names AND both hovers survive the merge — "put both their names in the hoverover"');
ok(/keys\[mi\]=keys\[mi-1\]/.test(LH),
   'x17 the next row measures against the SURVIVOR, so three tight levels collapse to one, not two');

// ---- 5 · THE EXPIRED BAND IS NOT DRAWN -------------------------------------------------------
// He read the amber boundary line running through its own label as a strikethrough, and separately
// `.g3ahdim` really did strike the rail markers through. A mark whose meaning lives only in a hover
// is a puzzle. After hours the band is simply absent; the AFTER HOURS chip says why in words.
ok(/\(emAH\?\[\]:\[\['EH',EB\.high,RB\.over\],\['EL',EB\.low,RB\.under\]\]\)/.test(LH),
   'x18 after hours the ladder draws NO EM lines and NO EM pills');
ok(/var EL_LAB=AH\?'':\(/.test(src) && /var EH_LAB=AH\?'':\(/.test(src),
   'x19 ...and the rail markers are absent rather than struck through');
ok(!/g3ahdim/.test(src.replace(/^[ \t]*\/\/.*$/gm,'').replace(/'#gpts-body \.g3ahdim[^']*'/,'')),
   'x20 nothing still ASKS for the struck-through styling');
// ⚠ the MEASUREMENT must survive the removal of its drawing (v11.95).
ok(/function emBand/.test(src), 'x21 emBand() itself is untouched — a badge went, not a measurement');

console.log('test_ladder: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
