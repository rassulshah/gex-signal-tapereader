// ============================================================================================
// test_v1572.js — (v15.72) THE FACE, HIS THREE ASKS + ONE BUG + ONE READ. Operator, 2026-09-04:
//   "the after hours message to the left is bad choice, it is taking up too much space. you can put it on the bottom of
//   the application, this will give you much more space for the king badges which you can make bigger. as well as the
//   size of the font in the node ladder." · "there is yellow in the rectangle right before the purple" · "when the king
//   rolls up and is below price it may be creating a floor (support) and be bullish and vice versa" · "yes .. build"
// ============================================================================================
const fs=require('fs'), cp=require('child_process'), path=require('path'), os=require('os');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,400):''));} };
const src=fs.readFileSync(process.env.GPTS_SRC||'./current/gex-signal-tapereader.user.js','utf8');
function ex(n){ const m=new RegExp('function\\s+'+n+'\\s*\\(','g').exec(src); if(!m) throw new Error('not found: '+n);
  let i=src.indexOf('{',m.index), d=0, j=i; for(;j<src.length;j++){ const ch=src[j]; if(ch==='{')d++; else if(ch==='}'){ d--; if(d===0) break; } } return src.slice(m.index,j+1); }
const exVar=(n)=>{ const i=src.indexOf('var '+n+'='); if(i<0) throw new Error('no var '+n); const j=src.indexOf(';\n', i); return src.slice(i, j+1); };
const build=(g,fns,tail)=>new Function('__g', Object.keys(g).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\nvar PAT_GROW_PCT=20;\n'+exVar('PAT_CLASSES')+'\n'+fns.map(ex).join('\n')+'\n'+(tail||''));
const esc=(s)=>String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;');

ok(/@version\s+15\.(7[2-9]|[89]\d)/.test(src) && /var GPTS_VERSION='15\.(7[2-9]|[89]\d)';/.test(src),'0a v15.72 or later in both spots');

// ---- 1 · the leaked border ---------------------------------------------------------------------------------------------------------
{
  const rules=(src.match(/#gpts-body \.g3pb\{[^}]*\}/g)||[]);
  ok(rules.length===1,'1a exactly ONE .g3pb rule in the panel (the dead pullback rule with the amber left border is gone)',rules);
  ok(/#gpts-body \.g3pb\{display:block;font-size:7\.5px;[^}]*color:#0b0e14;border-left:0\}/.test(src),'1b the pattern block states its own border: none',rules[0]);
  ok(!/border-left:2px solid #f2b45a\}/.test(src.slice(src.indexOf('#gpts-body .g3node{')-300, src.indexOf('#gpts-body .g3node{')+300)),'1c the amber left-border rule no longer sits beside .g3node');
}

// ---- 2 · the King cards: the whole row, bigger ------------------------------------------------------------------------------------------
{
  ok(/#gpts-body \.g3kz\{display:grid;grid-template-columns:1fr 1fr 1fr;gap:5px;margin:4px 0 3px;flex:1 1 100%;width:100%\}/.test(src),'2a .g3kz spans its flex row (it sat at 462 of 649 px beside the chip)');
  ok(/#gpts-body \.g3kx\{font-size:16\.5px;/.test(src) && /#gpts-body \.g3bk\{font-size:8\.6px;/.test(src) && /#gpts-body \.g3kl\{display:grid;grid-template-columns:50px 1fr;gap:4px;font-size:8\.6px;/.test(src),'2b the price 16.5 px, the KING titles 8.6, the GROWTH / ROLLED lines 8.6');
  ok(/#gpts-body \.g3pos\{margin-left:auto;font-size:7\.4px;/.test(src) && /#gpts-body \.g3rolled\{font-size:7\.4px;/.test(src) && /#gpts-body \.g3kx small\{font-size:8\.6px;/.test(src) && /#gpts-body \.g3kl em\{font-style:normal;font-size:7\.4px;/.test(src),'2c the ABOVE / BELOW and ROLLED pills 7.4, the strike 8.6, the labels 7.4');
  ok(/#gpts-body \.g3kc\{border:1px solid #1e2530;border-radius:6px;background:#12161f;padding:6px 8px 5px;min-width:0\}/.test(src),'2d the card padding grew with it');
}

// ---- 3 · the ladder: the font up, the columns to match ----------------------------------------------------------------------------------
{
  ok(/#gpts-body \.g3gr\{display:grid;grid-template-columns:56px 68px 118px 66px 94px 68px 70px 70px 70px;gap:5px;align-items:center;padding:0 6px;min-height:22px;[^}]*font-size:9px\}/.test(src),'3a rows 9 px, nine columns widened ~15%, min-height 22');
  ok(/#gpts-body \.g3gr\.hd\{font-size:7\.3px;/.test(src) && /#gpts-body \.g3gr\.lvl\{min-height:15px;font-size:8px\}/.test(src) && /#gpts-body \.g3gc\.lv\{font-size:7\.8px;/.test(src),'3b the header 7.3, the level rows 8, the level labels 7.8');
  ok(/#gpts-body \.g3kchip\{[^}]*height:17px;[^}]*font-size:8\.4px;/.test(src) && /#gpts-body \.g3bar\{[^}]*height:13px;[^}]*font-size:8\.4px;/.test(src) && /#gpts-body \.g3chip\{display:inline-block;font-size:7\.5px;/.test(src) && /#gpts-body \.g3now\{[^}]*font-size:10\.5px;/.test(src) && /#gpts-body \.g3gg\{font-size:9px;/.test(src),'3c the King chips, the % bars, the NEW / roll chips, NOW and the growth cells scaled with the row');
  ok(/#gpts-body \.g3pr\{display:block;font-style:normal;font-weight:800;font-size:8\.3px;color:#e3c341;/.test(src) && /#gpts-body \.g3stk\{display:inline-block;font-size:10\.5px;/.test(src) && /#gpts-body \.g3zt\{[^}]*font-size:7\.3px;/.test(src) && /#gpts-body \.g3px small\{[^}]*font-size:7\.8px;/.test(src),'3d the pattern strikes, the stack bracket, the zone tag and the small strike, likewise');
  ok(/Math\.round\(pctv\/100\*112\)/.test(src) && !/Math\.round\(pctv\/100\*98\)/.test(src),'3e the node bar scales to the 118 px NODE column (98 → 112)');
}

// ---- 4 · the AFTER HOURS chip, at the bottom ------------------------------------------------------------------------------------------------
{
  const fr=ex('secFrame');
  ok(!/class="g3ahchip"/.test(fr) && /the AFTER HOURS chip left this row for the bottom of the panel/.test(fr) && (src.match(/class="g3ahchip"/g)||[]).length===1,'4a the King row (secFrame) no longer emits the chip; exactly one emitter remains (afterHoursChipHtml)');
  const f=ex('afterHoursChipHtml');
  ok(/var SP=sessionPhase\(\); var AH=!!\(SP && !SP\.rth && SP\.mins>=SP\.close\);/.test(f) && /if\(!AH\) return '';/.test(f) && /AFTER HOURS · EM EXPIRED — re-anchors at the open/.test(f),'4b afterHoursChipHtml: the same predicate as the branch that retires the band; empty inside RTH');
  const g={ sessionPhase:()=>({ rth:false, mins:15*60+7, close:15*60 }), g3tip:(t)=>' title="'+esc(t)+'"' };
  const h=build(g,['afterHoursChipHtml'],'return afterHoursChipHtml();')(g);
  ok(/^<div class="g3ahchip" title="[^"]*expired at the close[^"]*">AFTER HOURS · EM EXPIRED — re-anchors at the open<\/div>$/.test(h),'4c after the close: the bar with its hover',h.slice(0,120));
  const g2={ sessionPhase:()=>({ rth:true, mins:10*60, close:15*60 }), g3tip:(t)=>'' };
  ok(build(g2,['afterHoursChipHtml'],'return afterHoursChipHtml();')(g2)==='','4d inside RTH: nothing');
  const g3={ sessionPhase:()=>({ rth:false, mins:7*60, close:15*60 }), g3tip:(t)=>'' };
  ok(build(g3,['afterHoursChipHtml'],'return afterHoursChipHtml();')(g3)==='','4e pre-market: nothing (the EM is not expired, it is not yet priced)');
  const g4={ sessionPhase:()=>{ throw new Error('x'); }, g3tip:(t)=>'' };
  ok(build(g4,['afterHoursChipHtml'],'return afterHoursChipHtml();')(g4)==='','4f a throwing clock: nothing, never a crash');
  const rd=ex('render');
  const iRp=rd.lastIndexOf("replayBarHtml('strip')"), iAh=rd.lastIndexOf('afterHoursChipHtml()'), iFt=rd.lastIndexOf('feedStatusHtml()');
  ok(iRp>0 && iAh>iRp && iFt>iAh,'4g render(): the replay strip, then the after-hours bar, then the footer — the bottom of the application',[iRp,iAh,iFt]);
  ok((rd.match(/afterHoursChipHtml\(\)/g)||[]).length===2,'4h …in both assembly paths (the mapped chart and the unmapped one)');
  ok(/#gpts-body \.g3ahchip\{display:block;margin:5px 0 0;padding:2px 6px;border-radius:3px;font-size:7\.6px;text-align:center;/.test(src),'4i the chip is a full-width centred bar');
}

// ---- 5 · the King's roll at the tap: four classes, both twins -------------------------------------------------------------------------------
{
  const P=new Function('var PAT_GROW_PCT=20;\n'+exVar('PAT_CLASSES')+' return PAT_CLASSES;')();
  const keys=P.map(p=>p[0]);
  const want=['king:floor:up','king:floor:dn','king:ceil:dn','king:ceil:up'];
  ok(want.every(k=>keys.includes(k)) && keys.indexOf('king:floor:up')===keys.indexOf('king:ceil')+1,'5a the four classes sit right after king:ceil',keys.slice(6,12));
  const kr=ex('kingRollsNow');
  ok(/kingsNow\('SPY'\)/.test(kr) && /kk\.moved && kk\.moved\.dir/.test(kr) && /out\[b\]=\(kk && kk\.moved && kk\.moved\.dir\)\?kk\.moved\.dir:null/.test(kr),'5b kingRollsNow reads each book’s King move today (KING_PATH) — up / dn / null');
  ok(/kroll:\(function\(\)\{ try\{ return kingRollsNow\(\); \}catch\(eKr\)\{ return null; \} \}\)\(\),/.test(src),'5c every ledger event is stamped with kroll at the tap');
  ok(/kroll:kingRollsNow\(\), pat:tapPatternStamp/.test(src),'5d __gptsDebug.stamp shows it');
  // the classes on fixtures — the JS twin
  const g={ PAT_GROW_PCT:20 };
  const tc=build(g,['tapClasses'],'return tapClasses;')(g);
  const c1=tc({ kings:['SPY'], kroll:{ SPX:'dn', SPY:'up', QQQ:null }, dir:1, pat:{spx:{node:false},spy:{node:true,k:770,pct:100,pos:true},qqq:null} });
  ok(c1.includes('king:floor') && c1.includes('king:floor:up') && !c1.includes('king:floor:dn') && !c1.includes('king:ceil:up'),'5e a SPY King that rolled UP, tapped from above (a floor held): king:floor:up — the rolling floor',c1);
  const c2=tc({ kings:['SPX'], kroll:{ SPX:'dn', SPY:'up', QQQ:null }, dir:-1, pat:{spx:{node:true,k:7720,pct:100,pos:true,g:5},spy:null,qqq:null} });
  ok(c2.includes('king:ceil') && c2.includes('king:ceil:dn') && !c2.includes('king:ceil:up') && !c2.includes('king:floor:dn'),'5f an SPX King that rolled DOWN, tapped from below (a ceiling held): king:ceil:dn — the rolling ceiling',c2);
  const c3=tc({ kings:['SPX'], kroll:{ SPX:'up', SPY:null, QQQ:null }, dir:-1, pat:{spx:{node:true,k:7720,pct:100,pos:true,g:5},spy:null,qqq:null} });
  ok(c3.includes('king:ceil:up') && !c3.includes('king:ceil:dn'),'5g rolled UP but tested from below: king:ceil:up (a magnet rolling away, not a wall)',c3);
  const c4=tc({ kings:['SPX','SPY'], kroll:{ SPX:'up', SPY:'dn', QQQ:null }, dir:1, pat:{spx:{node:true,k:7720,pct:100,pos:true,g:5},spy:{node:true,k:770,pct:100,pos:true},qqq:null} });
  ok(c4.includes('king:floor:up') && c4.includes('king:floor:dn'),'5h two Kings at the tap that disagree: the tap counts in both (said, not hidden)',c4);
  const c5=tc({ kings:['SPY'], dir:1, pat:{spx:{node:false},spy:{node:true,k:770,pct:100,pos:true},qqq:null} });
  ok(c5.includes('king:floor') && !c5.some(x=>/^king:(floor|ceil):/.test(x)),'5i a tap without kroll (pre-v15.72) says nothing about the roll',c5);
  const c6=tc({ kings:['SPY'], kroll:{ SPX:'up', SPY:null, QQQ:null }, dir:1, pat:{spx:{node:false},spy:{node:true,k:770,pct:100,pos:true},qqq:null} });
  ok(!c6.some(x=>/^king:(floor|ceil):/.test(x)),'5j the tapped King has no roll today (only another book’s did): no roll class',c6);
  const c7=tc({ kings:[], kroll:{ SPX:'up', SPY:'up', QQQ:'up' }, dir:1, pat:{spx:{node:true,k:7700,pct:40,pos:true,g:0},spy:null,qqq:null} });
  ok(!c7.some(x=>/^king:/.test(x) && x!=='king:none'),'5k not a King tap: no King roll class',c7);
  // the Python twin on the same taps
  const FIX=[
    { kings:['SPY'], kroll:{ SPX:'dn', SPY:'up', QQQ:null }, dir:1, cont:1, pat:{spx:{node:false},spy:{node:true,k:770,pct:100,pos:true,st:null,mem:false,rug:null},qqq:null}, name:'King', sig:'a', tapBar:0 },
    { kings:['SPX'], kroll:{ SPX:'dn', SPY:'up', QQQ:null }, dir:-1, cont:0, pat:{spx:{node:true,k:7720,pct:100,pos:true,st:null,mem:false,rug:null,nw:null,g:5},spy:null,qqq:null}, name:'King', sig:'b', tapBar:3 },
    { kings:['SPX'], kroll:{ SPX:'up', SPY:null, QQQ:null }, dir:-1, cont:1, pat:{spx:{node:true,k:7720,pct:100,pos:true,st:null,mem:false,rug:null,nw:null,g:5},spy:null,qqq:null}, name:'King', sig:'c', tapBar:6 },
    { kings:['SPX','SPY'], kroll:{ SPX:'up', SPY:'dn', QQQ:null }, dir:1, cont:0, pat:{spx:{node:true,k:7720,pct:100,pos:true,st:null,mem:false,rug:null,nw:null,g:5},spy:{node:true,k:770,pct:100,pos:true,st:null,mem:false,rug:null},qqq:null}, name:'King', sig:'d', tapBar:9 },
    { kings:['SPY'], dir:1, cont:1, pat:{spx:{node:false},spy:{node:true,k:770,pct:100,pos:true,st:null,mem:false,rug:null},qqq:null}, name:'King', sig:'e', tapBar:12 },
    { kings:[], kroll:{ SPX:'up', SPY:'up', QQQ:'up' }, dir:1, cont:1, pat:{spx:{node:true,k:7700,pct:40,pos:true,st:null,mem:false,rug:null,nw:null,g:0},spy:null,qqq:null}, name:'Floor', sig:'f', tapBar:15 },
  ];
  const gT={ RATE_MIN_N:15, wilsonLow:(r,n)=>{ if(!(n>0)) return 0; var z=1.96,p=r/n,d=1+z*z/n,c=p+z*z/(2*n),m=z*Math.sqrt((p*(1-p)+z*z/(4*n))/n); return Math.max(0,(c-m)/d); } };
  const T=build(gT,['tapClasses','patternTable'],'return patternTable;')(gT)(FIX);
  const R={}; T.rows.forEach(r=>{ R[r.key]=r; });
  ok(R['king:floor:up'].n===2 && R['king:floor:up'].held===1 && R['king:floor:dn'].n===1 && R['king:ceil:dn'].n===1 && R['king:ceil:dn'].held===0 && R['king:ceil:up'].n===1 && R['king:ceil:up'].held===1,'5l the table counts the four classes: floor:up 1/2 · floor:dn 0/1 · ceil:dn 0/1 · ceil:up 1/1',[R['king:floor:up'],R['king:ceil:dn']]);
  const tmp=path.join(os.tmpdir(),'gpts-kr-fixture-'+process.pid+'.json'); fs.writeFileSync(tmp, JSON.stringify({events:FIX}));
  let py=null; try{ py=JSON.parse(cp.execSync('python3 tools/nightly/patterns.py --json "'+tmp+'"',{encoding:'utf8'})); }catch(e){ py={err:String(e.message).slice(0,200)}; }
  try{ fs.unlinkSync(tmp); }catch(e){}
  const same=py && py.rows && JSON.stringify(py.rows)===JSON.stringify(JSON.parse(JSON.stringify(T.rows)));
  ok(same,'5m tools/nightly/patterns.py on the same six taps gives the SAME rows — the four classes and their labels, one definition in two languages',py&&py.rows?{py:py.rows.filter(r=>/^king:(floor|ceil):/.test(r.key)).map(r=>[r.key,r.n,r.held]),js:T.rows.filter(r=>/^king:(floor|ceil):/.test(r.key)).map(r=>[r.key,r.n,r.held])}:py);
  const pt=fs.readFileSync('tools/nightly/patterns.py','utf8');
  ok(/'king:floor:up', 'King as a floor, rolled UP today — the rolling floor'/.test(pt) && /KR = e\.get\('kroll'\) or None/.test(pt) && /r_up = any\(KR\.get\(b\) == 'up' for b in K\)/.test(pt),'5n the twin reads kroll the same way (the tapped books only)');
}

// ---- 6 · H8 / H9: the read written before the data, judged from the table Testing ⑦ renders ----------------------------------------------
{
  const R=JSON.parse(fs.readFileSync('learning/register.json','utf8'));
  const H8=R.hypotheses.find(h=>h.id==='H8'), H9=R.hypotheses.find(h=>h.id==='H9');
  ok(H8 && H8.pick==='pat' && H8.cls==='king:floor:up' && H8.outcome==='resume' && H8.base==='dir:up' && H8.minN===30 && H8.since==='2026-09-08' && H8.judgedBy==='nightly' && /rolled UP/.test(H8.claim) && H8.predict && H8.refuteIf && H8.written==='2026-09-04','6a H8: the rolling floor — class, outcome, base, minN 30, since the first stamped session, predict + refuteIf, written before the data',H8);
  ok(H9 && H9.pick==='pat' && H9.cls==='king:ceil:dn' && H9.outcome==='resume' && H9.base==='dir:dn' && H9.minN===30 && H9.since==='2026-09-08' && H9.judgedBy==='nightly','6b H9: the rolling ceiling, the mirror',H9);
  ok(/"when the king rolls up and is below price it may be creating a floor \(support\) and be bullish and vice versa"/.test(H8.note||'') && /rolling-floors-ceilings\.md/.test(H8.note||'') && /floor rolling up \(bullish\)/.test(H8.note||''),'6c his words and the doctrine article are in the note');
  const seed=new Function(exVar('PREREG_SEED')+' return PREREG_SEED;')();
  ok(seed.length===9 && seed[7].id==='H8' && seed[7].pick==='pat' && seed[7].judgedBy==='nightly' && seed[8].id==='H9','6d the panel’s seed carries H8 / H9 (renders before the first fetch)',seed.map(h=>h.id));
  const HS=new Function(exVar('HYP_STUDY')+' return HYP_STUDY;')();
  ok(HS.H8==='K2.6' && HS.H9==='K2.7','6e H8 → K2.6, H9 → K2.7 in the panel’s map');
  const rs=fs.readFileSync('tools/nightly/results.py','utf8');
  ok(/'H8': 'K2\.6', 'H9': 'K2\.7'/.test(rs) && /'K2\.6': \('pattern', \['king:floor:up', 'king:floor:dn'\]\)/.test(rs) && /'K2\.7': \('pattern', \['king:ceil:dn', 'king:ceil:up'\]\)/.test(rs),'6f …and in results.py, with the two study rows sourced from the classes');
  ok(/pat:function\(s\)\{ return \{n:0,hit:0\}; \}/.test(src),'6g the panel knows the pick "pat" (the live row reads the nightly’s verdict; it never counts on its own)');
  // the study rows
  const S=JSON.parse(fs.readFileSync('learning/studies.json','utf8')); const flat=[];
  (function walk(o){ if(Array.isArray(o)) o.forEach(walk); else if(o && typeof o==='object'){ if(o.id && o.q) flat.push(o); Object.keys(o).forEach(k=>walk(o[k])); } })(S);
  const k26=flat.find(x=>x.id==='K2.6'), k27=flat.find(x=>x.id==='K2.7');
  ok(k26 && k26.status==='REGISTERED' && /rolling floor/.test(k26.q) && /H8 thin: n=0 of 30/.test(k26.nightly||'') && k27 && k27.status==='REGISTERED' && /H9 thin: n=0 of 30/.test(k27.nightly||''),'6h K2.6 / K2.7 on the Analysis tab, REGISTERED, the nightly’s count beside them',[k26&&k26.nightly,k27&&k27.nightly]);
  // judge_pat, on fixtures
  const run=fs.readFileSync('tools/nightly/run.py','utf8');
  ok(/def judge_pat\(H, P\):/.test(run) && /verdicts = \[judge_pat\(H, patterns\) if H\.get\('pick'\) == 'pat' else v for H, v in zip\(H_list, verdicts\)\]/.test(run) && /if H\.get\('pick'\) == 'pat':/.test(ex_py(run,'judge')),'6i run.py: a pattern-class hypothesis is judged from the table after it is built, never by judge_sweep');
  const j=(P,H)=>JSON.parse(cp.execSync('python3 -c "import json,sys,importlib.util as u; sp=u.spec_from_file_location(\'run\',\'tools/nightly/run.py\'); m=u.module_from_spec(sp); sp.loader.exec_module(m); print(json.dumps(m.judge_pat(json.loads(sys.argv[1]), json.loads(sys.argv[2]))))" \''+JSON.stringify(H)+'\' \''+JSON.stringify(P)+'\'',{encoding:'utf8'}));
  const H={ id:'H8', claim:'x', minN:30, pick:'pat', cls:'king:floor:up', outcome:'resume', base:'dir:up' };
  const row=(key,n,hit)=>({ key, n:n, held:hit, resume:{ n:n, hit:hit } });
  let v=j({ rows:[row('king:floor:up',12,10), row('dir:up',40,26)] }, H);
  ok(v.verdict==='thin' && v.n===12 && v.nBase===40 && /12 resume-scored taps of 30 needed/.test(v.bar),'6j under minN: THIN, with the counts',v);
  v=j({ rows:[row('king:floor:up',30,27), row('dir:up',60,36)] }, H);
  ok(v.verdict==='cleared' && v.rate===90 && v.base===60 && v.ci[0]>60,'6k 27 of 30 (90%, low ≈74%) against a 60% base: CLEARED',v);
  v=j({ rows:[row('king:floor:up',30,20), row('dir:up',60,36)] }, H);
  ok(v.verdict==='refused' && /covers the dir:up base 60%/.test(v.bar),'6l 20 of 30 (67%, low ≈49%) against 60%: REFUSED — the CI covers the base',v);
  v=j({ rows:[row('king:floor:up',30,27)] }, H);
  ok(v.verdict==='thin' && /base dir:up: 0/.test(v.bar),'6m no base row: THIN, never a verdict against nothing',v);
  v=j({ rows:[row('king:floor:up',30,27), row('dir:up',60,36)] }, Object.assign({},H,{outcome:'held'}));
  ok(v.verdict==='cleared' && v.outcome==='held','6n the outcome is the hypothesis’s own field (held here)',v);
  const L=JSON.parse(fs.readFileSync('learning/log/2026-09-04.json','utf8'));
  const h8=(L.hypotheses||[]).find(x=>x.id==='H8'), h9=(L.hypotheses||[]).find(x=>x.id==='H9');
  ok(h8 && h8.verdict==='thin' && h8.n===0 && h8.nBase>0 && h9 && h9.verdict==='thin' && h9.nBase>0,'6o the committed 09-04 log carries H8 / H9 THIN with their base counts (no stamped tap yet)',[h8,h9]);
}
function ex_py(txt,name){ const i=txt.indexOf('def '+name+'('); if(i<0) return ''; const j=txt.indexOf('\ndef ', i+1); return txt.slice(i, j<0?txt.length:j); }

// ---- 7 · the record --------------------------------------------------------------------------------------------------------------------------
{
  const P=JSON.parse(/var PLAN_SEED=(\{.*?\});\n/.exec(src)[1]);
  const nx=P.roadmap.filter(r=>r.status==='next');
  ok(P.roadmap.some(r=>r.v==='15.72' && /THE FACE, HIS THREE ASKS/.test(r.title) && (r.status==='shipped' || nx.some(x=>x.v==='15.72'))) && P.roadmap.some(r=>r.v==='15.71' && r.status==='shipped') && P.roadmap.some(r=>/candidate score/.test(r.title) && r.v>='15.73'),'7a the plan: v15.71 shipped, v15.72 this build or shipped since, the score after it',nx.map(x=>x.v));
  const R=JSON.parse(fs.readFileSync('learning/recommendations.json','utf8')); const r8=R.rows.find(r=>r.id==='R-8');
  ok(r8 && r8.status==='implemented' && r8.version==='15.72' && r8.by==='operator','7b the face change has its Rec row: R-8, implemented in v15.72',r8&&[r8.status,r8.version]);
  const seedJs=JSON.parse(/var REC_SEED=(\{.*?\});\n/.exec(src)[1]); ok(JSON.stringify(seedJs)===JSON.stringify(R),'7c REC_SEED equals the file');
  const ga=fs.readFileSync('.gitattributes','utf8'); ok(/^\* -text$/m.test(ga) && /^\*\.bat -text$/m.test(ga),'7d .gitattributes: no line-ending conversion (the file said "*-text", which matches nothing; his git normalized the task scripts)');
  const gi=fs.readFileSync('.gitignore','utf8'); ok(/^Claude outputs\/$/m.test(gi),'7e the desktop app’s drop folder is ignored');
  const bat=fs.readFileSync('tools/gex-nightly.bat','utf8'); ok(/\r\n/.test(bat),'7f the task script is CRLF again');
  const gen=fs.readFileSync('tools/mockup-from-studies.py','utf8'); ok(/<div class="rs nt">⟳ <b>%s<\/b><\/div>/.test(gen),'7g the Analysis mockup generator draws the nightly’s count line (the look’s source — test_v1562 2e pins the tab equal to it)');
  const cl=fs.readFileSync('changelog/CHANGELOG.md','utf8'); ok(/## v15\.72/.test(cl) && cl.indexOf('## v15.72')<cl.indexOf('## v15.71'),'7h the CHANGELOG has the v15.72 entry on top');
  const ls=fs.readFileSync('session-state/LESSONS.md','utf8'); const logAt=ls.indexOf('## 2 · THE LESSON LOG'); ok(/### v15\.72/.test(ls.slice(logAt>=0?logAt:0)),'7i the lesson log carries the v15.72 entry');
  const rn=fs.readFileSync('session-state/latest-resume-note.md','utf8'); ok(/v15\.(7[2-9]|[89]\d)/.test(rn.slice(0,600)) && /rolling floor/i.test(rn),'7j the resume note is at v15.72 or later and names the rolling floor');
  const inv=fs.readFileSync('design/DASHBOARD-INVENTORY.md','utf8'); ok(/## 0h · v15\.72/.test(inv),'7k the inventory carries §0h');
  const fd=fs.readFileSync('skylit-docs/FINDINGS.md','utf8'); ok(/H-KR \(v15\.72\)|H8 \/ H9/.test(fd),'7l FINDINGS names the registered read');
}
console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
