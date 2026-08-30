// ============================================================================================
// test_rolllane.js — WHERE GAMMA LEFT, AND WHERE IT ARRIVED.
//
// Operator, 2026-08-30/31, with a sketch: "arrows that showed where the gamma was flowing out of
// and into ... stepped with a dot at the source ... i think they were animated also showing flow
// going from one node to another."
//
// ⚠⚠ THE TWO CAVEATS THIS FILE EXISTS TO PROTECT:
//  1. A ROLL IS INFERRED, NEVER OBSERVED. Nobody publishes that a position moved between strikes;
//     the panel pairs a fall at one node with a rise at another. That sentence has already been
//     nearly deleted once as a side effect of removing a drawing (v14.83, ROLL BIAS).
//  2. IT IS A RECORD OF WHAT MOVED, NOT A CLAIM THE GAINING NODE WILL HOLD. His claim is gx-004,
//     pre-registered 2026-08-29. Tested 2026-08-31 on the CORRECTED unit — a pullback being the
//     extreme of its own 30-minute neighbourhood, which he was right to insist on: the earlier
//     study counted every bar against every node and produced ~120 a day where a session has
//     THREE. 12 scorable pullbacks over 7 sessions, cells of 1-6. Unmeasurable. Say so.
// ============================================================================================
const fs=require('fs');
const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
const grab=(n)=>{ const i=src.indexOf('function '+n+'('); if(i<0) return ''; let d=0,st=false;
  for(let j=i;j<src.length;j++){ const c=src[j];
    if(c==='{'){d++;st=true;} else if(c==='}'){d--; if(st&&d===0) return src.slice(i,j+1);} } return ''; };
const RL=grab('ladderRollLane');
const RLC=RL.replace(/\/\/[^\n]*/g,'');

ok(!!RL, 'r1 the roll lane exists');
// ---- his sketch, literally: dot at source, out, across, back in with the head ----
ok(/'M'\+x0\+','\+ys[\s\S]{0,40}' H'\+xo\+' V'\+yd[\s\S]{0,30}' H'\+xi/.test(RLC),
   'r2 the path is STEPPED — out, across, back in — never diagonal');
ok(/<circle cx="'\+x0\+'"[\s\S]{0,80}r="2\.4"/.test(RLC), 'r3 a dot marks the SOURCE');
ok(/marker-end="url\(#g3rlh/.test(RLC), 'r4 ...and the head marks the DESTINATION');
ok(/xo=9\+\(n\*3\.5\)/.test(RLC), 'r5 steps NEST by depth, so two rolls never overlap');
// ---- live flows, latched sits still — existing doctrine ----
ok(/r\.live\?' g3ldflow':''/.test(RLC), 'r6 only a LIVE roll animates');
ok(/!r\.live && r\.ageMin!=null/.test(RLC), 'r7 ...and a latched one carries its age instead');
ok(/@keyframes g3ldflow\{to\{stroke-dashoffset:-18\}\}/.test(src),
   'r8 it reuses the panel\'s OWN flow animation, not a second one');
ok(/g3nomo \.g3ldrl/.test(src), 'r9 reduced motion stops the flow — the shape carries the meaning');
// ⚠ a stepped path MUST have fill:none or it fills as a wedge
ok(/\.g3ldrl\{fill:none/.test(src), 'r10 the stepped path is stroked, never filled');
// ---- placement: after ROC, so no label can be overwritten ----
// ⚠ 20px, not 44. `test_ladder` w1/w1b/w1c caught the first attempt at 48px extra — the guard
// exists precisely so a new column cannot widen the build unnoticed. It noticed; the lane was
// squeezed to land on w1's 640 ceiling instead.
ok(/LAD_ROLL=620, LAD_ROLLW=20/.test(src), 'r11 the lane sits after ROC — it cannot overwrite a label');
ok(/var LAD_W=640,/.test(src), 'r12 ...and LAD_W accounts for it rather than letting it overflow silently');
ok(/ladderRollLane\(ROLLS, Y, _dsc\)/.test(src), 'r13 it rides the SAME Y() the rows do');

// ---- THE CAVEATS ----
ok(/INFERRED from paired changes, never an observed transfer/.test(RL),
   'r14 the roll is declared an INFERENCE, in the words already used elsewhere');
ok(/RECORD OF WHAT MOVED, NOT A CLAIM/.test(RL),
   'r15 ...and it does not claim the gaining node will hold');
ok(/gx-004/.test(RL) && /150 sessions/.test(RL),
   'r16 ...naming the pre-registered hypothesis and the n it still needs');
ok(/12 pullbacks over 7 sessions/.test(RL),
   'r17 ...and the actual, underpowered evidence rather than a vague hedge');

console.log('test_rolllane: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
