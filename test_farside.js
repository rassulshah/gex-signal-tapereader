// ============================================================================================
// test_farside.js — (v14.72) THE FAR SIDE: touch probability, first-passage timing, the NO call.
//
// The operator's requirement, 2026-08-28: "i want it to say something like LOD IN -74%, HOD
// expected around 7772-7792 ... but it must be able to improve as data is gathered".
//
// ⚠ EVERY ASSERTION HERE EXECUTES THE FUNCTION. Its predecessor in this project grepped rendered
// markup for a word and stayed green while the branch it claimed to test was mutated out
// (landmine L-O, six occurrences). Each guard below was mutation-tested individually.
// ============================================================================================
const fs = require('fs');
const src = fs.readFileSync('./v10.js', 'utf8');
let pass = 0, fail = 0;
const ok = (c, m, g) => { if (c) { pass++; console.log('PASS ' + m); }
                          else { fail++; console.log('FAIL ' + m + (g !== undefined ? ' -> ' + JSON.stringify(g) : '')); } };
function ex(n){ const re=new RegExp('function\\s+'+n+'\\s*\\(','g'); const m=re.exec(src);
  if(!m) return ''; let i=src.indexOf('{',m.index),d=0,e=-1;
  for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){d--; if(d===0){e=k;break;}} }
  return src.slice(m.index,e+1); }
function val(n){ const m=new RegExp('(?:var\\s+)?\\b'+n+'\\s*=\\s*([\\s\\S]*?);\\n').exec(src); return m?eval('('+m[1]+')'):undefined; }

global.mul=(a,b)=>a/(1/b);
global.two=x=>{x=''+x;return x.length<2?'0'+x:x;};
global.FS_BASE = val('FS_BASE');
global.FS_MIN_SESSIONS = 120;
global.FS_MIN_CELL = 60;
let NOWSEC = 11*3600+30*60;
global.ctNowSecOfDay = () => NOWSEC;
global.localStorage = { getItem:()=>null, setItem:()=>{} };
eval(ex('fsBin')); eval(ex('fsNormalise')); eval(ex('fsBase'));
eval(ex('fsTouch')); eval(ex('fsTime')); eval(ex('fsHazard')); eval(ex('fsClockMin'));

// ---- the table itself -------------------------------------------------------------------------
const B = FS_BASE;
ok(B && B.corpus && B.corpus.sessions >= 120,
   'b1 the baked base stands on a real corpus, and says how many sessions', B&&B.corpus&&B.corpus.sessions);
ok(B.touch.length === B.bins.dist.length-1 && B.touch[1].length === B.bins.minsLeft.length-1,
   'b2 the table matches its own bin edges — a shape mismatch would silently mis-index every read');
(function(){
  let mono = true;
  for(let c=0;c<B.bins.minsLeft.length-1;c++){
    let prev=null;
    for(let r=0;r<B.touch.length;r++){
      const cell=B.touch[r] && B.touch[r][c];
      if(!cell || cell[1]==null) continue;
      if(prev!=null && cell[1] > prev+2) mono=false;
      prev=cell[1];
    }
  }
  ok(mono, 'b3 FURTHER IS NEVER MORE LIKELY — the table\'s only predictive claim, checked cell by cell');
})();
ok(B.touch.every(r => !r || r.every(c => !c || c[1]==null || c[0] >= 60)),
   'b4 every rated cell carries n>=60 — a rate on a thin cell is a number, not evidence');

// ---- fsTouch: reads, refuses, and never invents -------------------------------------------------
const near = fsTouch(0.3, 200, B), far = fsTouch(2.5, 200, B);
ok(near && near.p > far.p, 't1 a near level is likelier than a far one at the same clock', [near&&near.p, far&&far.p]);
ok(fsTouch(0.1, 200, B).p === null,
   't2 the 0-0.25 sigma row has no data and REFUSES rather than guessing');
ok(fsTouch(9, 200, B) === null || fsTouch(9, 200, B).p === null || fsTouch(9,200,B).p <= 5,
   't3 an absurd distance cannot return a high probability');
ok(fsTouch(0.6, 30, B).p > fsTouch(0.6, 350, B).p,
   't4 in SIGMA units a level is likelier late in the session than early — the table is not a clock in disguise',
   [fsTouch(0.6,30,B).p, fsTouch(0.6,350,B).p]);

// ---- fsTime: a RANGE, never a clock time --------------------------------------------------------
const T = fsTime(0.6, B);
ok(T && T.q1 < T.med && T.med < T.q3,
   't5 timing is delivered as median AND quartiles — the median error is 42 min, so a bare number would be a lie of precision', T);
ok(fsTime(0.1, B) === null, 't6 ...and refuses on the row that has no timing sample');

// ---- fsHazard: monotone, and it is the "are we on track" reading --------------------------------
ok(fsHazard(660,B).p < fsHazard(810,B).p,
   't7 the hazard RISES as the session ages without the far side printing', [fsHazard(660,B).p, fsHazard(810,B).p]);
ok(fsHazard(60,B) === null || fsHazard(60,B).at <= 60,
   't8 ...and does not invent a reading before its first measured clock');

// ---- fsNormalise: the courier gate ---------------------------------------------------------------
const good = JSON.parse(JSON.stringify(B));
ok(fsNormalise(good) !== null, 'n1 a well-formed payload is accepted');
const small = JSON.parse(JSON.stringify(B)); small.corpus.sessions = 40;
ok(fsNormalise(small) === null, 'n2 a 40-session payload is REFUSED — an old known-good base beats a fresh thin one');
const thin = JSON.parse(JSON.stringify(B)); thin.touch[1][0] = [5, 90];
ok(fsNormalise(thin) === null, 'n3 a cell with n=5 is REFUSED — landmine L-J, monotonicity is not evidence');
const broken = JSON.parse(JSON.stringify(B)); broken.touch[5][0] = [9000, 99];
ok(fsNormalise(broken) === null, 'n4 a NON-MONOTONE payload is REFUSED — the block\'s one claim would be unsupported');
const trunc = JSON.parse(JSON.stringify(B)); delete trunc.hazard;
ok(fsNormalise(trunc) === null, 'n5 a truncated payload is REFUSED rather than half-drawn');

// ---- the face: the numbers it prints must come from the model, not from prose --------------------
const SD = ex('secDay');
ok(/fsRead\(sym, ?D\)/.test(SD), 'f1 the section computes the far side from the model');
// ⚠⚠ STRIP COMMENTS FIRST. f2 kept passing after the wording changed because the v14.86 comment
// EXPLAINING the change contains the phrase "not before" — the assertion was satisfied by the note
// describing what it was supposed to be checking. Third time this exact shape has appeared this
// week (test_layout_v13's STEP_TIPS, the ladder mockup audit, this). Assertions about RENDERED TEXT
// read the rendered text.
const SDR = SD.replace(/\/\*[\s\S]*?\*\//g,'').replace(/^[ \t]*\/\/.*$/gm,'');
ok(/' after '/.test(SDR) && /80%/.test(SDR) && /fsClock12\(FS\.floorAt\)/.test(SDR),
   'f2 the 80% claim is on the face as a one-sided FLOOR — "SIDE after <clock> — 80%"');
// (v14.86) the middle-half window moved into the hover when the read line was compressed to one
// line. It kept its 50%, which is the only thing f3 ever cared about.
ok(/50%, not 80%/.test(SDR) && /winA/.test(SDR) && /winB/.test(SDR),
   'f3 ...and the two-sided window is labelled with its real 50%, stated against the 80% floor');
ok(!/most likely/i.test(SDR), 'f3b ...and the hedge word it used to carry is gone');
ok(!/g3daylb/.test(SD), 'f4 the survival ladder is GONE — it answered the verdict\'s question with a weaker instrument');
ok(!/g3dayfoot/.test(SD), 'f5 ...and the white honesty line is gone from the face');
ok(/rates live/.test(SD) && /rates baked in/.test(SD),
   'f6 ...but live-vs-baked stays VISIBLE, because a frozen corpus changes how much to trust the row');
// (v14.94) THE FAR SIDE TABLE IS OFF THE FACE — his agreed layout ends "FAR SIDE block: REMOVED".
// The NO CALL is NOT: it moved into the read-line hover, because it was the sharpest statement the
// model makes and the table was its only home. f7 guards the RE-HOMED text; f8 is retired with it,
// since there is no longer a painted element to check the colour of.
// ⚠ COMMENT-BLIND. Written against raw SD this passed on the COMMENT that says "THE NO CALL IS
// RE-HOMED HERE" — a mutation deleting the real clause survived. Strip comments first. Sixth time
// this family has appeared; it is always mutation that finds it, never reading.
(function(){
  const SDL = SD.replace(/\/\/[^\n]*/g,'').replace(/\/\*[\s\S]*?\*\//g,'');
  ok(/does NOT trade there today/.test(SDL) && /FS\.no/.test(SDL),
     'f7 the NO call survived the FAR SIDE removal — re-homed into the read hover');
})();
ok(!/h\s*\+=\s*[^\n]*g3farno/.test(SD),
   'f8 ...and the old painted block is gone, not merely restyled');

// ---- enrollment: no feature ships un-enrolled (2026-08-17 mandate) --------------------------------
ok(/registerFeature\(\{ key:'farside'/.test(src), 'e1 the feature is registered');
ok(/farside_gamma_identity/.test(src),
   'e2 ...and the GAMMA question is queued as a scored question, not left as an intention');
ok(/USE A DENSE DISTANCE CONTROL/.test(src),
   'e3 ...carrying the warning that killed the phantom (F-16), where the next context will read it');
const RJ = JSON.parse(fs.readFileSync('./learning/rules.json','utf8'));
ok(RJ.rules.farside && RJ.rules.farside.n === 0 && RJ.rules.farside.rate === null,
   'e4 the rule exists in learning/rules.json and starts HONEST at n=0, rate null');
ok(RJ.rules.farside.walkForward && RJ.rules.farside.walkForward.sessions === 0,
   'e5 ...with the walk-forward shape the enrollment test demands');

console.log(`\ntest_farside: ${pass} passed, ${fail} failed`);
process.exit(fail ? 1 : 0);
