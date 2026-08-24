// (v11.35) UI SCALE + POP-OUT SIZING.
// Popping out on a 2032px monitor opened a 369px window showing 7px type. Widening a window adds space;
// it does not enlarge anything. `zoom` is used rather than `transform:scale` because zoom participates in
// layout — a panel at width:100% still fits its container at any scale instead of overflowing sideways.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
const eq=(a,b,m)=>ok(JSON.stringify(a)===JSON.stringify(b),m,{got:a,want:b});
function noc(s){ return String(s).replace(/\/\*[\s\S]*?\*\//g,'').replace(/^\s*\/\/.*$/gm,''); }
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
// ⚠ INVERTED at v12.6. The pop-out grip is GONE. Chrome reverts every height change to a PiP window
// (five trials, asked=713 -> settled=693), and a normal window is resized by its own edge. A handle
// that cannot act is worse than none: it invites the drag that fails.
ok(/#gpts-grip\{display:none !important\}/.test(src),
   'the pop-out HIDES the grip, because no handle can resize a PiP window');
// (v11.95) and it is pinned to the WINDOW. Measured live: panel 581px in a 598px PiP window — the grip
// sits at the panel's bottom-right, so as soon as the panel grew past the window the HANDLE left the
// viewport and the drag simply stopped with no message. Unhiding it was not enough; it has to stay
// reachable, and the ceiling has to be the window rather than an arbitrary 2000.
ok(!/right:6px !important;bottom:22px !important/.test(noc(src)),
   'and nothing pins a pop-out grip any more \u2014 inset or otherwise (INVERTED v12.6)');
// ⚠ (v11.97) THE INSET IS THE WHOLE FIX. At right:0/bottom:0 the 16px grip sat exactly on the OS
// window-resize corner, so the operating system took the press and the page never saw a mousedown —
// while a synthetic drag dispatched into the document resized the panel perfectly. A handle the
// pointer cannot reach is the same as no handle.
ok(!/right:0 !important;bottom:0 !important/.test(src),
   'and is NOT flush to the corner, where the OS resize handle lives');
ok(!/#gpts-grip\{[^}]*width:22px !important/.test(noc(src)),
   'and it is not sized for hitting, because it is not shown (INVERTED v12.6)');
// (v11.96) THE CEILING IS NOT THE WINDOW. Capping the panel at the window height is exactly what
// "I still cannot increase the height" was: the pop-out body already scrolls, so a panel TALLER than
// its window is the normal case. v11.95 capped it at innerHeight-8 = 590 against a panel already at
// 581, which is why it moved a few pixels and stopped.
// (v12.3) SUPERSEDED. There is no pop-out PANEL ceiling any more, because the pop-out grip resizes
// the WINDOW rather than the panel — the panel simply fills whatever window it is given. The screen's
// available height is the real ceiling out there, and it is applied inside pipResize().
ok(/if\(nh<160\) nh=160; if\(nh>2000\) nh=2000;/.test(noc(src)),
   'the in-page drag is bounded; there is no second mode to bound (v12.6)');
ok(!/Math\.min\(maxOuter, target\+chromeH\)/.test(noc(src)),
   'there is no pop-out ceiling to compute, because nothing resizes a pop-out (INVERTED v12.6)');
ok(/PANEL\.ownerDocument\|\|document/.test(src),
   'and resize binds to whichever document owns the panel, so the drag works in the pop-out too');
ok(!/inPip=\(doc!==document\)/.test(noc(src)),
   'the handler no longer branches on pop-out vs page \u2014 it REFUSES pop-outs (INVERTED v12.6)');
ok(/\(PANEL\.ownerDocument\|\|document\)!==document\) return/.test(noc(src)),
   'a grip press while the panel is popped out is refused outright, so it can overwrite nothing');
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
  // ⚠⚠ THE COMMENT ABOVE THIS BLOCK WAS WRONG, AND IT SHIPPED AS IF IT WERE MEASURED.
  // It claimed resizeTo "IS permitted inside a drag handler, because a drag is a user gesture".
  // Only the FIRST call is: activation is CONSUMED, so every later move throws, and Chrome reverts
  // any height past its cap regardless. What was measured was the console refusal; the drag case was
  // ASSUMED. An assumption written in the voice of a measurement cost six versions.
  ok(!/resizeTo/.test(noc(mr)),
     'the grip does NOT resize a window \u2014 Chrome forbids and reverts it (INVERTED v12.6)');
  ok(!/pipResize/.test(noc(mr)),
     'and the pop-out resize path is gone, not merely unused (INVERTED v12.6)');
  ok(!/win\.outerHeight-win\.innerHeight/.test(noc(mr)),
     'no window chrome maths remains, since no window is measured (INVERTED v12.6)');
  ok(!/maxOuter/.test(noc(mr)),
     'and no screen ceiling is computed here any more (INVERTED v12.6)');
  ok(!/pendingH/.test(noc(mr)),
     'the retry-on-mouseup workaround is gone; it could never have worked (INVERTED v12.6)');
  ok(/localStorage\.setItem\(SIZE_KEY/.test(noc(mr)) && !/inPip/.test(noc(mr)),
     'the in-page size is saved unconditionally, there being only one mode now (v12.6)');
  ok(/if\(nh>2000\) nh=2000;/.test(mr),
     'the in-page panel still clamps, because nothing scrolls behind it');
}


// ---------- (v12.4 -> v12.6) POINTER CAPTURE KEPT; THE POP-OUT RESIZE RETIRED ----------
// v12.4 added pointer capture so a drag that leaves the browser window keeps delivering events.
// That is real and stays: it is what makes the IN-PAGE grip survive a drag past the window edge.
// What is RETIRED is everything that tried to resize a POP-OUT window from this handler.
// ⚠ Measured on the user's machine: `resizeTo()` on a Document PiP window requires user activation,
// CONSUMES it (one gesture buys exactly ONE resize), and any height beyond Chrome's cap is reverted.
// Five trials: `asked=713 -> settled=693 (reverted)`, while width resized freely throughout.
// These assertions are INVERTED, not deleted, so the retired behaviour can never quietly return.
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
     'left button only \u2014 a right-click on the grip must not start a resize');
  // --- INVERTED ---
  ok(!/resizeTo/.test(noc(mr)),
     'the grip NEVER resizes a window \u2014 Chrome forbids it and reverts it (v12.6)');
  ok(!/pipResize/.test(noc(mr)),
     'and the pop-out resize path is gone, not merely unused');
  ok(/\(PANEL\.ownerDocument\|\|document\)!==document\) return/.test(mr),
     'a grip press while the panel lives in a pop-out is refused outright');
  ok(/asked=713 -> settled=693 \(reverted\)/.test(mr),
     'and the measurement that retired it is recorded in the code');
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


// ---------- (v12.6) OPEN IN A NORMAL WINDOW ----------
// A Document PiP window cannot grow taller; a plain popup can. Both are offered because only PiP is
// always-on-top. ⚠ PANEL is ONE DOM node — it cannot live in two windows, so each closes the other.
{
  const wt=ex('winToggle'), pt=ex('pipToggle'), pv=ex('panelVisible');
  ok(/window\.open\('', 'gptsPanelWin'/.test(wt), 'the window pop-out uses a real popup, which has no height cap');
  ok(/if\(pipOpen\(\)\)\{ try\{ PIPWIN\.close\(\); \}catch\(e\)\{\} \}/.test(wt),
     'opening the window closes an open PiP — one panel, one home');
  ok(/if\(winOpen\(\)\)\{ try\{ WINWIN\.close\(\); \}catch\(e\)\{\} \}/.test(pt),
     'and opening the PiP closes an open window, the same rule both ways');
  ok(/pipOpen\(\) \|\| winOpen\(\)/.test(pv),
     'an open window counts as VISIBLE, or the ladder stops refreshing behind a painted panel');
  ok(/d\.body\.appendChild\(PANEL\)/.test(wt) && /pipCopyStyles\(d\)/.test(wt),
     'the popup gets the panel and the stylesheet it needs to render');
  ok(/'pagehide', winRestore/.test(wt), 'closing the window puts the panel back in the page');
  ok(/localStorage\.setItem\(WINBOX_KEY/.test(ex('winBoxSave')), 'the window remembers its own size and position');
  ok(/PANEL\.style\.height=''/.test(wt), 'and the in-page inline size is cleared, since it means nothing there');
}

// The grip must be HIDDEN in any pop-out — it cannot work in PiP and is redundant in a window.
{
  const pcs=ex('pipCopyStyles');
  ok(/#gpts-grip\{display:none !important\}/.test(pcs), 'the grip is hidden in every pop-out');
  ok(!/#gpts-grip\{display:block/.test(noc(pcs)), 'and is never re-shown there');
}

console.log('\n'+pass+' pass / '+fail+' fail');
