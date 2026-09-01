// render-face.js — RENDER THE PANEL HEADLESSLY, PARKED ON A RECORDED MINUTE, AND REPORT WHAT IT DREW.
//   node tools/render-face.js 2026-08-31 14:12
// ⚠⚠ WHY THIS EXISTS. Nine replay defects in a row were found by the operator looking at his own
// panel, because nothing here could DRAW the face — the tests execute functions in isolation, which
// proves a function returns the right value and proves nothing about whether the section that calls
// it survives to emit a heading. A section that throws is swallowed by design, so a broken replay
// looks exactly like a quiet one: missing headings, missing king path, missing statuses, no scroll.
// This loads the real userscript into jsdom, parks it, renders, and prints what the body contains
// plus every swallowed error. It is the check that should have existed before the slider shipped.
const fs=require('fs'), vm=require('vm'), path=require('path');
const { JSDOM }=require('jsdom');
const day=process.argv[2]||'2026-08-31', want=process.argv[3]||'14:12';

const dom=new JSDOM('<!doctype html><html><body></body></html>',
  { url:'https://app.skylit.ai/atlas', pretendToBeVisual:true });
const win=dom.window;
win.matchMedia=win.matchMedia||(q=>({matches:false,media:q,addListener(){},removeListener(){},addEventListener(){},removeEventListener(){}}));
// jsdom has no layout: every box measures 0. panelFit/ladderFit are measured separately (see below).
win.requestAnimationFrame=cb=>setTimeout(()=>cb(Date.now()),0);
win.cancelAnimationFrame=id=>clearTimeout(id);
const store={};
Object.defineProperty(win,'localStorage',{ value:{
  getItem:k=>(k in store)?store[k]:null, setItem:(k,v)=>{store[k]=String(v);},
  removeItem:k=>{delete store[k];}, clear:()=>{for(const k in store)delete store[k];},
  key:i=>Object.keys(store)[i]||null, get length(){return Object.keys(store).length;} }, configurable:true });
win.indexedDB=undefined;

// ⚠ NO TIMERS. boot() starts several intervals (the 3s velocity harvest among them) and jsdom keeps
// the event loop alive for every one of them, so the harness hangs instead of printing. A render is a
// synchronous act; nothing here needs a clock.
win.setInterval=()=>0; win.clearInterval=()=>{};
win.setTimeout=(fn)=>0;  win.clearTimeout=()=>{};
win.fetch=()=>new Promise(()=>{});

let src=fs.readFileSync('./current/gex-signal-tapereader.user.js','utf8');
// unwrap the IIFE so its internals are reachable in this context — the production file is untouched
const i0=src.indexOf('(function(){'), i1=src.lastIndexOf('})();');
const body=src.slice(i0+'(function(){'.length, i1);
const ctx=vm.createContext(win);
win.window=win;
try{ vm.runInContext(body, ctx, { filename:'gex.user.js' }); }
catch(e){ console.log('SCRIPT THREW AT LOAD: '+e.message); process.exit(1); }

const D=JSON.parse(fs.readFileSync('./data/'+day+'.json','utf8'));
const FR=(D.snaps&&D.snaps.SPY)||[];
const hhmm=t=>new Date(t).toLocaleTimeString('en-US',{timeZone:'America/Chicago',hour12:false,hour:'2-digit',minute:'2-digit'});
let idx=0, best=1e9;
const mins=s=>(+s.split(':')[0])*60+(+s.split(':')[1]);
FR.forEach((f,i)=>{ const d=Math.abs(mins(hhmm(f.t))-mins(want)); if(d<best){best=d;idx=i;} });

const run=code=>vm.runInContext(code, ctx);
run('typeof buildPanel==="function" ? buildPanel() : (typeof boot==="function" ? boot() : 0)');
run(`REPLAY.on=true; REPLAY.day=${JSON.stringify(day)}; REPLAY.frames=${JSON.stringify(FR)}; REPLAY.idx=${idx};`);
run('RENDER_ERRS.length=0');
try{ run('render()'); }catch(e){ console.log('render() THREW: '+e.message+'\n'+(e.stack||'').split('\n').slice(0,4).join('\n')); }

// ⚠ (v15.39) --probe <expr>: evaluate one expression INSIDE the rendered page and print it. The
// candle disagreement could only be diagnosed by asking the rendered face what it measured, and
// re-deriving it out here would have been a fourth opinion about the same numbers.
if(process.argv.indexOf('--probe')>=0){
  const expr=process.argv[process.argv.indexOf('--probe')+1];
  try{ console.log(String(run(expr))); }catch(e){ console.log('PROBE THREW: '+e.message); }
}
const html=run('elBody ? elBody.innerHTML : ""');
console.log('parked '+hhmm(FR[idx].t)+' CT  frame '+idx+'/'+FR.length+'   body '+html.length+' chars\n');
const errs=run('JSON.stringify(__gptsDebug.renderErrors())');
const E=JSON.parse(errs||'[]');
console.log('SWALLOWED ERRORS: '+E.length);
E.forEach(e=>console.log('  '+(e.where||e.w||'?')+'  ::  '+(e.msg||e.m||JSON.stringify(e)).slice(0,150)));

const SECTIONS=[
  ['replay strip','g3rp'], ['section headings','g3sh'], ['KING','KING'], ['king path svg','g3kpath'],
  ['king lanes','g3kl'], ['ladder','g3lad'], ['node % values','g3pct'], ['node states','g3st'],
  ['HOD/LOD','HOD'], ['candle','g3cndl'], ['DAY columns','DAYCOL'], ['roll arrows','g3roll'],
];
console.log('\nWHAT THE BODY CONTAINS:');
for(const [name,needle] of SECTIONS){
  const n=(html.split(needle).length-1);
  console.log('  '+(n?'yes':'NO ')+'  '+name.padEnd(18)+(n?('x'+n):'')+'   ['+needle+']');
}
if(process.argv.includes('--dump')) fs.writeFileSync('/tmp/face.html', html);
// ⚠ (v15.28) --page writes a STANDALONE document: the panel's own injected CSS plus the body, so a
// real browser can lay it out. jsdom has no layout engine, which is why the scroll and the clamp
// could only be greps — this is how that gap gets closed instead of excused.
if(process.argv.includes('--page')){
  const css=[...dom.window.document.querySelectorAll('style')].map(s=>s.textContent).join('\n');
  const panel=run('PANEL ? PANEL.getAttribute("style") : ""');
  fs.writeFileSync('/tmp/face-page.html',
    '<!doctype html><meta charset="utf-8"><style>body{margin:0;background:#0b0e14}\n'+css+'</style>'+
    '<div id="gpts-panel" style="'+panel+'"><div id="gpts-body" style="padding:9px 10px;position:relative;'+
    'flex:1 1 auto;min-height:0;overflow-y:auto;overflow-x:hidden">'+html+'</div></div>');
}

process.exit(0);
