// (v11.35) UI SCALE + POP-OUT SIZING.
// Popping out on a 2032px monitor opened a 369px window showing 7px type. Widening a window adds space;
// it does not enlarge anything. `zoom` is used rather than `transform:scale` because zoom participates in
// layout — a panel at width:100% still fits its container at any scale instead of overflowing sideways.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
const eq=(a,b,m)=>ok(JSON.stringify(a)===JSON.stringify(b),m,{got:a,want:b});
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}

let STORE={};
global.localStorage={ getItem:k=>(k in STORE?STORE[k]:null), setItem:(k,v)=>{STORE[k]=String(v);}, removeItem:k=>{delete STORE[k];} };
global.document={ getElementById:()=>null };
global.PANEL={ style:{} };
global.PIPWIN=null;
global.ZOOM_KEY='gpts_zoom_v1'; global.ZOOM_MIN=0.7; global.ZOOM_MAX=2.2; global.ZOOM_STEP=0.1;
global.UIZOOM=1;
eval(ex('pipOpen'));
eval(ex('zoomLoad')); eval(ex('zoomSave')); eval(ex('zoomApply')); eval(ex('zoomBy')); eval(ex('zoomReset'));

eq(UIZOOM,1,'the panel starts at 100%');
zoomBy(0.1); eq(UIZOOM,1.1,'one step up is 110%');
zoomBy(0.1); zoomBy(0.1); eq(UIZOOM,1.3,'steps accumulate without drifting into float dust');
ok(PANEL.style.zoom==='1.3','and the scale is applied to the panel',PANEL.style.zoom);
zoomReset(); eq(UIZOOM,1,'the label resets to 100%');
ok(PANEL.style.zoom==='','and at 100% the property is CLEARED, not set to "1" — an inert zoom still creates a containing block',PANEL.style.zoom);

for(let i=0;i<40;i++) zoomBy(0.1);
eq(UIZOOM,ZOOM_MAX,'it clamps at the top rather than scaling off the screen');
for(let i=0;i<40;i++) zoomBy(-0.1);
eq(UIZOOM,ZOOM_MIN,'and at the bottom rather than to nothing');

zoomReset(); zoomBy(0.4);
ok(STORE[ZOOM_KEY]==='1.4','the scale is persisted',STORE[ZOOM_KEY]);
global.UIZOOM=1; zoomLoad();
eq(UIZOOM,1.4,'and restored on the next load');
STORE[ZOOM_KEY]='99'; global.UIZOOM=1; zoomLoad();
eq(UIZOOM,1,'a stored value outside the range is ignored, not applied');
STORE[ZOOM_KEY]='not a number'; global.UIZOOM=1; zoomLoad();
eq(UIZOOM,1,'and so is a corrupt one');
delete STORE[ZOOM_KEY]; global.UIZOOM=1;
eq(zoomLoad(),1,'no stored value means 100%');

// ---- the pop-out is sized from the SCREEN ----
{
  const m=src.match(/var wantW=([^\n]+)\n\s*var wantH=([^\n]+)/);
  ok(!!m,'the pop-out computes its own size');
  ok(/sw\s*\*/.test(m[1]),'width is derived from the available SCREEN width, not the panel\'s current width',m[1].trim());
  ok(/Math\.max\(420/.test(m[1]),'with a floor so it can never open unusably narrow again');
  ok(/sh\s*\*/.test(m[2]) && /Math\.min\(sh/.test(m[2]),'height fills most of the screen but stays inside it',m[2].trim());
}
// ---- the PiP stylesheet must not fight the zoom ----
{
  ok(/max-width:100% !important/.test(src),'the panel is capped at the window width so a zoomed panel cannot scroll sideways');
  ok(/min-height:100vh/.test(src),'and fills the window vertically rather than floating in a short block');
  // (v11.93) THE GRIP IS BACK IN THE POP-OUT, and hiding it was only half the reason resize was dead
// there: mousemove/mouseup were bound to the ATLAS document while the panel lives in the PiP one, so
// the press armed and nothing ever moved. Unhiding alone would have shipped a handle that visibly
// does nothing. Both halves are asserted.
ok(/#gpts-grip\{display:block !important;cursor:ns-resize !important/.test(src),
   'the pop-out shows the grip, as a VERTICAL resizer');
// (v11.95) and it is pinned to the WINDOW. Measured live: panel 581px in a 598px PiP window — the grip
// sits at the panel's bottom-right, so as soon as the panel grew past the window the HANDLE left the
// viewport and the drag simply stopped with no message. Unhiding it was not enough; it has to stay
// reachable, and the ceiling has to be the window rather than an arbitrary 2000.
ok(/position:fixed !important;'\+\s*'right:6px !important;bottom:22px !important/.test(src),
   'the pop-out grip is pinned to the window AND INSET from the corner');
// ⚠ (v11.97) THE INSET IS THE WHOLE FIX. At right:0/bottom:0 the 16px grip sat exactly on the OS
// window-resize corner, so the operating system took the press and the page never saw a mousedown —
// while a synthetic drag dispatched into the document resized the panel perfectly. A handle the
// pointer cannot reach is the same as no handle.
ok(!/right:0 !important;bottom:0 !important/.test(src),
   'and is NOT flush to the corner, where the OS resize handle lives');
ok(/width:22px !important;height:22px !important/.test(src),
   'and is big enough to hit');
// (v11.96) THE CEILING IS NOT THE WINDOW. Capping the panel at the window height is exactly what
// "I still cannot increase the height" was: the pop-out body already scrolls, so a panel TALLER than
// its window is the normal case. v11.95 capped it at innerHeight-8 = 590 against a panel already at
// 581, which is why it moved a few pixels and stopped.
// (v12.3) SUPERSEDED. There is no pop-out PANEL ceiling any more, because the pop-out grip resizes
// the WINDOW rather than the panel — the panel simply fills whatever window it is given. The screen's
// available height is the real ceiling out there, and it is applied inside pipResize().
ok(/if\(nh>4000\) nh=4000;/.test(src),
   'the drag distance is bounded sanely for both modes');
ok(/Math\.min\(maxOuter, target\+chromeH\)/.test(src),
   'the pop-out ceiling is the room actually left on screen below the window top');
ok(/PANEL\.ownerDocument\|\|document/.test(src),
   'and resize binds to whichever document owns the panel, so the drag works in the pop-out too');
ok(/inPip=\(doc!==document\)/.test(src),
   'it knows which one it is in');
ok(/if\(inPip\) return;/.test(src),
   'and a pop-out height never overwrites the in-page size — that window has its own height');
// ⚠ STRIP COMMENTS FIRST. The comment explaining the removal CONTAINS `min-height:100vh`, so a raw
// grep finds the very string it is checking is gone — the same trap that has now bitten this project
// five times. Look at the code, not at the prose about the code.
{
  const code=src.replace(/\/\*[\s\S]*?\*\//g,'').split('\n').map(l=>l.replace(/^\s*\/\/.*$/,'')).join('\n');
  ok(!/min-height:100vh/.test(code),
     'the panel is no longer forced to fill the pop-out window, or the grip would have nothing to act on');
  // (v12.2) SUPERSEDED. v11.96 made the pop-out panel content-height with the WINDOW scrolling; that
  // put the grip at the true bottom of a 1000px panel inside a 600px window, permanently unreachable.
  // The panel fills the window and scrolls internally instead.
  ok(/height:100% !important/.test(code),
     'the pop-out panel FILLS its window rather than growing past it');
  // ⚠ THE REGRESSION THIS PINS. The original rule was `height:auto !important; min-height:100vh`.
  // v11.93 rewrote it for the grip and dropped `height:auto`, so the inline height restoreSize()
  // writes for the IN-PAGE panel applied in the pop-out and froze it there — measured live at 581px
  // in a 598px window with 1021px of content.
  ok(!/height:auto !important/.test(code),
     'and height carries NO !important, so a grip drag can still override it');
  ok(/PANEL\.style\.height=''; PANEL\.style\.width='';/.test(code),
     'entering the pop-out CLEARS the in-page inline size, or the stylesheet default can never apply');
  ok(/if\(PIPHOST\.h\) PANEL\.style\.height=PIPHOST\.h/.test(code),
     'and restoring puts the in-page size back');
}
}
// ---- zoom is re-applied across the document move ----
{
  const pipT=src.slice(src.indexOf('function pipToggle'), src.indexOf('function pipRestore'));
  ok(/zoomApply\(\)/.test(pipT),'zoom is re-applied when the panel moves INTO the pop-out — it changes documents');
  const rest=src.slice(src.indexOf('function pipRestore'));
  ok(/zoomApply\(\)/.test(rest.slice(0,600)),'and again when it comes back');
}

// ---- (v11.36) THE UNIT MISMATCH IN pickEdge --------------------------------------------------
// FLRCEIL_FAR is documented and reasoned about in STRIKES; `dist` is Math.abs(k-px), PRICE POINTS.
// On SPY they coincide because its strikes are 1 point apart, which is why this never bit. On SPX
// they are 5 apart, so the same constant silently means something 5x different.
{
  eval(ex('strikeStep'));
  eq(strikeStep([{k:760},{k:761},{k:762},{k:763}]),1,'a 1-point grid measures as 1 — SPY, where the bug hid');
  eq(strikeStep([{k:7600},{k:7605},{k:7610},{k:7615}]),5,'a 5-point grid measures as 5 — SPX, where it bites');
  eq(strikeStep([{k:100},{k:102.5},{k:105},{k:107.5}]),2.5,'a fractional grid is measured, not rounded');
  eq(strikeStep([{k:7600},{k:7605},{k:7610},{k:7650}]),5,'one wide gap does not drag the spacing — it is the MEDIAN, not the mean');
  eq(strikeStep([{k:760}]),1,'too few strikes falls back to 1 rather than dividing by nothing');
  eq(strikeStep([]),1,'and so does an empty map');
  eq(strikeStep([{k:760},{k:760},{k:760}]),1,'identical strikes cannot produce a zero step');
  ok(/FLRCEIL_FAR\s*\*\s*_step/.test(src),'and the threshold is now multiplied by the measured spacing');
}

// ---------- (v12.2) THE PANEL MUST CONTAIN ITS OWN CONTENT ----------
// Measured live: panel box 524px, content 1068px, overflow:visible — 544px of dashboard painted
// OUTSIDE the box with no panel background behind it. The user reported "the bottom of the pane is
// transparent" and "the drag button is in the middle": the grip WAS at the panel's bottom-right, but
// the panel's bottom sat 47% of the way down the visible stack.
// ⚠ A box that does not contain its content cannot be resized meaningfully — which is why every
// pop-out sizing complaint in this session traced back to here rather than to the grip.
{
  const code=src.replace(/\/\*[\s\S]*?\*\//g,'').split('\n').map(l=>l.replace(/^\s*\/\/.*$/,'')).join('\n');
  ok(/overflow:'hidden'[\s\S]{0,80}display:'flex'[\s\S]{0,40}flexDirection:'column'/.test(code),
     'the panel CLIPS and lays out as a column');
  ok(!/userSelect:'none', overflow:'visible'/.test(code),
     'and no longer lets its content paint outside the box');
  ok(/flex:'1 1 auto', minHeight:'0', overflowY:'auto'/.test(code),
     'the body is the scrolling region');
  ok(/minHeight:'0'/.test(code),
     'with minHeight:0 — without it a flex child refuses to shrink below its content and the overflow returns');
  ok(/height:100% !important/.test(code),
     'and the pop-out panel FILLS its window rather than growing past it');
  ok(/font:12px\/1\.4 Inter,Arial,sans-serif;overflow:hidden/.test(code),
     'so the pop-out window itself does not scroll — the panel does');
}
// ---------- (v12.2) A SAVED POSITION MUST NOT STRAND THE PANEL ----------
{
  const rp=ex('restorePos');
  ok(/if\(top<0\) PANEL\.style\.top='0px';/.test(rp),
     'a restored top above the viewport is clamped back — measured live at y = -33, header off-screen');
  ok(/window\.innerHeight-minVisible/.test(rp) && /window\.innerWidth-minVisible/.test(rp),
     'and a position past the bottom or right edge is pulled back too');
  ok(/minVisible=40/.test(rp),
     'keeping enough of the HEADER reachable, since dragging is how a stranded panel is rescued');
}


// ---------- (v12.3) THE POP-OUT GRIP RESIZES THE WINDOW ----------
// v12.2 made the popped-out panel `height:100% !important` so it would contain its content — which
// also made the grip inert out there, because !important beats the inline height the grip sets.
// ⚠ MEASURED: resizeTo() on a Document PiP window throws "requires user activation in document
// picture-in-picture" from an injected script, but IS permitted inside a drag handler, because a drag
// is a user gesture. That measurement is the whole reason this approach is viable.
{
  const mr=ex('makeResizable');
  ok(/win\.resizeTo\(win\.outerWidth, outer\)/.test(mr),
     'the pop-out grip resizes the WINDOW');
  ok(/if\(inPip\)\{ pipResize\(nh\); return; \}/.test(mr),
     'and never sets PANEL.style.height there, which height:100% !important would ignore anyway');
  ok(/win\.outerHeight-win\.innerHeight/.test(mr),
     'the window chrome is accounted for, so the CONTENT reaches the dragged height');
  ok(/maxOuter=Math\.max\(200, availTop\+avail-top\)/.test(mr),
     'and it never grows past the room left on screen below the window top');
  ok(/pendingH=target; return false;/.test(mr) && /if\(inPip && pendingH!=null\) pipResize\(pendingH\);/.test(mr),
     'a resize refused for lapsed activation is retried on mouseup — part of the same gesture');
  ok(/if\(inPip\) return;   \/\/ \u26a0 a pop-out size belongs to that WINDOW/.test(mr),
     'and a pop-out size never overwrites the saved in-page one');
  ok(/if\(nh>2000\) nh=2000;/.test(mr),
     'the in-page panel still clamps, because nothing scrolls behind it');
}


// ---------- (v12.4) POINTER CAPTURE — why the pop-out could shrink but never grow ----------
// The user could drag the grip UP (smaller) but not DOWN (bigger). That asymmetry IS the diagnosis:
// the grip is pinned just above the pop-out window's bottom edge, so dragging DOWN puts the pointer
// outside that window within a few pixels — and a plain `mousemove` listener stops receiving events
// the moment the pointer leaves the window. Dragging UP keeps the pointer inside, so it worked.
// ⚠ Measured first, and the obvious suspect was WRONG: the window had 127px of headroom to
// screen.availHeight and pipResize computed a larger target. Growth was permitted; it simply never
// got a second event to act on. Three earlier attempts at this bug all missed it for that reason.
{
  const mr=ex('makeResizable');
  ok(/grip\.setPointerCapture\(pointerId\)/.test(mr),
     'the grip CAPTURES the pointer, so events keep arriving once it leaves the window');
  ok(/addEventListener\('pointermove', onMove\)/.test(mr) && /addEventListener\('pointerup', onUp\)/.test(mr),
     'and the drag runs on pointer events, which mouse events cannot do across a window boundary');
  ok(/releasePointerCapture\(capId\)/.test(mr),
     'capture is released on mouseup, never leaked');
  ok(/typeof window\.PointerEvent==='function'/.test(mr) && /addEventListener\('mousedown', function\(e\)\{ if\(e\.button===0\) begin\(e, null\); \}\)/.test(mr),
     'with a mouse-event fallback where PointerEvent is unavailable');
  ok(/'pointerdown', function\(e\)\{ if\(e\.button===0\) begin\(e, e\.pointerId\); \}/.test(mr),
     'left button only — a right-click on the grip must not start a resize');
}


// ---------- (v12.5) A PERCENTAGE HEIGHT NEEDS A PARENT WITH A HEIGHT ----------
// The pop-out stylesheet told the panel `height:100% !important` while html/body had no height at all,
// so it computed to `auto` and the panel took its CONTENT height: 1104px inside a 598px window.
// makeResizable reads `oh` off that rect, so every downward drag began 500px past the window and hit
// the screen clamp on pixel one. Grow was dead; shrink worked. Five versions blamed the drag handler.
{
  const pcs=ex('pipCopyStyles');
  const wantsPct=/#gpts-panel\{[^}]*height:100%/.test(pcs);
  ok(wantsPct, 'the pop-out panel is told to fill its window');
  ok(!wantsPct || /html,body\{[^}]*height:100%/.test(pcs),
     'and html,body are GIVEN a height, or that 100% silently computes to auto');
  ok(/window innerHeight 598, panel box 1104/.test(pcs),
     'the measurement that proved it is recorded, not just the fix');
}

console.log('\n'+pass+' pass / '+fail+' fail');
