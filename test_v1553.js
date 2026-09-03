// ============================================================================================
// test_v1553.js — (v15.53) THE SIMPLIFICATION: eight defects fixed, ~4,300 lines archived.
//   D1 one `var PEAK` · D2 recNode records the right book · D3 hlNodeAt's PT leg no longer throws ·
//   D4 the READ is computed on the record · D5 no probe write per render · D6 one mass definition ·
//   D7 projReview left the export · ARCHIVE integrity: nothing archived is still referenced.
// Every function below is EXTRACTED AND RUN. GPTS_SRC points the harness at a mutated copy.
// ============================================================================================
const fs=require('fs');
const SRC=process.env.GPTS_SRC||'./current/gex-signal-tapereader.user.js';
const src=fs.readFileSync(SRC,'utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,240):''));} };
function ex(n){ const m=new RegExp('function\\s+'+n+'\\s*\\(','g').exec(src);
  if(!m) throw new Error('not found: '+n);
  let i=src.indexOf('{',m.index),d=0; for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){ d--; if(!d) return src.slice(m.index,k+1); } } }
const code=(function(){ // the source minus comments, for reference counts
  return src.split('\n').filter(l=>!l.trim().startsWith('//')).map(l=>l.replace(/(\s|;)\/\/.*$/,'')).join('\n'); })();
const build=(g,fns,tail)=>new Function('__g', Object.keys(g).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+fns.map(ex).join('\n')+'\n'+(tail||''));

// ---- D1 · one PEAK ------------------------------------------------------------------------------
{
  const decls=(code.match(/^var PEAK\s*=/gm)||[]).length;
  ok(decls===1,'D1a exactly ONE top-level `var PEAK` (there were two in one IIFE; the later wiped the session store)',decls);
  ok(/^var SESSPEAK\s*=/m.test(code),'D1b the session peak/low store has its own name');
  const g={ SESSPEAK:{ SPY:{ '765.00':{hi:31,lo:12} } } };
  const f=build(g,['nodePeak'],'return nodePeak;')(g);
  ok(f('SPY',765)&&f('SPY',765).hi===31,'D1c nodePeak reads the SESSION store, executed',f('SPY',765));
  ok(f('QQQ',765)===null,'D1d ...and returns null for a book with no store, never throws');
}

// ---- D2 · recNode records the book it was asked about ------------------------------------------
{
  const calls=[]; const g={};
  g.livePctAt=(s,k)=>{ calls.push(s); return s==='QQQ'?-42:88; };
  g.accumulationStateFor=()=>null; g.nodeHistory=()=>[]; g.nodePeak=()=>null; g.nodeLifecycle=()=>null;
  const f=build(g,['recNode'],'return recNode;')(g);
  const q=f({k:700,pos:true,abs:1e6,pct:50},'QQQ');
  ok(q.tp===-42 && calls[calls.length-1]==='QQQ','D2a a QQQ node records QQQ\'s tape percentage (it recorded SPY\'s before)',q.tp);
  const n=f({k:700,pos:true,abs:1e6,pct:50});
  ok(n.tp===null,'D2b with no symbol it records null — never the wrong book\'s number',n.tp);
}

// ---- D3 · hlNodeAt's PT leg --------------------------------------------------------------------
{
  const g={};
  g.atr=()=>0.5;
  g.deflKingsAt=(sym,rr,ms)=>(ms===111?[{k:7650,book:'SPXW'}]:[]);
  g.deflKings=()=>[{k:7660,book:'SPXW'}];
  g.deflNodeAt=(px,up,kings,aD)=>({ px:px, up:up, king:kings[0]&&kings[0].k, aD:aD });
  const f=build(g,['hlNodeAt'],'return hlNodeAt;')(g);
  const D={ ok:true, scale:10, first:'HOD', hod:770, lod:760, hodMs:1, lodMs:2, second:'LOD', secondT:100, clock:200 };
  let r=f('SPY',D,{ ok:true, ptPx:765 });
  ok(r.ok===true,'D3a with a second extreme AND a PT leg the function completes (it threw ReferenceError on `kings` before)',r.why);
  ok(r.pt && r.pt.king===7660,'D3b the PT leg is measured against the CURRENT king when no PT time is known',r.pt);
  r=f('SPY',D,{ ok:true, ptPx:765, ptMs:111 });
  ok(r.pt && r.pt.king===7650,'D3c ...and against the king AS IT STOOD at the PT time when that time is known',r.pt);
  ok(r.first && r.second,'D3d HodN and LodN are still produced beside it');
}

// ---- D4 · the READ is a computed property of the record ----------------------------------------
{
  ok(/function readState\(sym\)/.test(src),'D4a readState() exists');
  const dirRec=src.slice(src.indexOf("registerFeature({ key:'dir'"), src.indexOf("registerFeature({ key:'dir'")+6000);
  ok(/read:\(function\(\)\{ try\{ var r=readState\(sym\)/.test(dirRec),'D4b the dir feature CALLS readState on the closed bar instead of reading LAST_READ back from a render');
  ok(!/read:\(typeof LAST_READ!=='undefined'/.test(src),'D4c the old read-back is gone');
  // execute readState with stubs: the record must carry the sentence + voice id
  const g={ STATE:{ SPY:{ price:765 } }, LAST_READ:{}, READ_SYM:'SPY' };
  g.nodeMapModel=()=>({ ok:true, flr:760, ceil:770, levels:[] });
  g.spineOf=()=>({ dir:{ dir:'UP', grade:'B', relation:null, capped:null }, inPlay:null, leg:{ dir:'up', phase:'RLY', dirSrc:'sma' } });
  g.driftRead=()=>({ dir:0, verdict:'NONE' });
  g.read3Beat=()=>({ verdict:'BULLISH', sentence:'the rally leg holds', voiceId:'L3', structure:'floor 760 building', structureId:'S1' });
  const f=build(g,['readState'],'return readState;')(g);
  const r=f('SPY');
  ok(r && r.sentence==='the rally leg holds' && r.voiceId==='L3' && r.legDir==='up' && r.legPhase==='RLY','D4d readState returns the sentence, voice id and leg context',r);
  ok(g.LAST_READ.SPY && g.LAST_READ.SPY.voiceId==='L3','D4e ...and still publishes LAST_READ for any older reader');
}

// ---- D5 · no probe write per render ------------------------------------------------------------
{
  const dh=ex('depsHealth').split('\n').filter(l=>!l.trim().startsWith('//')).join('\n');   // code, not the comment that names the old call
  ok(!/__gptsDebug\.storage\(\)/.test(dh),'D5a depsHealth no longer calls the storage PROBE (a 20 KB write per 15-second render)');
  ok(/lsTotalKB\(\)/.test(dh) && /canWrite40KB:\(_room>=40 && !_recentFail\)/.test(dh),'D5b it derives headroom from the size and the recorder\'s own recent failures');
}

// ---- D6 · one definition of mass moved ---------------------------------------------------------
{
  const f=build({},['velMass15'],'return velMass15;')({});
  ok(f({cur:-82.0e6,d15:-22.4e6})>0,'D6a a short-gamma strike deepening from -59.6M to -82.0M GAINED mass (the signed d15 called it a source)',f({cur:-82.0e6,d15:-22.4e6}));
  ok(f({cur:-18.2e6,d15:22.0e6})<0,'D6b a strike decaying from -40.2M toward -18.2M SHED mass (the signed d15 called it a receiver)',f({cur:-18.2e6,d15:22.0e6}));
  ok(f({cur:96e3,d15:96e3})===96e3,'D6c on the positive side mass and signed delta agree');
  const nev=ex('nevScan'), rs=ex('rollScan');
  ok(/velMass15\(/.test(nev) && /velMass15/.test(rs),'D6d rollScan AND nevScan both use it');
  ok(!/v\.d15 < -ROLL_MIN_ABS/.test(nev) && !/m\.vel\.d15 > ROLL_MIN_ABS/.test(nev),'D6e the signed-delta pairing is gone from nevScan');
}

// ---- D7 · projReview left the export -----------------------------------------------------------
{
  ok(!/projReview:\(function/.test(code),'D7 the export no longer carries projReview (scores for a surface that stopped rendering at v14)');
}

// ---- ARCHIVE integrity ----------------------------------------------------------------------------
{
  const idx=fs.readFileSync('archive/v15.53/INDEX.md','utf8');
  const names=[...idx.matchAll(/\| `([A-Za-z_0-9$]+)` \|/g)].map(m=>m[1]);
  ok(names.length>=90,'A1 the archive index lists the retired blocks',names.length);
  const stillDefined=names.filter(n=>new RegExp('^function\\s+'+n.replace(/\$/g,'\\$')+'\\s*\\(|^var\\s+'+n.replace(/\$/g,'\\$')+'\\s*=','m').test(code));
  ok(stillDefined.length===0,'A2 none of them is still DEFINED in the live file',stillDefined.slice(0,8));
  const stillRef=names.filter(n=>n!=='_step' && new RegExp('(?<![A-Za-z0-9_$.\\-])'+n.replace(/\$/g,'\\$')+'(?![A-Za-z0-9_$])').test(code));
  ok(stillRef.length===0,'A3 none of them is still REFERENCED in live code (comments excluded)',stillRef.slice(0,8));
  const files=fs.readdirSync('archive/v15.53').filter(d=>fs.statSync('archive/v15.53/'+d).isDirectory());
  ok(files.length>=10,'A4 the archive is grouped by reason',files);
  ok(!/if\(CFG\.panelV3!==false\)\{/.test(code),'A5 render() no longer branches on a flag that was never set');
  ok(!/LOC_SHOW_(NODES|LEVELS|CHART)/.test(code),'A6 the LOC_SHOW_* hard-false flags are gone with their surfaces');
  ok(!/class="g3emw"/.test(code),'A7 the hidden EM rail is no longer built into the HTML');
  // the integrations he protected are intact
  ['irtBuildCsv','irtExportNow','ifChain','ifLadder','futBarsLoad','evCalLoad','ifManPrompt'].forEach(n=>ok(new RegExp('function\\s+'+n+'\\s*\\(').test(code),'A8 protected integration intact: '+n));
  ok(/function darkPoolLevels\(/.test(code) && /function onDarkPool\(/.test(code) && !/function dpLifecycle\(/.test(code),'A9 dark pool: capture and level reader KEPT, lifecycle archived (his decision)');
  ok(/function winToggle\(/.test(code) && !/function pipToggle\(/.test(code),'A10 pop-out: window kept, PiP archived (his decision)');
}

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
