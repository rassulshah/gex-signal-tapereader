// ============================================================================================
// test_replay_guard.js — (v15.45) A REPLAY PARKED ON YESTERDAY SAT THROUGH FOUR HOURS OF A LIVE
// SESSION AND THE PANEL RECORDED NOTHING.
//
// Operator, 2026-09-02, market open: "are you recording .. market is open".
// MEASURED on his panel:
//     strip    "◀ Tue 1 Sep ▶ … 13:57 ↺ REPLAY"     parked on the PREVIOUS day
//     real     12:54 CT Wednesday, RTH, 2h+ in      (and 4h+ by the time it was fixed)
//     store    day 2026-09-02:  ZERO FRAMES
// recorderBlind() gates all nine write paths while replaying — correctly — so the entire morning
// was never captured. ⚠ THE BADGE SAID WHICH MODE. IT NEVER SAID THE COST.
// ⚠ Not persisted state: REPLAY lives in memory, so a reload would have cleared it. The tab simply
// never closed. This is a SESSION-BOUNDARY bug, not a storage one.
// ============================================================================================
const fs=require('fs');
const src=fs.readFileSync('./current/gex-signal-tapereader.user.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,200):''));} };
function ex(n){ const m=new RegExp('function\\s+'+n+'\\s*\\(','g').exec(src);
  if(!m) throw new Error('not found: '+n);
  let i=src.indexOf('{',m.index),d=0; for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){ d--; if(!d) return src.slice(m.index,k+1); } } }

// ---- harness -------------------------------------------------------------------------------
let REPLAY={ on:true, day:'2026-09-01', frames:[1,2], idx:1, err:null };
let TODAY='2026-09-02';
let PHASE={ rth:true, label:'MIDDAY', leftMin:125 };
let exited=0;
global.ctOffsetSec=()=>5*3600;
global.sessionDayStr=()=>TODAY;
// ⚠⚠ (v15.48) THE STUB NOW REFUSES WHAT THE REAL FUNCTION REFUSES. `sessionPhase(now)` does
// `new Date(now.toLocaleString(...))` — a NUMBER has toLocaleString, so 48000 became "48,000",
// `new Date("48,000")` was Invalid Date, and every field came back NaN WITHOUT THROWING. Both
// guards I shipped were inert for a build. A permissive stub is how that survived a test suite.
let PHASE_ARGS=[];
global.sessionPhase=(now)=>{
  PHASE_ARGS.push(now);
  if(!(now instanceof Date)) throw new TypeError('sessionPhase expects a Date, got '+typeof now);
  return PHASE;
};
eval("global.liveSessionPhase=function(){ try{ return sessionPhase(new Date()); }catch(e){ return null; } };");
global.replayExit=()=>{ exited++; REPLAY.on=false; REPLAY.frames=[]; REPLAY.idx=-1; };
Object.defineProperty(global,'REPLAY',{get:()=>REPLAY,set:v=>{REPLAY=v;},configurable:true});
let RP_STALEGUARD=0;
eval(ex('replayStaleDayGuard').replace('function replayStaleDayGuard','global.replayStaleDayGuard=function'));

function reset(o){ REPLAY={on:true, day:'2026-09-01', frames:[1,2], idx:1, err:null}; exited=0;
  RP_STALEGUARD=0; TODAY='2026-09-02'; PHASE={rth:true,label:'MIDDAY',leftMin:125}; Object.assign(REPLAY,o||{}); }

// ===== 1 · HIS EXACT SITUATION ================================================================
reset();
let was=replayStaleDayGuard();
ok(was==='2026-09-01', 'g1 EXECUTED: a replay of YESTERDAY during today’s RTH hands itself back', was);
ok(exited===1 && REPLAY.on===false, 'g1b ...by actually exiting, not merely reporting', {exited, on:REPLAY.on});

// ===== 2 · AND IT NEVER FIGHTS HIM ============================================================
// ⚠ ONCE PER DAY. A guard that yanked him out on every render would be worse than the bug — he must
// be able to re-enter a past day the moment it hands back.
REPLAY.on=true; REPLAY.day='2026-09-01';
ok(replayStaleDayGuard()===null, 'g2 it does NOT fire a second time the same day', exited);
ok(REPLAY.on===true, 'g2b ...so re-entering a past day is honoured immediately', REPLAY.on);
// a NEW day arms it again
TODAY='2026-09-03';
ok(replayStaleDayGuard()==='2026-09-01', 'g2c ...and a new session arms it once more');

// ===== 3 · REWINDING **TODAY** IS DELIBERATE AND MUST BE LEFT ALONE ============================
reset({ day:'2026-09-02' });
ok(replayStaleDayGuard()===null && REPLAY.on===true,
   'g3 rewinding TODAY mid-session is a choice — never touched', {on:REPLAY.on, exited});

// ===== 4 · OUTSIDE RTH NOTHING IS BEING MISSED =================================================
reset(); PHASE={ rth:false, label:'AFTER HOURS' };
ok(replayStaleDayGuard()===null && REPLAY.on===true,
   'g4 outside RTH it leaves him alone — no session is running to lose', {on:REPLAY.on});
reset(); PHASE={ rth:false, label:'CLOSED' };
ok(replayStaleDayGuard()===null, 'g4b ...including overnight, when studying a past day is the point');

// ===== 5 · NOT REPLAYING AT ALL ================================================================
reset({ on:false });
ok(replayStaleDayGuard()===null, 'g5 live already: nothing to do');
reset({ day:null });
ok(replayStaleDayGuard()===null, 'g5b a replay with no day resolves to nothing rather than throwing');

// ===== 6 · ⚠⚠ THE CLOCK IS THE WALL CLOCK ======================================================
// `sessionPhase()` with NO ARGUMENT follows the slider (v15.18). Parked on yesterday at 13:57 it
// returns rth:true for a session that ENDED — so the guard would have agreed with the very state it
// exists to detect, and the banner would show on a quiet evening and vanish during the live open.
// Same trap as v15.43's deps clock, two builds later.
const gfn=ex('replayStaleDayGuard');
ok(/liveSessionPhase\(\)/.test(gfn), 'g6 the guard asks for the LIVE phase');
ok(!/sessionPhase\(\)/.test(gfn), 'g6b ...and NEVER the bare replay-aware call');
// ⚠⚠ (v15.48) AND `liveSessionPhase` MUST HAND OVER A **DATE**. I passed seconds for a build and
// nothing threw: Number.prototype.toLocaleString made "48,000", `new Date("48,000")` was Invalid
// Date, and rth came back false in the middle of RTH — so this guard NEVER FIRED.
const lsp=ex('liveSessionPhase');
ok(/sessionPhase\(new Date\(\)\)/.test(lsp), 'g6c liveSessionPhase passes a real Date');
ok(!/ctOffsetSec/.test(lsp) && !/%86400/.test(lsp),
   'g6d ...with no hand-rolled clock arithmetic — new Date() IS the wall clock');
// EXECUTED against a stub that refuses a non-Date, exactly as the real function's behaviour implies
PHASE_ARGS=[]; reset(); replayStaleDayGuard();
ok(PHASE_ARGS.length>0 && PHASE_ARGS[0] instanceof Date,
   'g6e EXECUTED: what actually reaches sessionPhase is a Date', typeof PHASE_ARGS[0]);
// ⚠ the failure mode itself, run: a number does NOT throw, it silently becomes Invalid Date
ok(isNaN(new Date((48000).toLocaleString('en-US')).getTime()),
   'g6f ...and a NUMBER would have become Invalid Date without throwing — why this hid');
// ⚠⚠ AND IT MUST ACTUALLY RUN. A guard nobody calls is a guard that does not exist — this was the
// one mutation the first suite missed, and it is the mutation that reproduces the original bug.
const rnd=ex('render');
ok(/replayStaleDayGuard\(\)/.test(rnd), 'g7 render() CALLS the guard');
ok(/if\(_sg\) RP_STALEMSG=_sg/.test(rnd), 'g7b ...and records what it handed back, so the face can say so');
ok(rnd.indexOf('replayStaleDayGuard')<rnd.indexOf('RENDER_SEQ++'),
   'g7c ...before anything is drawn, so the first frame after the handback is already live');

// ===== 7 · THE BANNER — the badge said WHICH MODE, never THE COST =============================
const bar=ex('replayBarHtml');
ok(/NOT RECORDING/.test(bar), 'b1 replay during a live session says NOT RECORDING, in words');
ok(/every write path is blind while replaying|write path is blind/i.test(bar),
   'b1b ...and names the mechanism, so it is a fact rather than a scold');
// ⚠ ONE regex, not an alternation. My first cut ORed two patterns and the second kept passing when
// the banner lost its click handler — an `||` in an assertion is two chances to be satisfied by the
// half you did not break. Anchor on the div that must carry BOTH.
const banner=(bar.match(/h\+='<div data-grp="exit"[\s\S]{0,900}?CLICK HERE FOR LIVE\.<\/div>';/)||[''])[0];
ok(banner.length>0, 'b1c the banner div itself carries data-grp="exit" — the fix is where the warning is');
ok(/cursor:pointer/.test(banner), 'b1c2 ...and looks clickable');
ok(/rgba\(240,97,109/.test(bar), 'b1d ...in the panel’s own red, not another amber pill among a dozen');
// ⚠ the wall clock again — a banner on the replay clock would be exactly backwards
ok(/var _lp=liveSessionPhase\(\);/.test(bar),
   'b2 the banner is gated on the LIVE phase, not the parked minute');
ok(!/ctOffsetSec/.test(bar), 'b2a ...through the shared helper, with no clock arithmetic of its own');
ok(/if\(on && _lp && _lp\.rth\)/.test(bar), 'b2b ...and only while replaying during a live RTH session');
// and the handback announces itself rather than silently changing what he was looking at
// ⚠ gated on the flag, not merely present in the file — an `if(false)` around it left the string
// intact and my first assertion passed on the corpse.
const hand=(bar.match(/if\(RP_STALEMSG\)\{[\s\S]{0,900}?\n    \}/)||[''])[0];
ok(hand.length>0 && /RETURNED TO LIVE/.test(hand),
   'b3 an automatic handback SAYS it happened, gated on the flag the guard sets');
ok(/will not hand itself back a second time today/.test(bar),
   'b3b ...and tells him it will not do it again today');

console.log('test_replay_guard: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
