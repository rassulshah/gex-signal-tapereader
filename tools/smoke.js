// Loads the whole userscript in a DOM stub and exercises every debug hook. The empty-book case is what
// throws. Kept IN THE REPO because the sandbox is ephemeral and this has been lost to a container reset
// twice; a check that only exists in /tmp is a check you will rewrite from memory at the worst moment.
const fs=require('fs'), vm=require('vm');
const src=fs.readFileSync('./v10.js','utf8');
function el(){ return { style:{}, id:'', className:'', textContent:'', innerHTML:'', appendChild(){}, addEventListener(){}, removeEventListener(){}, setAttribute(){}, getAttribute(){return null;}, querySelector(){return null;}, querySelectorAll(){return [];}, getBoundingClientRect(){return{left:0,top:0,width:440,height:600};}, classList:{add(){},remove(){},contains(){return false;}}, parentNode:null, offsetParent:null, children:[] }; }
const doc={ getElementById(){return null;}, createElement(){return el();}, head:el(), body:el(), documentElement:el(), querySelector(){return null;}, querySelectorAll(){return [];}, addEventListener(){}, removeEventListener(){}, createTreeWalker(){return{nextNode(){return null;}};}, visibilityState:'visible' };
const sb={ console, document:doc, navigator:{userAgent:'node'}, location:{href:'https://app.skylit.ai/atlas'}, localStorage:{ getItem(){return null;}, setItem(){}, removeItem(){} }, setInterval(){return 0;}, setTimeout(){return 0;}, clearInterval(){}, clearTimeout(){}, requestAnimationFrame(){return 0;}, fetch(){ return Promise.resolve({ok:false,status:0,json:()=>Promise.resolve({})}); }, XMLHttpRequest:function(){ this.open=function(){}; this.send=function(){}; this.addEventListener=function(){}; }, MutationObserver:function(){ this.observe=function(){}; this.disconnect=function(){}; }, Date, Math, JSON, parseFloat, parseInt, isNaN, isFinite, Object, Array, String, Number, Boolean, RegExp, Error, encodeURIComponent, decodeURIComponent, screen:{width:1920,height:1080,availWidth:1920,availHeight:1040} };
sb.window=sb; sb.globalThis=sb; sb.self=sb;
sb.window.addEventListener=function(){}; sb.addEventListener=function(){}; sb.removeEventListener=function(){}; sb.window.removeEventListener=function(){};
vm.createContext(sb);
try{ vm.runInContext(src, sb, {filename:'v10.js'}); }catch(e){ console.log('LOAD ERROR: '+e.message); process.exit(1); }
const D=sb.window.__gptsDebug||{};
let bad=0;
for(const n of ['phase','regime2','pa','bias','steps','roll','face','ifLadder','nodeChart','skew','accum','rolls','optKeys','ifShape']){
  try{ const r=D[n]?D[n]('SPY'):'(absent)'; console.log('ok  '+n.padEnd(10)+' -> '+String(typeof r==='string'?r:JSON.stringify(r)).replace(/\s+/g,' ').slice(0,80)); }
  catch(e){ console.log('THREW '+n+' :: '+e.message); bad++; }
}
// A section that renders empty because its own try/catch ate a ReferenceError looks identical to a
// section with nothing to show. This is the only thing that tells them apart.
try{
  const errs=D.renderErrors?D.renderErrors():[];
  if(errs.length){ bad+=errs.length; console.log('\nSWALLOWED IN RENDER:'); errs.forEach(e=>console.log('  '+e.tag+' x'+e.n+' :: '+e.msg)); }
  else console.log('\nno swallowed render errors');
}catch(e){}
console.log(bad?('\n'+bad+' PROBLEM(S)'):'\nSMOKE CLEAN — no throws, nothing swallowed');
process.exit(bad?1:0);
