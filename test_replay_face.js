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
  ok(xs.length>=4, 'f8'+cls+' the '+cls+' king lane draws the crown’s journey', xs.length);
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

console.log('test_replay_face: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
