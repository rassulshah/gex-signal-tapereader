// ============================================================================================
// test_v1552.js — (v15.52) THE GRADER BECOMES VISIBLE.
//   featPredBand · featStats (void / hitNull / band / dates) · effN=sessions for toClose ·
//   canFailHtml (the check that would have caught F-11) · preregStats/preregHtml (read-once).
// Every function is EXTRACTED AND RUN. GPTS_SRC points the harness at a mutated copy.
// ============================================================================================
const fs=require('fs');
const SRC=process.env.GPTS_SRC||'./current/gex-signal-tapereader.user.js';
const src=fs.readFileSync(SRC,'utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,240):''));} };
function ex(n){ const m=new RegExp('function\\s+'+n+'\\s*\\(','g').exec(src);
  if(!m) throw new Error('not found: '+n);
  let i=src.indexOf('{',m.index),d=0; for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){ d--; if(!d) return src.slice(m.index,k+1); } } }
function exVar(n){ const m=new RegExp('\\nvar '+n+'=').exec(src); if(!m) throw new Error('var not found: '+n);
  let i=src.indexOf('[',m.index),d=0; for(let k=i;k<src.length;k++){ if(src[k]==='[')d++; else if(src[k]===']'){ d--; if(!d) return 'var '+n+'='+src.slice(i,k+1)+';'; } } }
const build=(g,fns,tail)=>{ const code=Object.keys(g).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+fns.map(ex).join('\n')+'\n'+(tail||''); return new Function('__g',code); };

// ---- 1 · featPredBand ------------------------------------------------------------------------
{
  const f=build({},['featPredBand'],'return featPredBand;')({});
  ok(f({p:20})==='lo' && f({p:50})==='hi' && f({p:85})==='hi','1a a table probability splits at 50');
  ok(f({grade:'A'})==='hi' && f({grade:'C'})==='lo' && f({grade:'B'})===null,'1b grade A/C are the bands, B is excluded');
  ok(f({gradeAtFire:'A'})==='hi' && f({quality:'weak'})==='lo' && f({quality:'confirmed'})==='hi','1c fire-grade and wick quality are predictions too');
  ok(f({})===null && f(null)===null,'1d nothing predicted -> null, never a guess');
}

// ---- 2 · featStats: void / hitNull / band / dates ---------------------------------------------
{
  const mk=(recs)=>{
    const g={ FEAT_ARCHIVE:{} };
    g.recorderLoad=()=>({ days:{ '2026-09-01':{ feat:{ SPY:recs.filter(r=>r.d==='2026-09-01') } }, '2026-09-03':{ feat:{ SPY:recs.filter(r=>r.d==='2026-09-03') } } } });
    g.featureByKey=(k)=>(k==='lodhod')?{ key:'lodhod', toClose:true }:{ key:k };
    g.FEAT_FWD=10;
    return build(g,['featPredBand','featStats'],'return featStats("SPY");')(g);
  };
  const R=(d,key,extra)=>Object.assign({ d:d, key:key, resolved:true, hit:1, rec:{}, t:1 },extra||{});
  const st=mk([
    R('2026-09-01','lodhod',{ rec:{p:10} }),                         // proxy-scored: no atClose -> VOID
    R('2026-09-01','lodhod',{ rec:{p:90}, hit:0 }),                  // void too
    R('2026-09-03','lodhod',{ rec:{p:90}, atClose:true }),           // close-scored: counts
    R('2026-09-03','node',{ rec:{grade:'A'}, hit:1 }),
    R('2026-09-03','node',{ rec:{grade:'C'}, hit:0 }),
    R('2026-09-03','node',{ rec:{grade:'B'}, hit:1 }),
    R('2026-09-03','node',{ hit:null, hitNull:'threw' }),
    R('2026-09-03','node',{ hit:null, hitNull:'declined' }),
  ]);
  const L=st.byKey.lodhod, N=st.byKey.node;
  ok(L.toClose===true && L.void===2 && L.n===1 && L.hit===1,'2a lodhod: two proxy-scored rows are VOID, the close-scored row counts',L);
  ok(Object.keys(L.dates).length===2,'2b ...and dates are tracked per key (the honest n for a to-close feature)',L.dates);
  ok(N.n===3 && N.hit===2 && N.pending===2,'2c node: three scored, two pending',N);
  ok(N.hitNull.threw===1 && N.hitNull.declined===1,'2d hitNull tallies threw and declined separately',N.hitNull);
  ok(N.band.hi.n===1 && N.band.hi.hit===1 && N.band.lo.n===1 && N.band.lo.hit===0,'2e band: A -> hi (1/1), C -> lo (0/1), B excluded',N.band);
}

// ---- 3 · effN = sessions for a to-close feature ---------------------------------------------
{
  const mk=(bucket)=>{
    const g={ FEAT_FWD:10, RULE_UNLOCK_N:20, FEATURES:[{ key:'lodhod', rule:{ id:'lodhod' } }] };
    g.featStatsCached=()=>({ byKey:{ lodhod:bucket }, byGrade:{dir:{},node:{}}, cells:{} });
    g.deflStats=()=>null; g.registerCoreFeatures=()=>{};
    return build(g,['effN','ruleLocalRate'],'return ruleLocalRate("lodhod");')(g);
  };
  const dates={}; for(let i=0;i<4;i++) dates['2026-09-0'+(i+3)]=1;
  let r=mk({ n:362, hit:200, toClose:true, dates:dates });
  ok(r.effN===4,'3a a to-close feature with 362 rows over 4 sessions has eff n = 4, not 36',r);
  r=mk({ n:362, hit:200, toClose:false, dates:dates });
  ok(r.effN===36,'3b a 10-bar feature keeps n/FEAT_FWD (unchanged)',r);
}

// ---- 4 · canFailHtml — the F-11 check ----------------------------------------------------------
{
  const run=(byKey)=>{
    const g={ PAL:{ sub:'#888' }, FEAT_FWD:10 };
    g.featStatsCached=()=>({ byKey:byKey });
    g.g3esc=(s)=>String(s);
    return build(g,['effN','canFailHtml'],'return canFailHtml();')(g);
  };
  const B=(lo,hi,extra)=>Object.assign({ n:lo.n+hi.n, hit:lo.hit+hi.hit, band:{lo:lo,hi:hi}, dates:{a:1,b:1}, hitNull:{} },extra||{});
  let h=run({ tautology:B({n:40,hit:40},{n:120,hit:120}) });
  ok(/GATED/.test(h) && /cannot discriminate/.test(h),'4a 100% in the low band and 100% in the high band -> GATED · cannot discriminate (this is F-11; a gate since v15.54)');
  h=run({ good:B({n:40,hit:12},{n:40,hit:28}) });
  ok(/discriminates/.test(h) && !/CANNOT/.test(h),'4b 30% vs 70% -> discriminates');
  h=run({ inv:B({n:40,hit:28},{n:40,hit:12}) });
  ok(/GATED/.test(h) && /inverted/.test(h),'4c 70% low vs 30% high -> GATED · inverted');
  h=run({ thin:B({n:10,hit:5},{n:10,hit:9}) });
  ok(/thin/.test(h) && !/discriminates|CANNOT/.test(h),'4d under 30 per band -> thin, no verdict');
  h=run({ lodhod:{ n:0, hit:0, toClose:true, void:362, band:{lo:{n:0,hit:0},hi:{n:0,hit:0}}, dates:{}, hitNull:{} } });
  ok(/restarted at the close/.test(h) && /362 proxy rows void/.test(h),'4e a to-close feature with only void rows says so, with the count');
  h=run({ a:B({n:40,hit:40},{n:120,hit:120}), b:B({n:40,hit:12},{n:40,hit:28}) });
  ok(h.indexOf('GATED')<h.indexOf('discriminates'),'4f flagged rows sort first');
}

// ---- 5 · preregStats / preregHtml — read once, sessions from PREREG_FROM only ----------------
{
  const mk=(recs, deflN)=>{
    const g={ FEAT_ARCHIVE:{}, DEFL_ARCH_N:deflN||0, PAL:{sub:'#888'} };
    const days={}; recs.forEach(r=>{ (days[r.d]=days[r.d]||{feat:{SPY:[]}}).feat.SPY.push(r); });
    g.recorderLoad=()=>({ days:days });
    g.g3esc=(s)=>String(s); g.g3tip=(t)=>' title="'+String(t).replace(/"/g,'&quot;')+'"';
    const code=Object.keys(g).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\nvar PREREG_FROM="2026-09-03";\n'+exVar('PREREG_SEED')+'\nvar PREREG_PICK={ gradeA:function(s){ return s.gradeA; }, tap1:function(s){ return s.tap1; }, pol:function(s){ return s.pol; }, wick:function(s){ return s.wick; }, defl:function(s){ return s.defl; }, sweepNode:function(){ return {n:0,hit:0}; }, sweepEarly:function(){ return {n:0,hit:0}; } };\nvar HYP_STUDY={ H1:"F5.2", H2:"F2.1", H3:"F6.1", H4:"F1.4", H5:"H1.3", H6:"H2.7", H7:"H2.8" };\nvar REGISTER_KEY="x"; var localStorage={ getItem:function(){ return null; } };\n'+ex('preregList')+'\n'+ex('preregStats')+'\n'+ex('preregHtml')+'\nreturn { s:preregStats("SPY"), h:preregHtml() };';
    return (new Function('__g',code))(g);
  };
  const R=(d,key,t,rec,hit)=>({ d:d, key:key, t:t, resolved:true, hit:hit, rec:rec });
  // one node episode recorded on 3 bars (t=1,2,3) — must count ONCE, from its FIRST bar
  const recs=[
    R('2026-09-01','node',1,{k:765,grade:'A',tap:1,pol:'+'},0),      // before PREREG_FROM: ignored
    R('2026-09-03','node',1,{k:765,grade:'A',tap:0,pol:'+'},1),
    R('2026-09-03','node',2,{k:765,grade:'A',tap:1,pol:'+'},0),      // same episode, later bar: ignored
    R('2026-09-03','node',3,{k:765,grade:'A',tap:1,pol:'+'},0),
    R('2026-09-03','node',1,{k:770,grade:'C',tap:1,pol:'-'},0),
    R('2026-09-03','reaction',1,{k:765,quality:'confirmed'},1),
    R('2026-09-03','reaction',1,{k:770,quality:'weak'},0),
  ];
  const o=mk(recs, 12);
  ok(o.s.gradeA.n===1 && o.s.gradeA.hit===1,'5a H1: one grade-A EPISODE from 2026-09-03 (three bars, counted once, first bar wins; the 09-01 row ignored)',o.s.gradeA);
  ok(o.s.tap1.n===1 && o.s.tap1.hit===0,'5b H2: the 770 episode is tap 1; the 765 episode\'s FIRST bar was tap 0 so it is not',o.s.tap1);
  ok(o.s.pol.a.n===1 && o.s.pol.b.n===1,'5c H3: one +g and one -g episode',o.s.pol);
  ok(o.s.wick.a.n===1 && o.s.wick.b.n===1,'5d H4: one confirmed, one weak',o.s.wick);
  ok(o.s.defl.n===12,'5e H5 reads the defl store count',o.s.defl);
  ok((/thin — not read/.test(o.h) || (/>THIN</.test(o.h) && /not read/.test(o.h))) && !/held \d+%/.test(o.h.split('H1')[1]||''),'5f under the minimum n the RATE IS NOT SHOWN — read once, at threshold');
  ok(!/READ · held/.test(o.h) && !/READ · held/.test(o.h),'5g ...and no "READ" appears anywhere on thin data');
  ok(/BLOCKED/.test(o.h) && /12 of 50/.test(o.h),'5h H5 says BLOCKED with the ledger count over its minimum');
  // at threshold the rate appears
  const many=[]; for(let i=0;i<45;i++) many.push(R('2026-09-0'+(3+(i%6)),'node',1,{k:700+i,grade:'A',tap:0,pol:'+'},(i%3)?1:0));
  const o2=mk(many,0);
  ok(o2.s.gradeA.n===45 && (/READ · held 67%/.test(o2.h) || (/>READ</.test(o2.h) && /held 67% \(n=45\)/.test(o2.h))),'5i at n>=40 H1 is READ and the rate appears (30/45 = 67%)',o2.h.match(/H1.*?<\/tr>/s)?.[0].slice(-120));
}

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
