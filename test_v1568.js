// ============================================================================================
// test_v1568.js — (v15.68) THE LOOP CLOSES ON THE CLICK. Operator, 2026-09-04: "when i click on the save button and the push
//   happens, you have everything to start the analysis" · "i envision clicking on the save, the data getting saved and the
//   analysis occurring and the analysis tab being updated" · "so can you update that so you do what you need to do".
//   Executed, not grepped: tick.py's decision, results.py's numbers and their application to the registry (the review's
//   sentence survives a thin row), the seed's merge, run.py's guard and stamps, the panel's row, the re-fetch, the plan; the
//   Windows task's scripts checked line by line (no PowerShell, the lock, the fallback, the silent no-op).
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
const py=(args,opts)=>{ try{ return { out:cp.execSync('python3 '+args,Object.assign({encoding:'utf8',stdio:['ignore','pipe','pipe']},opts||{})), code:0 }; }catch(e){ return { out:String(e.stdout||''), err:String(e.stderr||''), code:e.status }; } };

ok(/@version\s+15\.(6[8-9]|[7-9]\d)/.test(src) && /var GPTS_VERSION='15\.(6[8-9]|[7-9]\d)';/.test(src),'0a v15.68+ in both spots');

// ---- 1 · tick.py: the task's decision — run only when the newest day file is newer than its log --------------------------
{
  const st=py('tools/nightly/tick.py --selftest');
  ok(st.code===0 && /tick\.py selftest ok/.test(st.out),'1a tick.py --selftest: no day file → nothing; no log → run; log newer → nothing; a second 💾 → run; the next day → run',st.out.slice(-200)+(st.err||''));
  const ck=py('tools/nightly/tick.py --check');
  ok(ck.code===0 && /(RUN for 20\d\d-\d\d-\d\d — |nothing to do — the log for 20\d\d-\d\d-\d\d is current)/.test(ck.out),'1b --check prints the decision for this repo and runs nothing',ck.out);
  const tk=fs.readFileSync('tools/nightly/tick.py','utf8');
  ok(/return 3\n/.test(tk) && /import run as _run/.test(tk) && /_run\.run\(day\)/.test(tk) && !/datetime\.date\.today|date\.today\(\)|strftime\('%Y-%m-%d'\)\s*[=<>]/.test(tk),'1c exit 3 = nothing to do; the run goes through run.run(day); "today" is the newest day file, never the clock or cmd’s %DATE%');
}

// ---- 2 · results.py: the numbers, the statuses, the registry patched — the review's sentence survives a thin row -----------
const LOG={ date:'2026-09-08', ranOn:'his machine', patterns:{ rows:[
    {key:'all',label:'every tap',n:51,held:24,broke:27,pending:2,rate:47,lo:34},
    {key:'king:any',label:'King · any book',n:20,held:9,broke:11,pending:0,rate:45,lo:26},
    {key:'king:SPX',label:'King · SPX',n:20,held:9,broke:11,pending:0,rate:45,lo:26},
    {key:'king:SPY',label:'King · SPY',n:3,held:2,broke:1,pending:1,rate:67,lo:21},
    {key:'king:floor',label:'King as a floor',n:16,held:8,broke:8,pending:0,rate:50,lo:28},
    {key:'king:grow',label:'growing',n:4,held:3,broke:1,pending:0,rate:75,lo:30},
    {key:'spx:rug',label:'SPX rug',n:4,held:2,broke:2,pending:1,rate:50,lo:15},
    {key:'spx:barney',label:'SPX barney',n:17,held:6,broke:11,pending:0,rate:35,lo:17} ] },
  hypotheses:[ {id:'H1',verdict:'thin',n:3,minN:40}, {id:'H2',verdict:'cleared',n:30,minN:30,rate:71.0,bar:'holds, CI excludes base'},
    {id:'H3',verdict:'refused',n:40,minN:40,rate:52.0,bar:'gap 0.1 pts >= 8'}, {id:'H5',verdict:'ready',n:51,minN:50,bar:'ledger has 51 events'} ] };
{
  const st=py('tools/nightly/results.py --selftest');
  ok(st.code===0 && /results\.py selftest ok/.test(st.out),'2a results.py --selftest passes',st.out.slice(-200)+(st.err||''));
  const tmp=path.join(os.tmpdir(),'gpts-log-'+process.pid+'.json'); fs.writeFileSync(tmp, JSON.stringify(LOG));
  const r=py('tools/nightly/results.py --json "'+tmp+'"'); let J=null; try{ J=JSON.parse(r.out); }catch(e){}
  try{ fs.unlinkSync(tmp); }catch(e){}
  ok(!!J && J.results,'2b --json returns the map and the computed rows',r.err||r.out.slice(0,200));
  const R=(J&&J.results)||{};
  ok(R['S0.1'] && R['S0.1'].status==='READ' && R['S0.1'].line==='any King held 9 / 20 = 45% (low 26%) · nightly 2026-09-08' && R['S0.1'].n===20 && R['S0.1'].by==='nightly','2c S0.1 (any King) reads at n=20: the held rate, the Wilson low, the date, by the nightly',R['S0.1']);
  ok(R['K1.1'] && R['K1.1'].line===R['S0.2'].line && R['K2.1'] && R['K2.1'].status==='READ' && /King as floor held 8 \/ 16 = 50%/.test(R['K2.1'].line),'2d K1.1 shares S0.2’s number (the SPX King); K2.1 reads from the King-as-floor class',[R['K1.1']&&R['K1.1'].line, R['K2.1']&&R['K2.1'].line]);
  ok(R['S0.3'] && R['S0.3'].status==='THIN' && R['S0.3'].line==='SPY King held 2 / 3 (thin) · nightly 2026-09-08' && R['S0.5'] && R['S0.5'].status==='THIN' && /growing held 3 \/ 4 \(thin\) · fading —/.test(R['S0.5'].line),'2e under 15 a row is THIN and its line says thin; a class with no tap prints —',[R['S0.3'],R['S0.5']]);
  ok(!R['S0.4'] && !R['K2.2'] && !R['S7.1'] && R['S7.2'] && R['S7.2'].status==='READ' && /SPX held 6 \/ 17 = 35% \(low 17%\) · SPY — · QQQ —/.test(R['S7.2'].line),'2f a study none of whose classes has a tap is NOT answered (the review’s row stands); the barney stack reads from SPX alone',[Object.keys(R), R['S7.2']]);
  ok(R['F5.2'] && R['F5.2'].status===null && R['F5.2'].line==='H1 thin: n=3 of 40 · nightly 2026-09-08' && R['F2.1'].status==='READ' && /^H2 CLEARED at n=30: 71(\.0)?% — holds/.test(R['F2.1'].line) && R['F6.1'].status==='REFUSED' && R['H1.3'].status==='READ NEXT' && !R['F1.4'],'2g the register: thin → no status (the count so far); cleared → READ; refused → REFUSED; ready → READ NEXT; a hypothesis not in the log → untouched',[R['F5.2'],R['F2.1']&&R['F2.1'].status,R['F6.1']&&R['F6.1'].status,R['H1.3']&&R['H1.3'].status]);
  // the panel's map equals the nightly's
  const HS=new Function(exVar('HYP_STUDY')+' return HYP_STUDY;')();
  ok(J && JSON.stringify(HS)===JSON.stringify(J.hypStudy),'2h the panel’s HYP_STUDY and results.py’s are the same map (H1→F5.2 … H7→H2.8)',[HS,J&&J.hypStudy]);
  ok(J && J.sources && ['S0.1','S0.2','S0.3','S0.4','S0.5','S0.7','K1.1','K1.2','K2.1','K2.2','S1.1','S1.2','S7.1','S7.2','F5.2','F2.1','F6.1','F1.4','H1.3','H2.7','H2.8'].every(k=>J.sources[k]),'2i every mapped study is one the registry actually has (21 rows)',J&&Object.keys(J.sources||{}));
  // apply(): executed through a temp registry
  const reg={schema:1,subjects:[{key:'S',name:'S',strap:'',subsections:[{key:'S0',name:'',decides:'',studies:[
    {id:'S0.1',q:'q',decides:'SIZE',claim:'C10',corpus:'x',status:'THIN',result:'51 scored taps — the review said'},
    {id:'F5.2',q:'q',decides:'SIZE',claim:'ours',corpus:'x',status:'REGISTERED',result:'exploratory 32% n=34 for A'},
    {id:'X9.9',q:'q',decides:'SIZE',claim:'ours',corpus:'x',status:'OPEN'} ]}]}]};
  const dir=fs.mkdtempSync(path.join(os.tmpdir(),'gpts-reg-')); fs.mkdirSync(path.join(dir,'learning','log'),{recursive:true});
  fs.writeFileSync(path.join(dir,'learning','studies.json'), JSON.stringify(reg)); fs.writeFileSync(path.join(dir,'learning','log','2026-09-08.json'), JSON.stringify(LOG));
  const w=py('- <<\'EOF\'\nimport sys, json; sys.path.insert(0, "tools/nightly"); import results\nprint(json.dumps(results.write('+JSON.stringify(dir)+')[1]))\nEOF');
  let S2=null, RJ=null; try{ S2=JSON.parse(fs.readFileSync(path.join(dir,'learning','studies.json'),'utf8')); RJ=JSON.parse(fs.readFileSync(path.join(dir,'learning','results.json'),'utf8')); }catch(e){}
  const rows={}; if(S2) S2.subjects[0].subsections[0].studies.forEach(x=>{ rows[x.id]=x; });
  ok(w.code===0 && /(^|\n)2\s*$/.test(w.out.trim()) && RJ && RJ.schema===1 && RJ.asOf==='2026-09-08' && RJ.writtenBy==='tools/nightly/results.py' && RJ.results['S0.1'],'2j write(): results.json written (asOf, writtenBy, the rows), 2 registry rows patched',w.err||w.out);
  ok(rows['S0.1'] && rows['S0.1'].status==='READ' && rows['S0.1'].by==='nightly' && rows['S0.1'].asOf==='2026-09-08' && rows['S0.1'].result===R['S0.1'].line && rows['S0.1'].nightly===R['S0.1'].line,'2k a row with a verdict: result = the machine’s line, status READ, by the nightly, asOf',rows['S0.1']);
  ok(rows['F5.2'] && rows['F5.2'].status==='REGISTERED' && rows['F5.2'].result==='exploratory 32% n=34 for A' && !rows['F5.2'].by && rows['F5.2'].nightly==='H1 thin: n=3 of 40 · nightly 2026-09-08','2l a thin row KEEPS the review’s sentence and status; the count so far rides beside it as `nightly`',rows['F5.2']);
  ok(rows['X9.9'] && rows['X9.9'].status==='OPEN' && !('nightly' in rows['X9.9']) && !('result' in rows['X9.9']) && S2.counts && S2.counts.studies===3 && S2.counts.byStatus.READ===1,'2m an unmapped row is untouched; the counts follow the statuses',[rows['X9.9'],S2&&S2.counts]);
  // the seed merges results.json when it regenerates — a review never erases the nightly's numbers
  const seedSrc=fs.readFileSync('tools/studies-seed.py','utf8');
  ok(/import results as _results/.test(seedSrc) && /_results\.apply\(S, json\.load\(io\.open\(_rp/.test(seedSrc) && /learning\/results\.json/.test(seedSrc),'2n tools/studies-seed.py merges learning/results.json over the seed rows before writing studies.json');
  const sd=fs.mkdtempSync(path.join(os.tmpdir(),'gpts-seed-')); fs.mkdirSync(path.join(sd,'learning')); fs.mkdirSync(path.join(sd,'tools','nightly'),{recursive:true});
  fs.copyFileSync('tools/studies-seed.py', path.join(sd,'tools','studies-seed.py')); fs.copyFileSync('tools/nightly/results.py', path.join(sd,'tools','nightly','results.py'));
  fs.writeFileSync(path.join(sd,'learning','results.json'), JSON.stringify(RJ));
  const sr=py('tools/studies-seed.py',{cwd:sd}); let S3=null; try{ S3=JSON.parse(fs.readFileSync(path.join(sd,'learning','studies.json'),'utf8')); }catch(e){}
  const r3={}; if(S3) S3.subjects.forEach(sj=>sj.subsections.forEach(ss=>ss.studies.forEach(x=>{ r3[x.id]=x; })));
  ok(sr.code===0 && /merged learning\/results\.json · \d+ rows/.test(sr.out) && r3['S0.1'] && r3['S0.1'].by==='nightly' && r3['S0.1'].status==='READ' && r3['F5.2'] && r3['F5.2'].status==='REGISTERED' && r3['F5.2'].nightly && r3['F2.1'] && r3['F2.1'].status==='READ','2o regenerating the seed keeps the nightly’s numbers: S0.1 READ by the nightly, F5.2 still REGISTERED with its count, F2.1 READ (H2 cleared)',sr.err||sr.out.slice(-200));
}

// ---- 3 · run.py: the hook, the stamps, the atomic log, the guard --------------------------------------------------------
{
  const run=fs.readFileSync('tools/nightly/run.py','utf8');
  ok(/_rs\.write\(ROOT, log\)/.test(run) && /ran_on = 'his machine' if os\.name == 'nt' else 'cloud'/.test(run) && /ranOn=ran_on, ranAt=/.test(run),'3a run.py writes the registry after the log and stamps ranOn (his machine / cloud) + ranAt');
  ok(/os\.replace\(_tmp, p\)/.test(run) && /out\['corpus'\]\['file'\] = os\.path\.relpath\(es, ROOT\)/.test(run),'3b the log is written atomically (the sync task must never commit half a file); the sweep table records a relative path');
  const before=fs.statSync('learning/log/2026-09-03.json').mtimeMs;
  const h=py('tools/nightly/run.py --help');
  ok(h.code===2 && /THE NIGHTLY, IN ONE COMMAND/.test(h.out) && fs.statSync('learning/log/2026-09-03.json').mtimeMs===before,'3c an unknown flag prints the usage and exits 2 WITHOUT running (LESSONS v15.67: --help ran the nightly)',[h.code,h.out.slice(0,80)]);
  const log=JSON.parse(fs.readFileSync('learning/log/2026-09-03.json','utf8'));
  ok(log.ranOn==='cloud' && /^\d{4}-\d\d-\d\dT\d\d:\d\d:\d\dZ$/.test(log.ranAt||'') && log.patterns && log.patterns.rows.length,'3d the committed 2026-09-03 log carries ranOn cloud, ranAt, the pattern table',[log.ranOn,log.ranAt]);
  const RJ=JSON.parse(fs.readFileSync('learning/results.json','utf8')); const S=JSON.parse(fs.readFileSync('learning/studies.json','utf8'));
  const byId={}; S.subjects.forEach(sj=>sj.subsections.forEach(ss=>ss.studies.forEach(x=>{ byId[x.id]=x; })));
  ok(RJ.asOf==='2026-09-03' && Object.keys(RJ.results).length>=7 && byId['H1.3'] && byId['H1.3'].status==='READ NEXT' && byId['H1.3'].by==='nightly' && byId['F2.1'] && byId['F2.1'].status==='REGISTERED' && /^H2 thin: n=1 of 30/.test(byId['F2.1'].nightly||''),'3e the committed registry: H1.3 → READ NEXT by the nightly (H5 ready at 51), F2.1 still REGISTERED with "H2 thin: n=1 of 30" beside the review’s sentence',[RJ.asOf, byId['H1.3']&&byId['H1.3'].status, byId['F2.1']&&byId['F2.1'].nightly]);
}

// ---- 4 · the Windows task: the scripts, line by line -----------------------------------------------------------------------
{
  const bat=fs.readFileSync('tools/gex-nightly.bat','latin1'), vbs=fs.readFileSync('tools/gex-nightly-hidden.vbs','latin1'), setup=fs.readFileSync('setup-gex-nightly.bat','latin1');
  ok(![bat,vbs,setup].some(t=>t.split(/\r?\n/).some(l=>!/^\s*(REM|')/i.test(l) && /powershell|pwsh/i.test(l))) && [bat,vbs,setup].every(t=>/^[\x00-\x7f]*$/.test(t) && /\r\n/.test(t)),'4a no PowerShell command anywhere (Avast; the word appears only in the REM that says so), ASCII, CRLF');
  ok(/set LOCK=%REPO%\\tools\\gex-nightly\.lock/.test(bat) && /if exist "%LOCK%" \(\r\n(?:[^\r\n]*\r\n)*?\s*exit \/b 0\r\n\)/.test(bat) && /echo %DATE% %TIME% > "%LOCK%"/.test(bat) && (bat.match(/del "%LOCK%" >nul 2>&1/g)||[]).length>=2,'4b the .bat takes a lock, skips the tick when one is held, and releases it on every path');
  ok(/where python >nul 2>&1 && set PY=python/.test(bat) && /if not defined PY where py >nul 2>&1 && set PY=py -3/.test(bat) && /PYTHON NOT FOUND/.test(bat),'4c python on the PATH, the py launcher as the fallback, a logged failure when neither exists');
  ok(/cd \/d "%REPO%"\r\n!PY! tools\\nightly\\tick\.py >> "%LOGF%" 2>&1/.test(bat) && /if "!RC!"=="0" echo .* nightly ran/.test(bat) && /if "!RC!"=="1" echo .* NIGHTLY FAILED/.test(bat) && /REM RC 3 = nothing to do: silent/.test(bat),'4d it runs tick.py from the repo, logs a run and a failure, and writes NOTHING for a no-op (144 ticks a day)');
  ok(/sh\.Run "cmd \/c """"C:\\Dev\\gex-signal-tapereader\\tools\\gex-nightly\.bat""""", 0, False/.test(vbs),'4e the .vbs launcher runs the .bat with window style 0 (hidden), like the sync’s');
  ok(/schtasks \/Create \/TN "GEX nightly" \/TR "wscript\.exe \/\/B \/\/Nologo \\"%REPO%\\tools\\gex-nightly-hidden\.vbs\\"" \/SC MINUTE \/MO 10 \/F/.test(setup),'4f the setup registers "GEX nightly": every 10 minutes, through the hidden launcher');
  ok(/schtasks \/Query \/TN "GEX sync"/.test(setup) && /Run setup-gex-sync\.bat first/.test(setup) && /for %%F in \("tools\\gex-nightly\.bat" "tools\\gex-nightly-hidden\.vbs" "tools\\nightly\\tick\.py" "tools\\nightly\\run\.py"\)/.test(setup) && /!PY! --version/.test(setup) && /tools\\nightly\\tick\.py --check/.test(setup),'4g …after checking the sync task exists (nothing would be pushed otherwise), the four files, and python; then prints tick’s decision');
  const gi=fs.readFileSync('.gitignore','utf8');
  ok(/^tools\/gex-nightly\.log$/m.test(gi) && /^tools\/gex-nightly\.lock$/m.test(gi) && /^\*\.json\.tmp$/m.test(gi),'4h .gitignore: the task’s log and lock and the atomic .tmp never enter git (the sync’s `git add -A` would sweep them)');
}

// ---- 5 · the panel: the row, the re-fetch, the loop status, the plan -----------------------------------------------------
{
  const g={ STUDY_STATUS_COL:{ READ:'#7cc7ff', REGISTERED:'#a371f7', OPEN:'#6c7889' }, g3esc:esc };
  const f=new Function('__g', Object.keys(g).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+ex('studyRowHtml')+'\nreturn studyRowHtml;')(g);
  const a=f({ id:'S0.1', q:'q', status:'READ', result:'held any King 9 / 20 = 45% (low 26%) · nightly 2026-09-08', nightly:'held any King 9 / 20 = 45% (low 26%) · nightly 2026-09-08', by:'nightly', asOf:'2026-09-08' });
  ok(/<div class="rs">→ <b>held any King 9 \/ 20 = 45% \(low 26%\) · nightly 2026-09-08<\/b> <span class="ci nt" title="[^"]*">· by the nightly, 2026-09-08<\/span><\/div>$/.test(a) && !/class="rs nt"/.test(a),'5a a row the nightly answered: its result tagged "by the nightly, <date>" — and no second line when the line IS the result',a);
  const b=f({ id:'F5.2', q:'q', status:'REGISTERED', result:'exploratory 32% n=34 for A', nightly:'H1 thin: n=3 of 40 · nightly 2026-09-08', asOf:'2026-09-08' });
  ok(/<div class="rs">→ <b>exploratory 32% n=34 for A<\/b><\/div><div class="rs nt" title="[^"]*">⟳ <b>H1 thin: n=3 of 40 · nightly 2026-09-08<\/b><\/div>$/.test(b) && !/by the nightly/.test(b),'5b a thin row: the review’s sentence untagged, the count so far as a second line',b);
  const c=f({ id:'X9.9', q:'q', status:'OPEN' });
  ok(!/class="rs/.test(c) && /<span class="vd" style="color:#6c7889">OPEN<\/span><\/div>$/.test(c),'5c an untouched row renders exactly as before',c);
  ok(/#gpts-body \.g3pan \.rs \.ci\.nt\{color:#7cc7ff\}#gpts-body \.g3pan \.rs\.nt b\{color:#7cc7ff;font-weight:500\}/.test(src) && /\.rs \.ci\.nt\{color:#7cc7ff\}/.test(fs.readFileSync('tools/panel-css.py','utf8')),'5d the nightly’s lines are blue (the READ colour) — in tools/panel-css.py, the one stylesheet, spliced');
  ok(/try\{ studiesFetch\(\); \}catch\(eSt\)\{\}/.test(ex('pipeCheck')),'5e pipeCheck re-fetches the registry every 10 minutes — the Analysis tab updates without a reload');
  ok(/\(N\.ranOn\?\(' · ran on '\+N\.ranOn\):''\)/.test(ex('loopStatus')),'5f the loop status names where the nightly ran (his machine / cloud)');
  const P=JSON.parse(/var PLAN_SEED=(\{.*?\});\n/.exec(src)[1]); const plan=JSON.parse(fs.readFileSync('learning/plan.json','utf8'));
  const r68=P.roadmap.find(r=>r.v==='15.68'); const later=P.roadmap.find(r=>/deflection candidate score/.test(r.title));
  ok(r68 && /THE LOOP CLOSES ON THE CLICK/.test(r68.title) && ['next','shipped'].includes(r68.status) && P.roadmap.some(r=>r.v==='15.67' && r.status==='shipped') && later && parseFloat(later.v)>15.68 && JSON.stringify(P)===JSON.stringify(plan),'5g the plan: v15.67 shipped, v15.68 = the loop closes on the click, the candidate score moved after it; PLAN_SEED = plan.json',[r68&&r68.status, later&&later.v]);
  ok(/his machine \(the GEX nightly task\)/.test(P.stages[3].who) && /results\.py/.test(P.stages[3].what) && P.system.components.some(c=>c.id==='nightly' && /GEX nightly/.test(c.where) && /tick\.py/.test(c.name) && /results\.py/.test(c.does)) && P.system.storage.some(k=>/learning\/results\.json/.test(k.key)),'5h the architecture: stage ④ runs on his machine; the nightly component names the task, tick.py and results.py; results.json in storage');
  const md=fs.readFileSync('design/ARCHITECTURE.md','utf8');
  ok(/GEX nightly/.test(md) && /results\.py/.test(md) && /learning\/results\.json/.test(md),'5i design/ARCHITECTURE.md carries the task, results.py and results.json (generated from the same seed)');
}

// ---- 6 · the records ---------------------------------------------------------------------------------------------------------
{
  const cl=fs.readFileSync('changelog/CHANGELOG.md','utf8');
  ok(/## v15\.68/.test(cl) && cl.indexOf('## v15.68')<cl.indexOf('## v15.67'),'6a the CHANGELOG has the v15.68 entry on top');
  const ls=fs.readFileSync('session-state/LESSONS.md','utf8'); const logAt=ls.indexOf('## 2 · THE LESSON LOG');
  ok(/### v15\.68/.test(ls.slice(logAt)),'6b the lesson log carries the v15.68 entry');
  const cfg=JSON.parse(fs.readFileSync('.gex-config.json','utf8'));
  ok(JSON.stringify(cfg).includes('setup-gex-nightly.bat') && JSON.stringify(cfg).includes('tools/nightly/results.py'),'6c .gex-config.json names the nightly task and results.py');
  const pr=fs.readFileSync('design/PROCESS.md','utf8');
  ok(/GEX nightly/.test(pr) && /results\.py/.test(pr),'6d PROCESS.md ④ names the task and the registry write');
}

console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
