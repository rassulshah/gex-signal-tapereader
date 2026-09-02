// ============================================================================================
// test_em_pin_ruler.js — (v15.41) A REPLAY PIN OUTLIVED THE REPLAY AND FLATTENED THE LADDER.
//
// Operator, 2026-09-02: "i think you messed it up again."
//
// MEASURED on his LIVE panel, after he rewound and came back:
//     pin  SPY|fut = { openU: 761.79, rr: 1, fam:'replay', replay:true }
//     band  anchoredAt 761.79 · low 729.29 · high 794.29      <- SPY space
//     band  now 7647.50 · hiWater 7673.75 · loWater 7621.50   <- ES space
// `emRailBounds` starts the frame at `B.low`, so the rail spanned 729..7674 — seven THOUSAND points
// — and every row on the ladder collapsed onto one line at the top of the frame.
//
// ⚠⚠ TWO CORRECT RULES COMBINED INTO A TRAP. `replayEmPin()` (v15.24) writes a pin so the band
// survives a rewind. The v15.26 guard refuses to rebuild a REPLAY pin against a live series, for a
// reason that is still valid. NEITHER ASKED WHETHER THE REPLAY WAS STILL HAPPENING.
// ============================================================================================
const fs=require('fs');
const src=fs.readFileSync('./current/gex-signal-tapereader.user.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,200):''));} };

// the guard block, isolated so an assertion cannot pass on some other function's copy
// ⚠ the block runs from the replay-state read to the line that USES both guards; anchoring on the
// first `}` cut it short and two assertions passed on an incomplete slice — the same "assert on the
// statement, not the file" fault as v15.40, one scope smaller.
const blk=(src.match(/var _inRp=false;[\s\S]*?if\(rec && \(!_exempt[^\n]*/)||[''])[0];
ok(blk.length>0, 'e0 the pin-ruler guard is findable as one block');

// ===== 1 · THE EXEMPTION IS SCOPED TO ACTUALLY REPLAYING =======================================
ok(/var _exempt=rpPin\(rec\);/.test(blk),
   'e1 the rebuild asks the shared predicate, not its own copy of the condition');

// ===== 1b · ONE PREDICATE, FOUR HEALS ==========================================================
// ⚠⚠ FOUR heals wrote `!rec.replay` — none asked whether the replay was still happening, so a pin
// written during a rewind escaped EVERY one of them in live, forever. Written out four times the
// condition drifted four ways; restating instead of sharing is why this existed at all.
ok(/function rpPin\(r\)\{ return !!\(r && r\.replay && _inRp\); \}/.test(src),
   'e1b0 the predicate is defined once, and means "exempt RIGHT NOW"');
ok(/var _inRp=false; try\{ _inRp=\(typeof replayOn==='function' && replayOn\(\)\); \}/.test(src),
   'e1b1 ...off the LIVE replay state');
const heals=(src.match(/rpPin\(rec\)/g)||[]).length;
ok(heals===5, 'e1b2 ...and ALL FIVE heals call it', heals);
// ⚠⚠ THE FIFTH IS THE ONE THAT WRITES. The ratio heal had NO replay guard at all and does
// `S.sym[emKey]=rec` — so in replay it PERSISTED the in-memory replay pin into the LIVE key, where
// the four read-side heals were forbidden from repairing it. One unguarded WRITE, four over-guarded
// READS, and a rewind closes the loop.
const ratioHeal=(src.match(/if\(!rpPin\(rec\) && !\(rec&&rec\.replay\) &&[\s\S]{0,220}/)||[''])[0];
ok(ratioHeal.length>0, 'e1b2a the ratio heal now refuses a replay pin');
ok(/ratioSrc==='live'/.test(ratioHeal), 'e1b2b ...and still keeps its own live-ratio condition');
ok(/rec\.rr=rr; S\.sym\[emKey\]=rec;/.test(src),
   'e1b2c ...and it is indeed a WRITE path, which is why the guard belongs there');
ok(!/!rec\.replay/.test(src.replace(/\/\/[^\n]*/g,'')),
   'e1b3 ...with no unscoped `!rec.replay` left in executable code');
ok(!/if\(rec && !rec\.replay && \(rec\.src!==_srcNow/.test(src),
   'e1c the old unscoped exemption is gone from the rebuild condition');

// ===== 2 · AND A GUARD THAT NEEDS NO FLAG AT ALL ===============================================
// ⚠ Flags describe INTENT. This measures the SYMPTOM: an anchor and a price on one chart cannot be
// a factor of two apart. 761.79 against 7647.50 is a factor of TEN.
ok(/_rulerOff/.test(blk), 'e2 there is a ruler-mismatch guard');
ok(/_q>2 \|\| _q<0\.5/.test(blk), 'e2b ...tripping outside a factor of two', (blk.match(/_q>[^;]*/)||[])[0]);
ok(/if\(_rulerOff\) out\.rulerOff=true;/.test(blk),
   'e2c ...and it is DISCLOSED on the band, not silently corrected');
ok(/\(!_exempt \|\| _rulerOff\)/.test(blk),
   'e2d ...and it OVERRIDES the exemption — no flag protects a wrong ruler');

// ⚠ EXECUTED. The condition is arithmetic; run it on his real numbers and on the safe ones.
function rulerOff(anchor, now){ const q=anchor/now; return (q>2 || q<0.5); }
ok(rulerOff(761.7863196896488, 7647.5) === true,
   'e3 EXECUTED: his exact pair (761.79 vs 7647.50) trips the guard');
// ⚠ RAW, NOT SCALED. My first cut multiplied rec.openU by a ratio before comparing — which made a
// THIRD place that scales openU, and `test_em_band` has pinned that count at TWO since v15.12.
// It was right, and the correction is also the better test: scaling first would compare two numbers
// after applying the very ratio in doubt. Both are SERIES values; on one ruler they are comparable.
ok(/var _a=\(typeof rec\.openU==='number' && rec\.openU>0\)\?rec\.openU:null;/.test(blk),
   'e3a0 the guard reads rec.openU RAW');
ok(/var _n=\(typeof nowU==='number' && nowU>0\)\?nowU:null;/.test(blk),
   'e3a1 ...against nowU RAW, with no ratio applied to either');
ok(!/rec\.openU\*/.test(blk), 'e3a2 ...and scales neither, so openU still has exactly two scale sites');
ok(rulerOff(7647.25, 7647.5) === false,
   'e3b ...and a healthy same-scale pair does not');
ok(rulerOff(7614.75, 7647.5) === false,
   'e3c ...nor an anchor a full expected-move away');
// ⚠ the boundary, both sides, so "factor of two" is a fact rather than a hope
ok(rulerOff(1.99, 1) === false && rulerOff(2.01, 1) === true, 'e3d the upper bound is exactly 2x');
ok(rulerOff(0.51, 1) === false && rulerOff(0.49, 1) === true, 'e3e ...and the lower bound is 0.5x');
// ⚠ a 10x mismatch in EITHER direction — SPY-pin-on-ES and ES-pin-on-SPY are both reachable
ok(rulerOff(7647.5, 761.79) === true, 'e3f ...and it catches the mismatch pointing the other way');

// ===== 3 · THE REBUILD CLEARS THE POISON =======================================================
// ⚠ The rebuilt record is a FRESH object with no `replay` key, so a healed pin cannot re-trip the
// exemption next render. If it merged into the old record the flag would survive the cure.
const rebuild=(src.match(/rec=\{ em:_emNow[\s\S]{0,320}?\};/)||[''])[0];
ok(rebuild.length>0, 'e4 the rebuild is findable');
ok(!/replay/.test(rebuild), 'e4b the rebuilt pin carries NO replay flag — the cure is not partial');
ok(/rebuiltFrom:/.test(rebuild), 'e4c ...and records what it was rebuilt from');
ok(/openU:cs\[0\]\.o/.test(rebuild), 'e4d ...anchored on the CURRENT series’ first bar');
ok(/rr:rr/.test(rebuild), 'e4e ...and carries the CURRENT ruler, not the stored one');

// ===== 4 · THE FRAME IS WHY THIS MATTERED ======================================================
// ⚠ emRailBounds STARTS at B.low/B.high, so a band on the wrong ruler is not a cosmetic error —
// it sets the floor of the drawn frame and everything real is squeezed against the ceiling.
const erb=(src.match(/function emRailBounds[\s\S]{0,220}/)||[''])[0];
ok(/out=\{ lo:B\.low, hi:B\.high/.test(erb),
   'e5 the frame STARTS at the band edges — which is why a mis-scaled band flattens the ladder');
// his measured numbers: the span the ladder was asked to draw
const span=7673.75-729.29;
ok(span>6900, 'e5b EXECUTED: his frame spanned '+Math.round(span)+' points', Math.round(span));
ok((52.25/span)<0.01,
   'e5c ...so the day’s whole 52-point range occupied under 1% of it', +(52.25/span*100).toFixed(2)+'%');

console.log('test_em_pin_ruler: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
