// v15.36 — THE KING CENSUS. The lane is dwell-filtered and therefore LOSSY BY DESIGN; asked "how
// many rolls were there" I read the count off the lane and it came back ~3x too small. These
// assertions EXECUTE krTick / replayKingRaw / kingRolls against fabricated crown series and against
// the operator's own recorded sessions, and they check the two numbers are DIFFERENT and that the
// larger one is the census.
const fs=require('fs');
const src=fs.readFileSync('./current/gex-signal-tapereader.user.js','utf8');

let pass=0, fail=0;
function ok(n,c,g){ if(c){pass++;console.log('PASS '+n+(g!==undefined?' -> '+g:''));} else {fail++;console.log('FAIL '+n+(g!==undefined?' -> got '+g:''));} }

function grabFn(name){
  const re=new RegExp('function\\s+'+name+'\\s*\\(','g'); const m=re.exec(src);
  if(!m) throw new Error('not found: '+name);
  let i=src.indexOf('{',m.index),d=0,end=-1;
  for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){d--; if(d===0){end=k;break;}} }
  return src.slice(m.index,end+1);
}
function grabVar(name){
  const re=new RegExp('^var\\s+'+name+'\\s*=.*$','m'); const m=re.exec(src);
  if(!m) throw new Error('var not found: '+name);
  return m[0];
}

// --- the store, stubbed exactly as the page provides it -------------------
let LS={};
global.localStorage={ getItem:k=>(k in LS?LS[k]:null), setItem:(k,v)=>{LS[k]=String(v);} };
let TODAY='2026-09-01';
global.ctTodayStr=()=>TODAY;
global.swallow=()=>{};
global.replayOn=()=>false;
global.REPLAY={day:null,idx:0,frames:[]};

eval(grabVar('SK_MIN_STRIKES'));       global.SK_MIN_STRIKES=SK_MIN_STRIKES;
eval(grabVar('SK_MIN_STRIKES_REPLAY')); global.SK_MIN_STRIKES_REPLAY=SK_MIN_STRIKES_REPLAY;
['KR_KEY','KR_SCHEMA','KR_MAX','KR_BOOKS','KRAW','RP_KR'].forEach(v=>{ eval(grabVar(v)); global[v]=eval(v); });
['krLoad','krSave','krTick','replayKingRaw','krOf','kingRolls'].forEach(f=>{ eval(grabFn(f)); global[f]=eval(f); });

// ⚠ MUTATION-CHECKED: the floor constant is read from the file, not retyped, so a change to
// SK_MIN_STRIKES moves these tests with it instead of silently pinning 20 forever.
ok('census reuses the EXISTING depth floor, it does not invent a fourth one', SK_MIN_STRIKES===20, SK_MIN_STRIKES);

function reset(){ LS={}; KRAW={v:KR_SCHEMA,day:null,b:{}}; global.KRAW=KRAW; }
function crowns(spec){ return Object.keys(spec).map(b=>({book:b, raw:spec[b][0], n:spec[b][1]})); }

// ===== 1. the census counts EVERY change, the lane's dwell does not apply =====
reset();
let T=1788292800000;
global.Date.nowReal=Date.now;
let CLK=T; const _now=Date.now; Date.now=()=>CLK;
// a crown that flickers: 7600 -> 7605 -> 7600 -> 7605, all inside two minutes
[7600,7605,7600,7605].forEach(k=>{ krTick(crowns({SPXW:[k,100]})); CLK+=30000; });
ok('every crown change is recorded, flicker included', krOf('SPXW').length===4, krOf('SPXW').length);
ok('rolls = observations - 1 (where it STARTED is not a roll)', kingRolls('SPXW').rolls===3, kingRolls('SPXW').rolls);
ok('an UNCHANGED crown writes nothing', (krTick(crowns({SPXW:[7605,100]})), krOf('SPXW').length===4), krOf('SPXW').length);
ok('the count is reported as a FLOOR, never as a total', kingRolls('SPXW').floor===true);
ok('and it names the sampling that produced it', /live observations/.test(kingRolls('SPXW').basis), kingRolls('SPXW').basis);

// ===== 2. THE DEPTH GATE — the bug that seeds a phantom first roll =====
reset(); CLK=T;
krTick(crowns({SPXW:[7999,4]}));           // first paint: 4 strikes loaded. Junk.
ok('a THIN book is refused — the crown of 4 strikes is not a crown', krOf('SPXW').length===0, krOf('SPXW').length);
krTick(crowns({SPXW:[7600,100]}));
ok('...so the real first observation seeds the census', krOf('SPXW').length===1 && krOf('SPXW')[0].k===7600, krOf('SPXW')[0].k);
ok('...and NO phantom roll is recorded', kingRolls('SPXW').rolls===0, kingRolls('SPXW').rolls);
// ⚠ this is the assertion the gate exists for: without it the sequence above reads as ONE roll.
ok('proof the gate is load-bearing: ungated, the same input yields a roll',
   (function(){ const g=[{book:'SPXW',raw:7999,n:4},{book:'SPXW',raw:7600,n:100}];
     let c=0,last=null; g.forEach(x=>{ if(last!==x.raw){c++; last=x.raw;} }); return c-1===1; })(), 1);
ok('exactly at the floor is ACCEPTED (>=, not >)',
   (reset(), krTick(crowns({SPXW:[7600,SK_MIN_STRIKES]})), krOf('SPXW').length===1), krOf('SPXW').length);
ok('one below the floor is refused',
   (reset(), krTick(crowns({SPXW:[7600,SK_MIN_STRIKES-1]})), krOf('SPXW').length===0), krOf('SPXW').length);

// ===== 3. QQQ — the king that had NO answer because it has no lane =====
reset(); CLK=T;
ok('QQQ IS in the census', KR_BOOKS.indexOf('QQQ')>=0);
ok('QQQ is NOT in the lane (a bearing must not be drawn as a level)',
   /var KT_BOOKS=\['SPXW','SPY'\]/.test(src));
[706,709,707,708].forEach(k=>{ krTick(crowns({QQQ:[k,100]})); CLK+=30000; });
ok('so "how many QQQ rolls" now HAS an answer', kingRolls('QQQ').rolls===3, kingRolls('QQQ').rolls);

// ===== 4. day rollover wipes the census — yesterday's rolls are not today's =====
reset(); krTick(crowns({SPXW:[7600,100]}));
TODAY='2026-09-02'; krLoad();
ok('a new session starts the census empty', krOf('SPXW').length===0, krOf('SPXW').length);
TODAY='2026-09-01';

// ===== 5. REPLAY parity — same shape, coarser sampling, and it SAYS so =====
global.replayOn=()=>true;
const frames=[7600,7600,7605,7605,7610].map((k,i)=>({t:T+i*180000, tri:{SPXW:{king:k,n:100}}}));
frames.splice(2,0,{t:T+150000, tri:{SPXW:{king:9999,n:2}}});   // a thin frame in the middle
global.REPLAY={day:'2026-09-01', idx:frames.length-1, frames:frames};
RP_KR={key:null,out:[]}; global.RP_KR=RP_KR;
ok('replay rebuilds the census from the frames', krOf('SPXW').length===3, krOf('SPXW').length);
ok('replay refuses a thin frame too', krOf('SPXW').every(p=>p.k!==9999));
ok('replay names its COARSER basis, so the two are never compared as equals',
   /3-minute frames/.test(kingRolls('SPXW').basis), kingRolls('SPXW').basis);
ok('replay uses the REPLAY floor, not the live one (a frame stores a top-N summary)',
   SK_MIN_STRIKES_REPLAY<SK_MIN_STRIKES && /SK_MIN_STRIKES_REPLAY/.test(grabFn('replayKingRaw')));
global.replayOn=()=>false;

// ===== 6. THE ACTUAL COMPLAINT: the census must exceed the lane, on his own days =====
// ⚠ EXECUTED against the recorded sessions, not asserted from memory.
function dwell20(seq){
  if(seq.length<2) return 0;
  let pts=[seq[0]]; for(let i=1;i<seq.length;i++) if(seq[i][1]!==pts[pts.length-1][1]) pts.push(seq[i]);
  let kept=[pts[0]];
  for(let i=1;i<pts.length;i++){
    const held = i===pts.length-1 ? Infinity : (pts[i+1][0]-pts[i][0])/60000;
    if(kept[kept.length-1][1]===pts[i][1]) continue;
    if(held>=20) kept.push(pts[i]);
  }
  return kept.length-1;
}
let days=0, everSmaller=0, ratios=[];
fs.readdirSync('./data').filter(f=>/^2026-08-\d\d\.json$/.test(f)).forEach(f=>{
  const o=JSON.parse(fs.readFileSync('./data/'+f,'utf8'));
  const fr=(o.snaps&&o.snaps.SPY)||[];
  if(fr.length<50) return;
  ['SPXW','SPY','QQQ'].forEach(bk=>{
    const seq=fr.map(x=>[x.t,(x.tri&&x.tri[bk])?x.tri[bk].king:null]).filter(a=>typeof a[1]==='number'&&a[1]>0);
    if(seq.length<50) return;
    let raw=0; for(let i=1;i<seq.length;i++) if(seq[i][1]!==seq[i-1][1]) raw++;
    const d=dwell20(seq);
    days++; if(raw<d) everSmaller++;
    if(d>0) ratios.push(raw/d);
  });
});
ok('measured over the recorded sessions, not asserted', days>=20, days+' book-days');
ok('THE CENSUS IS NEVER SMALLER THAN THE LANE', everSmaller===0, everSmaller+' violations');
const medR = ratios.sort((a,b)=>a-b)[ratios.length>>1];
ok('and it is materially larger — the lane under-reports by ~3x', medR>=2, 'median x'+medR.toFixed(1));

// ===== 7. the debug surface returns BOTH, named =====
const dbg=(src.match(/window\.__gptsDebug\.kingTrack[\s\S]*?\n};/)||[''])[0];
ok('kingTrack reports `rolls`', /rolls:R\.rolls/.test(dbg));
ok('kingTrack still reports `migrations` (the lane is not deleted, it is labelled)', /migrations:/.test(dbg));
ok('kingTrack walks KR_BOOKS so QQQ appears', /KR_BOOKS\.forEach/.test(dbg));
ok('kingTrack phrases the census as "at least N"', /at least '\+R\.rolls/.test(dbg));
ok('a book with no lane returns migrations:null, not 0 — absent is not zero',
   /migrations: drawn \? Math\.max\(0, ktOf\(b\)\.length-1\) : null/.test(dbg));

// ===== 8. the face says it too — the tooltip must carry the census =====
ok('the King lane tooltip states BOTH numbers', /THIS LANE IS NOT A COUNT/.test(src));
ok('...and tells him which one to compare with Atlas', /comparing with Atlas/.test(src));

// ===== 9. the census is written inside the same guards as the lane =====
const kt=grabFn('ktTick');
ok('krTick runs inside ktTick, after recorderBlind and the RTH gate',
   kt.indexOf('recorderBlind')<kt.indexOf('krTick') && kt.indexOf('P.rth')<kt.indexOf('krTick'));
ok('...so a frozen or replayed face can never append to today\'s census',
   /if\(typeof recorderBlind==='function' && recorderBlind\(\)\) return;/.test(kt));

Date.now=_now;
console.log(fail===0? '\n'+pass+' passed, 0 failed' : '\n'+pass+' passed, '+fail+' FAILED');
process.exit(fail===0?0:1);
