// ============================================================================================
// test_session_body.js — (v15.39) TWO CANDLES, ONE SESSION, AND THEY DISAGREED ABOUT ITS COLOUR.
//
// Operator, 2026-09-01: "look at the candles they look different in the app. in the now column you
// have a red candle and in the hod lod section you have a green candle."
//
// MEASURED ON HIS PANEL — both drawing 2026-09-01, both wrong to differ:
//     NOW column   open EB.open 7647.25   close LIVE tape   7644.25   -3.00   RED
//     ⓪a HOD/LOD   open hodLod  7647.00   close last bar    7647.50   +0.50   GREEN
// ⚠ The day was FLAT: +0.50 on a 52.25 range. The DISAGREEMENT was 6.5x the BODY it described.
// ⚠ And the panel was FROZEN ("frozen 2:59 pm") while the NOW candle tracked the after-hours tape.
//
// These assertions EXECUTE `sessionBody` against his exact numbers and prove the two surfaces
// cannot diverge — not by grepping for agreement, but by running the one function both now call.
// ============================================================================================
const fs=require('fs');
const src=fs.readFileSync('./current/gex-signal-tapereader.user.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,180):''));} };
function ex(n){ const m=new RegExp('function\\s+'+n+'\\s*\\(','g').exec(src);
  if(!m) throw new Error('not found: '+n);
  let i=src.indexOf('{',m.index),d=0; for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){ d--; if(!d) return src.slice(m.index,k+1); } } }

// ---- harness: hodLod and gdActual are the two inputs, and we control both --------------------
let HL=null, GD=null;
global.hodLod=()=>HL;
global.gdActual=()=>GD;
eval(ex('sessionBody'));

// his numbers, 2026-09-01
const TODAY = { ok:true, open:7647.00, hod:7673.75, lod:7621.50, scale:1 };

// ===== 1 · THE REPORTED CASE, EXECUTED =========================================================
HL=TODAY; GD={ now:7647.50 };
let S=sessionBody('SPY');
ok(!!S, 's1 EXECUTED: his session resolves');
ok(S.open===7647.00, 's1b open is the first RTH bar’s OPEN, not the band’s anchor', S.open);
ok(S.close===7647.50, 's1c close is the last CLOSED bar, not the live tick', S.close);
ok(Math.abs(S.pts-0.5)<1e-9 && S.up===true, 's1d the day is +0.50 and UP', [S.pts,S.up]);

// ⚠⚠ THE PROOF THE BUG IS GONE: the OLD inputs produced the opposite colour off the same session.
const OLD_ANCHOR=7647.25, OLD_LIVE=7644.25;
ok((OLD_LIVE>=OLD_ANCHOR)===false && S.up===true,
   's1e PROVEN: the retired inputs (anchor 7647.25 vs live 7644.25) give RED where the session is GREEN');
// ⚠ EXACTLY 6x, not "more than" — 3.00 / 0.50. My first cut wrote `>` and it failed on the real
// numbers, which is the assertion doing its job: I had rounded the ratio in my head.
ok(Math.abs(OLD_LIVE-OLD_ANCHOR)/Math.abs(S.pts)===6,
   's1f ...and the disagreement was 6x the body it described', Math.abs(OLD_LIVE-OLD_ANCHOR)/Math.abs(S.pts));

// ===== 2 · ONE CALL, SO THE TWO SURFACES CANNOT DIVERGE ========================================
// ⚠ Not "they agree" — they are THE SAME CALL. A test that compared two computations would pass
// the day someone re-derives one of them consistently-but-wrongly.
const dcs=ex('dayCandleSvg');
ok(/var SB=sessionBody\(sym\)/.test(dcs), 's2 the ⓪a candle reads sessionBody()');
ok(/var O=SB\.open, C=SB\.close/.test(dcs), 's2b ...for BOTH its open and its close');
ok(!/gdActual\(sym\)/.test(dcs), 's2c ...and no longer reaches for gdActual itself');
const now=(src.match(/var _SB=sessionBody\(sym\);[\s\S]{0,4000}?g3lddcb/)||[''])[0];
ok(now.length>0, 's3 the NOW-column candle reads sessionBody()');
// ⚠⚠ CONVERTED, and that qualifier is the whole lesson of v15.39c. `sessionBody()` reports BAR
// prices; every price on this rail is a bar price x `EB.emRr` (1.0031 measured 2026-08-31). v15.39b
// put bar prices on an EM rail and the real browser found it — the expected move collapsed to 1% of
// the view. THE COLOUR IS SCALE-INVARIANT; THE COORDINATES ARE NOT.
ok(/_SB\.open\*_rr/.test(now), 's3b ...for its open, CONVERTED into the rail\u2019s space');
ok(/_SB\.close\*_rr/.test(now) && /bodyB=Y\(_dC\)/.test(now),
   's3c ...AND for its close — the half that was the live tape');
ok(/var _rr=\(typeof EB\.emRr==='number' && EB\.emRr>0\)\?EB\.emRr:null/.test(now),
   's3b2 ...with the ratio taken from the band that owns the rail, never derived here');
ok(/if\(_SB && _rr &&/.test(now),
   's3b3 ...and NO ratio means NO candle \u2014 never a silent fallback to 1');
ok(/out\.emRr=useRr;/.test(src),
   's3b4 ...and emBand PUBLISHES that ratio, instead of applying it invisibly');
ok(/var up=_SB\.up/.test(now), 's3d ...and for its COLOUR');
ok(!/bodyB=tn/.test(now), 's3e ...the live-price body end is gone');
ok(!/typeof EB\.open==='number'\)\?EB\.open/.test(now), 's3f ...and the band anchor is gone');
// ⚠ the WICK stays on the band's own extremes — same rail, same space, no conversion needed
ok(/var _dH=\(typeof EB\.hiWater==='number'\)\?EB\.hiWater:null/.test(now),
   's3g the wick stays on EB.hiWater \u2014 already in this rail\u2019s space');

// ===== 3 · THE FREEZE — a session that ended must stop moving ==================================
// ⚠ gdActual reads measureBars, which the replay / frozen-book path already truncates. So a frozen
// day's close is the last bar IN THE TRUNCATED SERIES, and sessionBody needs no flag of its own.
// ⚠ THE BUG WAS THAT THE OTHER SURFACE NEVER ASKED: recorderBlind() guards every WRITE path, and
// this READ path was drawing the live after-hours tape onto a book frozen at 14:59.
HL=TODAY; GD={ now:7647.50 };
const frozen=sessionBody('SPY');
GD={ now:7644.25 };            // the after-hours tape, later
const live=sessionBody('SPY');
ok(frozen.close===7647.50 && live.close===7644.25,
   's4 EXECUTED: the close follows its SOURCE, so a truncated series ends the candle', [frozen.close,live.close]);
ok(frozen.up===true && live.up===false,
   's4b ...and that choice flips the colour — which is why the source has to be the frozen one', [frozen.up,live.up]);
ok(/measureBars/.test(ex('gdActual')),
   's4c ...and gdActual reads measureBars, the series the freeze truncates');

// ===== 4 · SCALE — the recurring failure of this file ==========================================
// ⚠ v15.24 put a stored ratio of 10.0353 against a series already at scale 1 and the ladder went
// blank. sessionBody must carry hodLod's OWN scale, never assume one.
HL={ ok:true, open:764.70, hod:767.375, lod:762.15, scale:10.0 }; GD={ now:764.75 };
S=sessionBody('SPY');
ok(S.open===7647 && S.close===7647.5, 's5 a cash-scale read is converted with hodLod’s OWN rr', [S.open,S.close]);
ok(S.scale===10.0, 's5b ...and the scale it used is reported, not hidden', S.scale);
HL={ ok:true, open:7647, hod:7673.75, lod:7621.5 }; GD={ now:7647.5 };   // scale absent
S=sessionBody('SPY');
ok(S.open===7647, 's5c a missing scale falls back to 1, it does not multiply by undefined', S.open);

// ===== 5 · FLATNESS IS REPORTED, NOT HIDDEN ====================================================
// ⚠ On his day the body was 1% of the range. A confident colour on a 1% body is a weak claim
// dressed as a strong one, and it is exactly when the input choice decides the answer.
HL=TODAY; GD={ now:7647.50 };
S=sessionBody('SPY');
ok(Math.abs(S.pctOfRange-0.5/52.25)<1e-9, 's6 the body is reported as a FRACTION OF THE RANGE', S.pctOfRange);
ok(S.pctOfRange<0.02, 's6b ...and his day lands under 2% — flat', S.pctOfRange);
ok(/A body this small is a FLAT day/.test(src), 's6c the tooltip says so when it is');
ok(/% of the day\\u2019s range/.test(src)||/% of the day’s range/.test(src),
   's6d ...and always states the figure, not only when small');

// ===== 6 · THE TOOLTIP NO LONGER ASSERTS AN INVARIANT IT CANNOT HOLD ===========================
// ⚠ The retired text: "The same numbers the ⓪a DAY section measures ... so the candle and the band
// can never describe different sessions." It said that while printing 7647->7644 against ⓪a's
// 7647.00->7647.50. A COMMENT CLAIMS; ONLY A SHARED FUNCTION GUARANTEES.
ok(!/can never describe different sessions/.test(src),
   's7 the false invariant claim is gone from the tooltip');
ok(/drawn from the SAME\s+.{0,20}sessionBody\(\) call/.test(src.replace(/\s+/g,' ')) ||
   /SAME sessionBody\(\) call/.test(src.replace(/\s+/g,' ')),
   's7b ...replaced by the mechanism that actually makes it true');
ok(/printed RED on a day/.test(src), 's7c ...and the tooltip records what went wrong, for him not me');

// ===== 7 · DEGRADES TO NOTHING, NEVER TO A GUESS ===============================================
HL=null; GD={ now:7647.5 };
ok(sessionBody('SPY')===null, 's8 no session read -> null, so the candle is ABSENT not invented');
HL=TODAY; GD=null;
ok(sessionBody('SPY')===null, 's8b no close -> null');
HL={ ok:false, open:7647 }; GD={ now:7647.5 };
ok(sessionBody('SPY')===null, 's8c a NOT-ok day read is refused');
HL=TODAY; GD={ now:'7647.5' };
ok(sessionBody('SPY')===null, 's8d a STRING close is refused, not concatenated');
ok(/if\(!SB\) return '';/.test(dcs), 's8e ...and ⓪a draws nothing rather than a partial candle');

// ⚠ THE CALL SITE MUST CONVERT TOO. `test_replay_face` c2e proves an unconverted body distorts the
// frame; this proves the one place that hands the body over does not do it. A wiring check is a
// grep and I would rather say so than dress it up — the CONSEQUENCE is executed next door.
const rbCall=(src.match(/var RB=emRailBounds\(EB, RAILPS,[\s\S]{0,400}?\}\)\(\)\);/)||[''])[0];
ok(rbCall.length>0, 's8f the frame call site is findable');
ok(/b\.hi\*EB\.emRr/.test(rbCall) && /b\.close\*EB\.emRr/.test(rbCall),
   's8g ...and it converts the body into the rail\u2019s space before handing it over');
ok(/!\(EB\.emRr>0\)\) return null/.test(rbCall),
   's8h ...and passes NOTHING rather than unconverted prices when the ratio is missing');

// ===== 8 · THE FRAME MUST HOLD WHAT WILL BE DRAWN ==============================================
// ⚠⚠ v15.39's first cut moved the CANDLE onto sessionBody() and left the FRAME on EB.hiWater —
// measured in the jsdom fixture, a 312px window onto a 300px frame, i.e. the window promising more
// than the frame held. `test_replay_face` y8g/y8h caught it, and `emRailBounds` now takes the
// candle's four prices. These assertions EXECUTE that guard rather than grepping for the argument.
eval(ex('emRailBounds'));
global.EM_RAIL_PAD=(function(){ const m=/var\s+EM_RAIL_PAD\s*=\s*([0-9.]+)/.exec(src); return m?parseFloat(m[1]):0.04; })();
const BAND={ low:7630, high:7660, now:7645, nowLive:7645, hiWater:7660, loWater:7630 };
let RB=emRailBounds(BAND, null, null);
ok(RB.hi<=7660.001 && RB.lo>=7629.999, 's9 baseline: with no candle the frame is the band', [RB.lo,RB.hi]);
// a session whose extremes run BEYOND what the band measured — the fixture's real situation
RB=emRailBounds(BAND, null, { hi:7690, lo:7600, open:7647, close:7647.5 });
ok(RB.hi>=7690, 's9b EXECUTED: the frame widens to hold the candle\u2019s HIGH', RB.hi);
ok(RB.lo<=7600, 's9c ...and its LOW', RB.lo);
// ⚠ AND THE BODY ENDS, which is the part a mutation showed was not load-bearing on today's data.
// A body outside its own wick is exactly the state v15.39 was fixing — the frame must still hold it
// rather than clip it, because clipping would hide the very disagreement we want visible.
RB=emRailBounds(BAND, null, { hi:7660, lo:7630, open:7647, close:7702 });
ok(RB.hi>=7702, 's9d ...and a CLOSE outside the wick still fits in the frame', RB.hi);
RB=emRailBounds(BAND, null, { hi:7660, lo:7630, open:7599, close:7647 });
ok(RB.lo<=7599, 's9e ...and an OPEN outside it', RB.lo);
// ⚠ and a junk body must not blow the frame open — null/NaN/0 are all reachable from a bad read
RB=emRailBounds(BAND, null, { hi:null, lo:NaN, open:0, close:undefined });
ok(RB.hi<=7660.001 && RB.lo>=7629.999,
   's9f a body of nulls, NaN and zero is IGNORED, not treated as a price', [RB.lo,RB.hi]);

console.log('test_session_body: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
