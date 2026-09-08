// ============================================================================================
// test_v1575.js — (v15.75) THE CLOSED STATE (R-12). Operator, 2026-09-07 (Labor Day), at his panel: "the application
//   doesn't seem to support a frozen state from when it was open so i can really work on it … there is no node
//   ladder". Measured on his panel that morning: every feed arriving, the clock past 08:30, the day line "recording ·
//   0 bars", the face EMPTY, and a drag on Friday's strip handed straight back to LIVE by the stale-day guard.
//   The session signal is the gamma payload's own minute series (levels[].t — 390 minutes of the LAST session), not
//   the clock. Functions run with stubs; the face renders in the harness (render-face.js --pre); GPTS_SRC points the
//   harness at a mutated copy.
// ============================================================================================
const fs=require('fs');
const SRC=process.env.GPTS_SRC||'./current/gex-signal-tapereader.user.js';
const src=fs.readFileSync(SRC,'utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,300):''));} };
function ex(n){ const m=new RegExp('function\\s+'+n+'\\s*\\(','g').exec(src); if(!m) throw new Error('not found: '+n);
  let i=m.index, d=0, started=false; for(; i<src.length; i++){ const c=src[i]; if(c==='{'){d++;started=true;} else if(c==='}'){d--; if(started&&d===0){i++;break;}} } return src.slice(m.index,i); }
const exVar=(n)=>{ const i=src.indexOf('var '+n+'='); if(i<0) throw new Error('no var '+n); const j=src.indexOf(';\n', i); return src.slice(i, j+1); };
const exVarLine=(n)=>{ const i=src.indexOf('var '+n+'='); if(i<0) throw new Error('no var '+n); const j=src.indexOf(';', i); return src.slice(i, j+1); };

ok(/@version\s+15\.(7[5-9]|[89]\d)/.test(src) && /var GPTS_VERSION='15\.(7[5-9]|[89]\d)';/.test(src),'0a v15.75 or later in both spots');

// the CT clock: 2026-09-07 (Labor Day, a Monday). A feed whose newest minute is Friday 14:59 CT = 2026-09-04T19:59:00Z.
const FRI_LAST=1788551940, MON_0831=1788784260;   // 2026-09-07T13:31:00Z
const feed=(t)=>({ j:{ success:true, levels:[{t:t-60,s:770,l:1},{t:t,s:771,l:2},{t:t-120,s:769,l:3}] }, ts:Date.now() });
const ctDateStr=(d)=>{ d=d||new Date(); const c=new Date(d.getTime()-5*3600000); return c.getUTCFullYear()+'-'+String(c.getUTCMonth()+1).padStart(2,'0')+'-'+String(c.getUTCDate()).padStart(2,'0'); };
const FNS=['feedNewestT','bookArrived','liveBookToday','closedDay','closedState','replayAutoPark','replayAutoRelease','replayExit','replayOn','replayFrame','replaySec','replayDayLabel','replaySecOf','closedChipHtml','replayStaleDayGuard'];
function world(o){
  o=o||{};
  const calls={ loads:[], renders:0, exits:0 };
  const g={
    LASTFEED:o.LASTFEED||{ SPY:feed(FRI_LAST) },
    ctTodayStr:()=>o.today||'2026-09-07', ctDateStr,
    liveSessionPhase:()=>o.phase||{ rth:true, dow:1, mins:8*60+40, open:8*60+30, close:15*60, label:'RTH' },
    sessionDayStr:()=>o.today||'2026-09-07',
    replayLoadDay:(day,cb)=>{ calls.loads.push(day); if(o.load) o.load(day,cb); },
    render:()=>{ calls.renders++; },
    hlClock:(s)=>String(Math.floor(s/3600)).padStart(2,'0')+':'+String(Math.floor((s%3600)/60)).padStart(2,'0'),
    g3tip:(t)=>' title="'+String(t).replace(/"/g,'&quot;')+'"', g3esc:(s)=>String(s==null?'':s).replace(/</g,'&lt;'),
    calls
  };
  const pre=[exVar('REPLAY'), 'var RP_STALEGUARD=0; var RP_STALEMSG=null;', exVarLine('RP_AUTO_OFF'), exVarLine('RP_AUTO_SKIP')].join('\n');
  const T=new Function('__g', Object.keys(g).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+pre+'\n'+FNS.map(ex).join('\n')+
    '\nvar __exit=replayExit; replayExit=function(){ __g.calls.exits++; return __exit(); };'+
    '\nif(__g.__replay) Object.assign(REPLAY, __g.__replay); if(__g.__autoOff!==undefined) RP_AUTO_OFF=__g.__autoOff; if(__g.__skip) Object.assign(RP_AUTO_SKIP, __g.__skip);'+
    '\nreturn { live:liveBookToday, arrived:bookArrived, day:closedDay, state:closedState, park:replayAutoPark, release:replayAutoRelease, guard:replayStaleDayGuard, chip:closedChipHtml, R:()=>REPLAY, off:()=>RP_AUTO_OFF, skip:()=>RP_AUTO_SKIP, latch:()=>RP_STALEGUARD };')(Object.assign(g,{ __replay:o.replay, __autoOff:o.autoOff, __skip:o.skip }));
  T.calls=calls; return T;
}
const DAYS=['2026-09-02','2026-09-03','2026-09-04','2026-09-06'];   // the store on his machine: Friday and a Sunday evening
const frames=[{t:1788551040},{t:1788551880},{t:1788551940}].map(f=>({ t:f.t*1000, tri:{}, vend:{rows:[1]}, px:770 }));

// ---- 1 · the session signal is the book's own newest minute ---------------------------------------------------------
{
  const T=world();
  ok(T.arrived()===true && T.live()===false,'1a Labor Day morning: the SPY feed has arrived and its newest minute is Friday 14:59 CT — no live book today',[T.arrived(),T.live()]);
  ok(world({ LASTFEED:{ SPY:feed(MON_0831) } }).live()===true,'1b a feed whose newest minute is today 08:31 CT is a live book');
  ok(world({ LASTFEED:{ SPY:feed(FRI_LAST), QQQ:feed(MON_0831) } }).live()===true,'1c either book counts');
  ok(world({ LASTFEED:{} }).arrived()===false && world({ LASTFEED:{} }).live()===false,'1d no feed yet: nothing has arrived, nothing is live');
  ok(world({ LASTFEED:{ SPY:{ j:{ levels:[] }, ts:1 } } }).live()===false && world({ LASTFEED:{ SPY:{ j:{ levels:[{t:'x'}] }, ts:1 } } }).live()===false,'1e an empty or garbage series is not a live book, and never throws');
  // 2026-09-04 22:00 CT = 2026-09-05T03:00Z: the CT day, not the UTC day, decides
  ok(world({ LASTFEED:{ SPY:feed(1788577200) }, today:'2026-09-04' }).live()===true && world({ LASTFEED:{ SPY:feed(1788577200) }, today:'2026-09-05' }).live()===false,'1f the day is the CT day (a 22:00 CT minute belongs to that CT day, not to the next UTC day)');
}

// ---- 2 · the closed state and the day it stands on --------------------------------------------------------------------
{
  const T=world({ replay:{ days:DAYS } });
  const S=T.state();
  ok(S.closed===true && S.day==='2026-09-04' && S.why==='no session so far today','2a Labor Day 08:40 CT: closed, standing on Friday — the Sunday-evening store is skipped',S);
  ok(world({ replay:{ days:DAYS }, phase:{ rth:false, dow:6, mins:600, open:510, close:900 }, today:'2026-09-05' }).state().why==='weekend','2b Saturday says weekend');
  ok(world({ replay:{ days:['2026-09-03','2026-09-04'] }, phase:{ rth:false, dow:2, mins:7*60+50, open:8*60+30, close:15*60 }, today:'2026-09-08' }).state().why==='before the open','2c Tuesday 07:50: before the open');
  ok(world({ LASTFEED:{ SPY:feed(MON_0831) }, replay:{ days:DAYS } }).state().closed===false,'2d a live book today: not closed');
  ok(world({ LASTFEED:{}, replay:{ days:DAYS } }).state().closed===false,'2e no book yet: not closed (the panel cannot know), no flash of Friday at page load before the feed');
  ok(world({ replay:{ days:[] } }).state().closed===false && world({ replay:{ days:null } }).state().closed===false,'2f nothing recorded: nothing to stand on');
  ok(world({ replay:{ days:DAYS.concat(['2026-09-07']) } }).state().closed===false,'2g today already recorded (after the close on a trading day): not closed — AFTER HOURS as before');
  ok(world({ replay:{ days:DAYS }, skip:{ '2026-09-04':true } }).state().day==='2026-09-03','2h a day that turned out empty is skipped: the one before it');
  ok(world({ replay:{ days:['2026-09-05','2026-09-06'] } }).state().closed===false,'2i only weekend days in the store: nothing to stand on');
}

// ---- 3 · the auto-park: once, on the last frame, blind, and released by the live book ---------------------------------
{
  const T=world({ replay:{ days:DAYS, day:'2026-09-04', frames:frames.slice(), idx:0 } });
  ok(T.park()===true && T.R().on===true && T.R().auto===true && T.R().idx===2 && T.calls.loads.length===0,'3a frames already loaded: parked on the LAST frame, auto, no load',[T.R().on,T.R().auto,T.R().idx]);
  ok(T.park()===false,'3b a second tick does not park again');
  ok(T.release()===false && T.R().on===true,'3c no live book: not released');
  // Friday's frames still in memory; today's first minute arrives
  const T2=world({ LASTFEED:{ SPY:feed(MON_0831) }, replay:{ days:DAYS, day:'2026-09-04', frames:frames.slice(), idx:2, on:true, auto:true } });
  ok(T2.release()===true && T2.R().on===false && T2.R().auto===false && T2.calls.exits===1 && T2.calls.renders===1,'3d the live book releases the auto-park by itself (replayExit, one render), no banner state set',[T2.R().on,T2.calls]);
  const T3=world({ LASTFEED:{ SPY:feed(MON_0831) }, replay:{ days:DAYS, day:'2026-09-04', frames:frames.slice(), idx:1, on:true, auto:false } });
  ok(T3.release()===false && T3.R().on===true,'3e his own replay is never released by this (the guard is the one that may)');
  const T4=world({ replay:{ days:DAYS, day:'2026-09-04', frames:frames.slice(), idx:1, on:true, auto:false } });
  ok(T4.park()===false && T4.R().auto===false,'3f his own replay stands: no auto-park over it');
  const T5=world({ replay:{ days:DAYS, day:'2026-09-04', frames:frames.slice(), idx:0 }, autoOff:'2026-09-07' });
  ok(T5.park()===false && T5.R().on===false,'3g he clicked LIVE today: no re-park today');
  const T6=world({ replay:{ days:DAYS, day:'2026-09-04', frames:frames.slice(), idx:0 }, autoOff:'2026-09-06' });
  ok(T6.park()===true,'3h …but yesterday\'s LIVE click does not carry');
  // frames not loaded: the loader is asked, and parks in its callback
  const T7=world({ replay:{ days:DAYS, day:'2026-09-06', frames:[] }, load:(day,cb)=>{ /* the loader fills REPLAY */ } });
  ok(T7.park()===true && T7.calls.loads.join()==='2026-09-04' && T7.R().on===false,'3i frames not loaded: asks the loader for Friday, parks nothing until it answers',[T7.calls.loads,T7.R().on]);
  const T8=world({ replay:{ days:DAYS, day:'2026-09-06', frames:[] }, load:(day,cb)=>{ const R=T8.R(); R.day=day; R.frames=frames.slice(); R.idx=2; cb(3); } });
  ok(T8.park()===true && T8.R().on===true && T8.R().auto===true && T8.R().idx===2 && T8.calls.renders===1,'3j …and parks on the last frame when the loader answers',[T8.R().on,T8.R().auto]);
  const T9=world({ replay:{ days:DAYS, day:'2026-09-06', frames:[] }, load:(day,cb)=>{ cb(0); } });
  ok(T9.park()===true && T9.R().on===false && T9.skip()['2026-09-04']===true && T9.state().day==='2026-09-03','3k a day with no RTH frames is marked and the next tick stands on the day before it',[T9.skip(),T9.state()]);
  const T10=world({ replay:{ days:DAYS, day:'2026-09-04', frames:frames.slice(), idx:0, loading:true } });
  ok(T10.park()===false,'3l a load in flight: wait');
}

// ---- 4 · the stale-day guard: a holiday has the clock and no session ---------------------------------------------------
{
  const T=world({ replay:{ days:DAYS, day:'2026-09-04', frames:frames.slice(), idx:2, on:true, auto:false } });
  ok(T.guard()===null && T.R().on===true && T.latch()===0,'4a Labor Day, RTH by the clock, HIS replay of Friday: NOT evicted — no live book, nothing is being missed',[T.guard(),T.R().on]);
  const T2=world({ LASTFEED:{ SPY:feed(MON_0831) }, replay:{ days:DAYS, day:'2026-09-04', frames:frames.slice(), idx:2, on:true, auto:false } });
  ok(T2.guard()==='2026-09-04' && T2.R().on===false && T2.latch()==='2026-09-07','4b a trading day with the live book: his replay of Friday IS handed back once, as before (v15.45)',[T2.R().on,T2.latch()]);
  const T3=world({ LASTFEED:{ SPY:feed(MON_0831) }, replay:{ days:DAYS, day:'2026-09-04', frames:frames.slice(), idx:2, on:true, auto:true } });
  ok(T3.guard()===null && T3.R().on===true,'4c an auto-park is never the guard\'s business (it releases itself)');
}

// ---- 5 · the face: the CLOSED bar, the badge, no NOT RECORDING, the day line -------------------------------------------
{
  const T=world({ replay:{ days:DAYS, day:'2026-09-04', frames:frames.slice(), idx:2, on:true, auto:true } });
  const h=T.chip();
  ok(/^<div class="g3closed" title="CLOSED — the market has not opened today \(no session so far today\)/.test(h) && /CLOSED · showing Fri 4 Sep at 14:59 — no session so far today · live resumes with today’s first book<\/div>$/.test(h),'5a the CLOSED bar: what is shown, why, what releases it',h.slice(0,160));
  ok(world({ replay:{ days:DAYS, day:'2026-09-04', frames:frames.slice(), idx:2, on:true, auto:false } }).chip()==='' && world({ replay:{ days:DAYS } }).chip()==='','5b no bar for his own replay, none when live');
  ok(/if\(on && REPLAY\.auto\)\{[\s\S]{0,400}\\u23f8 CLOSED<\/span>';\s*\} else if\(on\)\{/.test(src),'5c the strip badge says ⏸ CLOSED for an auto-park, ↺ REPLAY for his');
  ok(/if\(on && !REPLAY\.auto && _lp && _lp\.rth && liveBookToday\(\)\)\{/.test(src),'5d the NOT RECORDING banner is his replay during a LIVE session — never the closed state, never a holiday');
  const i1=src.indexOf("try{ html+=closedChipHtml(); }catch(eCC0){}"), i2=src.indexOf("try{ html+=dayLineHtml(); }catch(eDL0){}"), i3=src.indexOf("try{ html+=closedChipHtml(); }catch(eCC){}"), i4=src.indexOf("try{ html+=dayLineHtml(); }catch(eDL){}");
  ok(i1>0 && i2>i1 && i3>i2 && i4>i3,'5e render(): the CLOSED bar sits right above the day line, in both assembly paths',[i1,i2,i3,i4]);
  ok(/#gpts-body \.g3closed\{display:block;[^}]*color:#7cc7ff;[^}]*cursor:help\}/.test(src),'5f the CSS: a full-width blue bar, the hover carries the explanation');
  ok(/try\{ if\(!replayAutoRelease\(\)\) replayAutoPark\(\); \}catch\(eCS\)\{\}/.test(src) && src.indexOf('replayAutoRelease()) replayAutoPark()')<src.indexOf('if(!pastReset()){ try{ futModeRefresh(); }catch(eF0){} render(); return; }'),'5g tick(): release-or-park runs every tick, BEFORE the 08:30 gate (a weekend morning parks too)');
  ok(/REPLAY\.auto=false;\s*\/\/ \(v15\.75\) his hand on the strip makes it his replay/.test(src) && /if\(w==='exit'\)\{ ev\.stopPropagation\(\); if\(REPLAY\.auto\)\{ try\{ RP_AUTO_OFF=ctTodayStr\(\); \}catch\(eAO\)\{\} \} replayExit\(\); return; \}/.test(src) && /if\(w==='prev'\)\{ ev\.stopPropagation\(\); REPLAY\.auto=false;/.test(src),'5h the strip: a drag or ◀ ▶ makes it his replay; LIVE out of an auto-park stands for the day');
  ok(/function replayExit\(\)\{\s*REPLAY\.on=false; REPLAY\.auto=false;/.test(src),'5i replayExit clears auto');
}

// ---- 6 · the day line on a holiday: yesterday\'s completed line, not "recording · 0 bars" --------------------------------
{
  const gl={ ctTodayStr:()=>'2026-09-07', ctMarketHours:()=>true, ctAfterClose:()=>false, ctNowSecOfDay:()=>8*3600+40*60, liveBookToday:()=>false,
    saveState:()=>({code:'none'}), pipeLoad:()=>({ saveDate:'2026-09-04', saveHow:'repo folder', saveT:Date.parse('2026-09-04T20:01:00Z'), pushed:'yes', pushedDay:'2026-09-04' }),
    recorderLoad:()=>({ days:{ '2026-09-04':{ snaps:{ SPY:new Array(101).fill(0) } } } }), AUTOSAVE:{}, DAY_WRITTEN:{}, autosaveState:()=>({code:'idle'}),
    ANALYSIS_NIGHTLY:{ date:'2026-09-04', ranOn:'his machine', ranAt:'2026-09-05T04:45:27Z', patterns:{events:81,days:2}, hypotheses:[{id:'H5',verdict:'ready'}] },
    learnLoad:()=>({ rules:[] }), recMerged:()=>[{id:'R-1',status:'proposed'}], g3tip:(t)=>' title="x"', g3esc:(s)=>String(s==null?'':s) };
  const T=new Function('__g', Object.keys(gl).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+exVar('DAYLINE_ANALYSIS_LATE_MS')+'\nvar window={__gptsDebug:{}};\n'+['fmtCT','dayBarCount','dayLineState'].map(ex).join('\n')+'\nreturn dayLineState();')(gl);
  ok(T.day==='2026-09-04' && T.segs[0].key==='saved' && T.segs[0].state==='g' && /23:45 · your machine/.test((T.segs.find(s=>s.key==='analysis')||{}).text),'6a Labor Day 08:40 by the clock, no live book: the line is Friday\'s completed line',[T.day,T.segs.map(s=>s.key+'='+s.text)]);
  gl.liveBookToday=()=>true;
  const T2=new Function('__g', Object.keys(gl).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+exVar('DAYLINE_ANALYSIS_LATE_MS')+'\nvar window={__gptsDebug:{}};\n'+['fmtCT','dayBarCount','dayLineState'].map(ex).join('\n')+'\nreturn dayLineState();')(gl);
  ok(T2.day==='2026-09-07' && T2.segs[0].key==='data' && /recording · 0 bars/.test(T2.segs[0].text),'6b a trading day 08:40 with the live book and the first bar not yet closed: today, recording · 0 bars (the grace is the book, not a timer)',[T2.day,T2.segs[0]]);
}

// ---- 7 · the record ----------------------------------------------------------------------------------------------------
{
  ok(/window\.__gptsDebug\.closed=function\(\)/.test(src),'7a the probe: __gptsDebug.closed()');
  const P=JSON.parse(fs.readFileSync('learning/plan.json','utf8')); const nx=P.roadmap.filter(r=>r.status==='next');
  ok(P.roadmap.some(r=>r.v==='15.75' && /CLOSED STATE/.test(r.title) && (r.status==='shipped' || nx.some(x=>x.v==='15.75'))) && P.roadmap.some(r=>r.v==='15.74' && r.status==='shipped'),'7b the plan: v15.74 shipped, v15.75 this build or shipped',nx.map(x=>x.v));
  const seedJs=JSON.parse(/var PLAN_SEED=(\{.*?\});\n/.exec(src)[1]); ok(JSON.stringify(seedJs)===JSON.stringify(P),'7c PLAN_SEED equals the file');
  const R=JSON.parse(fs.readFileSync('learning/recommendations.json','utf8')); const r12=R.rows.find(r=>r.id==='R-12');
  ok(r12 && r12.status==='implemented' && r12.version==='15.75' && r12.by==='operator' && /closed state/i.test(r12.text),'7d R-12 on Rec, implemented in v15.75',r12&&[r12.status,r12.version]);
  const seedR=JSON.parse(/var REC_SEED=(\{.*?\});\n/.exec(src)[1]); ok(JSON.stringify(seedR)===JSON.stringify(R),'7e REC_SEED equals the file');
  const cl=fs.readFileSync('changelog/CHANGELOG.md','utf8'); ok(/## v15\.75/.test(cl) && cl.indexOf('## v15.75')<cl.indexOf('## v15.74b'),'7f the CHANGELOG has the v15.75 entry on top');
  const ls=fs.readFileSync('session-state/LESSONS.md','utf8'); const logAt=ls.indexOf('## 2 · THE LESSON LOG'); ok(/### v15\.75/.test(ls.slice(logAt>=0?logAt:0)),'7g the lesson log carries the v15.75 entry');
  const rn=fs.readFileSync('session-state/latest-resume-note.md','utf8'); ok(/v15\.(7[5-9]|[89]\d)/.test(rn.slice(0,600)) && /closed state/i.test(rn),'7h the resume note is at v15.75 or later and names the closed state');
  const cfg=JSON.parse(fs.readFileSync('.gex-config.json','utf8')); ok(/CLOSED STATE/.test(cfg.theWhatAndTheHow.closedState||'') && cfg.theWhatAndTheHow.pinnedBy.indexOf('test_v1575.js')>=0,'7i .gex-config.json names the closed state and this test');
  const inv=fs.readFileSync('design/DASHBOARD-INVENTORY.md','utf8'); ok(/## 0j · v15\.75/.test(inv),'7j the inventory carries §0j');
}
console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
