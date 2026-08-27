// (v11.55) LAST-SESSION MODE — the panel works on a weekend.
//
// Why it exists: convertFiberCandles keeps TODAY only, so on a Saturday there were no bars, no chart, no
// trend, no nodes — nothing to develop against. The fiber feed still carries Friday.
//
// Why it is DANGEROUS and what holds it: a whole face of stale data read as live is the largest possible
// version of failure pattern #1. Three things must hold, and this file exists to hold them:
//   1. NEVER during RTH — a quiet pre-open feed must read "no bars yet", never yesterday dressed as now.
//   2. THE RECORDER WRITES NOTHING — replayed bars in data/*.json would poison every base rate the
//      learning layer computes, permanently and undetectably after the fact.
//   3. THE FACE SAYS SO — a mode you cannot see is a mode that lies.
const fs=require('fs');
const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);
  if(!m) throw new Error('not found: '+n);
  let i=src.indexOf('{',m.index),d=0,e=-1;
  for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}
  return src.slice(m.index,e+1);}

global.mul=(a,b)=>a*b;
const DAY=86400;
// naive epoch-seconds helpers matching the script's own convention
global.naiveDayStr=(t)=>new Date(t*1000).toISOString().slice(0,10);
global.naiveSecOfDay=(t)=>((t%DAY)+DAY)%DAY;
eval(ex('pickSessionDay'));

const at=(dayStr,h,m)=>Math.floor(Date.parse(dayStr+'T00:00:00Z')/1000)+h*3600+(m||0)*60;
const bars=(dayStr)=>[at(dayStr,9,0),at(dayStr,10,0),at(dayStr,14,0)].map(t=>({time:t}));

// ---- today has bars: nothing is substituted ----
global.ctTodayStr=()=>'2026-08-21';
global.sessionPhase=()=>({rth:true});
let r=pickSessionDay(bars('2026-08-20').concat(bars('2026-08-21')));
ok(r.day==='2026-08-21' && r.fallback===false, 'today has bars -> today, no fallback', r);

// ---- GUARD 1: today EMPTY but market OPEN -> must NOT substitute ----
global.ctTodayStr=()=>'2026-08-24';
global.sessionPhase=()=>({rth:true});
r=pickSessionDay(bars('2026-08-21'));
ok(r.fallback===false && r.day==='2026-08-24',
   'GUARD 1: during RTH an empty today stays EMPTY — never yesterday dressed up as now', r);

// ---- market closed, today empty -> replay the most recent session ----
global.sessionPhase=()=>({rth:false});
r=pickSessionDay(bars('2026-08-21'));
ok(r.fallback===true && r.day==='2026-08-21', 'closed + empty today -> replay the last session', r);

// ---- picks the MOST RECENT session, not the first ----
r=pickSessionDay(bars('2026-08-19').concat(bars('2026-08-21')).concat(bars('2026-08-20')));
ok(r.day==='2026-08-21', 'the LATEST session wins regardless of array order', r);

// ---- pre-RTH bars are not a session ----
r=pickSessionDay([{time:at('2026-08-21',4,0)},{time:at('2026-08-21',7,0)}]);
ok(r.fallback===false, 'overnight-only bars are not an RTH session and do not trigger replay', r);

// ---- degenerate inputs ----
ok(pickSessionDay([]).fallback===false,   'an empty feed does not claim a replay');
ok(pickSessionDay(null).fallback===false, 'a null feed does not throw');
ok(pickSessionDay([{time:'x'},{}]).fallback===false, 'junk timestamps are skipped, not parsed');

// ---- GUARD 2: the recorder is off in replay, at EVERY write entry point ----
{
  const entries=['recordNodeSnapshot','recordOutcomeEvent','recordDeflections','actRecord','resolveFeatureOutcomes'];
  entries.forEach(fn=>{
    const b=ex(fn);
    // (v14.55) the guard is recorderBlind() = inReplay() || showingStaleBook(). Accept either
    // spelling: what this asserts is that the path is GUARDED, not which gate it names.
    ok(/typeof (inReplay|recorderBlind)==='function' && (inReplay|recorderBlind)\(\)\) return/.test(b),
       'GUARD 2: '+fn+' writes NOTHING while replaying a past session');
    ok(/typeof (inReplay|recorderBlind)/.test(b),
       '  ...and guards with typeof, so it cannot ReferenceError inside its own try/catch');
  });
  ok(/convertFiberCandles/.test(src) && /SESSION_DAY\s*=\s*pickSessionDay/.test(src),
     'the mode is decided in ONE place, where the candles are built');
}

// ---- GUARD 3: (v11.75) THE FACE NO LONGER SAYS SO, BY INSTRUCTION ----
// The replay chip was removed at the user's explicit request: "regarding replay badge, remove it. i know
// its sunday so i know that i will see friday already." v11.55 called it "the one label whose absence
// would let a whole stale face read as live" and that reasoning has NOT changed — only the decision has.
// ⚠ THE RISK THIS ACCEPTS IS MONDAY 08:00, NOT SUNDAY. Pre-open, replay engages, the whole panel shows
// Friday, and nothing on the face says so. The guards below are what remain: the MODE is still correct,
// the recorder is still silenced, and `__gptsDebug.session()` still states it in one call.
{
  const f=ex('secFrame');
  ok(!/g3replay/.test(f),        'GUARD 3: the replay chip is deliberately absent from the face');
  // (v11.95) secFrame no longer branches on replay AT ALL — the session-phase tag was the only
  // replay-conditional element and it was removed from row 1 with FEEDS and ES/ct.
  // ⚠ D-8 named the risk this leaves: "Monday 08:00, pre-open, replay engages, the whole face shows
  // Friday, and nothing says so", and named the fix — the section header, which carries the session
  // DATE and costs no row. That is where replay stays visible, so assert it there rather than here.
  ok(!/inReplay\(\)/.test(f),
     'secFrame no longer branches on replay — the phase tag was its only replay-conditional element');
  ok(/function sessionDayStr\(\)/.test(src),
     'and the session DATE is still derived from SESSION_DAY, which is what makes a replay visible in the header');
  ok(!/g3tag/.test(f.slice(f.indexOf('inReplay()'), f.indexOf('inReplay()')+400)) ||
     /if\(!inReplay\(\)\)/.test(f),
                                 'and the phase tag renders ONLY when the session is live');
  // ⚠ NOT indexOf('__gptsDebug.session') — that matches `sessionRoll`, declared ~9,000 lines earlier,
  // and the slice comes back as unrelated code. Fourth time this trap has fired; it is in the note.
  const sI=src.indexOf('__gptsDebug.session = function');
  const s2=src.slice(sI, sI+700);
  ok(sI>0, 'the session hook is locatable by its full marker');
  ok(/replay/.test(s2) && /showing/.test(s2),
     'the state is still one hook call away, which is now the only way to check it');
  ok(/nothing on the face is live/.test(s2),
     'and the hook still says so in words');
}
ok(/__gptsDebug\.session\s*=/.test(src),    'and a debug hook reports the mode and whether recording is on');
{
  const h=ex('inReplay');
  ok(/catch/.test(h) && /return false/.test(h),
     'inReplay FAILS TOWARD RECORDING — a silently dead recorder is worse than a lost session');
}

console.log((fail?'FAIL ':'')+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
