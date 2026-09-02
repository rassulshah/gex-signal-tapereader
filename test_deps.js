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
// ⚠ (v15.37) THE CALENDAR IS INJECTED BY THE GETTER, NOT PUT IN STORE. Every case below reassigns
// STORE wholesale to set up its own scenario; a calendar living in STORE would vanish with each one
// and every unrelated case would start failing on `cal.ff`. The FF cases override CAL directly.
const TODAY='2026-09-01';
let CAL=JSON.stringify({ day:TODAY, ev:[{t:Date.now(),title:'CPI m/m'}] });
global.localStorage={ getItem:k=>{
    if(k==='gpts_evcal_v1') return (k in STORE)?STORE[k]:CAL;
    return (k in STORE)?STORE[k]:null;
  }, setItem:(k,v)=>{STORE[k]=String(v);} };
global.EVCAL_KEY='gpts_evcal_v1';
global.EVCAL_STATE={ tried:true, err:null };
global.ctTodayStr=()=>TODAY;
global.IFC_KEY='gpts_if_chain_v1';
global.DEPS_IF_STALE_MIN=num('DEPS_IF_STALE_MIN');
global.DEPS_FUT_STALE_MIN=num('DEPS_FUT_STALE_MIN');
global.DEPS_IRT_STALE_MIN=num('DEPS_IRT_STALE_MIN');
global.CFG={ irt:{ on:true } };
global.IRT_LAST={ t:Date.now(), rows:12, err:null };
global.futBarsLoad=()=>({ _at:Date.now(), ES:{ rows:new Array(400).fill([1,2,3,4,5]) } });
global.irtBuildCsv=()=>({ csv:'x', rows:12 });
global.window={ __gptsDebug:{ storage:()=>({ pctFull:30, canWrite40KB:true, health:{ shed:0, quotaHits:0 } }) } };
// ⚠ (v15.43) the session-aware clock the deps are graded on. The harness controls it directly so
// "outside RTH" is a scenario we can RUN rather than wait for — and so the wall-clock/replay
// distinction is testable at all.
global.ctOffsetSec=()=>5*3600;
let IDLE={ idle:0, P:{ rth:true, dow:2, mins:600, open:510, close:900 } };
global.depsSessionIdleMin=()=>IDLE;
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


// ============================================================================================
// 6 · (v15.37) YAHOO FINANCE AND FOREXFACTORY — the two feeds that ran UNLABELLED for 27 builds
//
// Operator, 2026-09-01: "add indicator for yahoo finance integration also, call it YF and forex
// factory, call it FF. All of this integration better be mentioned somewhere. check where it is
// mentioned." ⚠ FF was mentioned in exactly two code comments: no deps item, no lamp, and NOTHING
// in DEPENDENCIES.md — whose own §0 warns that every dependency here fails silently.
// ============================================================================================

// ---- YF: the courier already had a check; it did NOT have a lamp -----------------------------
STORE={ gpts_if_chain_v1: chain() };
ok(byId(depsHealth(),'fut.courier').state==='OK', 'd12 YF: a fresh courier with rows is OK');
{
  const save=global.futBarsLoad;
  global.futBarsLoad=()=>null;
  ok(byId(depsHealth(),'fut.courier').state==='FAIL', 'd12b YF: no courier data FAILS');
  global.futBarsLoad=()=>({ _at:Date.now()-(DEPS_FUT_STALE_MIN+5)*MIN, ES:{ rows:[[1]] } });
  const y=byId(depsHealth(),'fut.courier');
  ok(y.state==='STALE' && /minutes old/.test(y.why), 'd12c YF: an old pull is STALE and says how old', y.why);
  // ⚠ THE FAILURE MODE THAT MATTERS: stale bars still have a high and a low. The panel does not go
  // blank, it goes WRONG — which is why this one needs a lamp and not just a console item.
  ok(y.rows>0, 'd12d YF: ...and it still reports rows, because stale data is not absent data', y.rows);
  global.futBarsLoad=save;
}

// ---- FF: a dependency that had no check at all -----------------------------------------------
STORE={ gpts_if_chain_v1: chain() };
CAL=JSON.stringify({ day:TODAY, ev:[{t:1,title:'CPI m/m'},{t:2,title:'FOMC Statement'}] });
let F=byId(depsHealth(),'cal.ff');
ok(!!F, 'd13 FF: the ForexFactory calendar IS a checked dependency now');
ok(F.state==='OK' && F.events===2, 'd13b FF: today\u2019s delivery with events is OK, and it counts them', F);

// ⚠⚠ THE ONE THAT WOULD HAVE BEEN GOT WRONG. Zero events is the MOST COMMON healthy answer.
CAL=JSON.stringify({ day:TODAY, ev:[] });
F=byId(depsHealth(),'cal.ff');
ok(F.state==='OK', 'd13c FF: ZERO EVENTS IS A VALID DELIVERY, not a failure', F);
ok(F.events===0, 'd13d FF: ...and it is reported as 0, not as null/absent', F.events);
// proof the distinction is load-bearing: counting instead of day-stamping calls this a failure
ok((function(){ const c=JSON.parse(CAL); return (c.ev.length>0)===false; })(),
   'd13e FF: ...a count-based check would have called every quiet day broken');

CAL=JSON.stringify({ day:'2026-08-29', ev:[{t:1,title:'NFP'}] });
F=byId(depsHealth(),'cal.ff');
ok(F.state==='STALE' && /not today/.test(F.why), 'd13f FF: another day\u2019s calendar is STALE and names the day', F.why);
ok(F.day==='2026-08-29', 'd13g FF: ...and reports WHICH day, so the staleness is checkable', F.day);

CAL=null;
F=byId(depsHealth(),'cal.ff');
ok(F.state==='FAIL', 'd13h FF: no delivery at all FAILS');
global.EVCAL_STATE={ tried:true, err:'Failed to fetch' };
ok(/Failed to fetch/.test(byId(depsHealth(),'cal.ff').why) &&
   /companion/.test(byId(depsHealth(),'cal.ff').why),
   'd13i FF: ...and when the PAGE fetch failed it says so, and names the companion as the courier',
   byId(depsHealth(),'cal.ff').why);
global.EVCAL_STATE={ tried:true, err:null };
CAL=JSON.stringify({ day:TODAY, ev:[] });

// ---- 7 · THE LAMPS — four feeds, four lamps, and the RIGHT KIND of number on each ------------
const lampFn=ex('feedLampsHtml');
ok(/lamp\('IRT'/.test(lampFn) && /lamp\('IF'/.test(lampFn), 'd14 the two original lamps survive');
ok(/lamp\('YF'/.test(lampFn), 'd14b YF has a lamp');
ok(/lamp\('FF'/.test(lampFn), 'd14c FF has a lamp');
ok(/by\['fut\.courier'\]/.test(lampFn), 'd14d YF reads the SAME deps item, not its own opinion');
ok(/by\['cal\.ff'\]/.test(lampFn), 'd14e FF reads the SAME deps item, not its own opinion');
// ⚠ the age/count distinction is the substance of this change, not decoration
ok(/function lamp\(label, it, extra, txt\)/.test(lampFn),
   'd14f lamp() takes a text override, so a feed can show something other than an age');
ok(/'ev'/.test(lampFn), 'd14g FF shows an EVENT COUNT');
ok(!/'FF',[\s\S]{0,900}?ageMin/.test(lampFn),
   'd14h ...and NOT an age — "FF 340m" on a healthy calendar would go red on its own');
ok(/0ev/.test(lampFn), 'd14i the FF hover states that 0ev is a healthy answer');
ok(/Yahoo|YAHOO/.test(lampFn), 'd14j the YF hover names Yahoo Finance, not just "the courier"');
ok(/FOREXFACTORY|ForexFactory/.test(lampFn), 'd14k the FF hover names ForexFactory');

// ⚠ THE HEADER IS ONE ROW AND IT DOES NOT WRAP. MEASURED in real Chromium at his panel width (673)
// with the shipping CSS: the four lamps occupy 185px and end at x=337, the right-hand controls
// start at x=529 — 192px of slack, and the narrowest the panel can go is 652 (panelWidthBounds()
// floor = LAD_W+34). This guards the thing that would eat that slack: a long label.
{
  const labels=(lampFn.match(/lamp\('([A-Z]+)'/g)||[]).map(x=>x.slice(6,-1));
  ok(labels.length===4, 'd14l exactly four lamps ride the header', labels);
  ok(labels.every(l=>l.length<=3), 'd14m every label is <=3 chars — the header is one row and does not wrap', labels);
}

// ---- 8 · AND IT IS WRITTEN DOWN — the half of the ask that was about DOCUMENTATION ------------
['Yahoo','ForexFactory','gpts_evcal_v1','ff_calendar_thisweek','cal.ff','fut.courier','YF','FF']
  .forEach(w => ok(dep.indexOf(w)>=0, 'd15·'+w+' DEPENDENCIES.md covers '+w));
ok(/ZERO EVENTS IS A VALID DELIVERY/.test(dep), 'd15b ...including the subtle part, in the doc');
ok(/0ev/.test(dep) && /counted, not aged|COUNTED, NOT AGED/i.test(dep),
   'd15c ...and why FF is counted rather than aged');
ok(/\| \*\*YF\*\* \|/.test(dep) && /\| \*\*FF\*\* \|/.test(dep),
   'd15d ...and the lamp table lists all four, so the face and the doc agree');
// ⚠ AND EACH ONE OWNS A SECTION. Asserting the WORD appears is not enough — "Yahoo" also occurs in
// a sentence inside another section, so a mutation that retitled the section away still passed.
// "mentioned somewhere" means findable by someone scanning the headings, which is the whole ask.
{
  const heads=(dep.match(/^## .*$/gm)||[]);
  ok(heads.some(h=>/YAHOO FINANCE/i.test(h) && /YF/.test(h)),
     'd15e Yahoo Finance owns a SECTION, titled with its lamp name', heads);
  ok(heads.some(h=>/FOREXFACTORY/i.test(h) && /FF/.test(h)),
     'd15f ForexFactory owns a SECTION, titled with its lamp name', heads);
  ok(heads.some(h=>/INSIDERFINANCE/i.test(h)) && heads.some(h=>/IRT/.test(h)),
     'd15g ...and the two original dependencies still do', heads);
}


// ============================================================================================
// 9 · (v15.43) THE CHECK HAD NO CONCEPT OF THE SESSION AND FAILED ALL NIGHT
//
// Measured on his panel at 04:52 CT: `deps 2X`, with if.SPX 386m, if.QQQ 385m, fut.courier 426m,
// irt.export 294m all STALE and if.usable FAIL — "running blind". Every one is the CORRECT
// overnight state; the market had been shut fourteen hours. ⚠ The dot is red ~17 hours a day, and a
// light that is red most of the time is one he stops reading — the exact failure `if.usable`'s own
// comment forbids. It guarded the SPX pin and not the clock.
// ⚠ THE RULE IS NOT "IGNORE STALENESS WHEN CLOSED". It is WAS IT FRESH WHEN THE SESSION ENDED.
// ============================================================================================
const RTH ={ idle:0,   P:{ rth:true,  dow:2, mins:600, open:510, close:900 } };
const NIGHT={ idle:832, P:{ rth:false, dow:3, mins:292, open:510, close:900 } };  // 04:52 Wed

// ---- the overnight state must be QUIET ------------------------------------------------------
STORE={ gpts_if_chain_v1: chain({ SPX:{ asOf:Date.now()-386*MIN, spot:7641, dte0:{ lv:{cr:7705, ps:7625}, em:{em:20.45} } },
                                  QQQ:{ asOf:Date.now()-385*MIN, spot:707,  dte0:{ lv:{cr:717, ps:707}, em:{em:2.91} } },
                                  _at:Date.now()-385*MIN }) };
global.futBarsLoad=()=>({ _at:Date.now()-426*MIN, ES:{ rows:new Array(400).fill([1,2,3,4,5]) } });
global.IRT_LAST={ t:Date.now()-294*MIN, rows:12, err:null };

IDLE=RTH;
let H9=depsHealth();
ok(byId(H9,'if.SPX').state==='STALE' && byId(H9,'fut.courier').state==='STALE',
   'd16 EXECUTED: judged as if it were RTH, his real overnight ages ARE stale — the old verdict');
ok(H9.ok===false, 'd16b ...and the whole check fails, which is the red dot he saw');

IDLE=NIGHT;
H9=depsHealth();
ok(byId(H9,'if.SPX').state==='OK', 'd17 EXECUTED: judged against the CLOSE, a 386m chain at 04:52 is OK',
   byId(H9,'if.SPX'));
ok(byId(H9,'fut.courier').state==='OK', 'd17b ...and a 426m courier pull is OK', byId(H9,'fut.courier').why);
ok(byId(H9,'irt.export').state==='OK',  'd17c ...and a 294m IRT write is OK');
ok(byId(H9,'if.usable').state==='OK',   'd17d ...so the panel is NOT "running blind" overnight');

// ---- but a REAL mid-session death must still fail --------------------------------------------
// ⚠ THIS IS THE HALF THAT MATTERS. A rule that just muted the night would hide a courier that
// stopped at 10:00 — the failure the check exists for, on the day it exists for.
global.futBarsLoad=()=>({ _at:Date.now()-(832+300)*MIN, ES:{ rows:new Array(400).fill([1,2,3,4,5]) } });
H9=depsHealth();
const dead=byId(H9,'fut.courier');
ok(dead.state==='STALE', 'd18 EXECUTED: a courier that died 300 minutes BEFORE the close still fails', dead.state);
ok(/minutes old/.test(dead.why) && dead.ageMin>1100,
   'd18b ...and reports the TRUE age, not the discounted one — the age is never hidden', dead.ageMin);
global.futBarsLoad=()=>({ _at:Date.now()-426*MIN, ES:{ rows:new Array(400).fill([1,2,3,4,5]) } });

// ---- the boundary, both sides ---------------------------------------------------------------
global.IRT_LAST={ t:Date.now()-(832+DEPS_IRT_STALE_MIN-1)*MIN, rows:12, err:null };
ok(byId(depsHealth(),'irt.export').state==='OK', 'd19 one minute inside the window at the close is OK');
global.IRT_LAST={ t:Date.now()-(832+DEPS_IRT_STALE_MIN+1)*MIN, rows:12, err:null };
ok(byId(depsHealth(),'irt.export').state==='STALE', 'd19b one minute outside it is STALE');
global.IRT_LAST={ t:Date.now()-294*MIN, rows:12, err:null };

// ---- it says WHY the night is quiet, rather than going silently green ------------------------
IDLE=NIGHT; H9=depsHealth();
ok(/market has been closed/.test(byId(H9,'fut.courier').why||''),
   'd20 an idle feed EXPLAINS itself instead of just turning green', byId(H9,'fut.courier').why);
ok(H9.idleMin===832 && H9.rthNow===false,
   'd20b ...and the check reports the clock it used, so the verdict is checkable', {idle:H9.idleMin, rth:H9.rthNow});
// ⚠ THE CLAMP IS DEFENSIVE AND SURVIVES MUTATION, AND SAYING SO IS THE HONEST ANSWER. Removing
// `Math.max(0,...)` changes no verdict today — a negative effective age is still below every
// threshold — so no behavioural test can catch it. It stays because an "age" that can go negative
// is nonsense the moment anyone does arithmetic on it, which is how `hlPT` produced 6895 points.
// Same standing as the deliberately-redundant clamp in the ladder view (v15.31).
ok(/function eff\(a\)\{ return \(typeof a==='number'\) \? Math\.max\(0, a-_idle\) : a; \}/.test(src),
   'd20c the effective age is floored at zero — defensive, and pinned as such');

// ---- ⚠⚠ THE CLOCK MUST BE THE WALL CLOCK, NOT sessionPhase()'s ------------------------------
// `sessionPhase()` with no argument is REPLAY-AWARE by design (v15.18): measured this minute,
// parked at 14:06, it returns rth:true / POWER HOUR while the real time was 04:52. Grading the LIVE
// feeds against the REPLAYED session would call correctly-idle overnight couriers broken the moment
// he rewinds — a bug I would have shipped by writing the obvious `sessionPhase()`.
const idleFn=ex('depsSessionIdleMin');
ok(/sessionPhase\(\(\(wall%86400\)\+86400\)%86400\)/.test(idleFn),
   'd21 the idle clock passes an EXPLICIT wall-clock second-of-day to sessionPhase()');
ok(/Date\.now\(\)\/1000 - ctOffsetSec\(\)/.test(idleFn),
   'd21b ...built from Date.now(), which the replay slider cannot move');
ok(!/sessionPhase\(\)/.test(idleFn),
   'd21c ...and NEVER calls the bare, replay-aware sessionPhase()');
// weekends walk back to the previous trading day rather than to "yesterday"
ok(/d=\(d\+6\)%7; back\+=1440; if\(d>=1 && d<=5\) break;/.test(idleFn),
   'd22 pre-open it walks back to the previous TRADING day, so Monday counts the weekend');
ok(/HOLIDAYS ARE NOT MODELLED/.test(idleFn),
   'd22b ...and the holiday gap is stated, erring toward noise rather than silence');

console.log('test_deps: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
