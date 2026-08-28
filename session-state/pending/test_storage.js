// ============================================================================================
// test_storage.js — (v14.68) BOUNDED WRITES AND LOUD FAILURE. FINDINGS F-10.
// ⚠ localStorage filled to exactly 10,240 KB on 2026-08-28 and EVERY setItem in the system failed
// behind catch(e){} for about a week. One fault, five symptoms. These tests EXECUTE lsPut against a
// fake storage with a real quota, because a grep cannot tell a bounded write from an unbounded one.
// ============================================================================================
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0,fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){ const re=new RegExp('function\\s+'+n+'\\s*\\(','g'); const m=re.exec(src);
  if(!m) return ''; let i=src.indexOf('{',m.index),d=0,e=-1;
  for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){d--; if(d===0){e=k;break;}} }
  return src.slice(m.index,e+1); }
function val(n){ const m=new RegExp('(?:var\\s+)?\\b'+n+'\\s*=\\s*([\\s\\S]*?);\\n').exec(src); return m?eval('('+m[1]+')'):undefined; }

// a fake localStorage with a REAL quota, so the quota path is exercised rather than described
let CAP=4000, STORE={};
function used(){ let t=0; for(const k in STORE) t+=(k.length+STORE[k].length)*2; return t; }
global.localStorage={ getItem:k=>(k in STORE?STORE[k]:null),
  setItem:(k,v)=>{ const prev=STORE[k]; delete STORE[k];
    if(used()+(k.length+String(v).length)*2>CAP){ if(prev!==undefined) STORE[k]=prev;
      const e=new Error('quota'); e.name='QuotaExceededError'; throw e; }
    STORE[k]=String(v); },
  removeItem:k=>{delete STORE[k];}, key:i=>Object.keys(STORE)[i],
  get length(){ return Object.keys(STORE).length; } };
let SWALLOWED=[]; global.swallow=(tag,e)=>SWALLOWED.push(tag);
global.LS_CAP_KB=val('LS_CAP_KB'); global.LS_BUDGET_KB=val('LS_BUDGET_KB'); global.LS_HEALTH=val('LS_HEALTH');
eval(ex('lsKB')); eval(ex('lsTotalKB')); eval(ex('lsPut'));

// ---- 1 · the budget is enforced BEFORE the write ---------------------------------------------
{
  STORE={}; SWALLOWED=[]; LS_HEALTH.shed=0;
  const big={ days:{ a:{ev:new Array(400).fill('xxxxxxxxxxxxxxxx')} } };
  const shed=d=>{ const e=d.days.a.ev; if(e.length<=10) return false; d.days.a.ev=e.slice(100); return true; };
  LS_BUDGET_KB['k1']=2;
  const r=lsPut('k1', big, shed);
  ok(r===true,'s1 a payload over budget is SHRUNK and written, not dropped');
  ok(lsKB(STORE['k1'])<=2,'s2 ...and the stored payload respects the byte budget', lsKB(STORE['k1']));
  ok(LS_HEALTH.shed>0,'s3 the shedder actually ran', LS_HEALTH.shed);
}
// ---- 2 · ⚠ THE FAILURE IS LOUD. This is the whole point of F-10. -----------------------------
{
  STORE={}; SWALLOWED=[]; LS_HEALTH.quotaHits=0; LS_HEALTH.lastErr=null;
  STORE['filler']='y'.repeat(1900);                 // leave almost no room
  LS_BUDGET_KB['k2']=999999;                        // budget won't save us; only the quota bites
  const r=lsPut('k2', {blob:'z'.repeat(2000)}, null);
  ok(r===false,'s4 an impossible write returns false rather than pretending', r);
  ok(LS_HEALTH.quotaHits===1,'s5 the quota hit is COUNTED', LS_HEALTH.quotaHits);
  ok(SWALLOWED.some(t=>/STORAGE FULL/.test(t)),
     's6 ...and reported through swallow() so it reaches renderErrors() — the week-long silence',
     SWALLOWED);
  ok(/QuotaExceeded/.test(LS_HEALTH.lastErr||''),'s7 lastErr names the real error', LS_HEALTH.lastErr);
  ok(/of \d+KB/.test(LS_HEALTH.lastErr||''),'s8 ...and carries the total vs the cap');
}
// ---- 3 · a shedder rescues a write that would otherwise fail ---------------------------------
{
  STORE={}; SWALLOWED=[]; LS_HEALTH.quotaHits=0;
  STORE['filler']='y'.repeat(1500);
  LS_BUDGET_KB['k3']=999999;
  const obj={ev:new Array(300).fill('abcdefgh')};
  const r=lsPut('k3', obj, d=>{ if(d.ev.length<=5) return false; d.ev=d.ev.slice(50); return true; });
  ok(r===true,'s9 a shedder rescues the write after the quota bites', r);
  ok(obj.ev.length<300,'s10 ...by actually shrinking the object', obj.ev.length);
}
// ---- 4 · the real shedders shed the right things ---------------------------------------------
{
  const rs=ex('recorderSave');
  ok(/day\.snaps/.test(rs),'s11 recorderSave sheds SNAPSHOTS — 5,957KB was mostly snaps, not feat');
  ok(/day\.feat/.test(rs),'s12 ...and feature records');
  ok(/ds\[i\]!==TODAY/.test(rs),'s13 ...dropping whole OLDER days first');
  ok(!/delete d\.days\[TODAY\]/.test(rs),'s14 ...and NEVER deleting today outright');
  const ns=ex('nevSave');
  ok(/lsPut/.test(ns),'s15 nevSave is budgeted — it had NO size cap at all');
  ok(/slice/.test(ns),'s16 ...and sheds oldest events first');
}
// ---- 5 · the budgets are real numbers under the cap ------------------------------------------
{
  const B=val('LS_BUDGET_KB'), cap=val('LS_CAP_KB');
  ok(B['gpts_recorder_v7']>0 && B['gpts_nodeevents_v1']>0,'s17 both offenders have a byte budget', B);
  ok(B['gpts_recorder_v7']+B['gpts_nodeevents_v1'] < cap*0.6,
     's18 ...and together they leave room for everything else', [B,cap]);
  ok(cap===10240,'s19 the cap is the measured Chrome limit, not a guess', cap);
}
console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
