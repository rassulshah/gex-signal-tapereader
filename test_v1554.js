// ============================================================================================
// test_v1554.js — (v15.54) THE WORKFLOW BUILD — design/ARCHITECTURE-E2E-WORKFLOW.md
//   7 THE GATE · 8 ONE REGISTER · 10 defl in the export · 12 the nightly (its own self-test) ·
//   13 read-back schema 2 · ANALYSIS episodes + Wilson + the ledger · HOD/LOD calibration · the memo.
// Every function is EXTRACTED AND RUN. GPTS_SRC points the harness at a mutated copy.
// ============================================================================================
const fs=require('fs'), cp=require('child_process');
const SRC=process.env.GPTS_SRC||'./current/gex-signal-tapereader.user.js';
const src=fs.readFileSync(SRC,'utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,240):''));} };
function ex(n){ const m=new RegExp('function\\s+'+n+'\\s*\\(','g').exec(src);
  if(!m) throw new Error('not found: '+n);
  let i=src.indexOf('{',m.index),d=0; for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){ d--; if(!d) return src.slice(m.index,k+1); } } }
function exVar(n){ const m=new RegExp('\\nvar '+n+'=').exec(src); if(!m) throw new Error('var not found: '+n);
  let i=src.indexOf('[',m.index),d=0; for(let k=i;k<src.length;k++){ if(src[k]==='[')d++; else if(src[k]===']'){ d--; if(!d) return 'var '+n+'='+src.slice(i,k+1)+';'; } } }
const build=(g,fns,tail)=>new Function('__g', Object.keys(g).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+fns.map(ex).join('\n')+'\n'+(tail||''));
const code=src.split('\n').filter(l=>!l.trim().startsWith('//')).join('\n');

// ---- 7 · THE GATE -------------------------------------------------------------------------------
{
  const mk=(lo,hi)=>{ const g={}; g.featStatsCached=()=>({ byKey:{ lodhod:{ band:{lo:lo,hi:hi} } } }); return build(g,['featGated'],'return featGated;')(g); };
  let f=mk({n:40,hit:40},{n:120,hit:120});
  ok(f('lodhod').gated===true && /CANNOT DISCRIMINATE/.test(f('lodhod').why),'7a 100% in both bands -> GATED (this is F-11)',f('lodhod'));
  f=mk({n:40,hit:28},{n:40,hit:12});
  ok(f('lodhod').gated===true && /INVERTED/.test(f('lodhod').why),'7b the high band scoring below the low band -> GATED, inverted');
  f=mk({n:40,hit:12},{n:40,hit:28});
  ok(f('lodhod').gated===false,'7c 30% vs 70% -> not gated');
  f=mk({n:10,hit:10},{n:10,hit:10});
  ok(f('lodhod').gated===false && f('lodhod').thin===true,'7d thin bands are NOT judged — a feature is gated only on 30+ per band');
  // ruleLocalRate: a gated feature has NO rate, and ruleTier cannot show 📊
  const g={ FEAT_FWD:10, RULE_UNLOCK_N:20, FEATURES:[{ key:'lodhod', rule:{ id:'lodhod' } }] };
  const dates={}; for(let i=0;i<25;i++) dates['d'+i]=1;   // 25 sessions: PAST the unlock n, so only the gate can say ⚖
  g.featStatsCached=()=>({ byKey:{ lodhod:{ n:362, hit:362, toClose:true, dates:dates, band:{lo:{n:40,hit:40},hi:{n:120,hit:120}} } }, byGrade:{dir:{},node:{}}, cells:{} });
  g.deflStats=()=>null; g.registerCoreFeatures=()=>{}; g.rulePromotedApplied=()=>true;
  const rlr=build(g,['effN','nTxt','featGated','ruleFeatKey','ruleLocalRate','ruleTier','pctN'],'return { lr:ruleLocalRate("lodhod"), tier:ruleTier("lodhod"), txt:pctN(null,362,true) };')(g);
  ok(rlr.lr.gated===true && rlr.lr.rate===null,'7e ruleLocalRate: a gated feature returns rate:null with gated:true',rlr.lr);
  ok(rlr.tier==='⚖' && rlr.lr.effN>=20,'7f ruleTier: a gated feature cannot be 📊 even when promoted AND past the unlock n (effN '+rlr.lr.effN+')',rlr.tier);
  ok(/gated/.test(rlr.txt) && !/%/.test(rlr.txt),'7g pctN prints "gated", never a percentage',rlr.txt);
  const rfk=build({},['ruleFeatKey'],'return ruleFeatKey;')({});
  ok(rfk('node.tap.1')==='node' && rfk('dir.A')==='dir' && rfk('kill.tap3')==='node' && rfk('lodhod')==='lodhod','7h a sub-rule is gated with its feature');
}

// ---- 8 · ONE REGISTER --------------------------------------------------------------------------
{
  const reg=JSON.parse(fs.readFileSync('learning/register.json','utf8'));
  const seed=new Function(exVar('PREREG_SEED')+' return PREREG_SEED;')();
  ok(reg.schema===1 && Array.isArray(reg.hypotheses) && reg.hypotheses.length===seed.length,'8a learning/register.json exists and has the seed\'s count',reg.hypotheses.length);
  const drift=seed.filter((h,i)=>!reg.hypotheses[i] || reg.hypotheses[i].id!==h.id || reg.hypotheses[i].minN!==h.minN || reg.hypotheses[i].pick!==h.pick || !!reg.hypotheses[i].gap!==!!h.gap || !!reg.hypotheses[i].blocked!==!!h.blocked);
  ok(drift.length===0,'8b the panel\'s seed and the file agree on id / minN / pick / gap / blocked — one register, not two',drift.map(h=>h.id));
  ok(reg.hypotheses.every(h=>h.written && h.predict && h.refuteIf),'8c every hypothesis carries a written date, a prediction and what refutes it');
  // preregList: file wins when present, seed otherwise; picks resolve to functions
  const store={}; const g={ PREREG_PICK:null, PREREG_SEED:null };
  const code2=exVar('PREREG_SEED')+'\n'+'var PREREG_PICK={ gradeA:function(s){ return s.gradeA; }, tap1:function(s){ return s.tap1; }, pol:function(s){ return s.pol; }, wick:function(s){ return s.wick; }, defl:function(s){ return s.defl; }, sweepNode:function(){ return {n:0}; }, sweepEarly:function(){ return {n:0}; } };\n'+
    "var REGISTER_KEY='gpts_register_v1'; var localStorage={ getItem:function(k){ return store[k]||null; }, setItem:function(k,v){ store[k]=v; } };\n"+ex('preregList')+'\nreturn preregList;';
  const preregList=new Function('store',code2)(store);
  let l=preregList();
  ok(l.length===9 && typeof l[0].pickFn==='function' && l[0].pickFn({gradeA:{n:3}}).n===3 && l.every(H=>typeof H.pickFn==='function'),'8d with nothing fetched the seed is used and picks resolve to functions (v15.55: 7 rows, H6/H7 added; v15.72: 9 rows, H8/H9 the rolling floor / ceiling)');
  store['gpts_register_v1']=JSON.stringify({ schema:1, hypotheses:[{ id:'H9', claim:'x', pick:'tap1', minN:5 }] });
  l=preregList();
  ok(l.length===1 && l[0].id==='H9','8e once the file is fetched, THE FILE is the register');
}

// ---- 10 · the export carries the event ledger --------------------------------------------------
{
  ok(/\n\s+defl:day\.defl\|\|\{\},/.test(code),'10a buildDayExport writes `defl` — what H5 is scored on');
  ok(!/projReview:\(function/.test(code),'10b ...and still does not write projReview');
}

// ---- 12 · THE NIGHTLY, one command, with its own self-test --------------------------------------
{
  const r=cp.spawnSync('python3',['tools/nightly/run.py','--selftest'],{encoding:'utf8'});
  ok(r.status===0 && /planted effect found: True/.test(r.stdout) && /refused\/thin: True/.test(r.stdout),'12a run.py --selftest: finds a planted effect AND refuses a planted nothing',(r.stdout||'').split('\n').slice(-2).join(' | '));
  ok(fs.existsSync('learning/log') && fs.readdirSync('learning/log').some(f=>/^2026-\d\d-\d\d\.json$/.test(f)),'12b the nightly writes learning/log/<day>.json');
  const logs=fs.readdirSync('learning/log').filter(f=>/^2026-\d\d-\d\d\.json$/.test(f)).sort();
  const last=JSON.parse(fs.readFileSync('learning/log/'+logs[logs.length-1],'utf8'));
  ok(last.schema===2 && Array.isArray(last.hypotheses) && typeof last.preopen==='string','12c ...in schema 2: verdicts + a pre-open line',Object.keys(last));
  ok(last.hypotheses.every(h=>['thin','cleared','refused','blocked','ready'].indexOf(h.verdict)>=0),'12d every verdict is one of thin/cleared/refused/blocked/ready');
  ok(last.hypotheses.filter(h=>h.verdict==='thin').every(h=>h.rate===undefined),'12e a THIN hypothesis has NO rate printed — read once, at minimum n');
}

// ---- 13 · read-back ----------------------------------------------------------------------------
{
  ok(/N\.hypotheses/.test(code) && /VERDICTS/.test(src) && /N\.lodhod && N\.lodhod\.cells/.test(code),'13a Analysis REVIEW renders schema-2 verdicts and the HOD/LOD calibration');
  const prl=build({},['pipeReviewLine'],'return pipeReviewLine;')({});
  ok(prl({preopen:'3 sessions · 41 episodes'})==='3 sessions · 41 episodes','13b the footer line prefers the schema-2 preopen');
  ok(prl({headline:'old shape'})==='old shape','13c ...and still reads the old shape');
}

// ---- ANALYSIS · episodes, Wilson, the ledger ----------------------------------------------------
{
  const g={ FEAT_ARCHIVE:{} };
  const recs=[
    { key:'node', t:3, resolved:true, hit:0, rec:{ k:765, grade:'A', tap:1, pol:'+' } },   // same episode, LATER bar
    { key:'node', t:1, resolved:true, hit:1, rec:{ k:765, grade:'A', tap:0, pol:'+' } },   // the FIRST bar — the one that counts
    { key:'node', t:1, resolved:true, hit:0, rec:{ k:770, grade:'C', tap:0, pol:'-' } },
  ];
  g.recorderLoad=()=>({ days:{ '2026-09-03':{ feat:{ SPY:recs } } } });
  const ds=build(g,['deflStats'],'return deflStats("SPY");')(g);
  ok(ds.n===2,'A1 three rows on two strikes are TWO episodes',ds.n);
  ok(ds.grade.A && ds.grade.A.n===1 && ds.grade.A.hit===1,'A2 the FIRST bar of an episode is the one counted (t=1 wins over t=3, whatever the stored order)',ds.grade);
  const w=build({},['wilsonCI'],'return wilsonCI;')({});
  ok(JSON.stringify(w(49,94))==='[42,62]','A3 Wilson: 49/94 -> [42,62] (the base-rate interval from FINDINGS-Q11)',w(49,94));
  ok(w(0,0)===null,'A4 ...and null on n=0');
  const tbl=ex('deflTableHtml');
  ok(/wilsonCI\(b\.hit,b\.n\)/.test(tbl) && /en>=15/.test(tbl) && !/effN\(b\.n\)/.test(tbl),'A5 the tables read at 15 EPISODES with an interval, not at n/FEAT_FWD');
  ok(/chance alone predicts/.test(ex('deflectionsSectionHtml')) && /nothing here is a finding/.test(ex('deflectionsSectionHtml')),'A6 the multiple-comparison ledger is printed beside the tables');
}

// ---- HOD/LOD calibration -------------------------------------------------------------------------
{
  const g={ FEAT_ARCHIVE:{} };
  g.recorderLoad=()=>({ days:{ '2026-09-04':{ feat:{ SPY:[
    { key:'lodhod', atClose:true, hit:1, rec:{ p:85 } }, { key:'lodhod', atClose:true, hit:0, rec:{ p:88 } },
    { key:'lodhod', hit:1, rec:{ p:85 } },                                   // proxy-scored: VOID, not counted
    { key:'lodhod', atClose:true, hit:1, rec:{ p:12 } } ] } } } });
  const c=build(g,['hodlodCalib'],'return hodlodCalib("SPY");')(g);
  ok(c.sessions===1 && c.cells['80–99'] && c.cells['80–99'].n===2 && c.cells['80–99'].hit===1,'H1 close-scored rows group by the table\'s own cell; the proxy-scored row is excluded',c);
  ok(c.cells['0–19'] && c.cells['0–19'].n===1,'H2 ...every cell is kept, including the one that predicts failure');
}

// ---- the memo ----------------------------------------------------------------------------------
{
  const g={ RMEMO:{ id:0, m:{} }, window:{ __gptsDebug:{} } };
  const r=build(g,['rmemo','rmemoNext'],'var calls=0; var f=function(){ return rmemo("k", function(){ calls++; return {calls:calls}; }); }; var a=f(), b=f(); rmemoNext(); var c=f(); return {a:a.calls,b:b.calls,c:c.calls,same:a===b};')(g);
  ok(r.a===1 && r.b===1 && r.same===true,'M1 within a frame the second call returns the SAME object without recomputing',r);
  ok(r.c===2,'M2 a new frame recomputes once',r);
  ['ifLadder','emBand','sessionBody','measureBars'].forEach(n=>ok(new RegExp('function '+n+'\\(sym\\)\\{ return rmemo\\(').test(src) && new RegExp('function '+n+'Raw\\(sym\\)').test(src),'M3 '+n+' is memoised per frame over its Raw body'));
  ok(/function render\(\)\{\n  rmemoNext\(\);/.test(src) && /function tick\(\)\{\n  rmemoNext\(\);/.test(src),'M4 render() and tick() each open a new frame');
}

// ---- the tabs, in workflow order ---------------------------------------------------------------
{
  const ab=ex('analysisBlock'), tb=ex('testingBlock');
  ok(/h\+=analysisSubjectsHtml\(sym, _live\);/.test(ab) && /_live\.H1=hodlodSectionHtml\(sym\)/.test(ab) && /_live\.F5=nl; _live\.D2=df;/.test(ab),'T1 (v15.55) Analysis emits the subject panel with the live evidence mapped to H1 / F1 / F5 / D2');
  ok(!/tabTile\('Direction'/.test(ab) && !/tabSection\('a8'/.test(ab),'T2 the headline tiles and the scorecard section are gone from Analysis');
  ok(/h\+=T_loop\+T_prereg\+T_canfail\+T_dash\+T_cov\+T_nightly\+T_self\+T_pat\+T_detail;/.test(tb),'T3 (v15.55) Testing emits the loop strip · ① REGISTER · ② GATE · ③ DASHBOARD · ④ RECORD · ⑤ NIGHTLY · ⑥ SELF-TEST · ⑦ PATTERNS (v15.67) · ⊕');
  ok(!/questionQueueHtml\(/.test(code),'T4 the question queue is archived — the register holds the questions now');
  ok(/featureScorecardsHtml\('SPY'\)/.test(tb),'T5 the feature scorecards live in Testing ⊕');
  ok(/GATED/.test(ex('canFailHtml')),'T6 ⑤b says GATED where the gate bites');
}

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
