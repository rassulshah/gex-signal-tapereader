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
ok(/position:fixed !important;'\+\s*'right:0 !important;bottom:0 !important/.test(src),
   'the pop-out grip is pinned to the window so it never scrolls out of reach');
ok(/inPip \? Math\.max\(240, \(doc\.defaultView \? doc\.defaultView\.innerHeight : 900\) - 8\) : 2000/.test(src),
   'and the pop-out height ceiling is the WINDOW, not the in-page 2000px clamp');
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
  ok(/min-height:120px/.test(code),
     'it has a small floor instead, so the grip can drag under the content');
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
console.log('\n'+pass+' pass / '+fail+' fail');
