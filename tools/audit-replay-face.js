// audit-replay-face.js — decode what the REPLAYED face says at a chosen minute, and put it beside
// the recorded book, so a claim about the arrows or the statuses can be checked instead of asserted.
//   node tools/audit-replay-face.js 2026-08-31 14:12
// ⚠ It EXECUTES the panel's own levelStateOf / rollScan / peakOf out of current/…user.js. Anything
// re-implemented here (the node filter, the velocity lookup) is marked; everything else is theirs.
const fs=require('fs');
const src=fs.readFileSync('./current/gex-signal-tapereader.user.js','utf8');
function ex(n){ const m=new RegExp('function\\s+'+n+'\\s*\\(','g').exec(src);
  if(!m) throw new Error('not found: '+n);
  let i=src.indexOf('{',m.index),d=0; for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){ d--; if(!d) return src.slice(m.index,k+1);} } }
const num=n=>{ const m=new RegExp('var\\s+'+n+'\\s*=\\s*(-?[0-9.]+)').exec(src); return m?parseFloat(m[1]):undefined; };

const day=process.argv[2]||'2026-08-31', want=process.argv[3]||'14:12';
const D=JSON.parse(fs.readFileSync('./data/'+day+'.json','utf8'));
const FR=(D.snaps&&D.snaps.SPY)||[];
// ⚠ CENTRAL TIME, because the panel is: ctNow() uses America/Chicago and replaySec() subtracts
// 5h. An audit on a different clock silently compares 15:12's book against 14:12's claim.
const hhmm=t=>new Date(t).toLocaleTimeString('en-US',{timeZone:'America/Chicago',hour12:false,hour:'2-digit',minute:'2-digit'});

global.mul=(a,b)=>a/(1/b);
global.swallow=()=>{};
global.REPLAY={ on:true, day:day, idx:0, frames:FR, days:null, loading:false, err:null };
global.RP_PEAK={ key:null, m:null };
global.PEAK={ m:{} };
global.LVL_SPENT_PEAK=num('LVL_SPENT_PEAK'); global.LVL_BUILD_P15=num('LVL_BUILD_P15');
global.LVL_WEAK_P15=num('LVL_WEAK_P15');     global.LVL_TURN_P15=num('LVL_TURN_P15');
global.ROLL_MIN_ABS=num('ROLL_MIN_ABS'); global.ROLL_MAX_DIST=num('ROLL_MAX_DIST'); global.ROLL_MIN_RATIO=num('ROLL_MIN_RATIO');
global.TAP_PROB={0:80,1:66,2:33};
global.nodeTapCount=()=>0;
eval(ex('replayOn')); eval(ex('replayFrame')); eval(ex('replayBookOf')); eval(ex('replayBook'));
eval(ex('replayEmptyBook')); eval(ex('replayPeakOf')); eval(ex('peakOf')); eval(ex('levelStateOf'));
eval(ex('rollScan'));
global.velOk=()=>true;
let BOOK=null;
global.velAt=k=>{ const v=BOOK&&BOOK.vel?(BOOK.vel[String(k)]||BOOK.vel[(+k).toFixed(2)]):null;
                  return v?{ v:v, age:0, stale:false, replay:true }:null; };

// park on the frame the slider would snap to
let idx=0, best=1e9;
FR.forEach((f,i)=>{ const d=Math.abs((+hhmm(f.t).split(':')[0]*60+ +hhmm(f.t).split(':')[1]) - (+want.split(':')[0]*60+ +want.split(':')[1]));
                    if(d<best){ best=d; idx=i; } });
REPLAY.idx=idx;
BOOK=replayBook('SPXW');
const king=(FR[idx].tri&&FR[idx].tri.SPXW&&FR[idx].tri.SPXW.king)||null;
console.log('parked  '+hhmm(FR[idx].t)+'  (frame '+idx+' of '+FR.length+')   SPXW king '+king);

// ⚠ RE-IMPLEMENTED: the node universe. The live rail derives it through emBand/emPiles, which needs
// candles and the EM pin; the confirmed equivalent is every strike at or above 20% of King.
const rows=Object.keys(BOOK.pct||{}).map(k=>({k:+k, pct:Math.abs(BOOK.pct[k]), cur:(BOOK.vel[k]||{}).cur}))
  .filter(r=>r.pct>=20).sort((a,b)=>b.pct-a.pct);

// the whole-day peak — what peakOf returned BEFORE v15.18
const dayPeak={}; FR.forEach(f=>((f.vend&&f.vend.rows)||[]).forEach(r=>{ const k=r[0], c=Math.abs(r[1]*1e6||0); }));
function wholeDayPeak(k){ let mx=0; for(let i=0;i<FR.length;i++){ REPLAY.idx=i; const b=replayBook('SPXW');
    const v=b&&b.vel&&b.vel[String(k)]; if(v&&typeof v.cur==='number') mx=Math.max(mx,Math.abs(v.cur)); } REPLAY.idx=idx; BOOK=replayBook('SPXW'); return mx||null; }

console.log('\nstrike   %King        |cur|      peak->now   ret    v15.18 STATE     (v15.17 ret / STATE)');
rows.forEach(r=>{
  const st=levelStateOf(r.k, null);
  const pkNow=peakOf(r.k), cur=Math.abs(r.cur||0);
  const ret=pkNow?cur/pkNow:null;
  const wd=wholeDayPeak(r.k), retOld=wd?cur/wd:null;
  RP_PEAK.key=null; PEAK.m={}; PEAK.m[r.k]=wd;                 // simulate the OLD denominator
  const saveOn=REPLAY.on; REPLAY.on=false; const stOld=levelStateOf(r.k, null); REPLAY.on=saveOn;
  PEAK.m={};
  console.log(String(r.k).padEnd(8)+String(r.pct).padStart(4)+'%'+
    String(Math.round(cur)).padStart(14)+String(Math.round(pkNow||0)).padStart(14)+
    (ret!=null?ret.toFixed(2):' -- ').padStart(7)+'   '+st.st.padEnd(12)+
    '  ('+(retOld!=null?retOld.toFixed(2):'--')+' / '+stOld.st+')');
});

const arrows=rollScan(rows.map(r=>r.k));
console.log('\nARROWS drawn at this minute: '+arrows.length);
arrows.forEach(a=>console.log('  '+a.from+' -> '+a.to+'   lost(d15) '+Math.round(a.lost)+
  '   got(d15) '+Math.round(a.got)+'   dist '+Math.abs(a.to-a.from)));
console.log('\nthe rows rollScan saw:');
rows.forEach(r=>{ const v=velAt(r.k).v;
  console.log('  '+r.k+'  cur '+String(Math.round(v.cur)).padStart(12)+
    '   d15 '+String(Math.round(v.d15)).padStart(12)+
    '   rp15 '+(v.rp15==null?'--':v.rp15.toFixed(1)+'%').padStart(8)+
    '   |cur| 15m ago '+Math.round(Math.abs(v.cur-v.d15))); });
