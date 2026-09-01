// ============================================================================================
// test_deps.js — (v15.22) THE DEPENDENCY HEALTH CHECK.
//
// Operator-mandated 2026-09-01, verbatim: "you need to make sure that when the application is
// working , there is some type of test to check insider finance to ensure we are getting data from
// it like the call wall, put wall , expected values, and you need to ensure that the application is
// writing to the king levels to the server for irt. these tests need to be done to ensure they are
// working ... it is fundamental to the application."
//
// ⚠⚠ WHAT THIS FILE CAN AND CANNOT DO, STATED SO NOBODY MISTAKES ONE FOR THE OTHER.
// It cannot tell you the system is working. Every dependency lives OUTSIDE this script — a
// companion userscript, InsiderFinance's payload, a Chrome directory permission — and none of them
// exist in Node. What it CAN do, and does, is prove the CHECK is correct: that `depsHealth()`
// notices a missing call wall, a stale chain, an IRT export that has not run, and does not cry wolf
// when the companion has simply stopped fetching a symbol nothing reads.
//   the CHECK is verified here · the SYSTEM is verified by __gptsDebug.deps() on the running panel
// ⚠ The live verdict rides the `deps` dot on the footer strip, so it is visible without a console.
// ============================================================================================
const fs=require('fs');
const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,200):''));} };
function ex(n){ const m=new RegExp('function\\s+'+n+'\\s*\\(','g').exec(src);
  if(!m) throw new Error('not found: '+n);
  let i=src.indexOf('{',m.index),d=0; for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){ d--; if(!d) return src.slice(m.index,k+1); } } }
const num=n=>parseFloat(new RegExp('var\\s+'+n+'\\s*=\\s*([0-9.]+)').exec(src)[1]);

// ---- harness: a storage and a clock we control ----------------------------------------------
let STORE={};
global.localStorage={ getItem:k=>(k in STORE)?STORE[k]:null, setItem:(k,v)=>{STORE[k]=String(v);} };
global.IFC_KEY='gpts_if_chain_v1';
global.DEPS_IF_STALE_MIN=num('DEPS_IF_STALE_MIN');
global.DEPS_FUT_STALE_MIN=num('DEPS_FUT_STALE_MIN');
global.DEPS_IRT_STALE_MIN=num('DEPS_IRT_STALE_MIN');
global.CFG={ irt:{ on:true } };
global.IRT_LAST={ t:Date.now(), rows:12, err:null };
global.futBarsLoad=()=>({ _at:Date.now(), ES:{ rows:new Array(400).fill([1,2,3,4,5]) } });
global.irtBuildCsv=()=>({ csv:'x', rows:12 });
global.window={ __gptsDebug:{ storage:()=>({ pctFull:30, canWrite40KB:true, health:{ shed:0, quotaHits:0 } }) } };
eval(ex('depsHealth'));

const MIN=60000;
function chain(over){
  const base={ SPX:{ asOf:Date.now(), spot:7641, dte0:{ lv:{ cr:7705, ps:7625 }, em:{ em:20.45 } } },
               QQQ:{ asOf:Date.now(), spot:707,  dte0:{ lv:{ cr:717,  ps:707  }, em:{ em:2.91  } } },
               _at:Date.now() };
  return JSON.stringify(Object.assign(base, over||{}));
}
const byId=(h,id)=>h.items.filter(x=>x.id===id)[0];

// ---- 1 · THE HEALTHY CASE MUST BE QUIET, OR NOBODY WILL READ THE NOISY ONE -------------------
STORE={ gpts_if_chain_v1: chain() };
let H=depsHealth();
ok(H.ok===true, 'd1 EXECUTED: a complete, fresh chain with an IRT write reports healthy', H.fails);
ok(byId(H,'if.SPX').state==='OK', 'd1b ...SPX OK', byId(H,'if.SPX'));
ok(byId(H,'if.SPX').cw===7705 && byId(H,'if.SPX').pw===7625 && byId(H,'if.SPX').em===20.45,
   'd1c ...and it REPORTS the three fields he named, so the hover can show them',
   byId(H,'if.SPX'));

// ---- 2 · EACH OF THE THREE FIELDS, MISSING ON ITS OWN ----------------------------------------
// ⚠ ONE AT A TIME. A single "the chain is broken" case would pass even if the check only ever
// looked at one field — which is exactly the shape of bug this whole file exists to prevent.
[['call wall', { SPX:{ asOf:Date.now(), spot:7641, dte0:{ lv:{ cr:null, ps:7625 }, em:{ em:20 } } } }],
 ['put wall',  { SPX:{ asOf:Date.now(), spot:7641, dte0:{ lv:{ cr:7705, ps:null }, em:{ em:20 } } } }],
 ['expected move', { SPX:{ asOf:Date.now(), spot:7641, dte0:{ lv:{ cr:7705, ps:7625 }, em:null } } }]
].forEach(function(t){
  STORE={ gpts_if_chain_v1: chain(t[1]) };
  const h=depsHealth(), it=byId(h,'if.SPX');
  ok(it.state==='FAIL' && it.why.indexOf(t[0])>=0,
     'd2·'+t[0]+' a chain missing the '+t[0]+' FAILS, and the reason names it', it.why);
});

// ---- 3 · STALENESS IS JUDGED AGAINST THE CLOCK, NOW, NOT AGAINST A STORED FLAG ---------------
// ⚠ THIS IS THE ONE THE LIVE PANEL WAS ALREADY FAILING. On 2026-09-01 the stored SPY chain was
// 15,328 minutes old (payload 2026-08-21) and its OWN `stale` field read false — because that flag
// was computed when the record was written and never again. A dependency check that trusts a
// stored freshness flag is checking that the flag exists.
STORE={ gpts_if_chain_v1: chain({ SPX:{ asOf:Date.now()-15328*MIN, spot:7641, stale:false,
                                        dte0:{ lv:{ cr:7705, ps:7625 }, em:{ em:20 } } } }) };
H=depsHealth();
ok(byId(H,'if.SPX').state==='STALE',
   'd3 an 11-day-old chain is STALE even though its own stale flag says otherwise', byId(H,'if.SPX'));
ok(byId(H,'if.SPX').ageMin>15000, 'd3b ...and the age is reported in minutes', byId(H,'if.SPX').ageMin);

// ---- 4 · AND IT MUST NOT CRY WOLF ------------------------------------------------------------
// Under the v15.06 SPX pin the companion stops fetching SPY. If that alone turned the dot red, the
// dot would be red every session and would stop being read — which is the same as not having it.
STORE={ gpts_if_chain_v1: chain({ SPY:{ asOf:Date.now()-15328*MIN, spot:765,
                                        dte0:{ lv:{ cr:766, ps:765 }, em:null } } }) };
H=depsHealth();
ok(byId(H,'if.SPY').state==='FAIL', 'd4 a stale SPY with no expected move is still reported');
ok(byId(H,'if.usable').state==='OK',
   'd4b ...but the OVERALL verdict stays OK while another symbol has a complete fresh chain',
   byId(H,'if.usable'));
ok(H.ok===false, 'd4c ...and the summary still carries the failure rather than hiding it', H.fails);

// ---- 5 · NO CHAIN AT ALL — the companion is not running --------------------------------------
STORE={};
H=depsHealth();
ok(byId(H,'if.chain').state==='FAIL' && /companion/.test(byId(H,'if.chain').why),
   'd5 no chain in storage names the companion, which is the thing to go and fix',
   byId(H,'if.chain'));

// ---- 6 · A WALL ON THE WRONG SCALE ------------------------------------------------------------
// ⚠ present and plausible are different tests. A call wall of 766 against a spot of 7641 is the
// SPY book sitting in the SPX slot — every level drawn from it would be a decade off and non-null.
STORE={ gpts_if_chain_v1: chain({ SPX:{ asOf:Date.now(), spot:7641,
                                        dte0:{ lv:{ cr:766, ps:7625 }, em:{ em:20 } } } }) };
ok(/off-scale/.test(byId(depsHealth(),'if.SPX').why),
   'd6 a wall a decade away from spot is caught as off-scale, not accepted as present');

// ---- 7 · IRT: THE BUILD AND THE WRITE FAIL DIFFERENTLY AND ARE REPORTED SEPARATELY ------------
STORE={ gpts_if_chain_v1: chain() };
// ⚠⚠ (v15.33) THE BUILD IS JUDGED BY WHAT THE LAST EXPORT WROTE, NOT BY A PROBE. Re-running
// `irtBuildCsv()` as a health check re-runs work whose inputs are live, so one unlucky instant
// reported FAIL while the real export was fine — measured on his panel with IRT_LAST at
// {rows:6, how:'file', inPlace:true, err:null}. A health check observes; it does not perturb.
global.IRT_LAST={ t:Date.now(), rows:0, err:null };
global.irtBuildCsv=()=>({ csv:'x', rows:12 });          // a probe would say "fine" — it must not be asked
ok(byId(depsHealth(),'irt.build').state==='FAIL',
   'd7 an export that wrote NO rows is a build failure, whatever a fresh probe would say');
ok(/last export wrote no rows/.test(byId(depsHealth(),'irt.build').why),
   'd7·why ...and says which fact it is reporting', byId(depsHealth(),'irt.build').why);
global.IRT_LAST={ t:Date.now(), rows:6, err:null };
ok(byId(depsHealth(),'irt.build').state==='OK',
   'd7b six rows on the file is a build that WORKED, even if a probe would fail right now');
// only with no export at all this session does the probe get asked
global.IRT_LAST={ t:0, rows:null, err:null };
global.irtBuildCsv=()=>({ csv:'', rows:0 });
ok(/no export has run yet/.test(byId(depsHealth(),'irt.build').why),
   'd7c ...and the probe is the FALLBACK, used only when nothing has run to observe',
   byId(depsHealth(),'irt.build').why);
global.IRT_LAST={ t:Date.now(), rows:12, err:null };
global.irtBuildCsv=()=>({ csv:'x', rows:12 });
global.IRT_LAST={ t:Date.now()-60*MIN, rows:12, err:null };
ok(byId(depsHealth(),'irt.export').state==='STALE',
   'd7b an export that has not run for an hour is STALE');
global.IRT_LAST={ t:Date.now(), rows:0, err:'NotAllowedError' };
ok(byId(depsHealth(),'irt.export').state==='FAIL' &&
   /NotAllowedError/.test(byId(depsHealth(),'irt.export').why),
   'd7c ...and a permission error is reported VERBATIM — it is the actionable part');
global.IRT_LAST={ t:Date.now(), rows:12, err:null };
global.CFG={ irt:{ on:false } };
ok(byId(depsHealth(),'irt.export').state==='OFF',
   'd7d ...while switched off is OFF, not a failure — that is a choice, not a fault');
global.CFG={ irt:{ on:true } };

// ---- 8 · THE COURIER AND THE RECORDER --------------------------------------------------------
global.futBarsLoad=()=>null;
ok(byId(depsHealth(),'fut.courier').state==='FAIL',
   'd8 no courier data fails — the ⓪a candle has nothing to measure');
global.futBarsLoad=()=>({ _at:Date.now()-60*MIN, ES:{ rows:[[1,2,3,4,5]] } });
ok(byId(depsHealth(),'fut.courier').state==='STALE', 'd8b an hour-old pull is stale');
global.futBarsLoad=()=>({ _at:Date.now(), ES:{ rows:new Array(400).fill([1,2,3,4,5]) } });
global.window.__gptsDebug.storage=()=>({ pctFull:99, canWrite40KB:false, health:{ shed:3, quotaHits:2 } });
ok(byId(depsHealth(),'rec.storage').state==='FAIL',
   'd8c a recorder that can no longer write is a failure, not a warning');

// ---- 9 · IT IS WIRED TO THE FACE AND TO THE CONSOLE -------------------------------------------
ok(/window\.__gptsDebug\.deps=function/.test(src), 'd9 exposed as __gptsDebug.deps()');
const ps=ex('pipeStages');
ok(/depsHealth\(\)/.test(ps), 'd9b ...and the footer strip computes it, so it is visible without a console');
ok(/key:'deps'/.test(ps), 'd9c ...as its own chip');
ok(/DH\.ok\?\(DH\.stales\.length\?'amber':'green'\):'red'/.test(ps.replace(/\s/g,'')),
   'd9d ...red on a failure, amber on staleness, green only when everything is live');
// the doc the next context is required to read
const dep=fs.readFileSync('./design/DEPENDENCIES.md','utf8');
['InsiderFinance','call wall','put wall','expected move','IRT','__gptsDebug.deps()','companion']
  .forEach(w => ok(dep.indexOf(w)>=0, 'd10·'+w+' DEPENDENCIES.md covers '+w));
const skill=fs.readFileSync('./skills/gex/SKILL.md','utf8');
ok(/DEPENDENCIES\.md/.test(skill), 'd11 the gex skill points LOAD at it, so a new context cannot miss it');
ok(/__gptsDebug\.deps\(\)/.test(skill), 'd11b ...and names the live check by the command to run');

console.log('test_deps: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
