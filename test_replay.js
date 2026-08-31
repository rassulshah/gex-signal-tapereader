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
  eval(ex('replayBarHtml'));
  REPLAY.on=false; REPLAY.frames=FR; REPLAY.idx=2; REPLAY.day='2026-08-31'; REPLAY.days=['2026-08-28','2026-08-31']; REPLAY.err=null;
  const live=replayBarHtml();
  ok(typeof live==='string' && live.length>0, 'x1 the strip renders LIVE without throwing', live.length);
  ok(/LIVE/.test(live) && !/REPLAY<\/span>/.test(live), 'x2 ...and says LIVE, not REPLAY');
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

console.log('test_replay: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
