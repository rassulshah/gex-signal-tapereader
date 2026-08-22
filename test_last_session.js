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
    ok(/typeof inReplay==='function' && inReplay\(\)\) return/.test(b),
       'GUARD 2: '+fn+' writes NOTHING while replaying a past session');
    ok(/typeof inReplay/.test(b),
       '  ...and guards with typeof, so it cannot ReferenceError inside its own try/catch');
  });
  ok(/convertFiberCandles/.test(src) && /SESSION_DAY\s*=\s*pickSessionDay/.test(src),
     'the mode is decided in ONE place, where the candles are built');
}

// ---- GUARD 3: the face says so ----
{
  const f=ex('secFrame');
  ok(/g3replay/.test(f),                    'GUARD 3: the face carries a replay chip');
  ok(/REPLAY/.test(f),                      'and it says REPLAY in words, not a symbol alone');
  ok(/SESSION_DAY\.day/.test(f),            'and names WHICH session is on screen');
  ok(/g3replay[\s\S]{0,600}\} else \{[\s\S]{0,400}g3tag/.test(f),
     'it REPLACES the phase tag rather than sitting beside it — two chips would let the eye take the wrong one');
  ok(/#gpts-body \.g3replay\{/.test(src),   'the chip has its own style');
}
ok(/__gptsDebug\.session\s*=/.test(src),    'and a debug hook reports the mode and whether recording is on');
{
  const h=ex('inReplay');
  ok(/catch/.test(h) && /return false/.test(h),
     'inReplay FAILS TOWARD RECORDING — a silently dead recorder is worse than a lost session');
}

console.log((fail?'FAIL ':'')+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
