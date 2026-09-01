// ============================================================================================
// test_parked_specs.js — (v15.38) PARKED WORK MUST STAY FINDABLE, AND MUST STAY PARKED.
//
// Operator, 2026-09-01: "I want you to hold this implementation detail somewhere, maybe in a
// roadmap document. We will come back to it once the application with the current markets is
// optimal."
//
// ⚠⚠ WHY A TEST AND NOT JUST A FILE. This project's own record on unenforced documents is
// unambiguous: `latest-resume-note.md` went ELEVEN builds stale while `CHAT-HISTORY.md` stayed
// current for exactly one reason — a test went red when it wasn't. And v15.37 found ForexFactory
// undocumented for twenty-seven builds INSIDE the file whose §0 warns that undocumented
// dependencies fail silently. **Writing the warning is not obeying it.**
//
// ⚠ WHAT THIS FILE DEFENDS is not prose, it is the THREE MEASURED TRAPS. Each one would ship a
// wrong number, silently, and each was found by measurement rather than reasoning. If a future
// context deletes them to "tidy up", the research is gone and the trap comes back.
// ============================================================================================
const fs=require('fs');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,160):''));} };

const SPEC='./design/spec-futures-gamma-markets.md';
ok(fs.existsSync(SPEC), 'p1 the parked spec exists at the registered path');
const s=fs.existsSync(SPEC)?fs.readFileSync(SPEC,'utf8'):'';

// ---- 1 · IT IS PARKED, AND SAYS SO WHERE A READER CANNOT MISS IT ----------------------------
// ⚠ A spec that reads as a plan invites an unprompted build. He decides when this unparks.
ok(/^# SPEC \(PARKED\)/m.test(s), 'p2 the TITLE says PARKED — not a line buried on page three');
ok(/DO NOT START THIS BUILD/.test(s), 'p3 ...and it says so imperatively');
ok(/once the application with the current markets is optimal/.test(s),
   'p4 his own words are quoted, so the condition to unpark is his, not a later guess');

// ---- 2 · THE THREE MEASURED TRAPS ------------------------------------------------------------
// ⚠⚠ TRAP 1 — the obvious rule is the wrong one. "Front month" puts gold and copper on September.
ok(/\*1[\s\S]{0,80}NEAREST BY DATE/.test(s) && /\*0[\s\S]{0,80}MOST ACTIVE/.test(s),
   'p5 TRAP 1: the *0 / *1 distinction is stated');
['GCU26','GCZ26','HGU26','HGZ26','CLV26','NGV26','E6U26'].forEach(c=>
  ok(s.indexOf(c)>=0, 'p5·'+c+' ...with the measured contract code '+c));
ok(/GCU26[\s\S]{0,60}GCZ26/.test(s),
   'p5b ...shown SIDE BY SIDE, so the wrong answer is visible next to the right one');

// ⚠⚠ TRAP 2 — the root map cannot be derived. Copper changes letters; euro takes no suffix.
// ⚠ TWICE, DELIBERATELY: once as the market's own bold warning and once in the root table. My
// first cut asserted "the words CPE and HGE appear somewhere", and a mutation that deleted the
// bold warning still passed on the table row alone. An assertion satisfied by the survivor of the
// thing you deleted is not an assertion. (Caught by mutation, not by reading.)
ok(/\*\*THE DTN ROOT IS `CPE`, NOT `HGE`\.\*\*/.test(s),
   'p6 TRAP 2: copper\u2019s entry carries the bold CPE-not-HGE warning');
ok(/\|\s*CPE\s+\u26a0 NOT HGE|HG\s+CPE\s+\u26a0 NOT HGE/.test(s) || /CPE\s+\u26a0 NOT HGE/.test(s),
   'p6a ...and the root table repeats it where the table is read');
ok(/root is `E6`, NOT `6E`|`E6`, NOT `6E`/.test(s) || /E6`?, NOT.{0,6}`?6E/.test(s),
   'p6b ...and Barchart\'s euro root is E6, not 6E');
ok(/404/.test(s), 'p6c ...with the consequence named: 6EU26 returns a 404');
ok(/THE ROOT MAP IS A HAND-WRITTEN TABLE, NEVER DERIVED/.test(s),
   'p6d ...and the section states the rule as a rule');
ok(/derived from the exchange symbol breaks on copper and euro/.test(s),
   'p6e ...naming WHICH two markets break it \u2014 a rule with no case is a slogan');

// ⚠⚠ TRAP 3 — the chain is not in the HTML, which is the whole reason for the delivery design.
ok(/466,?227|466 ?KB/.test(s), 'p7 TRAP 3: the measurement is recorded, not paraphrased');
ok(/ZERO times|zero times/.test(s), 'p7b ...including the count that makes it conclusive');

// ---- 3 · THE INSIGHT THAT SIZES THE WHOLE BUILD ----------------------------------------------
// ⚠ Without this a future context builds a poller, which buys nothing and trips the throttle.
ok(/ONCE A DAY BY THE EXCHANGE/i.test(s), 'p8 open interest is published once a day — stated');
ok(/Do not build a poller|DO NOT BUILD A POLLER/i.test(s), 'p8b ...and the instruction it implies');

// ---- 4 · WHAT WAS RULED OUT, SO IT IS NOT RE-RESEARCHED ---------------------------------------
// ⚠ Half a day went into proving these negatives. An undocumented negative gets re-investigated.
ok(/zero snapshots/i.test(s) && /`ES1`|ES1/.test(s),
   'p9 Skylit has NO futures gamma — including ES — measured, not assumed');
ok(/InsiderFinance is equity\/ETF-only|equity\/ETF-only/.test(s), 'p9b ...nor InsiderFinance');
ok(/dropped|DROPPED/.test(s) && /not a fallback|NOT a fallback/i.test(s),
   'p9c the ETF conversion was dropped AND is explicitly not a fallback');
ok(/11\.0278/.test(s) && /0\.64103/.test(s),
   'p9d ...but the measured ratios survive, in case a conversion is ever needed again');

// ---- 5 · THE UNKNOWNS ARE LISTED --------------------------------------------------------------
// ⚠ A spec that lists only what is known reads as finished. These are the parts that are not.
ok(/UNKNOWNS, LISTED SO THEY ARE NOT DISCOVERED LATE/i.test(s), 'p10 the unknowns have their own section');
ok(/UNVERIFIED/.test(s), 'p10b ...and unverified claims are marked inline too');
ok(/weakest link/i.test(s),
   'p10c ...including the weakest link in the CHOSEN design, not just the rejected ones');

// ---- 6 · IT IS REACHABLE — a spec nobody finds is a spec nobody has -----------------------------
const skill=fs.readFileSync('./skills/gex/SKILL.md','utf8');
ok(skill.indexOf('design/spec-futures-gamma-markets.md')>=0,
   'p11 `load gex` points a new context at it');
ok(/NEVER re-research it/i.test(skill), 'p11b ...and tells it not to redo the research');
ok(/do not start it unprompted|Parked means parked/i.test(skill),
   'p11c ...and not to start it unprompted');
const road=fs.readFileSync('./roadmap/PRODUCT-ROADMAP.md','utf8');
ok(road.indexOf('design/spec-futures-gamma-markets.md')>=0, 'p12 the roadmap points at it');
ok(/PARKED — researched, specified, deliberately not started/.test(road),
   'p12b ...under a heading that says what parked means');
// ⚠ the board itself is eleven builds stale; saying so beats a stale board that reads as current
ok(/THIS BOARD IS STALE/.test(road) && /v10\.29/.test(road) && /v15\.3/.test(road),
   'p12c the roadmap discloses its OWN staleness, with both version numbers');

console.log('test_parked_specs: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
