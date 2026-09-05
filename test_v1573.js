// ============================================================================================
// test_v1573.js — (v15.73) THE DAY LINE. Operator, 2026-09-05: "there needs to be some message at the bottom that tells me
//   that says something like 9/4- data saved analysis complete, testing complete, recommendations made. something
//   descriptive" · "i like it." — the process reporting on itself: the date, then saved · analysis · testing · learning ·
//   rec, each with its evidence and a colour, from facts the panel already holds; the hover is the whole sentence.
// ============================================================================================
const fs=require('fs');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,400):''));} };
const src=fs.readFileSync(process.env.GPTS_SRC||'./current/gex-signal-tapereader.user.js','utf8');
function ex(n){ const m=new RegExp('function\\s+'+n+'\\s*\\(','g').exec(src); if(!m) throw new Error('not found: '+n);
  let i=src.indexOf('{',m.index), d=0, j=i; for(;j<src.length;j++){ const ch=src[j]; if(ch==='{')d++; else if(ch==='}'){ d--; if(d===0) break; } } return src.slice(m.index,j+1); }
const exVar=(n)=>{ const i=src.indexOf('var '+n+'='); if(i<0) throw new Error('no var '+n); const j=src.indexOf(';\n', i); return src.slice(i, j+1); };
const FNS=['fmtCT','dayBarCount','dayLineState','dayLineHtml'];
const build=(g,tail)=>new Function('__g', Object.keys(g).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+exVar('DAYLINE_ANALYSIS_LATE_MS')+'\nvar window={__gptsDebug:{}};\n'+FNS.map(ex).join('\n')+'\n'+(tail||'return { state:dayLineState, html:dayLineHtml, fmt:fmtCT };'));
const esc=(s)=>String(s==null?'':s).replace(/"/g,'&quot;').replace(/</g,'&lt;');
// a world: today 2026-09-05 (Friday), the clock in CT seconds, the record's pieces
function world(o){
  o=o||{};
  const NOW=o.now!=null?o.now:Date.parse('2026-09-05T'+(o.utc||'15:40')+':00Z');
  class D extends Date{ constructor(...a){ super(...(a.length?a:[NOW])); } static now(){ return NOW; } }
  const days={}; if(o.barsToday!=null) days['2026-09-05']={ snaps:{ SPY:new Array(o.barsToday).fill({t:1}) } }; if(o.barsYday!=null) days['2026-09-04']={ snaps:{ SPY:new Array(o.barsYday).fill({t:1}) } };
  return { Date:D, ctTodayStr:()=>'2026-09-05', ctMarketHours:()=>!!o.rth, ctAfterClose:()=>!!o.after, ctNowSecOfDay:()=>(o.sec!=null?o.sec:10*3600),
    saveState:()=>(o.sv||{code:'none'}), pipeLoad:()=>(o.P||{}), ANALYSIS_NIGHTLY:(o.NJ||null), recorderLoad:()=>({days}),
    AUTOSAVE:(o.A||{lastWrite:null}), DAY_WRITTEN:(o.DW||{}), autosaveState:()=>(o.as||{code:'idle'}), learnLoad:()=>({rules:(o.rules||[])}), recMerged:()=>(o.rec||[]),
    g3tip:(t)=>t?(' title="'+esc(t)+'"'):'', g3esc:esc };
}
const LOG=(d)=>({ date:d, ranOn:'his machine', ranAt:'2026-09-05T20:11:03Z', patterns:{ events:81, days:3 }, hypotheses:[{id:'H1',verdict:'thin'},{id:'H2',verdict:'thin'},{id:'H3',verdict:'thin'},{id:'H4',verdict:'thin'},{id:'H5',verdict:'ready'},{id:'H6',verdict:'thin'},{id:'H7',verdict:'thin'},{id:'H8',verdict:'thin'},{id:'H9',verdict:'thin'}] });
const RULES=(d)=>['L1','L2','L3','L4','L5','L6','L7','L8','L9'].map(id=>({id, verdict:'thin', asOf:d}));
const REC=[{id:'R-1',status:'proposed',by:'review',asOf:'2026-09-04'},{id:'R-2',status:'proposed',by:'review',asOf:'2026-09-04'},{id:'R-3',status:'proposed',by:'review',asOf:'2026-09-04'},{id:'R-4',status:'proposed',by:'review',asOf:'2026-09-04'},{id:'R-5',status:'proposed',by:'review',asOf:'2026-09-04'},{id:'R-6',status:'proposed',by:'review',asOf:'2026-09-04'},{id:'R-7',status:'implemented',by:'operator',asOf:'2026-09-04'}];
const seg=(st,k)=>st.segs.find(s=>s.key===k);

ok(/@version\s+15\.(7[3-9]|[89]\d)/.test(src) && /var GPTS_VERSION='15\.(7[3-9]|[89]\d)';/.test(src),'0a v15.73 or later in both spots');

// ---- 1 · the session running -----------------------------------------------------------------------------------------------------------------
{
  const g=world({ rth:true, sec:10*3600+40*60, barsToday:41, rec:REC, rules:RULES('2026-09-04') });
  const st=build(g)(g).state();
  ok(st.day==='2026-09-05' && st.isToday && st.segs.map(s=>s.key).join()==='data,saved,analysis,testing,learning,rec','1a 10:40, recording: today, six segments in the process order',st.segs.map(s=>s.key));
  ok(seg(st,'data').state==='g' && seg(st,'data').text==='recording · 41 bars','1b data — recording · 41 bars, green',seg(st,'data'));
  ok(seg(st,'saved').state==='k' && /after the close, by the panel/.test(seg(st,'saved').text) && seg(st,'analysis').state==='k' && /≤ 10 min after/.test(seg(st,'analysis').text) && seg(st,'testing').state==='k' && seg(st,'learning').state==='k','1c saved · analysis · testing · learning grey: not knowable yet, each saying what will happen',[seg(st,'saved').text,seg(st,'analysis').text]);
  ok(seg(st,'rec').state==='a' && seg(st,'rec').text==='6 awaiting your ✓','1d rec — 6 awaiting your ✓, amber (his action), no "new" count without a log',seg(st,'rec'));
  const g0=world({ rth:true, sec:8*3600+31*60, barsToday:0, rec:[] });
  const st0=build(g0)(g0).state();
  ok(seg(st0,'data').state==='a' && seg(st0,'data').text==='recording · 0 bars' && seg(st0,'rec').state==='g' && seg(st0,'rec').text==='0 awaiting your ✓','1e 08:31, no bar yet: data amber; nothing awaiting on Rec → green',[seg(st0,'data'),seg(st0,'rec')]);
}

// ---- 2 · after the close, the file written, the nightly not yet run --------------------------------------------------------------------------------
{
  const T=Date.parse('2026-09-05T20:01:00Z');   // 15:01 CT
  const g=world({ after:true, sec:15*3600+4*60, now:T+3*60000, barsToday:131, sv:{code:'saved', t:T}, A:{ lastWrite:{ date:'2026-09-05', by:'auto', t:T } }, P:{ pushedDay:'2026-09-05', pushed:'yes' }, rec:REC, rules:RULES('2026-09-04') });
  const st=build(g)(g).state();
  ok(seg(st,'saved').state==='g' && seg(st,'saved').text==='15:01 · the panel · 131 bars' && /written at 15:01 CT by the panel \(131 bars\), on GitHub\./.test(seg(st,'saved').tip),'2a saved — 15:01 · the panel · 131 bars, green; the hover names the file and GitHub',seg(st,'saved'));
  ok(seg(st,'analysis').state==='a' && /not yet — the task runs every 10 min/.test(seg(st,'analysis').text),'2b analysis — not yet, amber, three minutes after the save',seg(st,'analysis'));
  ok(seg(st,'testing').state==='k' && seg(st,'learning').state==='k' && seg(st,'rec').text==='6 awaiting your ✓','2c testing · learning grey; rec unchanged');
  const g2=world({ after:true, sec:15*3600+45*60, now:T+44*60000, barsToday:131, sv:{code:'saved', t:T}, A:{ lastWrite:{ date:'2026-09-05', by:'click', t:T } }, rec:REC });
  const st2=build(g2)(g2).state();
  ok(seg(st2,'analysis').state==='r' && /overdue — is the GEX nightly task installed\?/.test(seg(st2,'analysis').text) && /tools\\gex-nightly\.log/.test(seg(st2,'analysis').tip),'2d 44 minutes after the save and no log: analysis RED, and it names the task and its log',seg(st2,'analysis'));
  ok(seg(st2,'saved').text==='15:01 · your click · 131 bars','2e a save by his click says so',seg(st2,'saved').text);
}

// ---- 3 · everything done ------------------------------------------------------------------------------------------------------------------------
{
  const T=Date.parse('2026-09-05T20:01:00Z');
  const g=world({ after:true, sec:15*3600+14*60, now:T+13*60000, barsToday:131, sv:{code:'saved', t:T}, A:{ lastWrite:{ date:'2026-09-05', by:'auto', t:T } }, NJ:LOG('2026-09-05'), rules:RULES('2026-09-05'), rec:REC.concat([{id:'M-1',status:'proposed',by:'nightly',asOf:'2026-09-05'}]) });
  const f=build(g)(g); const st=f.state();
  ok(seg(st,'analysis').state==='g' && seg(st,'analysis').text==='15:11 · your machine · 81 taps' && /learning\/log\/2026-09-05\.json/.test(seg(st,'analysis').tip) && /81 taps counted across 3 days/.test(seg(st,'analysis').tip),'3a analysis — 15:11 · your machine · 81 taps, green (the tip says counted across N days)',seg(st,'analysis'));
  ok(seg(st,'testing').state==='g' && seg(st,'testing').text==='9 claims · 1 ready · 8 thin','3b testing — 9 claims · 1 ready · 8 thin',seg(st,'testing'));
  ok(seg(st,'learning').state==='g' && seg(st,'learning').text==='9 rules carry the record','3c learning — 9 rules carry the record',seg(st,'learning'));
  ok(seg(st,'rec').state==='a' && seg(st,'rec').text==='7 awaiting your ✓ · 1 new' && /1 new from the record/.test(seg(st,'rec').tip),'3d rec — 7 awaiting your ✓ · 1 new (a machine row from tonight)',seg(st,'rec'));
  const h=f.html();
  ok(/^<div class="g3pline" title="What has the process done with 9\/5\? Saved — data\/2026-09-05\.json is in the repo folder, written at 15:01 CT by the panel \(131 bars\)\. · Analysis — complete: the nightly ran at 15:11 CT on your machine/.test(h),'3e the hover is the whole sentence, opening with the question and the date as 9/5',h.slice(0,220));
  ok(/<b class="d">9\/5<\/b>/.test(h) && (h.match(/<span class="g3pls" data-k="/g)||[]).length===5 && /data-k="saved" data-s="g"/.test(h) && /data-k="rec" data-s="a"/.test(h) && /<i style="background:#2ec27e"><\/i><em>saved<\/em><span>15:01 · the panel · 131 bars<\/span>/.test(h),'3f the markup: the date, five segments with their state and colour dot',h.slice(0,400));
  const g2=world({ after:true, sec:15*3600+14*60, now:T+13*60000, barsToday:131, sv:{code:'saved', t:T}, NJ:LOG('2026-09-05'), rules:RULES('2026-09-04'), rec:REC });
  const st2=build(g2)(g2).state();
  ok(seg(st2,'learning').state==='a' && /not fetched yet/.test(seg(st2,'learning').text) && seg(st2,'rec').text==='6 awaiting your ✓ · 0 new','3g the log is in but the Learn file is not yet: learning amber "not fetched yet"; rec says 0 new',[seg(st2,'learning'),seg(st2,'rec').text]);
  const gr=world({ after:true, sec:15*3600+14*60, now:T+13*60000, barsToday:131, sv:{code:'saved', t:T}, NJ:Object.assign(LOG('2026-09-05'),{ ranOn:'cloud', hypotheses:[{id:'H2',verdict:'cleared'},{id:'H3',verdict:'refused'},{id:'H5',verdict:'ready'}] }), rules:RULES('2026-09-05'), rec:[] });
  const str=build(gr)(gr).state();
  ok(seg(str,'analysis').text==='15:11 · the cloud · 81 taps' && seg(str,'testing').text==='3 claims · 1 cleared · 1 refused · 1 ready' && seg(str,'rec').state==='g' && seg(str,'rec').text==='0 awaiting your ✓ · 0 new','3h a cloud run says the cloud; cleared / refused / ready in that order; nothing awaiting → green',[seg(str,'analysis').text,seg(str,'testing').text,seg(str,'rec')]);
}

// ---- 4 · the save that did not happen ----------------------------------------------------------------------------------------------------------
{
  const g=world({ after:true, sec:15*3600+4*60, barsToday:131, as:{code:'pending'}, rec:REC });
  const st=build(g)(g).state();
  ok(seg(st,'saved').state==='a' && /not yet — the panel is writing it/.test(seg(st,'saved').text) && seg(st,'analysis').state==='k','4a 15:04, not yet confirmed: saved amber, analysis grey (nothing to read yet)',seg(st,'saved'));
  const g2=world({ after:true, sec:15*3600+20*60, barsToday:131, as:{code:'pending'}, rec:REC });
  ok(seg(build(g2)(g2).state(),'saved').state==='r' && /overdue — see the 💾 chip/.test(seg(build(g2)(g2).state(),'saved').text),'4b 15:20 and still not confirmed: saved RED, pointing at the chip');
  const g3=world({ after:true, sec:15*3600+4*60, barsToday:131, as:{code:'due', blocked:{why:'permission'}}, rec:REC });
  ok(seg(build(g3)(g3).state(),'saved').state==='r' && /cannot write — click 💾/.test(seg(build(g3)(g3).state(),'saved').text) && /\(permission\)/.test(seg(build(g3)(g3).state(),'saved').tip),'4c blocked on the permission: saved RED "cannot write — click 💾", the reason in the hover');
  const g4=world({ after:true, sec:15*3600+4*60, barsToday:0, as:{code:'nodata'}, rec:REC });
  ok(seg(build(g4)(g4).state(),'data').state==='k' && /nothing recorded today/.test(seg(build(g4)(g4).state(),'data').text),'4d nothing recorded: data grey "nothing recorded today", never a red');
}

// ---- 5 · the next morning: yesterday's line stays until today records -------------------------------------------------------------------------
{
  const T=Date.parse('2026-09-04T20:01:00Z');
  const g=world({ sec:7*3600+30*60, now:Date.parse('2026-09-05T12:30:00Z'), barsYday:131, P:{ saveDate:'2026-09-04', saveHow:'repo folder', saveT:T, pushedDay:'2026-09-04', pushed:'yes' }, DW:{ '2026-09-04':{ t:T, how:'repo folder', by:'auto' } }, NJ:LOG('2026-09-04'), rules:RULES('2026-09-04'), rec:REC });
  const f=build(g)(g); const st=f.state();
  ok(st.day==='2026-09-04' && !st.isToday && seg(st,'saved').state==='g' && seg(st,'saved').text==='15:01 · the panel · 131 bars' && seg(st,'analysis').state==='g' && seg(st,'learning').state==='g' && /9\/4/.test(f.html()),'5a 07:30 the next day, nothing recorded yet: yesterday’s completed line, dated 9/4',[st.day,(seg(st,'saved')||{}).text]);
  const g2=world({ rth:true, sec:8*3600+33*60, barsToday:1, barsYday:131, P:{ saveDate:'2026-09-04', saveHow:'repo folder', saveT:T }, NJ:LOG('2026-09-04'), rules:RULES('2026-09-04'), rec:REC });
  const st2=build(g2)(g2).state();
  ok(st2.day==='2026-09-05' && st2.isToday && seg(st2,'data').text==='recording · 1 bars' && seg(st2,'analysis').state==='k','5b 08:33, the first bar of today: today’s line takes over (yesterday’s log does not count for today)',[st2.day,seg(st2,'data').text]);
  const g3=world({ sec:7*3600+30*60, now:Date.parse('2026-09-05T12:30:00Z'), barsYday:131, P:{}, NJ:null, rules:[], rec:REC });
  const st3=build(g3)(g3).state();
  ok(st3.day==='2026-09-05' && seg(st3,'data').state==='k' && seg(st3,'data').text==='—','5c nothing known about any day: today, grey, no invented facts',st3);
  const g4=world({ sec:7*3600+30*60, now:Date.parse('2026-09-05T12:30:00Z'), barsYday:131, P:{ saveDate:'2026-09-04', saveHow:'download', saveT:T }, NJ:null, rules:[], rec:REC });
  const st4=build(g4)(g4).state();
  ok(st4.day==='2026-09-04' && (seg(st4,'data')||{}).state==='k' && (seg(st4,'data')||{}).text==='131 bars, not saved','5d yesterday exported as a download only: "131 bars, not saved" — a download is not a save',seg(st4,'data'));
  const g5=world({ sec:7*3600+30*60, now:Date.parse('2026-09-05T12:30:00Z'), barsYday:131, P:{ saveDate:'2026-09-04', saveHow:'download', saveT:T }, DW:{ '2026-09-04':{ t:T, how:'download' } }, NJ:null, rules:[], rec:REC });
  const st5=build(g5)(g5).state();
  ok(st5.day==='2026-09-04' && (seg(st5,'data')||{}).state==='k' && (seg(st5,'data')||{}).text==='131 bars, not saved','5e …and a written-day mark that says download is not a save either',seg(st5,'data'));
}

// ---- 6 · the clock, the wiring, the CSS ------------------------------------------------------------------------------------------------------------
{
  const f=build(world({}))(world({}));
  ok(f.fmt('2026-09-05T20:11:03Z')==='15:11' && f.fmt(Date.parse('2026-09-05T20:01:00Z'))==='15:01' && f.fmt('garbage')==='' && f.fmt(null)==='','6a fmtCT: an ISO string or epoch ms → HH:MM Chicago, 24 h; garbage → empty',[f.fmt('2026-09-05T20:11:03Z'),f.fmt('garbage')]);
  const rd=ex('render');
  const iAh=rd.lastIndexOf('afterHoursChipHtml()'), iDl=rd.lastIndexOf('dayLineHtml()'), iFt=rd.lastIndexOf('feedStatusHtml()');
  ok(iAh>0 && iDl>iAh && iFt>iDl && (rd.match(/dayLineHtml\(\)/g)||[]).length===2,'6b render(): the after-hours bar, then the day line, then the footer — in both assembly paths',[iAh,iDl,iFt]);
  ok(/#gpts-body \.g3pline\{display:flex;flex-wrap:wrap;[^}]*border-left:3px solid #7cc7ff;[^}]*font-size:8\.4px;[^}]*white-space:nowrap;cursor:help\}/.test(src) && !/#gpts-body \.g3pline\{[^}]*text-overflow/.test(src) && /#gpts-body \.g3pls em\{[^}]*text-transform:uppercase\}/.test(src),'6c the CSS: segments never clip — the row wraps at a segment boundary (no ellipsis), labels uppercase');
  ok(/window\.__gptsDebug\.dayLine=function\(\)/.test(src),'6d the probe: __gptsDebug.dayLine()');
  ok(/DAYLINE_ANALYSIS_LATE_MS=1800000/.test(src),'6e the overdue bar is a named constant (30 minutes)');
  const gT=world({ after:true, sec:15*3600+4*60, barsToday:3, sv:{code:'saved', t:Date.parse('2026-09-05T20:01:00Z')}, rec:REC }); gT.saveState=()=>{ throw new Error('x'); }; gT.recMerged=()=>{ throw new Error('y'); };
  const stT=build(gT)(gT).state();
  ok(stT && stT.segs.length>=5 && seg(stT,'rec').text==='0 awaiting your ✓','6f a throwing source is treated as unknown, never a crash of the footer',stT&&stT.segs.map(s=>s.key));
}

// ---- 7 · the record -------------------------------------------------------------------------------------------------------------------------------
{
  const P=JSON.parse(/var PLAN_SEED=(\{.*?\});\n/.exec(src)[1]);
  const nx=P.roadmap.filter(r=>r.status==='next');
  ok(P.roadmap.some(r=>r.v==='15.73' && /THE DAY LINE/.test(r.title) && (r.status==='shipped' || nx.some(x=>x.v==='15.73'))) && P.roadmap.some(r=>r.v==='15.72' && r.status==='shipped') && P.roadmap.some(r=>/candidate score/.test(r.title) && r.v>='15.74'),'7a the plan: v15.72 shipped, v15.73 this build or shipped since, the score after it',nx.map(x=>x.v));
  const R=JSON.parse(fs.readFileSync('learning/recommendations.json','utf8')); const r9=R.rows.find(r=>r.id==='R-9');
  ok(r9 && r9.status==='implemented' && r9.version==='15.73' && r9.by==='operator','7b the face change has its Rec row: R-9, implemented in v15.73',r9&&[r9.status,r9.version]);
  const seedJs=JSON.parse(/var REC_SEED=(\{.*?\});\n/.exec(src)[1]); ok(JSON.stringify(seedJs)===JSON.stringify(R),'7c REC_SEED equals the file');
  const doc=fs.readFileSync('design/DATA-ANALYSIS-PROCESS.md','utf8'); ok(/the day line/i.test(doc) && /v15\.73/.test(doc),'7d DATA-ANALYSIS-PROCESS.md names the day line as the process reporting on itself');
  const inv=fs.readFileSync('design/DASHBOARD-INVENTORY.md','utf8'); ok(/## 0i · v15\.73/.test(inv),'7e the inventory carries §0i');
  const cl=fs.readFileSync('changelog/CHANGELOG.md','utf8'); ok(/## v15\.73/.test(cl) && cl.indexOf('## v15.73')<cl.indexOf('## v15.72'),'7f the CHANGELOG has the v15.73 entry on top');
  const ls=fs.readFileSync('session-state/LESSONS.md','utf8'); const logAt=ls.indexOf('## 2 · THE LESSON LOG'); ok(/### v15\.73/.test(ls.slice(logAt>=0?logAt:0)),'7g the lesson log carries the v15.73 entry');
  const rn=fs.readFileSync('session-state/latest-resume-note.md','utf8'); ok(/v15\.(7[3-9]|[89]\d)/.test(rn.slice(0,600)) && /day line/i.test(rn),'7h the resume note is at v15.73 or later and names the day line');
  const cfg=JSON.parse(fs.readFileSync('.gex-config.json','utf8')); ok(/THE DAY LINE/.test(cfg.theWhatAndTheHow.dayLine||'') && cfg.theWhatAndTheHow.pinnedBy.indexOf('test_v1573.js')>=0,'7i .gex-config.json names the line and this test');
}
// ---- 8 · the item id is unique against the store (the flake test_v1560 §1 showed once in twelve runs) ----
{
  const store={}; let draws=0;
  const g={ localStorage:{ getItem:k=>(k in store)?store[k]:null, setItem:(k,v)=>{ store[k]=String(v); } }, ITEMS_KEY:'gpts_items_v1', ITEM_KINDS:{ issue:'ISSUE' },
    ctTodayStr:()=>'2026-09-05', Date:{ now:()=>1757000000000 }, Math:{ floor:Math.floor, random:()=>{ draws++; return draws<=2?0:(1/1296); } } };
  const T=new Function('__g', Object.keys(g).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+['itemsLoad','itemsSave','itemsAdd'].map(ex).join('\n')+'\nreturn { add:itemsAdd, load:itemsLoad };')(g);
  const a=T.add('issue','the first item'), b=T.add('issue','the second item');
  ok(a && b && a.id!==b.id && T.load().length===2 && draws===3,'8a two adds in the same millisecond with the same random digit still get two ids (the second redraws)',[a&&a.id,b&&b.id,draws]);
}
console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
