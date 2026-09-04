// ============================================================================================
// test_roadmap.js — (v15.50) THE DEFLECTION ROADMAP STAYS HONEST ABOUT ITS OWN HEADLINE.
//
// Operator, 2026-09-02: "create a complete feature enhancement roadmap of what needs to be done."
// ⚠⚠ THE THING MOST AT RISK OF BEING EDITED AWAY IS THE UNCOMFORTABLE PART: the touch itself has
// NO EDGE (t=+0.41 / -0.32, both null, 56% break). A roadmap that quietly loses that line becomes a
// build list for a confident surface over a coin-flip signal — the exact failure PURPOSE §4.4 names.
// ============================================================================================
const fs=require('fs');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,160):''));} };
const P='./roadmap/DEFLECTION-ROADMAP.md';
ok(fs.existsSync(P), 'r1 the roadmap exists');
const s=fs.existsSync(P)?fs.readFileSync(P,'utf8'):'';

// ---- 1 · IT IS JUDGED AGAINST PURPOSE, NOT ITS OWN TASTE -------------------------------------
ok(/design\/PURPOSE\.md/.test(s), 'r2 it names PURPOSE.md as the standard');
ok(/helps call a deflection|helps judge one forming|helps score one afterwards/.test(s),
   'r2b ...and states the test a feature must pass');

// ---- 2 · THE SETTLED GEOMETRY IS CARRIED, NOT RE-DERIVED -------------------------------------
['DEFL_NEAR = 1.0 ATR','DEFL_THRU = 1.5 ATR','never the close','one price event = ONE deflection',
 '30-minute neighbourhood','TOP FEW NODES BY DOLLARS'].forEach(w=>
  ok(s.indexOf(w)>=0, 'r3·'+w.slice(0,22)+' the settled definition carries "'+w+'"'));
ok(/do not relitigate/i.test(s), 'r3b ...and says not to relitigate it');

// ---- 3 · ⚠ THE HEADLINE THAT MUST NOT BE SOFTENED --------------------------------------------
ok(/no edge/i.test(s), 'r4 THE TOUCH HAS NO EDGE is stated');
ok(/t = \+0\.41/.test(s) && /t = −0\.32|t = -0\.32/.test(s),
   'r4b ...with both t-statistics, so it is a measurement and not an opinion');
ok(/56% break/i.test(s), 'r4c ...and the break share');
ok(/mirror images/i.test(s), 'r4d ...and WHY they cancel');
ok(/Q11/.test(s) && /not built|NOT BUILT/.test(s), 'r5 Q11 is named as the product gap and unbuilt');
ok(/cosmetic until Q11/i.test(s),
   'r5b ...and everything downstream is called cosmetic until it is answered');

// ---- 4 · THE ORDER, AND THE REASON FOR IT ----------------------------------------------------
ok(/C DELIBERATELY FOLLOWS B/i.test(s), 'r6 the live surface deliberately comes AFTER the answer');
ok(/coin-flip|t = \+0\.41/.test(s.split('C DELIBERATELY FOLLOWS B')[1]||''),
   'r6b ...and the reason is the measurement, not taste');
ok(/THE EXCEPTION IS ITEM 1/i.test(s), 'r6c ...with the MARK correctness repair exempted');

// ---- 5 · IT NAMES WHAT ONLY HE CAN DECIDE ----------------------------------------------------
ok(/WHAT I WILL NOT SPECIFY WITHOUT HIM/i.test(s), 'r7 there is a section for his decisions');
ok(/re-arm distance/i.test(s), 'r7b ...including the re-arm distance');
ok(/must not be tuned to make a number look good/i.test(s),
   'r7c ...with the instruction not to tune it to flatter a result');
ok(/labelled exhaustively/i.test(s), 'r8 the exhaustively-labelled session is named as critical path');
ok(/precision is unmeasurable|precision has no denominator/i.test(s),
   'r8b ...and why: precision has no denominator without it');

// ---- 6 · ARCHIVE MEANS THE DISPLAY, NEVER THE MEASUREMENT ------------------------------------
ok(/Archive the DISPLAY, never the MEASUREMENT/i.test(s), 'r9 the v11.95 rule is carried into the cuts');
ok(/write-only/.test(s) && /shadowed/.test(s), 'r9b ...and the cuts cite the audit’s own findings');

// ---- 7 · THE FOUNDATION IS NOT HOUSEKEEPING ---------------------------------------------------
ok(/3\.44 ?MB/.test(s) && /3\.6 ?MB/.test(s), 'r10 the recorder ceiling is stated with numbers');
ok(/purpose-level gap, not housekeeping/i.test(s), 'r10b ...and framed as purpose-level');
ok(/ANSWERED \(0\)/.test(s) && /TESTING \(83\)/.test(s),
   'r11 the learning loop’s measured state — 0 of 83 — is stated');
ok(/never finished a lap|queue, not a loop/i.test(s), 'r11b ...and named for what it is');

// ---- 8 · THE CAVEAT SURVIVES ------------------------------------------------------------------
// ⚠ the roadmap must leave room for the answer "there is no edge here" WITHOUT that being a failure.
ok(/hypothesis he trades, not a proven law/i.test(s), 'r12 the mechanism stays a hypothesis');
ok(/A null result is an acceptable outcome and not a failure of the project/i.test(s),
   'r12b ...and a null result is explicitly an acceptable outcome');
ok(/the failure would be a\s*\n?panel that looks confident about it anyway/i.test(s.replace(/\s+/g,' ')) ||
   /failure would be a panel that looks confident/i.test(s.replace(/\s+/g,' ')),
   'r12c ...and names what the REAL failure would be instead');

// ---- 9 · REACHABLE ---------------------------------------------------------------------------
const skill=fs.readFileSync('./skills/gex/SKILL.md','utf8');
ok(skill.indexOf('roadmap/DEFLECTION-ROADMAP.md')>=0, 'r13 `load gex` points at it');
ok(/DO NOT BUILD THE LIVE DEFLECTION SURFACE FIRST/i.test(skill),
   'r13b ...and carries the sequencing warning where a new context will meet it');

console.log('test_roadmap: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
