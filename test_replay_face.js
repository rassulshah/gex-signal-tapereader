// ============================================================================================
// test_replay_face.js — (v15.18) THE FACE IS RENDERED, NOT REASONED ABOUT.
//
// Operator, 2026-09-01: "there are things that are missing , even the headings are missing , the
// king path in the king lanes are missing. i cant scroll up or down either. double check and figure
// out.. my patience with you is running dry."
//
// ⚠⚠ WHY THIS FILE EXISTS. Every other test in this project executes a FUNCTION. That proves a
// function returns the right value and proves NOTHING about whether the section that calls it
// survives to draw. Nine replay defects in a row were found by the operator looking at his own
// panel, because nothing here could draw the panel. A section that refuses is swallowed BY DESIGN,
// so a broken replay looks exactly like a quiet one — and one refusal upstream (`emBand`) took the
// ladder, the node states, the percentages, the king lanes, the roll arrows and the ROC column with
// it, with no error anywhere.
//
// This loads the REAL userscript into jsdom, parks it on a REAL recorded minute out of
// data/2026-08-31.json, renders, and asserts on what came out. tools/render-face.js is the same
// harness with a human-readable report.
// ⚠ jsdom has no layout engine: every box measures 0, so nothing here can assert about SCROLLING or
// fitted widths. Those are CSS invariants (the panel's max-height) and are asserted as such.
// ============================================================================================
const fs=require('fs'), vm=require('vm');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,220):''));} };
let JSDOM=null;
try{ JSDOM=require('jsdom').JSDOM; }catch(e){}
if(!JSDOM){ console.log('test_replay_face: SKIPPED — jsdom not installed (npm install jsdom)'); process.exit(0); }

const src=fs.readFileSync('./v10.js','utf8');
const DAY='2026-08-31';
const D=JSON.parse(fs.readFileSync('./data/'+DAY+'.json','utf8'));
const FR=(D.snaps&&D.snaps.SPY)||[];
const hhmm=t=>new Date(t).toLocaleTimeString('en-US',{timeZone:'America/Chicago',hour12:false,hour:'2-digit',minute:'2-digit'});
const mins=s=>(+s.split(':')[0])*60+(+s.split(':')[1]);
function frameAt(want){ let i=0,b=1e9; FR.forEach((f,j)=>{const d=Math.abs(mins(hhmm(f.t))-mins(want)); if(d<b){b=d;i=j;}}); return i; }

// one render, returning the body html plus a hook to ask the loaded script questions
function renderAt(want, opts){
  opts=opts||{};
  const dom=new JSDOM('<!doctype html><html><body></body></html>',
    { url:'https://app.skylit.ai/atlas', pretendToBeVisual:true });
  const win=dom.window;
  win.matchMedia=win.matchMedia||(q=>({matches:false,media:q,addListener(){},removeListener(){},addEventListener(){},removeEventListener(){}}));
  win.requestAnimationFrame=cb=>0; win.cancelAnimationFrame=()=>{};
  // no timers: boot() starts intervals and jsdom would keep the loop alive forever
  win.setInterval=()=>0; win.clearInterval=()=>{}; win.setTimeout=()=>0; win.clearTimeout=()=>{};
  win.fetch=()=>new Promise(()=>{});
  const store={};
  Object.defineProperty(win,'localStorage',{ value:{
    getItem:k=>(k in store)?store[k]:null, setItem:(k,v)=>{store[k]=String(v);},
    removeItem:k=>{delete store[k];}, clear:()=>{}, key:i=>Object.keys(store)[i]||null,
    get length(){return Object.keys(store).length;} }, configurable:true });
  win.indexedDB=undefined;
  const i0=src.indexOf('(function(){'), i1=src.lastIndexOf('})();');
  const ctx=vm.createContext(win); win.window=win;
  const log=console.log; console.log=()=>{};                  // the script announces its five parts
  try{ vm.runInContext(src.slice(i0+'(function(){'.length, i1), ctx, {filename:'gex.user.js'}); }
  finally{ console.log=log; }
  const run=c=>vm.runInContext(c, ctx);
  run('typeof buildPanel==="function" ? buildPanel() : (typeof boot==="function" ? boot() : 0)');
  const idx=frameAt(want);
  run(`REPLAY.on=${opts.live?'false':'true'}; REPLAY.day=${JSON.stringify(DAY)};
       REPLAY.frames=${JSON.stringify(FR)}; REPLAY.idx=${idx};`);
  run('RENDER_ERRS.length=0');
  let threw=null;
  try{ run('render()'); }catch(e){ threw=e.message; }
  const html=run('elBody?elBody.innerHTML:""');
  // ⚠ NOT textContent: adjacent elements have no whitespace between them, so "HL RNG" and its value
  // arrive glued to their neighbours and every field regex silently matches nothing. Tags become
  // spaces instead — the reading a person gets off the screen.
  const text=html.replace(/<[^>]+>/g,' ').replace(/\s+/g,' ');
  return { html:html, threw:threw, run:run, idx:idx, text:text,
           errs:JSON.parse(run('JSON.stringify(__gptsDebug.renderErrors())')||'[]') };
}

const R=renderAt('14:12');
ok(!R.threw, 'f0 the panel renders on a replayed minute without throwing', R.threw);
ok(R.errs.length===0, 'f0b ...and swallows nothing on the way', R.errs);

// ---- 1 · THE BAND, WHICH IS THE GATE EVERYTHING BELOW IT LIVES INSIDE -----------------------
// emBand pins the day's expected move ONCE from the live 0DTE straddle, keyed to the session shown.
// A replayed day has no such record, so it fell through to today's chain — which after hours does
// not quote — and returned "no EM". The ladder, the states, the lanes and the arrows all vanished.
ok(!/no EM/.test(R.text), 'f1 the replayed band does NOT fall through to today’s straddle chain');
const EB=JSON.parse(R.run('JSON.stringify((function(){var b=emBand("SPY");return {ok:b.ok,why:b.why,em:b.em,open:b.open,pin:!!b.replayPin};})())'));
ok(EB.ok===true, 'f2 EXECUTED: the band comes back OK with no chain present at all', EB);
ok(EB.pin===true, 'f2b ...pinned from the frame, not from a live capture');
ok(Math.abs(EB.em-3.49)<0.001, 'f2c ...with the day’s own recorded expected move', EB.em);

// ---- 2 · THE NODE RAIL: EVERY RECORDED STRIKE, AT ITS RECORDED %KING -------------------------
// The frame's own tri.SPXW.top for this minute is 7675/-100, 7700/-63, 7685/42, 7695/24, 7670/-22.
const TOP=FR[R.idx].tri.SPXW.top;
ok(TOP.length>=5, 'f3 the fixture frame carries its own recorded ladder', TOP.length);
// ⚠ the KING's own row is labelled KING rather than ±100% — that is the live rail's own choice and
// replay must not differ from it, so the crown is asserted by its label and the rest by their value.
ok(/765\.77[\s\S]{0,80}KING/.test(R.html.replace(/<[^>]+>/g,' ')),
   'f4k the crown’s row is labelled KING, exactly as the live rail labels it');
TOP.slice(0,5).forEach(function(row){
  if(Math.abs(row[1])===100) return;
  const pctTxt=(row[1]<0?'\u2212':'+')+Math.abs(row[1])+'%';
  ok(R.html.indexOf(pctTxt)>=0,
     'f4 the rail draws '+row[0]+' at its RECORDED '+pctTxt+', not a recomputed one', pctTxt);
});
ok(/SPENT|BUILDING|WEAKENING|TURN/.test(R.text), 'f5 the level states are on the rail');
ok((R.html.match(/g3ldst/g)||[]).length>=2, 'f5b ...on more than one row',
   (R.html.match(/g3ldst/g)||[]).length);

// ---- 3 · THE ROC COLUMN ---------------------------------------------------------------------
ok((R.html.match(/g3ldroc/g)||[]).length>=5, 'f6 the ROC column draws in replay rather than sitting empty',
   (R.html.match(/g3ldroc/g)||[]).length);
ok(R.html.indexOf('g3ldrocr')>=0, 'f6b ...marked as THIS panel’s measure');
// ⚠ SCOPED TO THE ROC CELL. The first cut tested the WHOLE body for "not Skylit", which the level
// hover also says — so it passed with the ROC tip crediting Skylit for our number. Survived mutation.
const rocTip=(R.html.match(/class="g3ldroc[^"]*"[^>]*title="([^"]*)"/)||[])[1]||'';
ok(/measured by THIS PANEL/.test(rocTip),
   'f6c ...and the ROC hover itself says the number is ours', rocTip.slice(0,90));
ok(!/Skylit\u2019s own rate of change/.test(rocTip),
   'f6d ...never crediting Skylit for a percent they did not supply', rocTip.slice(0,90));

// ---- 4 · THE CLOCK --------------------------------------------------------------------------
// sessionPhase() read the wall clock, so a Monday 14:12 said AFTER HOURS — and that branch RETIRES
// the target, the budget and the roll arrows.
ok(!/AFTER HOURS/.test(R.text), 'f7 a bar parked inside RTH is not labelled AFTER HOURS');
const SP=JSON.parse(R.run('JSON.stringify((function(){var p=sessionPhase();return {rth:p.rth,mins:p.mins,label:p.label};})())'));
ok(SP.rth===true, 'f7b EXECUTED: sessionPhase reports the PARKED minute, in RTH', SP);
ok(SP.mins===mins('14:12'), 'f7c ...to the minute', SP.mins);

// ---- 5 · THE KING LANE'S TIME AXIS ----------------------------------------------------------
// The runs were always emitted; with Date.now() the axis ran from the replayed open to tonight, so
// a whole session's journey was crushed into the first few pixels and read as "missing".
// ⚠ PER LANE, NOT ACROSS BOTH. The first cut measured min-to-max over every run on the panel, which
// spans the two COLUMNS and is wide however badly each one is crushed — it passed with the axis back
// on Date.now(). Survived mutation. Each lane is measured on its own, by its own class.
function laneRuns(html, cls){
  return [...html.matchAll(new RegExp('class="g3ldkrun g3ldk'+cls+'[^"]*"\\s*style="left:([\\d.]+)px','g'))].map(m=>+m[1]);
}
['S','Y'].forEach(function(cls){
  const xs=laneRuns(R.html, cls);
  // ⚠ (v15.23) THE COUNT IS NOW BOUNDED ABOVE AS WELL. Dwell became a DURATION (20 minutes,
  // measured over the 11 recorded sessions: SPXW median 2 [0-4], SPY 3 [0-5]), because a count of
  // observations meant ~6 seconds live and 6 minutes in replay — the same constant, two rules, and
  // a lane the operator called "too erratic ... there should only be a couple of movements in a
  // day". A lower bound alone would pass a lane that had gone back to fifteen.
  ok(xs.length>=2 && xs.length<=7,
     'f8'+cls+' the '+cls+' king lane draws a HANDFUL of runs — a journey, not a flicker reel', xs.length);
  const span=xs.length?(Math.max(...xs)-Math.min(...xs)):0;
  // the lane column is LAD_KSW/LAD_KYW wide (~26px); a session's runs must use most of it. With the
  // wall clock the whole morning landed inside ~6px.
  ok(span>12, 'f8'+cls+'b ...spread across ITS OWN lane, not crushed at the open',
     {cls:cls, span:+span.toFixed(1), xs:xs});
});

// ---- 5b · THE ES CHART, WHERE THE PIN USED TO BE HEALED AWAY --------------------------------
// ⚠ The EM floor is computed with the LIVE ratio: on a futures chart it is ~10x, so a recorded band
// in the book's own points (3.49) is judged against a floor of ~7.7 and discarded as "implausibly
// small" — the exact v15.12 failure, one build later, on a recorded pin instead of a captured one.
// Operator, 2026-08-31: "when i switch to es, it doesn't work.. do i have to be on the spy".
{
  const RF=renderAt('14:12');
  // ⚠ dispIsFut() reads FUTMODE.fam && FUTMODE.ok — not `on`, not CFG. A stub with the wrong keys
  // leaves the panel on the cash ruler, the floor stays small, and the assertion passes without ever
  // reaching the branch it is about. That is how this one survived its first mutation.
  RF.run('FUTMODE={fam:"ES", ok:true, live:true, r:10.04, futPx:7690, approx:false};');
  ok(RF.run('dispIsFut()')===true, 'f8es0 the harness really is on a futures-scaled chart');
  ok(RF.run('dispR()')>9, 'f8es0b ...at the ES ratio, which is what makes the floor bite');
  const B2=JSON.parse(RF.run('JSON.stringify((function(){ var b=emBand("SPY"); return {ok:b.ok,why:b.why,healed:!!b.emHealed,pin:!!b.replayPin,em:b.em}; })())'));
  ok(B2.ok===true, 'f8es EXECUTED on a futures-scaled chart: the replayed band still stands', B2);
  ok(B2.healed!==true, 'f8es2 ...and is NOT healed away by a floor computed on the other ruler', B2);
}

// ---- 6 · THE SCALE, AND THE LEVELS THAT MUST NOT BE INVENTED ---------------------------------
const IL=JSON.parse(R.run('JSON.stringify((function(){var l=ifLadder("SPY");return {ds:l.dispScale,rows:(l.rows||[]).length,src:l.scaleSrc,err:l.err};})())'));
const F=FR[R.idx];
ok(Math.abs(IL.ds-(F.px/F.xm.SPXW.px))<1e-5,
   'f9 the ladder’s scale is the FRAME’S own basis, px / xm.SPXW.px', IL);
ok(IL.rows===0, 'f9b ...and it carries NO level rows: today’s levels over a past day would be a lie', IL.rows);

// ---- 7 · A PANEL CAN NEVER OUTGROW THE WINDOW ------------------------------------------------
// ⚠ Asserted on the STYLE, not on a measurement: jsdom has no layout. "I cannot scroll" has been
// reported three times and each time the body scrolled correctly while the panel itself was taller
// than the viewport, with its top and bottom off-screen.
const ph=R.run('document.getElementById("gpts-panel").style.maxHeight');
ok(/100vh/.test(ph||''), 'f10 the panel carries a viewport max-height in its own CSS', ph);
ok(R.run('document.getElementById("gpts-body").style.overflowY')==='auto',
   'f10b ...and the body is the scrolling region inside it');

// ---- 8 · AND THE VALUES ACTUALLY MOVE WITH THE SLIDER ----------------------------------------
// "when i select a time, do the values change in the hod/lod section, the candle values"
const A=renderAt('09:00'), B=renderAt('14:12');
const num=(t,re)=>{ const m=t.match(re); return m?m[1]:null; };
ok(num(A.text,/LOD (\d{2}:\d{2})/)!==num(B.text,/LOD (\d{2}:\d{2})/),
   'f11 the HOD/LOD section is a different reading at 09:00 than at 14:12',
   [num(A.text,/LOD (\d{2}:\d{2})/), num(B.text,/LOD (\d{2}:\d{2})/)]);
ok(num(A.text,/HL RNG ([\d.]+)/)!==num(B.text,/HL RNG ([\d.]+)/),
   'f11b ...and so are the candle/DAY figures',
   [num(A.text,/HL RNG ([\d.]+)/), num(B.text,/HL RNG ([\d.]+)/)]);
ok(num(A.text,/KING ([\d.]+)/)!==num(B.text,/KING ([\d.]+)/),
   'f11c ...and the King itself', [num(A.text,/KING ([\d.]+)/), num(B.text,/KING ([\d.]+)/)]);

// ---- 9 · THE READ IS OFF, AND IT IS A SETTING RATHER THAN A DELETION -------------------------
// Operator, 2026-08-31: "take out the read. I might come back to it later." Restated 2026-09-01:
// "i said to remove the read , where it says event day" — the FIRST removal took the wrong line.
// The one he means is `.g3tread`, the paragraph opening "EVENT day · Trinity 2-of-3 …".
{
  ok(R.html.indexOf('g3tread')<0, 'r1 the read line is NOT drawn', R.html.indexOf('g3tread'));
  ok(!/EVENT day|RANGE day \u00b7 Trinity/.test(R.text), 'r1b ...and its opening words are nowhere on the face');
  const cfg=JSON.parse(R.run('JSON.stringify({read:CFG.read, dayHL:CFG.dayHL})'));
  ok(cfg.read===false, 'r2 CFG.read is off by default', cfg);
  // ⚠ everything that BUILDS the read still runs — he may want it back, and a deletion would make
  // that a build instead of a toggle. Turning it on must actually bring it back.
  R.run('CFG.read=true; render();');
  const on=R.run('elBody.innerHTML');
  ok(on.indexOf('g3tread')>=0, 'r3 EXECUTED: switching it on brings the read back, so nothing was deleted');
  ok(/EVENT day|RANGE day|day \u00b7/.test(on.replace(/<[^>]+>/g,' ')), 'r3b ...with its day-type opening intact');
  // ...and the render audit must not call the deliberate absence a fault
  R.run('CFG.read=false; render();');
  const aud=JSON.parse(R.run('JSON.stringify(__gptsDebug.audit())'));
  // ⚠ THIS IS WHERE THE AUDIT'S OWN BUG WAS FOUND. It read `body.innerText` — a rendering-dependent
  // property that any layout-free DOM returns undefined for — so it tested the string "undefined"
  // against itself, reported "the face prints undefined somewhere", and then threw on `.split`.
  // Fixed at v15.20 (itxt()). Still scoped to the read here, because the harness has no companion
  // and therefore legitimately has no EM pill.
  const readViols=(aud.violations||[]).filter(v=>/read/i.test(v));
  ok(readViols.length===0, 'r4 the render audit does not call the deliberate absence a fault', readViols);
  ok(!(aud.violations||[]).some(v=>/prints "undefined"/.test(v)),
     'r4c the audit no longer invents a violation out of its own missing probe', aud.violations);
  R.run('CFG.read=true; render();');
  const audOn=JSON.parse(R.run('JSON.stringify(__gptsDebug.audit())'));
  ok((audOn.violations||[]).filter(v=>/read/i.test(v)).length===0,
     'r4b ...and still checks the read properly when it IS switched on',
     (audOn.violations||[]).filter(v=>/read/i.test(v)));
  R.run('CFG.read=false; render();');
}

// ---- 10 · THE LADDER'S COLUMN HEADERS -------------------------------------------------------
// Operator, 2026-09-01: "there are also no headers ? what happened to them."
// They were never lost — never CARRIED OVER. v14.46 replaced the rail + node profile with the
// ladder, and the profile's header row (.g3ndhd) went with the surface it belonged to. Ten columns
// arrived unlabelled, and nothing failed, because a missing label throws nothing.
{
  const hd=(R.html.match(/<div class="g3ladhd">([\s\S]*?)<\/div>/)||[])[1]||'';
  ok(!!hd, 'h1 the ladder draws a header row');
  const labels=[...hd.matchAll(/>([^<>]{1,16})<\/span>/g)].map(m=>m[1]);
  ok(labels.length>=10, 'h2 ...one label per column', labels.length);
  ['LEVEL','PRICE','MARK','TAPS','STATE'].forEach(function(w){
    ok(labels.indexOf(w)>=0, 'h2·'+w+' ...including '+w, labels);
  });
  // ⚠ THE POSITIONS COME FROM THE COLUMNS' OWN CONSTANTS. A header at a hard-coded x is a header
  // that names its neighbour the moment someone nudges a column, and it would still pass a test
  // that only checked the WORDS were present.
  const xs=[...hd.matchAll(/left:(\d+)px;width:(\d+)px/g)].map(m=>[+m[1],+m[2]]);
  ok(xs.length===labels.length, 'h3 every header carries an explicit left and width', xs.length);
  const K=n=>R.run(n);
  [['LEVEL','LAD_LVL','LAD_LVLW'],['PRICE','LAD_PXC','LAD_PXW'],
   ['STATE','LAD_ST','LAD_STW'],['TAPS','LAD_TAP','LAD_TAPW']].forEach(function(t){
    const i=labels.indexOf(t[0]);
    ok(i>=0 && xs[i][0]===K(t[1]) && xs[i][1]===K(t[2]),
       'h4·'+t[0]+' ...taken from '+t[1]+'/'+t[2]+', so the label cannot drift off its column',
       {header:xs[i], column:[K(t[1]),K(t[2])]});
  });
  ok(xs.every((p,i)=> i===0 || p[0]>=xs[i-1][0]),
     'h5 ...and they run left to right in column order', xs.map(p=>p[0]));
}

// ---- 11 · THE ROC COLUMN IS 15m ONLY, AND THE 5m STILL DECIDES -------------------------------
// Operator, 2026-09-01: "lets get rid of the 5m roc and keep the 15m roc to be consistent with the
// delta profile. is the state also looking at 15 min?"
// ⚠ The answer to the second question is IN these assertions: BUILDING and WEAKENING are 15m, and
// TURN needs the 5m AND the 60m as well — so the 5m leaves the DISPLAY and stays in the DECISION.
{
  const cells=[...R.html.matchAll(/class="g3ldroc[^"]*"[^>]*>([\s\S]*?)<\/span>/g)]
    .map(m=>m[1].replace(/<[^>]+>/g,'').trim());
  ok(cells.length>=4, 'p1 the ROC column draws', cells.length);
  // one percentage, plus an optional 60m arrow when the hour disagrees — never two plain numbers
  cells.forEach(function(c,i){
    const plain=(c.match(/-?\d+%/g)||[]).filter(x=>true);
    const has60=/[\u25b2\u25bc]/.test(c);
    ok(plain.length===(has60?2:1),
       'p1·'+i+' ...one 15m figure per row'+(has60?' plus the 60m disagreement arrow':''), c);
  });
  const tip=(R.html.match(/class="g3ldroc[^"]*"[^>]*title="([^"]*)"/)||[])[1]||'';
  ok(/5m/.test(tip) && /15m/.test(tip) && /60m/.test(tip),
     'p2 ...while the hover still carries all three windows', tip.slice(0,80));
  const hd=(R.html.match(/<div class="g3ladhd">([\s\S]*?)<\/div>/)||[])[1]||'';
  ok(/ROC 15m/.test(hd) && !/ROC 5m/.test(hd), 'p3 ...and the header says 15m');
  // ⚠ EXECUTED: the 5m must still be able to change the STATE, or this was a removal, not a re-fit.
  const LS=R.run('levelStateOf.toString()');
  ok(/P5!==0/.test(LS) && /\(P5>0\)===\(P15>0\)/.test(LS),
     'p4 the TURN state still requires the 5m to agree with the 15m');
  ok(/P15>=LVL_BUILD_P15/.test(LS) && /P15<=LVL_WEAK_P15/.test(LS),
     'p5 ...and BUILDING and WEAKENING are decided on the 15m, matching the Δ column');
}

// ---- 12 · THE CROSS-EXAMINATION: DOES THE REPLAYED FACE MATCH WHAT WAS RECORDED AS LIVE? -----
// Operator, 2026-09-01: "determine that when i use the replay feature and go back during the day, it
// will look like this in terms of what will be displayed ... really examine what is being displayed
// to ensure consistency."
// ⚠⚠ THE FRAME IS THE WITNESS. Every frame stores what the LIVE face was reading at that minute —
// `tri.<book>.top` (the ladder's own %King rows), `tri.<book>.king` (the crowns) and `feat.emband`
// (the band). So the replayed render can be checked against the recording rather than against my
// expectation of it. Anything that disagrees is a surface reading a live source behind the seam.
{
  const F=FR[R.idx];
  // a · the ladder rows ARE the recorded rows, strike for strike
  const drawn=[...R.html.matchAll(/class="g3ldpx"[^>]*>([\d.]+)</g)].map(m=>+m[1]);
  ok(drawn.length>0, 'x1 the replayed ladder draws rows', drawn.length);
  const scale=F.px/F.xm.SPXW.px;
  F.tri.SPXW.top.slice(0,5).forEach(function(row){
    const disp=+(row[0]*scale).toFixed(0);
    ok(drawn.some(p=>Math.abs(p-disp)<=1),
       'x2·'+row[0]+' the recorded strike '+row[0]+' is on the rail at '+disp, drawn);
  });
  // b · the CROWNS are the frame's, not today's
  ['SPXW','SPY'].forEach(function(bk){
    const k=F.tri[bk] && F.tri[bk].king;
    if(!(k>0)) return;
    const disp=(bk==='SPXW') ? +(k*scale).toFixed(0) : k;
    ok(R.html.indexOf(String(disp))>=0,
       'x3·'+bk+' the '+bk+' crown drawn is the one the FRAME recorded ('+k+' → '+disp+')');
  });
  // c · the BAND is the frame's own recorded band
  const eb=F.feat && F.feat.emband;
  if(eb && eb.ok){
    const B=JSON.parse(R.run('JSON.stringify((function(){var b=emBand("SPY");return {em:b.em,open:b.open,ok:b.ok};})())'));
    ok(Math.abs(B.em-eb.em)<0.01,
       'x4 the replayed EM width IS the width recorded at that minute', [B.em, eb.em]);
    ok(Math.abs(B.open-eb.open)<0.01,
       'x4b ...and so is the anchor, so EH/EL sit where they sat', [B.open, eb.open]);
  }
  // d · the CLOCK on the face is the parked minute, not this instant
  const clockCT=new Date(F.t).toLocaleTimeString('en-US',
      {timeZone:'America/Chicago',hour12:false,hour:'2-digit',minute:'2-digit'});
  ok(R.text.indexOf(clockCT)>=0, 'x5 the ⓪a clock reads the parked minute', clockCT);
  // e · and NOTHING on the replayed face may carry a live timestamp
  ok(!/LIVE/.test((R.html.match(/class="g3rp"[\s\S]{0,400}?<\/div>/)||[''])[0]),
     'x6 the strip says REPLAY, not LIVE, while the handle is parked');
}

// ---- 13 · (v15.25) THE ROLL IS NAMED ON THE ROW, AND THE 60m ARROW IS COLOUR-CODED -----------
// Operator, 2026-09-01: "i need to be able to detect rolls. right now i dont see the roll arrows or
// any indication that shows where the gamma is coming from and where its going" — measured on his
// panel, the lane WAS drawing four real rolls as stepped paths in a 20px column at the far right,
// with no strike named anywhere. And: "the roc 15 has up and dn arrows ... should be color coded."
{
  const chips=[...R.html.matchAll(/class="g3ldrl g3ldrl(\w+)"[^>]*>([^<]+)</g)].map(m=>[m[1],m[2]]);
  ok(chips.length>0, 'q1 the ladder names the roll on the row it belongs to', chips.length);
  ok(chips.some(c=>c[0]==='out'&&/\u21e2\d{4}/.test(c[1])),
     'q2 ...a source says WHERE ITS MASS WENT, with the strike', chips);
  ok(chips.every(c=>/\d{4}/.test(c[1])), 'q3 ...and every chip names the other strike', chips);
  // ⚠ the lane is KEPT: it is the only thing that shows two rolls nesting rather than crossing
  ok(/marker-end/.test(R.html), 'q4 ...while the stepped lane still draws them too');
  const hd=(R.html.match(/<div class="g3ladhd">([\s\S]*?)<\/div>/)||[])[1]||'';
  ok(/ROLL/.test(hd), 'q5 ...and the column is labelled');
  // colour: green up, red down, the panel's own direction colours
  const arrows=[...R.html.matchAll(/g3ld60 (g3ld60[ud])"[^>]*>([\u25b2\u25bc])/g)].map(m=>[m[1],m[2]]);
  if(arrows.length){
    ok(arrows.every(a=>(a[0]==='g3ld60u')===(a[1]==='\u25b2')),
       'q6 the 60m arrow is coloured by ITS OWN direction, not by the row', arrows);
  }
  ok(/g3ld60u\{color:#2ec27e\}/.test(src) && /g3ld60d\{color:#f0616d\}/.test(src),
     'q7 ...green up and red down, the same two colours the rest of the panel uses');
  // and the state is held still, with the measured constant
  ok(/var LVL_HOLD_MIN=5;/.test(src), 'q8 a new state must hold five measured minutes before it shows');
  // ⚠⚠ EXECUTED OVER TIME. q8 greps the constant, and a constant nothing reads is decoration —
  // removing the levelHold() call from out() survived that assertion. This drives the clock.
  const seq=JSON.parse(R.run(`(function(){
    REPLAY.on=false; LVL_HOLD={};
    var a=levelHold(7000,'HOLDING');          // seeds
    var b=levelHold(7000,'BUILDING');         // probation opens — must still show the old state
    var c=levelHold(7000,'BUILDING');         // still inside the hold
    LVL_HOLD[7000].pendT -= 6*60000;          // six minutes later
    var d=levelHold(7000,'BUILDING');         // now it switches
    var e=levelHold(7000,'HOLDING');          // and a flip back opens its own probation
    REPLAY.on=true;
    return JSON.stringify([a,b,c,d,e]);
  })()`));
  ok(seq[0]==='HOLDING' && seq[1]==='HOLDING' && seq[2]==='HOLDING',
     'q8b EXECUTED: a new state does NOT show while it is inside the hold', seq);
  ok(seq[3]==='BUILDING', 'q8c ...and DOES once it has held past five minutes', seq);
  ok(seq[4]==='BUILDING', 'q8d ...and flipping back starts its own probation, so it cannot oscillate', seq);
  // ⚠⚠ AND THROUGH levelStateOf, NOT just levelHold. A7 survived its first mutation because the
  // assertions above call the helper DIRECTLY — so deleting the call from out() left them green
  // while the face went back to flickering. The state engine is what the face reads; drive that.
  const via=JSON.parse(R.run(`(function(){
    REPLAY.on=false; LVL_HOLD={}; PEAK={m:{7000:1000}};
    var row=function(p15){ return { v:{ k:7000, cur:1000, p5:p15, p15:p15, p60:0 }, age:0, stale:false }; };
    var saved=velAt;
    velAt=function(){ return row(0); };   var a=levelStateOf(7000,null).st;   // HOLDING
    velAt=function(){ return row(40); };  var b=levelStateOf(7000,null).st;   // wants BUILDING
    var c=levelStateOf(7000,null).st;                                          // still inside the hold
    // ⚠ if levelStateOf never routes through levelHold, this cache is empty — report that as an
    // ANSWER rather than throwing, so the assertion fails for its own reason instead of crashing.
    if(!LVL_HOLD[7000]) return JSON.stringify([a,b,c,'NOT-HELD']);
    LVL_HOLD[7000].pendT -= 6*60000;
    var d=levelStateOf(7000,null).st;                                          // now it may switch
    velAt=saved; REPLAY.on=true;
    return JSON.stringify([a,b,c,d]);
  })()`));
  ok(via[0]==='HOLDING' && via[1]==='HOLDING' && via[2]==='HOLDING',
     'q8e EXECUTED THROUGH levelStateOf: the face holds its state while the reading flips', via);
  ok(via[3]==='BUILDING', 'q8f ...and changes once the new reading has held', via);
  // ⚠ this file has no ex() — it renders the panel rather than extracting functions. Read the source.
  const lh=(src.match(/function levelHold\([\s\S]*?\n\}/)||[''])[0];
  ok(/replayOn\(\)\) return raw;/.test(lh),
     'q9 ...and replay is exempt: the slider jumps, so a per-strike cache would carry state across a leap');
}

// ---- 14 · (v15.26) THE LADDER MUST ACTUALLY BE A LADDER -------------------------------------
// ⚠⚠ WRITTEN BECAUSE v15.24 SHIPPED A BLANK FACE AND EVERY TEST WAS GREEN. A stale scale made the
// rail frame span ~69,000 points, so `Y(p) = H - ((p-lo)/span)*H` returned ~H for every strike and
// ALL THIRTEEN ROWS stacked on one line at y=639.7 of a 640px frame. The rows were in the DOM, the
// audit was ok, renderErrors was empty, and the operator saw an empty panel.
// ⚠ Every assertion in this file until now checked that something was PRESENT. Presence is not
// legibility: these check that the rows are SPREAD, which is the property a ladder actually has.
{
  const tops=[...R.html.matchAll(/class="g3ldpx" style="top:([\d.]+)px"/g)].map(m=>+m[1]);
  ok(tops.length>=4, 'y1 the ladder draws several rows', tops.length);
  const lo=Math.min(...tops), hi=Math.max(...tops);
  ok((hi-lo)>40, 'y2 EXECUTED: the rows are SPREAD down the frame, not stacked on one line',
     {lo:+lo.toFixed(1), hi:+hi.toFixed(1), spread:+(hi-lo).toFixed(1)});
  const distinct=new Set(tops.map(t=>Math.round(t))).size;
  ok(distinct>=Math.min(4, tops.length),
     'y3 ...and they sit at DISTINCT heights — a price axis that maps every strike to one pixel is not an axis',
     {rows:tops.length, distinct});
  // ⚠⚠ (v15.27) y2 MEASURED SPREAD AND A TWO-CLUSTER COLLAPSE PASSES IT. Measured on his live panel:
  // twelve rows at tops 0.7-6.7 and ONE at 636.2 — min-to-max was 635px, so y2 said "spread" about a
  // ladder with everything crushed into six pixels. The single outlier was a SPY-scale level (768)
  // on a ladder of ES strikes, which stretched the frame to ~7,000 points.
  // ⚠ **A RANGE IS NOT A DISTRIBUTION.** What a price axis actually promises is that CONSECUTIVE
  // rows are separated, so the gaps are what gets asserted.
  const sorted=[...tops].sort((a,b)=>a-b);
  const gaps=sorted.slice(1).map((t,i)=>t-sorted[i]).sort((a,b)=>a-b);
  const medGap=gaps.length?gaps[Math.floor(gaps.length/2)]:0;
  ok(medGap>=6,
     'y3b ...with a MEDIAN GAP between neighbours, not one outlier stretching an empty frame',
     {medianGap:+medGap.toFixed(2), tops:sorted.map(t=>+t.toFixed(1))});
  // and no single tenth of the frame may hold most of the ladder
  const H=Math.max(...sorted, 1), band=H*0.1;
  const worst=Math.max(...sorted.map(t=>sorted.filter(u=>Math.abs(u-t)<=band).length));
  ok(worst <= Math.ceil(tops.length*0.7),
     'y3c ...and no tenth of the frame holds more than 70% of the rows',
     {rows:tops.length, worstCluster:worst});
  // ⚠⚠ AND THE GUARD IS PROVEN AGAINST THE REAL BROKEN GEOMETRY, NOT ONLY AGAINST A HEALTHY ONE.
  // A check that has never failed is a check nobody has tested. These are the ACTUAL tops read off
  // his panel on 2026-09-01 while he was looking at it: twelve ES strikes crushed into six pixels
  // and one SPY-scale level at 636.2 stretching the frame. y2 (min-to-max) passed this. Both new
  // guards must reject it, or they are decoration.
  {
    const broken=[0.7,1.2,1.7,2.6,3.2,3.5,4.4,4.9,5.3,5.8,6.3,6.7,636.2];
    const bs=[...broken].sort((a,b)=>a-b);
    const bg=bs.slice(1).map((t,i)=>t-bs[i]).sort((a,b)=>a-b);
    const bMed=bg[Math.floor(bg.length/2)];
    ok(!(bMed>=6), 'y4a the MEDIAN-GAP guard rejects the geometry he actually saw', bMed);
    const bH=Math.max(...bs), bBand=bH*0.1;
    const bWorst=Math.max(...bs.map(t=>bs.filter(u=>Math.abs(u-t)<=bBand).length));
    ok(!(bWorst<=Math.ceil(broken.length*0.7)),
       'y4b ...and so does the CLUSTER guard', {worst:bWorst, of:broken.length});
    // ⚠ and the guard that let it through is recorded, so nobody reinstates it as sufficient
    ok((Math.max(...bs)-Math.min(...bs))>40,
       'y4c ...while the old spread test PASSES it — a range is not a distribution');
  }
  // the frame itself: a band edge and the rows it contains must be the same order of magnitude
  const EBn=JSON.parse(R.run('JSON.stringify((function(){var b=emBand("SPY");return {lo:b.low,hi:b.high,hw:b.hiWater};})())'));
  ok((EBn.hi-EBn.lo)<500,
     'y4 the expected-move band spans POINTS, not tens of thousands', EBn);
  if(EBn.hw!=null) ok(EBn.hw<EBn.hi*1.5 && EBn.hw>EBn.lo*0.5,
     'y5 ...and the high-water mark is on the same ruler as the band', EBn);
}

console.log('test_replay_face: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
