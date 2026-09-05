// ============================================================================================
// test_v1567.js — (v15.67) SCORE THE SETUPS AND PATTERNS + THE COMPLETE ARCHITECTURE. Operator, 2026-09-04: "whether the
//   node deflections consider the setup / pattern type and if that is being tracked and scored. for example, rug, reverse
//   rug, barney and pika stack, king node deflections" → "you need to score these setups and patterns to get proper
//   probabilities and insights. build and make sure you are updating the architecture document and tab in the application
//   which should be a complete architecture / design of the application's process, including the integrations with
//   inside finance and yahoo data that is used to get hod lod statistics daily."
//   Executed, not grepped: the stamp, the scale conversion (the v15.63 tally bug), the classes, the table, the section,
//   the architecture sections; the Python twin pinned equal on one fixture; the documents pinned to the seed.
// ============================================================================================
const fs=require('fs'); const cp=require('child_process'); const os=require('os'); const path=require('path');
const SRC=process.env.GPTS_SRC||'./current/gex-signal-tapereader.user.js';
const src=fs.readFileSync(SRC,'utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,500):''));} };
function ex(n){ const m=new RegExp('function\\s+'+n+'\\s*\\(','g').exec(src); if(!m) throw new Error('not found: '+n);
  let i=src.indexOf('{',m.index),d=0; for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){ d--; if(!d) return src.slice(m.index,k+1); } } }
const exVar=(n)=>{ const i=src.indexOf('var '+n+'='); if(i<0) throw new Error('no var '+n); const j=src.indexOf(';\n', i); return src.slice(i, j+1); };
const esc=s=>String(s==null?'':s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;');
const tip=t=>t?(' title="'+esc(t)+'"'):'';
const two=x=>{ x=''+x; return x.length<2?'0'+x:x; };
const PAL={ ink:'#e6edf3', sub:'#8b98a5', gold:'#e3c341', line:'#2a3140', card:'#161b22', blue:'#5ea4ff', longAccent:'#2ec27e', shortAccent:'#f0616d' };
const GRIDVARS=()=>exVar('GRID_NEW_BARS')+exVar('GRID_NEW_GROW_X')+exVar('GRID_NEW_GROW_PCT')+exVar('GRID_STACK_STEPS')+exVar('GRID_STACK_MIN_PCT')+exVar('GRID_STACK_MAX_PCT')+exVar('GRID_RUG_FLOOR_STEPS')+exVar('GRID_LVL_SNAP_PTS')+exVar('GROWTH_WINDOWS')+exVar('NODEBORN_KEY')+exVar('NODE_BORN')+exVar('REPLAY_BORN')+exVar('PAT_TOL_SPX')+exVar('PAT_GROW_PCT')+exVar('PAT_CLASSES')+exVar('DEFL_ARCH')+'\n';
const build=(g,fns,tail)=>new Function('__g', Object.keys(g).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+GRIDVARS()+fns.map(ex).join('\n')+'\n'+(tail||''));

ok(/@version\s+15\.(6[7-9]|[7-9]\d)/.test(src) && /var GPTS_VERSION='15\.(6[7-9]|[7-9]\d)';/.test(src),'0a v15.67+ in both spots');

// ---- 1 · TWO SCALES: the ledger's strike (the book's own) → the chart's frame, once, through tapDisp ---------------
{
  const g={ ifLadder:()=>({ px:7756, dispScale:1.00108, undPx:773.03, rows:[] }), readTrinityHeaders:()=>({ SPY:{px:773.03}, QQQ:{px:717.66} }), STATE:{ SPY:{price:773.03} },
    kingsNow:()=>({ SPX:{disp:7706}, SPY:{disp:7736}, QQQ:{disp:7750}, px:7756 }), LAD_KING_TEST_PTS:2 };
  const f=build(g,['gridReplay','gridDisp','tapDisp','kingsAtStrike'],'return { tapDisp, kingsAtStrike };')(g);
  const d=f.tapDisp('SPY', 768);
  ok(Math.abs(d-768*7756/773.03)<0.01 && Math.abs(d-7705.6)<0.1,'1a a SPY tap at 768 is 7705.6 in the chart’s frame (768 × px/undPx) — the ratio gridBookNodes converts SPY strikes with',d);
  ok(Math.abs(f.tapDisp('QQQ', 717)-7756*717/717.66)<0.01,'1b a QQQ tap converts by the QQQ price from the Trinity headers (px × k / qqqPx)',f.tapDisp('QQQ',717));
  ok(f.tapDisp('ES', 7700)===null && f.tapDisp('SPY', 0)===null,'1c an unknown book or a zero strike has no frame: null, never a guess');
  const K=f.kingsAtStrike('SPY', 768);
  ok(K.length===1 && K[0]==='SPX','1d THE v15.63 BUG: a SPY tap at 768 against the SPX King at 7706 — compared raw (768 vs 7706) it was never a King tap; converted it is',K);
  ok(f.kingsAtStrike('SPY', 769).length===0,'1e …and 769 (→ 7715.6) is not within 2 chart points of any King',f.kingsAtStrike('SPY',769));
  const g2=Object.assign({}, g, { ifLadder:()=>({ err:'companion not installed' }) });
  const f2=build(g2,['gridReplay','gridDisp','tapDisp','kingsAtStrike'],'return { tapDisp, kingsAtStrike };')(g2);
  ok(f2.tapDisp('SPY',768)===null && f2.kingsAtStrike('SPY',768).length===0,'1f no chart frame (no companion) → no conversion and no Kings, never an exception');
}

// ---- 2 · THE STAMP: what the face showed at the tap, per book, by the face's own functions ---------------------------
{
  const now=1788465600000;
  const VEL={ 7755:{cur:55.9e6,d15:11.9e6}, 7750:{cur:273.2e6,d15:7e6}, 7745:{cur:123.6e6,d15:12.3e6}, 7740:{cur:94.4e6,d15:19.5e6}, 7735:{cur:69.9e6,d15:17.2e6}, 7730:{cur:29.4e6,d15:-1.8e6} };
  const store={ gpts_nodeborn_v2: JSON.stringify({day:'2026-09-03', m:{ '7740':{t:now-5*180000, mag:40e6, pct:22} }, below:{}}) };
  const tape={ pct:{'7755':20,'7750':100,'7745':45,'7740':35,'7735':26,'7730':11}, king:7750, count:100 };
  // the chart's frame: px 7730, SPX strikes at scale 1 (7740 is 7740), SPY × 10 exactly (773 → 7730), QQQ 719 → 7744.4
  const ladders={ SPY:{ pct:{'774':22,'773':100,'772':-45,'771':-38}, king:773, kingKd:390226 }, QQQ:{ pct:{'720':17,'719':47,'718':100,'715':-27}, king:718, kingKd:257257 } };
  const g={ VEL, CFG:{nodeThresh:20, growthWin:15}, STATE:{ SPY:{price:773.0} }, TODAY:'2026-09-03', ctTodayStr:()=>'2026-09-03',
    recorderLoad:()=>({days:{'2026-09-03':{snaps:{SPY:[]},defl:{}}}}), localStorage:{getItem:k=>store[k]||null,setItem:(k,v)=>{store[k]=v;}},
    ifLadder:()=>({ px:7730, dispScale:1.0, undPx:773.0, rows:[] }), readTrinityHeaders:()=>({ SPY:{px:773.0}, QQQ:{px:717.66} }),
    tapeMap:()=>tape, laddersByDollar:()=>ladders, g3tip:tip, g3esc:esc, LAD_KING_TEST_PTS:2, RATE_MIN_N:15, Date:class extends Date{ static now(){ return now; } } };
  const fns=['gridStep','gridReplay','gridNow','bornFromSnaps','nodeBornLoad','nodeBornSave','nodeBornOf','nodeAgeBars','nodeIsNew','nodeGrowth','growthWin','gridDisp','gridBook','gridBookNodes','gridSetups','kingPathSeed','kingPathTouch','kingTriTouch','kingGrowth','kingsNow','tapDisp','tapPatternStamp','kingsAtStrike','tapClasses'];
  const f=build(g,fns,exVar('KING_PATH')+' return { stamp:tapPatternStamp, kings:kingsAtStrike, classes:tapClasses, disp:tapDisp };')(g);
  const S=f.stamp('SPY', 774);   // → 7740 on the chart
  ok(S.spx && S.spx.node===true && S.spx.k===7740 && S.spx.pct===35 && S.spx.pos===true && S.spx.st==='pika' && S.spx.mem===true && S.spx.rug===null,'2a a SPY tap at 774 lands on SPX 7740 — a MEMBER of the pika stack 7740–7750 (35%, +γ), stamped by gridSetups itself',S.spx);
  ok(S.spx.g===26 && S.spx.nw && S.spx.nw.age===5 && S.spx.nw.x===2.4,'2b the SPX node’s growth over the window (+26%) and its NEW verdict (born 5 bars ago at 40M, ×2.4 now) ride the stamp',S.spx);
  ok(S.spy && S.spy.node===true && S.spy.k===774 && S.spy.pct===22 && S.spy.st===null && S.spy.rug===null,'2c the SPY book at the same price: 774 (22%), no stack (under the member cut), no rug',S.spy);
  ok(S.qqq && S.qqq.node===true && S.qqq.k===719 && S.qqq.st==='pika' && S.qqq.mem===true,'2d the QQQ book: 719 (→ 7744.4, within one SPXW strike) is a member of the QQQ pika stack 718–719',S.qqq);
  const S2=f.stamp('SPY', 775);  // → 7750: the SPX King, the named pika; SPY has no strike at 775; QQQ 719 is 5.6 away — off
  ok(S2.spx.k===7750 && S2.spx.st==='pika' && S2.spx.mem===false && S2.spx.pct===100 && S2.spy && S2.spy.node===false && S2.qqq && S2.qqq.node===false,'2e a tap at the SPX King (7750): the NAMED pika (mem false); SPY and QQQ have no node within one strike → node:false, not null',{spx:S2.spx,spy:S2.spy,qqq:S2.qqq});
  const S3=f.stamp('SPY', 772);  // → 7720: nothing on SPX (7730 is 11%, under the threshold); SPY 772 is the named barney; QQQ 715 → 7701 off
  ok(S3.spx && S3.spx.node===false && S3.spy && S3.spy.st==='barney' && S3.spy.mem===false && S3.spy.pos===false,'2f a tap at SPY 772 (→ 7720): no SPX node there; the SPY book shows the NAMED barney stack 771–772',{spx:S3.spx,spy:S3.spy});
  ok(f.kings('SPY', 775).join()==='SPX' && f.kings('SPY', 773).join()==='SPY','2g kingsAtStrike through the real kingsNow: 775 → the SPX King (7750), 773 → the SPY King (7730); QQQ’s (7733.7) is 3.7 away — not within 2',[f.kings('SPY',775),f.kings('SPY',773)]);
  const g3=Object.assign({}, g, { tapeMap:()=>null });
  const f3=build(g3,fns,exVar('KING_PATH')+' return tapPatternStamp;')(g3);
  const S4=f3('SPY', 774);
  ok(S4.spx===null && S4.spy && S4.spy.node===true,'2h a book that cannot be read at the tap (no SPXW tape) is null — never mistaken for "no node"',S4);
  // the tolerance: a converted price 1.4 points off the strike still finds it (half an SPXW strike); 3 points off does not
  const g5=Object.assign({}, g, { ifLadder:()=>({ px:7731.4, dispScale:1.0, undPx:773.0, rows:[] }) });   // spyR = 10.0018 → 774 → 7741.4
  const S5=build(g5,fns,exVar('KING_PATH')+' return tapPatternStamp;')(g5)('SPY', 774);
  ok(S5.spx && S5.spx.k===7740 && S5.spx.st==='pika','2h2 a SPY tap converting to 7741.4 is still the 7740 node (within half a strike)',S5.spx);
  const g6=Object.assign({}, g, { ifLadder:()=>({ px:7733, dispScale:1.0, undPx:773.0, rows:[] }) });   // spyR = 10.0039 → 774 → 7743: 3 off 7740, 2 off 7745 → 7745 (45%, member)
  const S6=build(g6,fns,exVar('KING_PATH')+' return tapPatternStamp;')(g6)('SPY', 774);
  ok(S6.spx && S6.spx.k===7745 && S6.spx.pct===45,'2h3 …and the NEAREST node wins when two are in range (7743 → 7745, not 7740)',S6.spx);
  // the classes of a stamped tap
  const ev={ kings:['SPX'], pat:S, name:'⭑ Floor deflection (BO·FT retest)', dir:1, cont:1 };
  const C=f.classes(ev);
  ok(['all','king:SPX','king:any','spx:pika','spx:new','spx:grow','spx:pos','qqq:pika','old:Floor','dir:up'].every(k=>C.includes(k)) && !C.includes('spy:pika') && !C.includes('king:none') && !C.includes('spx:none'),'2i the tap belongs to every class it matches: King·SPX, SPX pika, NEW, growing, +γ, QQQ pika, old Floor (the ⭑ and the BO·FT flavour stripped), UP',C);
  const C2=f.classes({ kings:[], pat:S3, name:'Ceiling deflection', dir:-1, cont:0 });
  ok(C2.includes('king:none') && C2.includes('spx:none') && C2.includes('spy:barney') && !C2.includes('spx:pos') && !C2.includes('spx:neg') && C2.includes('old:Ceiling') && C2.includes('dir:dn'),'2j no King, no SPX node (spx:none), a SPY barney, DOWN — and no polarity class without an SPX node',C2);
  const C3=f.classes({ name:'Gate deflection', dir:1 });
  ok(C3.join()==='all,old:Gate,dir:up','2k a tap recorded before v15.67 (no stamp) counts only in the old-name / direction classes — it says nothing about the King (kings was [] on every tap until the scale fix)',C3);
}

// ---- 3 · THE TABLE: held / broke / pending per class, a rate at n ≥ 15 with its Wilson low, the Python twin equal -----
const FIX=(()=>{ const ev=[];
  const st=(spx,spy,qqq)=>({spx,spy,qqq});
  const spxNode=(o)=>Object.assign({node:true,k:7740,pct:40,pos:true,st:null,mem:false,rug:null,nw:null,g:null},o||{});
  for(let i=0;i<20;i++) ev.push({ kings:['SPX'], pat:st(spxNode({st:'pika',mem:i%2===0}),{node:false},null), name:'King deflection', dir:1, cont:(i<9)?1:0 });   // King·SPX 9/20 · spx:pika 9/20
  for(let i=0;i<8;i++) ev.push({ kings:[], pat:st(spxNode({pos:false,st:'barney',g:-30}),{node:true,k:772,pct:-45,pos:false,st:'barney',mem:false,rug:null},null), name:'Ceiling deflection', dir:-1, cont:(i<1)?1:0 });   // 1/8 = 12.5% → 13
  for(let i=0;i<3;i++) ev.push({ kings:['SPY','QQQ'], pat:st(spxNode({rug:'rug',g:25,nw:{age:2,x:3,g:25}}),{node:true,k:773,pct:100,pos:true,st:null,mem:false,rug:null},{node:true,k:718,pct:100,pos:true,st:'pika',mem:false,rug:'rrug'}), name:'Rug', dir:1, cont:null });   // pending
  ev.push({ kings:[], name:'⭑ Gate deflection (BO·FT retest)', dir:1, cont:1 });   // an old tap, no stamp
  ev.forEach((e,i)=>{ e.sig=(e.name||'x')+'@'+i; e.tapBar=i*3; });   // one signature per event, as recordDeflections writes them
  return ev; })();
{
  const g={ RATE_MIN_N:15, wilsonLow:(r,n)=>{ if(!(n>0)) return 0; var z=1.96,p=r/n,d=1+z*z/n,c=p+z*z/(2*n),m=z*Math.sqrt((p*(1-p)+z*z/(4*n))/n); return Math.max(0,(c-m)/d); }, g3esc:esc, PAL, tabEmpty:t=>'<div class="empty">'+t+'</div>' };
  const f=build(g,['tapClasses','patternTable','patternRowsHtml'],'return { table:patternTable, html:patternRowsHtml };')(g);
  const T=f.table(FIX); const R={}; T.rows.forEach(r=>{ R[r.key]=r; });
  ok(T.events===32 && T.stamped===31,'3a 32 taps, 31 stamped (the old one is counted, not stamped)',[T.events,T.stamped]);
  ok(R['all'].n===29 && R['all'].held===11 && R['all'].broke===18 && R['all'].pending===3 && R['all'].rate===38,'3b every tap: 11 held / 18 broke of 29 scored (38%), 3 pending — pending is shown, never rated',R['all']);
  ok(R['king:SPX'].n===20 && R['king:SPX'].rate===45 && R['king:SPX'].lo===26,'3c King · SPX: 9 of 20 = 45%, Wilson low 26%',R['king:SPX']);
  ok(R['spx:pika'].n===20 && R['spx:pika'].held===9 && R['spx:barney'].n===8 && R['spx:barney'].rate===13 && R['spx:barney'].lo===2,'3d SPX pika 9/20 (named and members together) · SPX barney 1/8 = 12.5 → 13% (Math.round), low 2%',[R['spx:pika'],R['spx:barney']]);
  ok(R['spx:fade'].n===8 && R['spx:grow'].pending===3 && R['spx:new'].pending===3 && R['spx:rug'].pending===3 && R['qqq:rrug'].pending===3 && R['qqq:pika'].pending===3 && !R['spy:pika'],'3e fading 8 scored; growing / NEW / SPX rug / QQQ rrug / QQQ pika all 3 pending; a class with nothing in it has no row',Object.keys(R));
  ok(R['spx:pos'].n===20 && R['spx:neg'].n===8 && R['king:none'].n===8 && R['king:any'].n===20 && R['king:SPY'].pending===3 && R['old:Gate'].held===1 && R['old:King'].n===20 && R['dir:dn'].n===8,'3f polarity, King-any / -none, the old names (⭑ … (BO·FT retest) stripped) and the directions',[R['spx:pos'].n,R['spx:neg'].n,R['king:none'].n,R['old:Gate']]);
  const h=f.html(Object.assign({days:2}, T), 'the day files (2026-09-05)');
  ok(/the day files \(2026-09-05\) · 32 taps · 31 stamped with the face’s patterns \(v15\.67\+\) · 2 days/.test(h),'3g the caption: source, taps, stamped, days',h.slice(0,200));
  ok(/King · SPX<\/td><td[^>]*>9<\/td><td[^>]*>11<\/td><td[^>]*>45% <i[^>]*>≥26% · n=20<\/i>/.test(h),'3h a row at n ≥ 15 prints its rate with the Wilson low and n',h);
  ok(/SPX barney stack \(named or member\)<\/td><td[^>]*>1<\/td><td[^>]*>7<\/td><td[^>]*>thin \(n=8\)/.test(h) && /SPX node NEW at the tap<\/td><td[^>]*>0<\/td><td[^>]*>0<\/td><td[^>]*>thin \(n=0\) <i[^>]*>· 3 pending<\/i>/.test(h),'3i under 15 the row says thin (n=…); pending prints beside it',h);
  ok(/color:#2ec27e;font-weight:700[^>]*>60%/.test(f.html(f.table(FIX.slice(0,20).map((e,i)=>Object.assign({},e,{cont:i<12?1:0}))),'x')) && /color:#e6edf3;font-weight:700[^>]*>45%/.test(h) && /color:#f0616d;font-weight:700[^>]*>38%/.test(h),'3j colour by the rate: ≥ 57 green (12/20), ≤ 43 red (38%), between ink (45%)',h.match(/color:#[0-9a-f]{6};font-weight:700[^>]*>\d+%/g));
  ok(/<div class="empty">nothing: no taps yet\.<\/div>/.test(f.html({rows:[],events:0,stamped:0},'nothing')) && /no taps yet/.test(f.html(null,'x')),'3k an empty ledger prints the honest line');
  // the Python twin on the same fixture — the nightly and the panel must never disagree
  const tmp=path.join(os.tmpdir(),'gpts-pat-fixture-'+process.pid+'.json'); fs.writeFileSync(tmp, JSON.stringify({events:FIX}));
  let py=null; try{ py=JSON.parse(cp.execSync('python3 tools/nightly/patterns.py --json "'+tmp+'"',{encoding:'utf8'})); }catch(e){ py={err:String(e.message).slice(0,200)}; }
  try{ fs.unlinkSync(tmp); }catch(e){}
  const same=py && py.rows && JSON.stringify(py.rows)===JSON.stringify(JSON.parse(JSON.stringify(T.rows))) && py.events===T.events && py.stamped===T.stamped;
  ok(same,'3l tools/nightly/patterns.py on the same 32 taps gives the SAME rows, rates and Wilson lows as the panel (one definition, two languages, pinned)',py&&py.rows?{py:py.rows.map(r=>[r.key,r.n,r.rate,r.lo]),js:T.rows.map(r=>[r.key,r.n,r.rate,r.lo])}:py);
  let st=''; try{ st=cp.execSync('python3 tools/nightly/patterns.py --selftest',{encoding:'utf8'}); }catch(e){ st=String(e.stdout||e.message); }
  ok(/patterns\.py selftest ok/.test(st),'3m patterns.py --selftest passes',st.slice(0,200));
}

// ---- 4 · THE LEDGER: today's recorder + the archive, one event per sym|date|sig|tapBar ---------------------------------
{
  const e1={ sig:'Floor deflection:up@768.00', key:'Floor deflection:up', strike:768, tapBar:3, cont:1, name:'Floor deflection', dir:1 };
  const e1b=Object.assign({}, e1, { tapBar:40, cont:0 });   // the same strike tapped again 37 bars later — a second event
  const e2={ sig:'King deflection:dn@773.00', key:'King deflection:dn', strike:773, tapBar:9, cont:null, name:'King deflection', dir:-1 };
  const g={ recorderLoad:()=>({ days:{ '2026-09-04':{ defl:{ SPY:[e1,e1b,e2] } }, '2026-09-03':{ defl:{ SPY:[Object.assign({},e1,{cont:0})] } } } }) };
  const arch={ SPY:{ '2026-09-04':[Object.assign({},e1,{id:'SPY|2026-09-04|Floor deflection:up@768.00|3',sym:'SPY',date:'2026-09-04'})], '2026-09-02':[Object.assign({},e2,{tapBar:5,cont:1})] } };
  const f=new Function('__g', Object.keys(g).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\nvar DEFL_ARCH=__g.arch;\n'+ex('ledgerEventsAll')+'\nreturn ledgerEventsAll();')(Object.assign({arch},g));
  ok(f.length===5,'4a 3 today + 1 yesterday in the recorder, 1 archived copy of today’s first tap (dropped) + 1 archived older day = 5 events',f.length);
  ok(f.filter(e=>e.sig===e1.sig).length===3,'4b the same strike tapped at bar 3 and at bar 40 are two events; the archive’s copy of bar 3 is not a third',f.map(e=>[e.sig,e.tapBar]));
}

// ---- 5 · THE SECTION: live table + the nightly's, the honest line when the log carries none --------------------------
{
  const g={ RATE_MIN_N:15, DEFL_CONT_PTS:2, DEFL_FWD_BARS:10, wilsonLow:(r,n)=>{ if(!(n>0)) return 0; var z=1.96,p=r/n,d=1+z*z/n,c=p+z*z/(2*n),m=z*Math.sqrt((p*(1-p)+z*z/(4*n))/n); return Math.max(0,(c-m)/d); }, g3esc:esc, PAL, tabEmpty:t=>'<div class="empty">'+t+'</div>',
    recorderLoad:()=>({ days:{ '2026-09-04':{ defl:{ SPY:FIX } } } }), ANALYSIS_NIGHTLY:null };
  const mk=(gg)=>new Function('__g', Object.keys(gg).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+GRIDVARS()+['tapClasses','patternTable','patternRowsHtml','ledgerEventsAll','patternScoresHtml'].map(ex).join('\n')+'\nreturn patternScoresHtml();');
  const h=mk(g)(g);
  ok(/this browser’s ledger · 32 taps · 31 stamped/.test(h) && /THE NIGHTLY · EVERY RECORDED DAY/.test(h) && /no pattern table in the nightly log yet — tools\/nightly\/patterns\.py writes it/.test(h),'5a the live table from the ledger, then the nightly heading and the honest line when the log has no table',h.slice(0,300));
  const g2=Object.assign({}, g, { ANALYSIS_NIGHTLY:{ date:'2026-09-03', patterns:{ rows:[{key:'all',label:'every tap',n:51,held:24,broke:27,pending:2,rate:47,lo:34}], events:53, stamped:0, days:1 } } });
  const h2=mk(g2)(g2);
  ok(/the day files \(2026-09-03\) · 53 taps · 0 stamped with the face’s patterns \(v15\.67\+\) · 1 day<\/div>/.test(h2) && /every tap<\/td><td[^>]*>24<\/td><td[^>]*>27<\/td><td[^>]*>47% <i[^>]*>≥34% · n=51<\/i> <i[^>]*>· 2 pending<\/i>/.test(h2),'5b the nightly’s table renders from ANALYSIS_NIGHTLY.patterns with its date, days and rows',h2.slice(-900));
  ok(/held = price continued away from the node by 2 in the book’s own units \(SPY \/ QQQ points\) within 10 bars/.test(h) && /A rate prints at n ≥ 15 with its Wilson lower bound/.test(h) && /S0 \(the King\), S1 \(rug \/ reverse rug\), S7 \(the pika cloud\), S2 \(the gate\)/.test(h),'5c the footnote names the outcome rule, the floor and the studies');
  const tb=ex('testingBlock');
  ok(/panSection\('t11','\\u2466','THE PATTERNS','the held rate by setup \\u00d7 book/.test(tb) && /h\+=T_loop\+T_prereg\+T_canfail\+T_dash\+T_cov\+T_nightly\+T_self\+T_pat\+T_detail;/.test(tb),'5d the Testing tab assembles ⑦ THE PATTERNS after ⑥ THE SUITE, before DETAIL');
  ok(/pat:\(function\(\)\{ try\{ return tapPatternStamp\(sym, L\.k\); \}catch\(eP\)\{ return null; \} \}\)\(\),/.test(ex('recordDeflections')),'5e recordDeflections stamps every new event with tapPatternStamp(sym, L.k) — the book’s own strike, converted inside');
}

// ---- 6 · THE HOOK, executed: a recorded tap carries the stamp and the Kings -----------------------------------------------
{
  const pushed=[]; const dayObj={ date:'2026-09-04', defl:{} };
  const g={ RECORDER_SYMS:['SPY','QQQ'], TODAY:'2026-09-04', recorderBlind:()=>false, nodeMapModel:()=>({ ok:true, levels:[{k:774}] }), STATE:{ SPY:{ price:773.4, candles:new Array(12) } },
    recorderLoad:()=>({}), recorderDay:()=>dayObj, recorderSave:()=>{ pushed.push('saved'); }, repoUpsertDefl:()=>{ pushed.push('upsert'); }, RECORDER_MAX_EVENTS:500, DEFL_FWD_BARS:10,
    deflectionAt:()=>({ dir:1, awayPts:1.2, bars:2 }), classifyDeflection:()=>({ name:'Floor deflection', chips:[{t:'Flr',c:'green'},{t:'deflect',c:'purple'}], prio:50, isFBO:false, brokeFT:false }),
    kingsAtStrike:(sym,k)=>(k===774?['SPX']:[]), tapPatternStamp:(sym,k)=>({ spx:{node:true,k:7740,st:'pika',mem:true}, spy:{node:true,k:k}, qqq:null }), labelDeflectionOutcomes:()=>false, Date:class extends Date{ static now(){ return 1788465600000; } } };
  new Function('__g', Object.keys(g).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+ex('deflSetupKey')+'\n'+ex('recordDeflections')+'\nrecordDeflections("SPY");')(g);
  const e=(dayObj.defl.SPY||[])[0];
  ok(!!e && e.strike===774 && e.kings.join()==='SPX' && e.pat && e.pat.spx.st==='pika' && e.pat.spy.k===774 && e.pat.qqq===null && e.cont===null && pushed.join()==='saved,upsert','6a the recorded event carries kings (from kingsAtStrike) and pat (from tapPatternStamp) at the moment of the tap, then is saved and upserted',e);
}

// ---- 7 · THE ARCHITECTURE: the seed's system section renders, and the document is generated from the same seed --------
{
  const seed=JSON.parse(/var PLAN_SEED=(\{.*?\});\n/.exec(src)[1]);
  const plan=JSON.parse(fs.readFileSync('learning/plan.json','utf8'));
  ok(JSON.stringify(seed.system)===JSON.stringify(plan.system) && seed.system && seed.system.components.length>=8 && seed.system.integrations.length===5,'7a PLAN_SEED.system equals learning/plan.json.system (9 components, 5 integrations)',[seed.system&&seed.system.components.length, seed.system&&seed.system.integrations.length]);
  const g={ PAL, g3esc:esc, secOpen:(id,d)=>true, tabEmpty:t=>'<div class="empty">'+t+'</div>' };
  const f=new Function('__g', Object.keys(g).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+ex('tabSection')+'\n'+ex('architectureSystemHtml')+'\nreturn architectureSystemHtml;')(g);
  const h=f(seed.system);
  ok(/data-gsec="ar6"[^>]*>[\s\S]*⑥ THE SYSTEM · the components/.test(h) && /data-gsec="ar7"[^>]*>[\s\S]*⑦ THE INTEGRATIONS · Skylit · InsiderFinance · Yahoo · ForexFactory · GitHub/.test(h) && /data-gsec="ar8"[^>]*>[\s\S]*⑧ THE HOD\/LOD STATISTICS, DAILY/.test(h) && /data-gsec="ar9"[^>]*>[\s\S]*⑨ STORAGE · every key and store/.test(h),'7b four sections render: ⑥ components · ⑦ integrations · ⑧ the HOD/LOD statistics daily · ⑨ storage',h.slice(0,300));
  const comps=(h.match(/<tr style="border-top[^>]*><td style="padding:2px 4px;white-space:normal;vertical-align:top;font-weight:800/g)||[]).length;
  ok(comps===seed.system.components.length && /THE PANEL — current\/gex-signal-tapereader\.user\.js/.test(h) && /THE COMPANION — current\/gex-if-levels\.user\.js \(v1\.18\)/.test(h) && /THE NIGHTLY — tools\/nightly\/run\.py \(\+ tick\.py, patterns\.py, results\.py, tape\.py\)/.test(h),'7c every component is a row: the panel, the companion, the stores, the day file, the couriers to git, the nightly, the review, the corpora, the suite',comps);
  ok(/InsiderFinance \(the second book — STRUCTURE\)/.test(h) && /gpts_if_chain_v1 → ifChain\(\) · ifLadder\(\) · gLevels\(\)/.test(h) && /every 5 min/.test(h) && /dte0 · toFri · all/.test(h),'7d the InsiderFinance card: the companion’s 5-minute poll, the three windows, the key and the functions that read it');
  ok(/Yahoo Finance \(price bars\)/.test(h) && /Yahoo's v8 chart endpoint, &lt;SYM&gt;=F\?interval=1m&amp;range=5d/.test(h) && /never touches Yahoo — test_futbars f30/.test(h) && /every 5 min in RTH, hourly off-hours/.test(h) && /gpts_futbars_v1 → futBarsLoad\(\)/.test(h) && /1-minute history is ≤ 7 days at the source/.test(h),'7e the Yahoo card: the endpoint, the cadence, the key, the 7-day limit');
  const steps=(h.match(/<li style="white-space:normal">/g)||[]).length;
  ok(steps===seed.system.hodlod.steps.length && steps===8 && /tools\/study-hodlod\.py re-derives BASERATES\.json/.test(h) && /8 posr rows × 7 forty-five-minute blocks \(9:15–13:45 CT\), 284 sessions, 38,054 observations, AUC 0\.879/.test(h) && /the corrected ones: IN 63% \(n=284, F-12\), NOT-IN 85% \(n=230, F-11\/F-12\) — never the withdrawn 92%/.test(h),'7f the HOD/LOD pipeline: eight steps from the companion’s fetch to the scored READ, the corrected rates only',steps);
  ok(/gpts_recorder_v7/.test(h) && /IndexedDB gpts_repo_v1 \(v4\): snaps · tape · defl · feat · kv/.test(h) && /gpts_nodeborn_v2/.test(h),'7g storage: the recorder key, the IDB stores, the births key');
  ok(/<div class="empty">plan\.json carries no system section/.test(f(null)),'7h no system section → the honest line');
  const ab=ex('architectureBlock');
  ok(/h\+=architectureSystemHtml\(PL\.system\|\|null\)/.test(ab) && /design\/ARCHITECTURE\.md · learning\/plan\.json/.test(ab),'7i architectureBlock renders the system after ⑤ and names design/ARCHITECTURE.md as a source');
  const md=fs.existsSync('design/ARCHITECTURE.md')?fs.readFileSync('design/ARCHITECTURE.md','utf8'):'';
  const allIn=seed.system.components.every(c=>md.includes(c.name)) && seed.system.integrations.every(i=>md.includes(i.name) && md.includes(i.how) && md.includes(i.keys)) && seed.system.hodlod.steps.every(s=>md.includes(s)) && seed.system.storage.every(k=>md.includes(k.key));
  ok(allIn && /Generated from `tools\/plan-seed\.py`/.test(md) && /never this file by hand/.test(md),'7j design/ARCHITECTURE.md carries every component, integration (how · keys), pipeline step and storage key of the seed — generated, never hand-edited',allIn);
  const seedPy=fs.readFileSync('tools/plan-seed.py','utf8');
  ok(/def architecture_md\(\):/.test(seedPy) && /design\/ARCHITECTURE\.md/.test(seedPy) && /"system": \{/.test(seedPy),'7k tools/plan-seed.py is the one source: PLAN["system"] + architecture_md() write the document');
  const r67=plan.roadmap.find(r=>r.v==='15.67'); const later=plan.roadmap.find(r=>/deflection candidate score/.test(r.title));
  ok(r67 && /SCORE THE SETUPS AND PATTERNS/.test(r67.title) && /the complete architecture on the ⚙ tab/.test(r67.title) && ['next','shipped'].includes(r67.status) && plan.roadmap.some(r=>r.v==='15.66' && r.status==='shipped') && later && parseFloat(later.v)>15.67,'7l the roadmap: v15.66 shipped, v15.67 = the setups scored + the architecture, the candidate score moved after it',[r67&&r67.status, later&&later.v]);
}

// ---- 8 · THE NIGHTLY and the records ----------------------------------------------------------------------------------
{
  const run=fs.readFileSync('tools/nightly/run.py','utf8');
  ok(/patterns = _pt\.table_for\(frm=frm\)/.test(run) && /tape=tape_cov, patterns=patterns\)/.test(run),'8a run.py writes the pattern table into the log as `patterns` (days from the register’s from)');
  const log=fs.existsSync('learning/log/2026-09-03.json')?JSON.parse(fs.readFileSync('learning/log/2026-09-03.json','utf8')):null;
  ok(log && log.patterns && log.patterns.events===53 && log.patterns.stamped===0 && log.patterns.rows.some(r=>r.key==='old:Floor' && r.n===18 && r.rate===61),'8b the 2026-09-03 log carries the table: 53 taps, none stamped (pre-v15.67), old Floor 11/18 = 61%',log&&log.patterns&&log.patterns.rows.map(r=>[r.key,r.n,r.rate]));
  const cl=fs.readFileSync('changelog/CHANGELOG.md','utf8');
  ok(/## v15\.67/.test(cl) && cl.indexOf('## v15.67')<cl.indexOf('## v15.66'),'8c the CHANGELOG has the v15.67 entry on top');
  const ls=fs.existsSync('session-state/LESSONS.md')?fs.readFileSync('session-state/LESSONS.md','utf8'):'';
  const logAt=ls.indexOf('## 2 · THE LESSON LOG');
  ok(/### v15\.67/.test(ls.slice(logAt)) && /768 against/.test(ls),'8d the lesson log carries the v15.67 entry (two scales: 768 against 7712 — the tally that never counted a King)');
  const cfg=JSON.parse(fs.readFileSync('.gex-config.json','utf8'));
  ok(JSON.stringify(cfg).includes('design/ARCHITECTURE.md'),'8e .gex-config.json points at design/ARCHITECTURE.md');
}

console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
