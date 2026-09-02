// ============================================================================================
// test_replay.js — (v15.10) THE REPLAY SLIDER.
//
// Operator, 2026-08-31: "i wanted a slider that i could slide to see how the app looked earlier on
// in the day" · "It should also work after hours or on the weekend, so if it is saturday, i want to
// go back to friday and see what happened."
//
// ⚠⚠ EVERY ASSERTION HERE EXECUTES THE FUNCTION. This project's most expensive recurring defect is
// a test that greps source and passes on a build that emits the wrong number (PROJECT-CONSTANTS
// pattern 8, three occurrences). The fixture is THREE REAL RECORDED FRAMES out of
// data/2026-08-31.json — 09:30, 13:00 and 14:45 — so the numbers below are the operator's own book,
// not invented ones, and a wrong reconstruction shows up as a wrong strike rather than a style
// difference.
// ============================================================================================
const fs=require('fs');
const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){
  const re=new RegExp('function\\s+'+n+'\\s*\\(','g'); const m=re.exec(src);
  if(!m) throw new Error('function not found: '+n);
  let i=src.indexOf('{',m.index), d=0, e=-1;
  for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){ d--; if(d===0){ e=k; break; } } }
  return src.slice(m.index,e+1);
}
function val(n){ const m=new RegExp('(?:var\\s+)?\\b'+n+'\\s*=\\s*([\\s\\S]*?);\\n').exec(src); return m?eval('('+m[1]+')'):undefined; }
// strip comments before any assertion about EMITTED text — a comment that quotes the thing it
// explains has produced nine false passes in this project.
const decomment=s=>s.replace(/\/\*[\s\S]*?\*\//g,'').replace(/^\s*\/\/.*$/gm,'');
// ⚠ numeric constant reader — val() stops at the first `;`+newline and several of these carry
// trailing comments (the landmine beside L-K).
const num=n=>{ const m=new RegExp('var\\s+'+n+'\\s*=\\s*(-?[0-9.]+)').exec(src); return m?parseFloat(m[1]):undefined; };

const FR=JSON.parse(fs.readFileSync('./tools/fixtures/replay-frames.json','utf8'));

global.mul=(a,b)=>a/(1/b);
global.REPLAY={ on:false, day:null, idx:-1, frames:[], days:null, loading:false, err:null };
global.PAL={ bg:'#0b0e14', card:'#12161f', line:'#1e2530', ink:'#e6edf3', sub:'#8b98a9',
             time:'#f3f6fa', amber:'#f2b45a', longAccent:'#2ec27e', blue:'#4a90d9' };
global.CFG={};
global.swallow=()=>{};
global.two=x=>{ x=''+x; return x.length<2?'0'+x:x; };
global.RP_OPEN_SEC=mul(8,3600)+mul(30,60);
global.RP_CLOSE_SEC=mul(15,3600);
eval(ex('replayOn')); eval(ex('replayFrame')); eval(ex('replaySecOf')); eval(ex('replaySec'));
eval(ex('replayBookOf')); eval(ex('replayBook')); eval(ex('replayEmptyBook'));
eval(ex('replayDayLabel')); eval(ex('replaySeek')); eval(ex('replaySeekPct')); eval(ex('replayStep'));
eval(ex('measureBars')); eval(ex('recorderBlind')); eval(ex('hlClock'));
global.render=()=>{};
global.inReplay=()=>false;
global.showingStaleBook=()=>false;
global.dispIsFut=()=>false;
global.closedCandles=()=>[];

// ---- 1 · THE GATE ---------------------------------------------------------------------------
// ⚠ `on` alone must not arm it. A day that loaded zero frames would otherwise intercept tapeMap
// and blank the live face — the "correct change routed onto a broken path" failure of v15.04.
REPLAY.on=false; REPLAY.frames=FR; REPLAY.idx=1;
ok(replayOn()===false, 'r1 on=false does not arm replay, even with frames loaded');
REPLAY.on=true; REPLAY.frames=[]; REPLAY.idx=-1;
ok(replayOn()===false, 'r2 armed with NO frames is still off — an empty day cannot intercept the live face');
REPLAY.frames=FR; REPLAY.idx=-1;
ok(replayOn()===false, 'r3 ...nor can a negative index');
REPLAY.idx=99;
ok(replayOn()===false, 'r4 ...nor an index past the end');
REPLAY.idx=1;
ok(replayOn()===true,  'r5 armed, frames loaded, index in range');

// ---- 2 · THE D-10 GUARD — the single biggest risk in this feature ------------------------------
// ⚠⚠ Writing a replayed bar as though it were now is not wrong data, it is MISLABELLED data, and
// nothing downstream can detect it afterwards. recorderBlind() is the one gate nine write paths
// already call, so this one assertion covers all nine.
ok(recorderBlind()===true, 'r6 THE RECORDER IS BLIND while the slider is engaged (DECISIONS D-10)');
REPLAY.on=false;
ok(recorderBlind()===false, 'r7 ...and sees again the moment it is released');
REPLAY.on=true;

// ---- 3 · WHICH BOOK OWNS A STRIKE -------------------------------------------------------------
// ⚠⚠ THE LOAD-BEARING CASE IS SPY vs QQQ. SPXW (~7700) is a decade away and any rule separates it;
// SPY 767 and QQQ 716 are SEVEN PERCENT apart, and mixing two books into one ladder is the error
// that has produced four separate phantom bugs here.
const tri=FR[1].tri;
ok(replayBookOf(7700, tri)==='SPXW', 'r8 an SPX strike lands in the SPXW book');
ok(replayBookOf(767,  tri)==='SPY',  'r9 767 is SPY, not QQQ — seven percent apart and correctly split');
ok(replayBookOf(716,  tri)==='QQQ',  'r10 ...and 716 is QQQ, not SPY');
ok(replayBookOf(15.5, tri)==='VIX',  'r11 VIX separates too');
ok(replayBookOf(0, tri)===null,      'r12 a nonsense strike belongs to no book rather than the nearest one');

// ---- 4 · THE RECONSTRUCTION IS EXACT ----------------------------------------------------------
// ⚠⚠ THIS IS THE ASSERTION THE WHOLE FEATURE RESTS ON. The frame stores Skylit's dollar values, not
// percentages; the ladder is rebuilt by normalising them against the largest strike IN THAT BOOK.
// If that ruler is ever taken from the live book, or from another book, every %King on a replayed
// ladder is silently wrong. It is checked against the frame's OWN recorded `tri.top`, which was
// computed independently by the panel at capture time.
REPLAY.idx=1;
const B=replayBook('SPXW');
ok(!!B && B.count>0, 'r13 the SPXW book rebuilds from a real recorded frame', B&&B.count);
ok(B.king===7700, 'r14 the king is the book\'s own king, not the SPY one', B&&B.king);
{
  const recorded=FR[1].tri.SPXW.top;      // what the panel itself recorded at 13:00
  let worst=0, mism=[];
  recorded.forEach(([k,p])=>{ const got=B.pct[String(k)];
    if(got===undefined){ mism.push([k,'absent']); return; }
    const d=Math.abs(got-p); if(d>worst) worst=d;
    if(d>1) mism.push([k,p,got]); });
  ok(mism.length===0, 'r15 EVERY recorded top strike reproduces to within a rounding point', mism);
  ok(worst<=1, 'r16 ...worst error is a single point of %King', worst);
}
ok(B.pct['7700']===-100, 'r17 the King reads -100: the SIGN is carried, not just the magnitude', B.pct['7700']);
ok(B.kingKd>0 && Math.abs(B.kingKd-95471)<200, 'r18 kingKd is the king\'s own dollars in THOUSANDS', B.kingKd);
ok(B.src==='replay' && B.replay===true, 'r19 the book announces itself as replayed — no consumer can mistake it for live');
// ⚠⚠ EVERY VELOCITY ROW CARRIES ITS STRIKE, AND THIS IS NOT COSMETIC. `rollScan` does
//   rows.push(e.v) … if(dst.k===src.k) continue … Math.abs(dst.k-src.k)>ROLL_MAX_DIST
// so a missing `k` makes every pair compare undefined===undefined — TRUE — and every roll candidate
// is silently discarded as "the same strike". v15.14 shipped that way and drew ZERO arrows on a
// session holding 2,406 roll sightings. Nothing threw; the feature was just always empty.
{
  const vk=Object.keys(B.vel);
  ok(vk.length>0, 'r19b the book carries velocity rows at all', vk.length);
  ok(vk.every(k=>typeof B.vel[k].k==='number' && B.vel[k].k>0),
     'r19c ...and EVERY one carries its own strike, which rollScan indexes pairs by');
  ok(vk.every(k=>Math.abs(B.vel[k].k-parseFloat(k))<1e-9),
     'r19d ...and it is the strike it is keyed under, not another row\'s');
  ok(vk.every(k=>typeof B.vel[k].d15==='number'),
     'r19e ...and d15, which is the quantity a roll is detected from');
  // the real proof: distinct strikes, so a pair can differ
  const ks=vk.map(k=>B.vel[k].k);
  ok(new Set(ks).size===ks.length, 'r19f ...and the strikes are distinct — pairs can actually differ');
}
{
  const bq=replayBook('QQQ');
  ok(!!bq && bq.king===716, 'r20 the QQQ book rebuilds independently with its OWN king', bq&&bq.king);
  ok(bq.pct['7700']===undefined, 'r21 ...and carries no SPX strike — the books do not bleed into each other');
}

// ---- 5 · A DIFFERENT FRAME IS A DIFFERENT BOOK ------------------------------------------------
// The point of the feature: dragging must actually change what the face reads.
REPLAY.idx=0;
{
  const early=replayBook('SPY');
  REPLAY.idx=1;
  const later=replayBook('SPY');
  ok(early && later, 'r22 both frames rebuild');
  ok(early.king!==later.king || JSON.stringify(early.pct)!==JSON.stringify(later.pct),
     'r23 09:30 and 13:00 produce DIFFERENT books — the slider moves the ladder, not just the clock');
}

// ---- 6 · REFUSE, NEVER FALL THROUGH -----------------------------------------------------------
// ⚠⚠ Serving live numbers under a REPLAY badge would be invisible: every value plausible, every
// label wrong. An empty book is a gap the face already explains.
{
  const E=replayEmptyBook();
  ok(E.count===0 && E.empty===true && E.replay===true, 'r24 the refusal is a BOOK-SHAPED empty, not null');
  const tm=decomment(ex('tapeMap'));
  ok(/return RB \|\| replayEmptyBook\(\);/.test(tm),
     'r25 tapeMap returns the empty book rather than falling through to the live tape');
  const rbIdx=tm.indexOf('replayBook'), liveIdx=tm.indexOf('tapeMapLive');
  ok(rbIdx>-1 && liveIdx>-1 && rbIdx<liveIdx, 'r26 ...and the replay branch is reached BEFORE the live one');
}

// ---- 7 · THE BARS COME FROM THE FRAMES --------------------------------------------------------
// On a PAST day the ES courier holds only the newest session and the chart holds today, so reading
// either would put today's candle under Friday's ladder.
REPLAY.idx=2;
{
  const MB=measureBars('SPY');
  ok(MB.src==='replay', 'r27 in replay the bar source is the frames themselves, not ES or the chart', MB.src);
  ok(MB.bars.length===3, 'r28 ...truncated at the parked frame, inclusive', MB.bars.length);
  ok(MB.approxOpen===true, 'r29 ...and it declares that the per-bar OPEN is reconstructed, not recorded');
  ok(MB.bars[1].h===FR[1].h && MB.bars[1].l===FR[1].l && MB.bars[1].c===FR[1].px,
     'r30 HIGH, LOW and CLOSE are the RECORDED values, exact');
  ok(MB.bars[1].o===FR[0].px, 'r31 the open is the PREVIOUS close — stated, not silently invented', MB.bars[1].o);
  ok(MB.bars[0].o===FR[0].px, 'r32 ...and the first bar opens at its own close, having no predecessor');
  REPLAY.idx=0;
  ok(measureBars('SPY').bars.length===1, 'r33 dragging back SHORTENS the series — the day has not happened yet');
  REPLAY.idx=2;
}
// ⚠ THE HARNESS TRAP THIS FEATURE ALREADY FELL INTO ONCE. measureBars is eval'd in isolation by
// nine test files that do not define replayOn; a bare call threw, the catch called an undefined
// swallow(), and hodLod read {ok:false} — "the session has no bars". The typeof guard is the fix
// and it is pinned here so it cannot be tidied away.
{
  const mb=decomment(ex('measureBars'));
  ok(/typeof replayOn==='function' && replayOn\(\)/.test(mb),
     'r34 measureBars guards its new dependency with typeof — absent must mean ABSENT, never a throw');
}

// ---- 8 · THE HANDLE SNAPS TO A FRAME THAT EXISTS ----------------------------------------------
// A slider reporting a time no frame was taken at is inventing a reading.
{
  REPLAY.frames=FR; REPLAY.idx=0;
  replaySeekPct(1);   ok(REPLAY.idx===2, 's1 seeking to the end lands on the last frame', REPLAY.idx);
  replaySeekPct(0);   ok(REPLAY.idx===0, 's2 ...and to the start on the first', REPLAY.idx);
  // 13:00 is 4.5h after the 08:30 open, on a 6.5h track = 0.692
  replaySeekPct(0.692);
  ok(REPLAY.idx===1, 's3 asking for 13:00 lands on the 13:00 frame', REPLAY.idx);
  ok(hlClock(replaySec())==='13:00', 's4 ...and the clock reports the frame it actually landed on', hlClock(replaySec()));
  // between two frames it must not report a time nothing was recorded at
  replaySeekPct(0.5);
  const at=hlClock(replaySec());
  ok(FR.some(f=>hlClock(replaySecOf(f))===at), 's5 a mid-gap request still reports a RECORDED minute', at);
  replaySeek(-5); ok(REPLAY.idx===0, 's6 seeking before the open clamps rather than throwing');
  replaySeek(999); ok(REPLAY.idx===2, 's7 ...and past the close');
}
// ⚠⚠ s3-s7 CANNOT TELL SNAPPING FROM LINEAR INDEX MAPPING, and I only found that by mutating.
// With three near-evenly-spaced frames, `round(p*(n-1))` lands on the same indices as nearest-in-
// time, so a mutation that threw the snap away survived every assertion above. THE FIXTURE WAS THE
// WEAKNESS, NOT THE RULE — a gap is required to separate the two behaviours, and a gap is exactly
// the case the feature exists to handle honestly (a day the panel was not open for).
{
  const mkT=(h,m)=>Date.UTC(2026,7,31,h+5,m,0);   // CT -> the epoch the frames carry
  REPLAY.frames=[{t:mkT(8,30),px:1,h:1,l:1},{t:mkT(9,0),px:2,h:2,l:2},{t:mkT(14,30),px:3,h:3,l:3}];
  REPLAY.idx=0;
  // 12:00 is 3h30 past the open on a 6h30 track = 0.538. Nearest IN TIME is 14:30 (150 min away),
  // not 09:00 (180 min). A linear index would give frame 1; snapping gives frame 2.
  replaySeekPct(0.538);
  ok(REPLAY.idx===2, 's8 across a GAP the handle snaps to the nearest frame in TIME, not by index', REPLAY.idx);
  ok(hlClock(replaySec())==='14:30', 's9 ...and reports 14:30, a minute that was actually recorded', hlClock(replaySec()));
  replaySeekPct(0.10);
  ok(REPLAY.idx===1 && hlClock(replaySec())==='09:00', 's10 ...and a request inside the dense end still lands there', hlClock(replaySec()));
  REPLAY.frames=FR; REPLAY.idx=2;
}

// ---- 9 · THE DAY LABEL ------------------------------------------------------------------------
ok(replayDayLabel('2026-08-28')==='Fri 28 Aug', 'd1 the day reads as a weekday he can recognise', replayDayLabel('2026-08-28'));
ok(replayDayLabel('2026-08-31')==='Mon 31 Aug', 'd2 ...for the Monday too', replayDayLabel('2026-08-31'));
ok(replayDayLabel('')==='',                     'd3 a missing day does not render "Invalid Date"');

// ---- 10 · THE FACE ----------------------------------------------------------------------------
{
  const strip=decomment(ex('replayBarHtml'));
  ok(/data-grp="track"/.test(strip),  'f1 the track is delegable — render() replaces innerHTML every tick');
  ok(/data-grp="prev"/.test(strip) && /data-grp="next"/.test(strip),
     'f2 the day steppers are there, so Saturday can look at Friday');
  ok(/data-grp="exit"/.test(strip),   'f3 ...and there is a way back to live');
  ok(/REPLAY\.err/.test(strip),       'f4 an empty day states itself rather than drawing a blank track');
  ok(/not running/.test(strip),       'f5 ...and says the panel was not running, not that the market was quiet');
  const wire=decomment(ex('wireBodyDelegation'));
  ok(/__gptsRpWired/.test(wire),      'f6 the strip is wired ONCE, not restacked on every render');
  ok(/pointermove/.test(wire) && /pointerup/.test(wire), 'f7 it is a DRAG, not just a click');
  ok(/ev\.preventDefault\(\); ev\.stopPropagation\(\)/.test(wire),
     'f8 ...and the drag does not also drag the panel across the screen');
  const rnd=decomment(ex('render'));
  ok(/html\+=replayBarHtml\(\)/.test(rnd), 'f9 the strip is mounted in render');
  const tIdx=rnd.indexOf('TESTING_VIEW'), aIdx=rnd.indexOf('ANALYSIS_VIEW'), rIdx=rnd.indexOf('replayBarHtml');
  ok(tIdx<rIdx && aIdx<rIdx, 'f10 ...after both other tabs have returned, so it shows on the dashboard only');
}

// ---- 10b · THE STRIP IS EXECUTED, NOT JUST GREPPED --------------------------------------------
// ⚠⚠ f1-f10 above are source greps, and a grep cannot prove a renderer runs. This project shipped
// 47 grep assertions over a section that was ABSENT from the DOM (v14.95, `SYM` vs `sym`), because
// every one of them was reading text rather than output. So the strip is run.
{
  global.g3esc=x=>String(x==null?'':x).replace(/"/g,'&quot;').replace(/</g,'&lt;');
  global.g3tip=t=>t?(' title="'+g3esc(t)+'"'):'';
  global.replayEnsure=()=>{};
  // ⚠⚠ (v15.45) THE HARNESS MUST SUPPLY THE MODULE'S GLOBALS, OR THE OUTER CATCH EATS THE RENDER.
  // `replayBarHtml` swallows and returns '' — so a missing global does not look like an error, it
  // looks like an empty strip, and x1 caught exactly that when the NOT RECORDING banner landed.
  // ⚠ Stubbing them also EXERCISES the new banner instead of letting its own try/catch hide it.
  global.RP_STALEMSG=null;
  global.ctOffsetSec=()=>5*3600;
  global.sessionPhase=()=>({ rth:false, label:'CLOSED', leftMin:null });
  global.replayDayLabel=d=>String(d);
  eval(ex('replayBarHtml'));
  REPLAY.on=false; REPLAY.frames=FR; REPLAY.idx=2; REPLAY.day='2026-08-31'; REPLAY.days=['2026-08-28','2026-08-31']; REPLAY.err=null;
  const live=replayBarHtml();
  ok(typeof live==='string' && live.length>0, 'x1 the strip renders LIVE without throwing', live.length);
  // ⚠ WAS `/LIVE/.test(live)` AND IT COULD NOT SEE THE BUG IT WAS MEANT TO GUARD. His strip read
  // "LIVE LIVE" because the clock slot ALSO printed the badge's word. The badge says the MODE; the
  // clock says the TIME — and when live, the useful time is the newest recorded frame.
  ok((live.match(/LIVE/g)||[]).length===1, 'x2 the word LIVE appears ONCE, on the badge only',
     (live.match(/LIVE/g)||[]).length);
  ok(!/REPLAY<\/span>/.test(live), 'x2b ...and it does not say REPLAY while live');
  // ⚠⚠ (v15.45) AND THE NOT-RECORDING BANNER, EXECUTED ON ALL FOUR COMBINATIONS. He lost a whole
  // morning to a replay parked on yesterday during a live session, with nothing on the face saying
  // the cost. A banner that appears in the wrong state is worse than none, so each state is run.
  ok(!/NOT RECORDING/.test(live), 'x2c live + closed: no banner');
  global.sessionPhase=()=>({ rth:true, label:'MIDDAY', leftMin:125 });
  ok(!/NOT RECORDING/.test(replayBarHtml()), 'x2d live + RTH: still no banner — it is recording');
  REPLAY.on=true;
  const warn=replayBarHtml();
  ok(/NOT RECORDING/.test(warn), 'x2e REPLAY + live RTH: the banner FIRES');
  ok(/MIDDAY/.test(warn) && /125 min left/.test(warn),
     'x2f ...naming the session he is missing, so it is a fact and not a scold');
  global.sessionPhase=()=>({ rth:false, label:'AFTER HOURS', leftMin:null });
  ok(!/NOT RECORDING/.test(replayBarHtml()),
     'x2g REPLAY + market closed: silent — studying a past day at night is the point');
  REPLAY.on=false; global.sessionPhase=()=>({ rth:false, label:'CLOSED', leftMin:null });
  ok(/14:45/.test(live), 'x2c the clock shows the NEWEST recorded frame, so the store\'s age is visible');
  ok((live.match(/<i style="position:absolute/g)||[]).length===3, 'x3 one tick per loaded frame', (live.match(/<i style="position:absolute/g)||[]).length);
  ok(/Mon 31 Aug/.test(live), 'x4 the day reads as a weekday');
  REPLAY.on=true;
  const rep=replayBarHtml();
  ok(/REPLAY/.test(rep), 'x5 armed, the strip says REPLAY');
  ok(/14:45/.test(rep), 'x6 ...and prints the clock of the frame it is parked on');
  // ⚠ WAS `ok(/242,180,90/.test(rep))` AND IT WAS A FAKE ASSERTION — the amber also appears in the
  // border and the badge, so deleting the background tint left it green. Mutation caught it. The
  // property is that the two modes are VISUALLY DIFFERENT AT THE STRIP'S GROUND, so compare them.
  const grd=h=>{ const m=/height:30px;background:([^;]+);/.exec(h); return m?m[1]:null; };
  ok(grd(live)!==null && grd(rep)!==null && grd(live)!==grd(rep),
     'x7 ...on a different ground from LIVE, so a replayed face cannot read as a live one',
     [grd(live), grd(rep)]);
  REPLAY.idx=0;
  ok(/09:30/.test(replayBarHtml()), 'x8 dragging back changes the clock the strip prints');
  REPLAY.err='no RTH frames recorded for Sat 22 Aug'; REPLAY.frames=[]; REPLAY.idx=-1;
  const bad=replayBarHtml();
  ok(/Sat 22 Aug/.test(bad) && /not running/.test(bad), 'x9 an empty day names itself and blames the recorder, not the market');
  ok((bad.match(/<i style="position:absolute/g)||[]).length===0, 'x10 ...and draws no ticks it does not have');
  REPLAY.frames=FR; REPLAY.idx=2; REPLAY.err=null;
}

// ---- 11 · THE TWO SMALL ITEMS SHIPPED IN THE SAME BUILD ---------------------------------------
{
  // "put the version in the header where it says Tapereader, so i know what version it is"
  ok(/tver\.textContent='v'\+GPTS_VERSION;/.test(src),
     'v1 the header version READS GPTS_VERSION — it cannot drift from the footer');
  ok(/tver\.id='gpts-hdrver'/.test(src), 'v2 ...and is addressable, so a test can find it on the live panel');
  // "take out the read"
  const sf=decomment(ex('secFrame'));
  ok(!/g3read/.test(sf), 'v3 the READ row is off the face');
  ok(/emRead\(EB, sym\)/.test(sf), 'v4 ...while emRead still runs, keeping the forecast ban live');
  // the vend cap is the depth of every future replay
  ok(val('VEND_MAX_ROWS')===90, 'v5 the vendor row cap is 90 — the depth a replayed ladder can reach', val('VEND_MAX_ROWS'));
  ok(/rows\.slice\(0,VEND_MAX_ROWS\)/.test(src), 'v6 ...and the recorder actually uses the constant, not a literal');
}

// ---- 12 · (v15.11) THE FACE MUST REPLAY AS RICHLY AS IT LIVES -----------------------------------
// He installed v15.10, dragged to 13:33, and reported "the levels the king lanes the gamma profile,
// the status's etc. all missing". Measured on his panel, same instant, replay vs live:
//     ladder node bars 1 vs 4 · king pills 2 vs 5 · ROC values 14 vs 26 · states {SPENT} vs {BUILDING,SPENT}
// Nothing was broken — replay was simply far thinner than the live face it claims to reproduce.
{
  // ⚠ NOT val() — that helper reads up to the first `;` FOLLOWED BY A NEWLINE, and ACC_WINDOW
  // carries a trailing comment, which is the exact landmine PROJECT-CONSTANTS records next to L-K.
  // A dedicated numeric read, rather than editing a long-standing declaration to suit a test.
  const num=n=>{ const m=new RegExp('var\\s+'+n+'\\s*=\\s*(-?[0-9.]+)').exec(src); return m?parseFloat(m[1]):undefined; };
  global.ACC_WINDOW=num('ACC_WINDOW');
  ok(global.ACC_WINDOW>0, 'a0 ACC_WINDOW is readable — the replay window is the LIVE window', global.ACC_WINDOW); global.SLICE_KEY='gpts_slices_v7'; global.TODAY='2026-08-31';
  global.localStorage={ getItem:()=>null };
  eval(ex('slicesFor'));
  REPLAY.on=true; REPLAY.frames=FR; REPLAY.idx=2; REPLAY.day='2026-08-31';

  // ⚠⚠ THE ACCUMULATION LAYER IS FED THE REAL SEQUENCE, NOT A PROXY LABEL. rawAccumMap builds every
  // node's abs-sequence from slicesFor(), and BUILDING/STEADY/FADING, the day peak and the
  // DEFENDING/ABANDONING marks all fall out of it. Replay must not carry a SECOND state rule wearing
  // the same words — that is mislabelling, the failure this project keeps paying for.
  const sl=slicesFor('SPXW');
  ok(sl.length>0, 'a1 replay synthesises slices, so the LIVE accumulation rule can run unchanged', sl.length);
  ok(sl.length===3, 'a2 one slice per frame up to the parked one', sl.length);
  ok(sl.every(x=>Array.isArray(x.l) && x.l.length>0), 'a3 every slice carries rows');
  ok(sl.every(x=>x.l.every(n=>typeof n.k==='number' && typeof n.v==='number' && (n.d===1||n.d===-1))),
     'a4 ...in the LIVE slice shape {k,v,d,net} — rawAccumMap reads n.v and n.d by name');
  ok(sl[0].l.every(n=>n.v>=0), 'a5 v is a MAGNITUDE, as the live feed supplies it');
  ok(sl[0].l.some(n=>n.d===-1), 'a6 ...and polarity survives in d, taken from the recorded sign');
  ok(sl.every(x=>x.l.every(n=>n.k>1000)), 'a7 one book per ladder — no SPY strike leaks into the SPXW slices');
  {
    const q=slicesFor('QQQ');
    ok(q.length>0 && q.every(x=>x.l.every(n=>n.k<1000 && n.k>600)), 'a8 ...and the QQQ slices carry only QQQ', q.length);
  }
  // a sequence is only a sequence if it MOVES with the handle
  REPLAY.idx=0;
  ok(slicesFor('SPXW').length===1, 'a9 dragging back shortens the sequence — history it has not seen yet');
  REPLAY.idx=2;
  // ⚠ THE SOURCE MUST BE THE FRAMES, NOT localStorage: on a PAST day the live slice store holds
  // today, and TODAY's accumulation under Friday's ladder is the mislabelling this exists to avoid.
  const slSrc=decomment(ex('slicesFor'));
  const rIdx=slSrc.indexOf('REPLAY.frames'), lsIdx=slSrc.indexOf('localStorage');
  ok(rIdx>-1 && lsIdx>-1 && rIdx<lsIdx, 'a10 the replay branch is reached BEFORE the localStorage one');
}

// ---- 13 · (v15.11) ALL THREE CROWNS COME FROM THE FRAME ----------------------------------------
// ⚠⚠ v15.10 DREW TODAY'S KINGS ON A REPLAYED PAST DAY. ladderKings reads the SPXW crown from a latch
// keyed to ctTodayStr() and the SPY/QQQ crowns from LASTFEED — all three live. On Friday that is not
// missing data, it is MISLABELLED data. Every crown is recorded: tri.{SPXW,SPY,QQQ}.king, plus
// xm.QQQ.px for QQQ's proportional bearing.
{
  const lk=decomment(ex('ladderKings'));
  ok(/replayOn\(\)/.test(lk), 'k1 ladderKings knows about replay at all');
  const rIdx=lk.indexOf('replayOn'), latchIdx=lk.indexOf('KING_LATCH_KEY'), feedIdx=lk.indexOf('LASTFEED');
  ok(rIdx>-1 && latchIdx>rIdx, 'k2 ...and the replay branch precedes TODAY\'s SPXW latch');
  ok(rIdx>-1 && feedIdx>rIdx, 'k3 ...and precedes the LIVE feed the SPY and QQQ crowns come from');
  // ⚠⚠ k4/k5 WERE SOURCE GREPS AND BOTH SURVIVED MUTATION. Gating a crown off with `if(false && ...)`
  // leaves `RTri.SPY.king` in the text, and turning QQQ from a bearing into a raw level leaves
  // `RXm.QQQ.px` sitting in the tip string. A grep cannot tell a live branch from a dead one — the
  // sixth occurrence of that in this project. So ladderKings is EXECUTED.
  global.ifDispScale=()=>1.0023;
  global.KING_LATCH_KEY='gpts_kingtrack_latch';
  global.LASTFEED={}; global.FEED_STALE_MS=12000; global.STATE={QQQ:{}};
  global.extractWalls=()=>null;
  eval(ex('ladderKings'));
  REPLAY.on=true; REPLAY.frames=FR; REPLAY.idx=1;
  const KS=ladderKings({ now:7700, nowLive:7700, scaleUsed:10.0387 }, 'SPY');
  const bookOf=b2=>KS.filter(x=>x.book===b2)[0];
  ok(KS.length===3, 'k4 THREE crowns come back, not one — executed, not grepped', KS.map(x=>x.book));
  ok(!!bookOf('SPXW') && bookOf('SPXW').raw===7700, 'k4b the SPXW crown is the frame\'s', bookOf('SPXW')&&bookOf('SPXW').raw);
  // ⚠ `raw` alone survived a mutation that dropped the conversion — a crown at its RAW strike sits
  // ~18 points off on an ES rail, which is exactly the scale class of bug this project keeps hitting.
  ok(!!bookOf('SPXW') && Math.abs(bookOf('SPXW').at-7700*1.0023)<0.01,
     'k4b2 ...drawn at the CONVERTED price, not the raw strike', bookOf('SPXW')&&bookOf('SPXW').at);
  ok(!!bookOf('SPY') && Math.abs(bookOf('SPY').at-767*10.0387)<0.01,
     'k4c2 ...and the SPY crown converts on the rail\'s own ratio', bookOf('SPY')&&bookOf('SPY').at);
  ok(!!bookOf('SPY') && bookOf('SPY').raw===767, 'k4c the SPY crown is the frame\'s', bookOf('SPY')&&bookOf('SPY').raw);
  ok(!!bookOf('QQQ') && bookOf('QQQ').raw===716, 'k4d the QQQ crown is the frame\'s', bookOf('QQQ')&&bookOf('QQQ').raw);
  {
    const q=bookOf('QQQ');
    ok(q && q.kind==='proportional', 'k5 QQQ is drawn as a BEARING, never a converted level', q&&q.kind);
    // the bearing is price x (its king / its own price) — NOT the raw strike, which would sit at 716
    // on a 7700 rail and be off the chart entirely.
    const want=7700*(716/FR[1].xm.QQQ.px);
    ok(q && Math.abs(q.at-want)<0.5, 'k5b ...positioned off QQQ\'s OWN recorded price, not its strike', q&&Math.round(q.at));
    ok(q && q.at>7000, 'k5c ...so it lands on the rail rather than 6,900 points below it', q&&Math.round(q.at));
  }
  ok(/return out;\n    \}/.test(lk) || /return out;/.test(lk.slice(rIdx, latchIdx>rIdx?latchIdx:undefined)),
     'k6 the replay branch RETURNS — it cannot fall through and append today\'s crowns as well');
}

// ---- 14 · (v15.13) THE THREE REASONS HE COULD NOT SEE THE ARROWS ------------------------------
// Operator, 2026-09-01, having asked twice: "i dont see the king lane ... i dont see the arrows and
// i dont see the node profile except 1". MEASURED: .g3ladwrap scrollWidth 640, clientWidth 535 —
// 105px off the right edge, scrollLeft 0. The roll lane is at x 620-640, ENTIRELY inside that strip.
{
  // --- a · the panel grows to fit the ladder ---
  global.CFG={}; global.window={innerWidth:2314};
  let PANW=560, saved={};
  global.PANEL={ style:{width:'560px',height:''}, getBoundingClientRect:()=>({width:PANW}) };
  global.elBody={ querySelector:()=>({scrollWidth:640, clientWidth:535}) };
  global.localStorage={ getItem:k=>(k in saved?saved[k]:null), setItem:(k,v)=>{saved[k]=String(v);} };
  global.SIZE_KEY='gpts_panelsize_v7';
  eval(ex('ladderFit'));
  ladderFit();
  ok(PANEL.style.width==='667px', 'w1 the panel grows by exactly the overflow (+2), 560 -> 667', PANEL.style.width);
  ok(saved['gpts_ladfit_v1']==='667', 'w2 ...and remembers the target so it cannot re-fire every render');
  // once it fits, it must never touch the width again
  PANW=667; PANEL.style.width='667px';
  global.elBody={ querySelector:()=>({scrollWidth:640, clientWidth:665}) };
  ladderFit();
  ok(PANEL.style.width==='667px', 'w3 a ladder that FITS is left alone — no growth, ever', PANEL.style.width);
  // and a manual shrink is not fought on the same target
  PANW=560; PANEL.style.width='560px';
  global.elBody={ querySelector:()=>({scrollWidth:640, clientWidth:535}) };
  ladderFit();
  ok(PANEL.style.width==='560px', 'w4 ...and it does not fight a manual resize back to the same target', PANEL.style.width);
  // bounded by the viewport rather than growing off screen
  saved={}; PANW=560; PANEL.style.width='560px'; global.window={innerWidth:600};
  ladderFit();
  ok(parseInt(PANEL.style.width,10)<=588, 'w5 the growth is capped by the viewport, not the ladder', PANEL.style.width);
}
{
  // --- b · rolls and velocities are live in a replayed session ---
  global.VEL_META={ ok:false, n:0 };
  global.sessionPhase=()=>({rth:false});          // after hours, exactly his case
  eval(ex('velOk')); eval(ex('rollsLive'));
  REPLAY.on=false;
  ok(velOk()===false,    'v1 live + no harvest: velocities are not ok, as before');
  ok(rollsLive()===false,'v2 live + after hours: rolls do not draw, as before');
  REPLAY.on=true; REPLAY.frames=FR; REPLAY.idx=1;
  ok(velOk()===true,     'v3 REPLAY: the frame carries vend, so velocities ARE ok');
  ok(rollsLive()===true, 'v4 REPLAY: a replayed bar is inside RTH by construction — arrows may draw');
  REPLAY.frames=[{t:FR[1].t,px:1,h:1,l:1}]; REPLAY.idx=0;
  ok(velOk()===false,    'v5 ...but a frame with NO vend is still not ok — no inventing velocities');
  REPLAY.frames=FR; REPLAY.idx=1;
}
{
  // --- c · the replayed roll latch reuses the LIVE scan, and restores the index ---
  const rr=decomment(ex('replayRolls'));
  ok(/rollScan\(/.test(rr), 'c1 it calls the LIVE rollScan — no second roll geometry');
  ok(/rollLatchKey\(/.test(rr), 'c2 ...and the live key, so from>to pairs match');
  ok(/finally\{ REPLAY\.idx=save; \}/.test(rr),
     'c3 the index is restored in a FINALLY — a throw must never leave the panel parked elsewhere');
  ok(/e3\.count<ROLL_SIG_N/.test(rr), 'c4 one sighting is noise and is never drawn, exactly as live');
  ok(/RP_ROLLS\.key===key/.test(rr), 'c5 memoised per (day, idx, sym) — dragging does not rescan the session');
  // ⚠⚠ v15.13 SHIPPED WITH THE BOOKS MISMATCHED AND HIS PANEL SHOWED IT: rollLane 0, rollPaths 0.
  // rollLatched is called with the CHART symbol ('SPY') while velAt in replay serves the GOVERNING
  // book (SPXW), so rollScan was handed SPY strikes and looked them up in the SPX book.
  ok(/lastBookGov\(sym\)/.test(rr), 'c7 replayRolls resolves the GOVERNING book, as velAt does');
  ok(/replayBook\(bkName\)/.test(rr), 'c8 ...and scans THAT book, so the strikes and the lookups agree');
  ok(!/replayBook\(sym\)/.test(rr), 'c9 ...never the chart symbol, which would find nothing');
  const rl=decomment(ex('rollLatched'));
  const rIdx=rl.indexOf('replayRolls'), lIdx=rl.indexOf('ROLL_LATCH');
  ok(rIdx>-1 && lIdx>rIdx, 'c6 rollLatched serves the REPLAYED set before touching today\'s live latch');
}

// ---- 15 · (v15.14) THE KING LANE — the crown's MOVEMENT through the day -----------------------
// Operator, asked twice: "i dont see the king lane with the movement the kings made". `ktOf()` was
// returning [] so the lane drew its "no migration recorded" placeholder — which reads as broken.
{
  // ⚠ (v15.23) dwell is MINUTES now, and these fixtures use real millisecond stamps so the rule
  // being tested is the one that ships. `mk()` below spaces bars 3 minutes apart, as the recorder does.
  global.KT_DWELL_MIN=20; global.KT_MAX=40;
  global.KTRACK={ v:1, day:'2026-08-31', b:{} };
  // ⚠ the memo object lives at module scope and the memo check sits OUTSIDE the function's try, so
  // an undefined RP_KT throws straight past it into ktOf's catch and returns [] — a green-looking
  // empty. Stub it, or this whole block tests the fallback instead of the feature.
  global.RP_KT={ key:null, out:[] };
  // ⚠ (v15.24) ktFilterDwell too — ktOf calls it, and without it every call falls through ktOf's
  // own catch and returns [], which reads as "the lane has no points" rather than "the harness is
  // missing a function". The same swallow-shaped failure the seam lessons keep producing.
  eval(ex('replayKingTrack')); eval(ex('ktFilterDwell')); eval(ex('ktOf'));
  REPLAY.on=true; REPLAY.frames=FR; REPLAY.idx=2; REPLAY.day='2026-08-31';
  const J=ktOf('SPXW');
  ok(J.length>0, 'kt1 the lane has points in replay where the live latch had none', J.length);
  ok(J[0].seed===true, 'kt2 the FIRST point is a seed — the origin a run is measured from');
  ok(J[0].k===FR[0].tri.SPXW.king, 'kt3 ...and it is the crown recorded in the first frame', J[0].k);
  ok(J.every(p=>typeof p.t==='number' && p.k>0), 'kt4 every point carries a time and a strike');
  // the SPY book is its own journey, not a copy
  const JY=ktOf('SPY');
  ok(JY.length>0 && JY[0].k===FR[0].tri.SPY.king, 'kt5 the SPY lane is the SPY book\'s own crown', JY[0].k);
  ok(JY[0].k!==J[0].k, 'kt6 ...and the two books do not share a journey');
  // ⚠ THE DWELL RULE SURVIVES: a strike seen ONCE is a flicker and is not a migration.
  {
    // bars three minutes apart, like the recorder's
    const T0=Date.parse('2026-08-31T14:00:00Z');
    const mk=(i,kk)=>({t:T0+i*180000, tri:{SPXW:{king:kk}}, exp:null});
    const run=(...ks)=>{ REPLAY.frames=ks.map((k,i)=>mk(i,k)); REPLAY.idx=ks.length-1; RP_KT.key=null;
                         return ktOf('SPXW'); };
    ok(run(7700,7700,7705,7700,7700).length===1,
       'kt7 a strike seen ONCE and reverting is a FLICKER, not a migration');
    // ⚠⚠ AT 20 MINUTES, THREE MORE 3-MINUTE BARS IS SIX MINUTES — NOT a migration. The old fixture
    // asserted that two sightings promoted a strike, which was only ever true because the constant
    // was a COUNT of 2. A duration cannot be satisfied by adding one bar.
    // ⚠⚠ (v15.24) THE LAST POINT IS ALWAYS DRAWN, AND THAT IS DELIBERATE. Since the dwell rule
    // moved to READ time, the final point is the crown's CURRENT seat — its run has not finished, so
    // its duration is not yet decided, and refusing to draw it would leave the lane ending in empty
    // space while the crown is plainly somewhere. It is a position, not yet a migration claim.
    // A flicker therefore survives only while it is the LAST point, which costs at most one run.
    const six=run(7700,7700,7705,7705,7705);
    ok(six.length===2 && six[1].k===7705,
       'kt8 a six-minute hold is drawn while it is the CURRENT seat', six.map(x=>x.k));
    const later=run(7700,7700,7705,7705,7705,7700,7700,7700,7700);
    ok(later.length===1,
       'kt8b ...and once it has been REPLACED, a six-minute hold is dropped as the flicker it was',
       later.map(x=>x.k));
    const held=[7700,7700].concat(new Array(9).fill(7705)).concat([7700,7700]);
    const M=run.apply(null, held);
    ok(M.length===3 && M[1].k===7705,
       'kt8c ...while a 24-minute hold survives being replaced', M.map(x=>x.k));
    // ⚠⚠ kt7/kt8 CANNOT SEE WHETHER KT_DWELL IS HONOURED — at 2 the loop structurally needs two
    // sightings to reach the push, so deleting the dwell check changes nothing and survived mutation.
    // The property is that the CONSTANT governs, not that the code happens to need two passes.
    // ⚠ MY FIRST VERSION OF kt8b WAS WRONG, NOT THE CODE: it reused kt8's fixture, which has THREE
    // sightings of 7705 — at dwell 3 that IS a migration. The case has to have exactly two.
    // ⚠ THE CONSTANT MUST GOVERN, not the loop's shape. Same fixture, two thresholds, two answers.
    global.KT_DWELL_MIN=60; RP_KT.key=null;
    // `held` ends on 7700, so the 24-minute 7705 run has been REPLACED and is judged: at 60 minutes
    // it is a flicker, and only the seed plus the current seat survive — which are the same strike,
    // so the lane collapses to one point. That is the correct answer and my first expectation of 2
    // was simply wrong about the fixture.
    ok(run.apply(null, held).length===1,
       'kt8d at a 60-minute dwell the 24-minute hold is dropped — the constant governs',
       run.apply(null, held).map(x=>x.k));
    global.KT_DWELL_MIN=5; RP_KT.key=null;
    ok(run(7700,7700,7705,7705,7705,7700,7700,7700).length===3,
       'kt8e ...and at five minutes the same six-minute hold survives replacement');
    // ⚠ A GAP IN THE RECORDING MUST NOT PROMOTE A FLICKER. Two frames far apart are two sightings,
    // not a long hold — which is precisely the confusion a COUNT could never notice.
    global.KT_DWELL_MIN=20; RP_KT.key=null;
    REPLAY.frames=[mk(0,7700), mk(1,7700), mk(2,7705), {t:T0+3*3600000, tri:{SPXW:{king:7705}}, exp:null}];
    REPLAY.idx=3; RP_KT.key=null;
    const G=ktOf('SPXW');
    ok(G.length===2, 'kt8f two sightings THREE HOURS apart do satisfy a duration — it is a real hold', G.length);
    global.KT_DWELL_MIN=20;
  }
  // ---- (v15.24) THE RULE IS ENFORCED WHERE THE LANE IS READ ---------------------------------
  // v15.23 made dwell a duration in both WRITE paths, and the operator's lane stayed erratic:
  // KTRACK already held the day's points, recorded under the old ~6-second rule, and a threshold
  // enforced only at write time cannot reach a record that already exists.
  {
    const T0=Date.parse('2026-08-31T14:00:00Z'), M=60000;
    // a track as the OLD rule would have written it: three flickers and two real holds
    const stored=[ {t:T0,           k:7700, seed:true},
                   {t:T0+2*M,       k:7705},          // held 2 minutes  → flicker
                   {t:T0+4*M,       k:7710},          // held 3 minutes  → flicker
                   {t:T0+7*M,       k:7715},          // held 40 minutes → REAL
                   {t:T0+47*M,      k:7720},          // held 1 minute   → flicker
                   {t:T0+48*M,      k:7725} ];        // last point, still holding → kept
    global.KT_DWELL_MIN=20;
    const F=ktFilterDwell(stored);
    ok(F.length===3, 'kt13 EXECUTED: a track written under the old rule is filtered when it is READ', F.map(p=>p.k));
    ok(F[0].seed===true, 'kt13b ...the seed survives — it is an origin, not a migration');
    ok(F[1].k===7715, 'kt13c ...the 40-minute hold survives', F.map(p=>p.k));
    ok(F[2].k===7725, 'kt13d ...and the LAST point survives: it is still holding, so its run is not yet decided');
    ok(F.every(p=>p.k!==7705 && p.k!==7710 && p.k!==7720), 'kt13e ...and every flicker is gone', F.map(p=>p.k));
    // ⚠ the live lane must obey it too, not only replay — that is the case he was looking at
    REPLAY.on=false;
    global.KTRACK={ v:1, day:'x', b:{ SPXW:stored } };
    ok(ktOf('SPXW').length===3, 'kt14 the LIVE lane is filtered by the same rule', ktOf('SPXW').length);
    global.KT_DWELL_MIN=0;
    ok(ktOf('SPXW').length===stored.length, 'kt14b ...and at a zero dwell nothing is dropped, so the filter is the constant');
    global.KT_DWELL_MIN=20; REPLAY.on=true;
  }

  // and the journey grows as the handle moves
  REPLAY.frames=FR; RP_KT.key=null; REPLAY.idx=0;
  const early=ktOf('SPXW').length; RP_KT.key=null; REPLAY.idx=2;
  const later=ktOf('SPXW').length;
  ok(later>=early, 'kt9 dragging forward can only ADD to the journey, never lose it', [early,later]);
  // live must still read the live latch
  REPLAY.on=false;
  global.KTRACK={ v:1, day:'x', b:{ SPXW:[{t:1,k:7777}] } };
  ok(ktOf('SPXW')[0].k===7777, 'kt10 live still reads KTRACK, untouched');
  REPLAY.on=true;
}
{
  const kt=decomment(ex('ktTick'));
  ok(/if\(!arr\.length\)\{ arr\.push\(/.test(kt),
     'kt11 the LIVE track seeds its first observation, so a crown that never moves still draws a run');
  ok(/recorderBlind\(\)/.test(kt), 'kt12 ...and the track still never writes during a replay');
}

// ---- 16 · (v15.15) THE BAND MUST CENTRE ON THE REPLAYED BAR ------------------------------------
// Operator: "the nodeprofile has only 1 node. no arrows, no status. nothing." · "it also says out of sync"
// emBand takes `now` from closedCandles() — the LIVE last close — while the nodes came from the
// frame. emPiles CLIPS every pile to that band, so a 13:12 book measured against a 21:00 band left
// ONE surviving pile: one node bar, no states (they hang off the node rows), nothing for rollScan.
{
  global.STATE={ SPY:{ candles:[{o:9,h:9,l:9,c:9,so:1}] } };
  eval(ex('closedCandles'));
  REPLAY.on=true; REPLAY.frames=FR; REPLAY.idx=2;
  const cs=closedCandles('SPY');
  ok(cs.length===3, 'b1 in replay the BAND reads the frame bars, not the live candles', cs.length);
  ok(cs[cs.length-1].c===FR[2].px, 'b2 ...so `now` is the price at the parked bar', cs[cs.length-1].c);
  REPLAY.idx=0;
  ok(closedCandles('SPY').length===1, 'b3 ...and it shortens with the handle');
  REPLAY.idx=2;
  ok(Math.abs(cs[0].c-FR[0].px)<1e-9, 'b4 the series is the frames in order, underlying scale — no conversion');
  REPLAY.on=false;
  ok(closedCandles('SPY')[0].c===9, 'b5 live still reads STATE candles, untouched');
  REPLAY.on=true;
}
{
  // the sync banner cannot judge a recorded frame
  global.RECON_STATE={};
  global.kingFromTapeTag=()=>1; global.kingFromFeed=()=>2; global.kingFromTapeMax=()=>3;   // maximal disagreement
  global.reconcileVotes=()=>({ok:false, reason:'split'});
  global.RECON_LOG_MAX=20; global.RECON_FAIL_ESCAL=3;
  eval(ex('tapeSync'));
  REPLAY.on=true;
  const r=tapeSync('SPY');
  ok(r.ok===true, 'sy1 in replay the tape-sync gate stands down — a frame cannot be out of sync with live');
  ok(r.reason==='replay', 'sy2 ...and says why, rather than silently reporting health', r.reason);
  ok(r.streak===0, 'sy3 ...and does not accumulate a failure streak while parked');
  REPLAY.on=false;
  const r2=tapeSync('SPY');
  ok(r2.ok===false, 'sy4 live, three disagreeing votes still raise the banner — the guard is intact');
  REPLAY.on=true;
}

// ---- 17 · (v15.16) THE FLOOR THAT REFUSED 120 OF 129 FRAMES ------------------------------------
// Operator: "it still has 100% for one strike, the entire node profile is missing, the arrows dont
// make sense, there are no statuses. total failure." — and it was ONE CONSTANT.
// `SK_MIN_STRIKES = 20` is a LIVE-PARSE health heuristic ("below 20 the DOM changed"). A recorded
// frame's depth is not evidence of a broken parse; it is what the recorder stored, and `vend` was
// capped at 40 rows ACROSS FOUR BOOKS until v15.10. MEASURED on 2026-08-31: 129 frames, SPXW strikes
// min 13 / median 17 / max 40 — so the live floor refused 120 of them, and a skPiles refusal returns
// NO PILES: no nodes, no states (they hang off the node rows), nothing for rollScan to pair.
{
  const sk=decomment(ex('skPiles'));
  ok(/SK_MIN_STRIKES_REPLAY/.test(sk), 'g1 skPiles has a replay floor distinct from the live one');
  ok(/_floor=_rp\?SK_MIN_STRIKES_REPLAY:SK_MIN_STRIKES/.test(sk),
     'g2 ...and picks between them on replayOn(), never one for both');
  ok(num('SK_MIN_STRIKES_REPLAY')<num('SK_MIN_STRIKES'),
     'g3 the replay floor is LOWER — a recorded book is not judged on live-parse health',
     [num('SK_MIN_STRIKES_REPLAY'), num('SK_MIN_STRIKES')]);
  // ⚠ the real bar: the floor must admit the depth his 18 recorded days actually hold.
  ok(num('SK_MIN_STRIKES_REPLAY')<=13,
     'g4 ...and low enough for the THINNEST frame measured (13 SPXW strikes on 2026-08-31)',
     num('SK_MIN_STRIKES_REPLAY'));
  ok(/out\.shallow=out\.count/.test(sk),
     'g5 a replayed book reports its DEPTH, so a shallow ladder cannot read as a thin market');
  ok(/too few to draw a book/.test(sk),
     'g6 ...and a genuinely empty frame still refuses, with a reason about the RECORDING');
  // the live floor must be untouched — this is a health check and it still has to bite
  ok(num('SK_MIN_STRIKES')===20, 'g7 the LIVE floor is unchanged at 20 — the guard still works live');
}
{
  // the strip discloses the depth
  const strip=decomment(ex('replayBarHtml'));
  // ⚠ g8/g9 WERE SOURCE GREPS AND SURVIVED MUTATION: gating the emit with `if(false)` leaves both
  // strings in the text. Bind to the STATEMENT that produces the output, not to words near it.
  ok(/if\(_dp!=null\) h\+=/.test(strip),
     'g8 the depth is EMITTED under a live condition, not merely mentioned');
  ok(/_dp=\(_sp&&_sp\.shallow!=null\)\?_sp\.shallow:null/.test(strip),
     'g8b ...and the value comes from skPiles\' recorded depth');
  ok(/A LIVE ladder reads about 100/.test(strip),
     'g9 ...and the hover says what it is being compared against, not just a bare number');
}

// ---- 18 · (v15.17) THE ARROWS MUST PAIR ROWS THE LADDER ACTUALLY DRAWS ------------------------
// Operator: "see the arrows and determine if they make sense." DECODED off his panel at 14:12 — all
// four were REAL pairs, and all four were wrong to show: 7625->7650 (an $82K shed), 7630->7650,
// 7645->7670, 7655->7670, while the KING's own roll 7675->7670 ($22.4M) and 7700->7685 ($15.3M)
// were absent. The live latch feeds rollScan from tradeNodes() — the nodes ON the rail; replay fed
// it every stored strike. **A true claim about a row the face does not show is worse than silence.**
{
  const rr2=decomment(ex('replayRolls'));
  ok(/CFG\.nodeThresh/.test(rr2), 'n1 the replayed scan reads the node threshold the ladder draws by');
  ok(/Math\.abs\(bk\.pct\[kk\]\)>=_thr/.test(rr2),
     'n2 ...and filters strikes by it, so an arrow cannot point at an undrawn row');
  ok(!/if\(kn>0\) ks\.push\(kn\);/.test(rr2),
     'n3 ...never the unfiltered "every stored strike" form that shipped in v15.13-v15.16');
}
// ---- 19 · (v15.17) A PANEL TALLER THAN THE WINDOW CANNOT SCROLL --------------------------------
// "I also cannot scroll up and down." MEASURED: panel 1016px, viewport 557px, top -307, bottom 152px
// below the screen; body scrollHeight 986 === clientHeight 986, so overflow-y:auto had nothing to do.
{
  let H=1016, T=-307, saved={};
  global.PANEL={ style:{width:'667px',height:'1016px',top:'-307px'},
                 getBoundingClientRect:()=>({height:H, top:T}) };
  global.window={innerHeight:557};
  global.localStorage={ getItem:k=>(k in saved?saved[k]:null), setItem:(k,v)=>{saved[k]=String(v);} };
  global.SIZE_KEY='gpts_panelsize_v7'; global.CFG={};
  eval(ex('panelFit'));
  panelFit();
  ok(PANEL.style.top==='4px', 'p1 a panel dragged above the top edge is pulled back on screen', PANEL.style.top);
  ok(parseInt(PANEL.style.height,10)<=557-4-8+1,
     'p2 ...and its height is clamped to the room below it, so the body can scroll', PANEL.style.height);
  ok(parseInt(PANEL.style.height,10)>200, 'p3 ...but never collapsed to nothing', PANEL.style.height);
  ok(saved['gpts_panelsize_v7'], 'p4 the clamp is remembered, so it survives a reload');
  // a panel that already fits must never be touched
  H=400; T=10; PANEL.style.height='400px'; PANEL.style.top='10px';
  panelFit();
  ok(PANEL.style.height==='400px', 'p5 a panel that FITS is left alone — this only ever shrinks', PANEL.style.height);
  ok(PANEL.style.top==='10px', 'p6 ...and an on-screen position is not moved');
  // and the order matters: widening can change the content height
  const rnd=decomment(ex('render'));
  const lIdx=rnd.indexOf('ladderFit'), pIdx=rnd.indexOf('panelFit');
  ok(lIdx>-1 && pIdx>lIdx, 'p7 the vertical clamp runs AFTER the width fit, on the settled box');
}

// ---- 20 · (v15.18) THE DAY PEAK MUST NOT INCLUDE THE BAR'S OWN FUTURE ---------------------------
// Operator: "how is it that the stats of all except 1 is spent and only 1 is weakening.. something
// is not right." He was right. `levelStateOf` computes ret = |cur| / peakOf(k) and calls a level
// SPENT below 0.50. In replay the NUMERATOR was the frame's mass and the DENOMINATOR was the LIVE
// day-peak tracker — the maximum over the WHOLE session, including every bar AFTER the one drawn.
//
// MEASURED at 14:12 on 2026-08-31, retention against each denominator:
//        strike      |cur|   peak->14:12  ret    peak WHOLE DAY  ret
//        7675     81988795     81988795  1.00         115827347  0.71
//        7700     51413336    111492387  0.46         173133335  0.30
//        7685     34840580     34840580  1.00         503965848  0.07   <-- at its OWN peak, called SPENT
//        7695     19423931     39483659  0.49          97244233  0.20
//        7670     18218115     40808410  0.45          40808410  0.45
// 7685 sat at 100% of its peak-to-date and was labelled SPENT because a spike that had not happened
// yet was in the denominator.
{
  global.PEAK={ m:{ 7685:503965848, 7675:115827347 } };   // the LIVE tracker: whole-day maxima
  global.RP_PEAK={ key:null, m:null };
  eval(ex('replayPeakOf')); eval(ex('peakOf'));
  const mk=(t,rows)=>({t:t, tri:{SPXW:{king:7675}}, vend:{rows:rows}});
  //                     k        cur       d5 d15 d60 d1d
  REPLAY.on=true; REPLAY.day='2026-08-31';
  REPLAY.frames=[ mk(1,[[7685, 34840580,0,0,0,0]]),
                  mk(2,[[7685, 20000000,0,0,0,0]]),
                  mk(3,[[7685,503965848,0,0,0,0]]) ];   // the spike happens at frame 3
  REPLAY.idx=1;
  ok(peakOf(7685)===34840580,
     'q1 the replayed peak is the max UP TO the parked bar — the later spike has not happened yet',
     peakOf(7685));
  ok(Math.abs(34840580/peakOf(7685)-1)<1e-9,
     'q2 ...so a level at its own peak-to-date reads 100% retention, not 7%');
  RP_PEAK.key=null; REPLAY.idx=2;
  ok(peakOf(7685)===503965848, 'q3 dragging PAST the spike includes it — the peak grows with the handle', peakOf(7685));
  RP_PEAK.key=null; REPLAY.idx=1;
  ok(peakOf(9999)===null, 'q4 a strike the frames never carried has NO peak, rather than a wrong one');
  REPLAY.on=false;
  ok(peakOf(7685)===503965848, 'q5 live still reads the live tracker, untouched', peakOf(7685));
  REPLAY.on=true;
}
{
  // taps are not recorded per frame — a confident zero would invent "no interaction at all"
  const ls=decomment(ex('levelStateOf'));
  ok(/tapsKnown/.test(ls), 'q6 the state engine distinguishes "no taps" from "taps not tracked"');
  ok(/tapsKnown && tapsN===0/.test(ls),
     'q7 ...and only claims DECAYING when a zero is actually KNOWN');
  // ⚠ q8 WAS A SOURCE GREP AND SURVIVED MUTATION — gating the assignment with `if(false)` leaves the
  // text in place. levelStateOf is EXECUTED instead, which is the only way to see which branch ran.
  global.LVL_TURN_P15=num('LVL_TURN_P15'); global.LVL_WEAK_P15=num('LVL_WEAK_P15');
  global.LVL_BUILD_P15=num('LVL_BUILD_P15'); global.LVL_SPENT_PEAK=num('LVL_SPENT_PEAK');
  global.TAP_PROB=[80,66,33];
  global.nodeTapCount=()=>0;                       // the LIVE counter would say a confident zero
  global.velAt=()=>({ v:{ cur:100, p5:0, p15:0, p60:0 }, stale:false });
  global.PEAK={ m:{} };
  global.RP_PEAK={ key:null, m:null };
  eval(ex('replayPeakOf')); eval(ex('peakOf')); eval(ex('levelStateOf'));
  REPLAY.on=true; REPLAY.day='d'; REPLAY.frames=[{t:1,tri:{SPXW:{king:7675}},vend:{rows:[[7675,100,0,0,0,0]]}}]; REPLAY.idx=0;
  const stR=levelStateOf(7675, null);
  ok(stR.taps===null, 'q8 EXECUTED: in replay the tap count is UNKNOWN, not a confident zero', stR.taps);
  ok(!/no interaction at all/.test(stR.why),
     'q8b ...so DECAYING — "a quiet death" — is never claimed from a count we do not have', stR.why);
  ok(/not recorded per frame/.test(stR.why),
     'q8c ...and the face says WHY it is silent rather than just omitting it', stR.why);
  REPLAY.on=false;
  const stL=levelStateOf(7675, null);
  ok(stL.taps===0, 'q8d live, a real zero is still a real zero', stL.taps);
  REPLAY.on=true;

  // ---- s · THE RATE OF CHANGE A REPLAYED ROW CAN HONESTLY CARRY --------------------------------
  // ⚠⚠ Until v15.18 a replayed row had NO p5/p15/p60 — a recorded row is [k,cur,d5,d15,d60,d1d] and
  // the vendor's percents are not among them — while every state branch but SPENT sits behind those
  // fields being numbers. So a replayed face could only ever say SPENT, WEAKENING-via-roll-source
  // or HOLDING, which is exactly what the operator saw: "2 weakening and everything else spent".
  // These use the REAL 14:12 CT rows of 2026-08-31.
  eval(ex('replayBook')); eval(ex('replayBookOf'));
  REPLAY.on=true; REPLAY.day='d'; REPLAY.idx=0;
  REPLAY.frames=[{ t:1, tri:{ SPXW:{ king:7675 } }, vend:{ rows:[
    [7675,-81988795,-8775320,-22426842,-88620743,-78949899],
    [7670,-18218115,   803045, 21974932,  2050583,-13918766] ]}}];
  const RB=replayBook('SPXW');
  const r75=RB.vel['7675'], r70=RB.vel['7670'];
  ok(typeof r75.p15!=='number',
     's1 a replayed row NEVER carries p15 — that name means Skylit\'s own percent15Min', r75.p15);
  ok(typeof r75.rp15==='number', 's2 ...it carries rp15, this panel\'s own measure, under its own name');
  ok(Math.round(r75.rp15)===38,
     's3 ...and rp15 is the change in MASS: -59.6M to -82.0M is +38%, not -38%', r75.rp15);
  ok(Math.round(r70.rp15)===-55, 's4 ...while 7670, decaying toward zero, reads -55%', r70.rp15);

  global.velAt=k=>{ const v=RB.vel[String(k)]; return v?{ v:v, age:0, stale:false }:null; };
  PEAK={ m:{} }; RP_PEAK={ key:null, m:null };
  const s75=levelStateOf(7675, null);
  ok(s75.st==='BUILDING',
     's5 EXECUTED: the King deepening into its own peak is BUILDING, not WEAKENING', s75.st);
  ok(/not Skylit/.test(s75.why),
     's6 ...and the hover says the number is ours, measured from the recorded deltas', s75.why);
  // and the vendor's own percent still wins wherever it exists — live behaviour is untouched
  global.velAt=()=>({ v:{ k:7675, cur:-81988795, d15:-22426842, rp15:37.7,
                          p5:-9, p15:-38, p60:-12 }, age:0, stale:false });
  const sVend=levelStateOf(7675, null);
  ok(sVend.st==='WEAKENING' && !/not Skylit/.test(sVend.why),
     's7 a row that HAS the vendor percents uses them, and says nothing about a derivation', sVend.st);
  // a strike with no mass 15m ago has no percentage — undefined, never Infinity
  REPLAY.frames=[{ t:1, tri:{ SPXW:{ king:7675 } }, vend:{ rows:[[7675,5000000,0,5000000,0,0]] }}];
  const RB0=replayBook('SPXW');
  ok(RB0.vel['7675'].rp15===undefined,
     's8 a prior of ZERO yields no percent at all, rather than Infinity', RB0.vel['7675'].rp15);
  global.velAt=()=>({ v:RB0.vel['7675'], age:0, stale:false });
  const s9=levelStateOf(7675, null);
  ok(s9.st==='HOLDING',
     's9 ...and the state engine simply says nothing about a rate it does not have', s9);
  REPLAY.on=true;
}

// ---- 16 · (v15.24) A FRAME WITH NO BOOK IS NOT A SNAPSHOT ------------------------------------
// Operator, 2026-09-01: "why the replay feature currently cannot capture a snapshot of the day".
// MEASURED on his recording that morning: 34 frames and EIGHT of them carried no `tri`, no `vend`,
// no `px` — empty shells written on bars where the tape had nothing. The slider offered them as
// seekable ticks, so the handle landed on one and the face went blank. That reads as "replay is
// broken" rather than "nothing was recorded at 09:46".
{
  eval(ex('replayUsable'));
  const full={ t:1, px:766, tri:{SPXW:{king:7675}}, vend:{ rows:[[7675,1,0,0,0,0]] } };
  ok(replayUsable(full)===true, 'u1 a frame with a book is usable');
  ok(replayUsable(Object.assign({},full,{tri:null}))===false, 'u2 ...without tri it is not');
  ok(replayUsable(Object.assign({},full,{vend:{rows:[]}}))===false, 'u3 ...nor with an empty vendor list');
  ok(replayUsable(Object.assign({},full,{px:null}))===false, 'u4 ...nor without a price');
  ok(replayUsable(null)===false, 'u5 ...and a missing frame is not usable');
  // ⚠ THE TEST IS THE BOOK, NOT THE PRICE: a pre-open frame can carry a price and no book, and it
  // is still useless to every surface replay draws.
  ok(replayUsable({ t:1, px:766, tri:{SPXW:{king:7675}}, vend:{rows:[]} })===false,
     'u6 a price with no book is still not a snapshot');
  // ⚠ AND THE LOADER MUST DROP THE ONES ALREADY ON DISK. The write guard cannot reach the eighteen
  // days already recorded, where the slider would still offer ticks that lead to a blank face.
  // This SURVIVED its first mutation because nothing executed the loader — the assertions above only
  // exercised the predicate, and a predicate nobody calls filters nothing.
  {
    // ⚠ REAL TIMESTAMPS. `replaySecOf` is eval'd from the source at the top of this file, so
    // assigning global.replaySecOf shadows nothing — the real one runs, reads `f.t`, and a fixture
    // stamped t:1000 lands in 1970 and is filtered out as after-hours. The first cut of u8 read 0
    // frames for that reason and looked like the new filter dropping everything.
    const T=Date.parse('2026-08-31T14:00:00Z');           // 09:00 CT
    const good=[{t:T, px:766, tri:{SPXW:{king:7675}}, vend:{rows:[[7675,1,0,0,0,0]]}},
                {t:T+360000, px:767, tri:{SPXW:{king:7680}}, vend:{rows:[[7680,1,0,0,0,0]]}}];
    const empty={t:T+180000};
    const mixed=[good[0], empty, good[1]];
    global.repoDay=(date,cb)=>cb({ snaps:{ SPY:mixed } });
    global.replayDayLabel=()=>'a day';
    global.RP_OPEN_SEC=8*3600+30*60; global.RP_CLOSE_SEC=15*3600;
    global.render=()=>{};
    eval(ex('replayLoadDay'));
    let n=null; replayLoadDay('2026-08-31', c=>{ n=c; });
    ok(n===2, 'u8 EXECUTED: loading a day drops the frames that carry no book', n);
    ok(REPLAY.frames.every(replayUsable), 'u8b ...so every tick on the track leads to a face');
    ok(REPLAY.skipped===1, 'u8c ...and it reports how many it dropped rather than hiding them', REPLAY.skipped);
    // a day of nothing BUT empties must say so, not read as "no frames recorded"
    global.repoDay=(date,cb)=>cb({ snaps:{ SPY:[empty] } });
    replayLoadDay('2026-08-31', ()=>{});
    ok(/carry no book/.test(REPLAY.err||''),
       'u9 a day of empty frames says the frames carry no book, not that none were recorded', REPLAY.err);
  }

  // and the WRITE path refuses to store one in the first place
  const rec=src.slice(src.indexOf('A FRAME WITH NO BOOK IS NOT A SNAPSHOT'));
  ok(/var _hasBook=[\s\S]{0,220}if\(!_hasBook\)\{[\s\S]{0,160}return;/.test(rec),
     'u7 the recorder returns instead of pushing an empty frame');
}

console.log('test_replay: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
