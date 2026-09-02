// ============================================================================================
// test_purpose.js — (v15.45) THE APPLICATION'S PURPOSE, IN HIS WORDS, KEPT FINDABLE AND INTACT.
//
// Operator, 2026-09-02: "i wanted to explain the purpose of the application, which should guide you
// and future context and should be written somewhere to ensure you know it in future sessions. ...
// From this point on you should know what my intentions are, document them so you and your future
// context know and can help make the application better around these goals."
//
// ⚠⚠ WHY A TEST. This project's record on unenforced documents is unambiguous: the resume note went
// eleven builds stale while CHAT-HISTORY stayed current for exactly one reason — a test went red
// when it wasn't. A statement of PURPOSE is the last thing that should be allowed to rot, because
// every other document is judged against it.
// ============================================================================================
const fs=require('fs');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,180):''));} };

const P='./design/PURPOSE.md';
ok(fs.existsSync(P), 'p1 the purpose document exists');
const s=fs.existsSync(P)?fs.readFileSync(P,'utf8'):'';

// ---- 1 · THE OBJECTIVE, NOT PARAPHRASED -----------------------------------------------------
ok(/high of the day and low of day/i.test(s), 'p2 the PRIMARY objective is the HOD and the LOD');
ok(/profit from the move from high to low or low to high/i.test(s),
   'p2b ...and WHY: to trade the move between them');
ok(/pullback turning points/i.test(s), 'p3 the SECONDARY objective — pullback reversals — is recorded');
// ⚠ his exact words are quoted, because a paraphrase is where intent quietly drifts
ok(/^>/m.test(s) && /"?The purpose of the application is to be able to identify/i.test(s),
   'p3b ...and his statement is QUOTED, not summarised');

// ---- 2 · THE MECHANISM ----------------------------------------------------------------------
ok(/gamma node deflects price/i.test(s), 'p4 the causal claim: a gamma node deflects price');
ok(/deflection IS the turning point|deflection is the turning point/i.test(s),
   'p4b ...and the deflection IS the turning point');
// ⚠⚠ the two reversal kinds mean OPPOSITE trades — this distinction is the expensive one
ok(/TREND REVERSAL/.test(s) && /PULLBACK REVERSAL/.test(s), 'p5 both kinds of reversal are named');
ok(/trend CONTINUATION|continuation, not an end/i.test(s),
   'p5b ...and a pullback deflection means CONTINUATION, not an end');
ok(/expensive error/i.test(s), 'p5c ...with the cost of confusing them stated');

// ---- 3 · THE THREE SURFACES AND WHAT EACH IS FOR ---------------------------------------------
ok(/HOD\/LOD section/.test(s) && /measure the day/i.test(s), 'p6 ⓪a MEASURES the day');
ok(/node ladder/i.test(s) && /attracts/i.test(s) && /deflects/i.test(s),
   'p6b the ladder watches the king, which ATTRACTS as well as DEFLECTS');
// his stated column narrative, in his order
['WHERE gamma is rolling','HOW MUCH is moving','WHAT IT MEANS','AS A RATE'].forEach(w=>
  ok(s.indexOf(w)>=0, 'p7·'+w.slice(0,14)+' the column narrative names "'+w+'"'));
ok(s.indexOf('WHERE gamma is rolling')<s.indexOf('HOW MUCH is moving') &&
   s.indexOf('HOW MUCH is moving')<s.indexOf('WHAT IT MEANS') &&
   s.indexOf('WHAT IT MEANS')<s.indexOf('AS A RATE'),
   'p7b ...IN HIS ORDER — arrows, delta, state, roc');
ok(/Do not reorder these without asking him/i.test(s), 'p7c ...and says not to reorder them');
ok(/is a deflection being built right now/i.test(s),
   'p8 the columns exist to answer "is a deflection being BUILT" — the leading signal');

// ---- 4 · IT GOVERNS, AND IT ADMITS WHAT IT IS ------------------------------------------------
ok(/PURPOSE wins and the other is wrong|this file wins and the other one is wrong/i.test(s),
   'p9 it outranks the roadmap, DECISIONS and DEPENDENCIES when they conflict');
// ⚠ THE HONEST PART: the mechanism is a hypothesis he trades, not a proven law. A purpose document
// that asserted the claim would licence building surfaces that assume it and cannot be scored.
ok(/HYPOTHESIS HE IS TRADING|hypothesis he is trading/i.test(s),
   'p10 the mechanism is framed as a HYPOTHESIS, not a law');
ok(/checkable/i.test(s), 'p10b ...that the panel must keep CHECKABLE rather than assert');

// ---- 5 · REACHABLE — a purpose nobody reads is a purpose nobody has --------------------------
const skill=fs.readFileSync('./skills/gex/SKILL.md','utf8');
ok(skill.indexOf('design/PURPOSE.md')>=0, 'p11 `load gex` points a new context at it');
ok(/READ IT FIRST/i.test(skill), 'p11b ...and says to read it FIRST');
ok(skill.indexOf('1a-00')<skill.indexOf('1a-0.'), 'p11c ...ahead of the dependency briefing');
ok(/PURPOSE wins/i.test(skill), 'p11d ...and that it outranks the other documents');

console.log('test_purpose: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
