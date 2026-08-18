// (v10.53 E / v10.54 GROUP 5 ⑤) THE REVIEW'S OWN TEST — AND THE PANEL'S. tools/synth_day.js plants three properties at known
// strengths; docs/REVIEW-ACCEPTANCE.md states what a competent weekly review must report
// about them. This test proves the acceptance file is TRUTHFUL — that the planted edge,
// the 1-way trap and the regime split really are in the generated day, at the stated
// numbers — so a reviewer that misses them has genuinely failed, not been misled.
const fs=require('fs'); const path=require('path'); const os=require('os');
const SYN=require('./tools/synth_day.js');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m+(g!==undefined?' -> '+g:''));} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+g:''));} };

const day=SYN.build();
const DIR_PTS=SYN.DIR_PTS;

// ---------------- 1. SHAPE: it is a day file, with snap.feat ----------------
ok(day.selftest===true, '1a the file is marked selftest — it can never be mistaken for a trading day');
ok(Array.isArray(day.snaps.SPY) && day.snaps.SPY.length===120, '1b 120 bars under snaps[SYM]', day.snaps.SPY.length);
ok(day.snaps.SPY.every(s=>s.feat && typeof s.feat==='object'), '1c every snapshot carries snap.feat (the FEATURES records)');
ok(day.snaps.SPY.every(s=>s.out10 && typeof s.out10.mfe==='number' && typeof s.out10.mae==='number'),
   '1d every snapshot carries a resolved out10 {mfe,mae}');
ok(day.snaps.SPY.every(s=>s.rg && (s.rg.tag==='trend'||s.rg.tag==='chop')), '1e every bar carries its regime tag');
ok(Array.isArray(day.feat.SPY) && day.feat.SPY.length===120*4, '1f the resolved outcome queue mirrors the records', day.feat.SPY.length);
ok(day.feat.SPY.every(r=>r.rec && r.rec.regime && typeof r.rec.regime.tag==='string'
                         && typeof r.rec.regime.opex==='boolean' && typeof r.rec.regime.event==='boolean'),
   '1g EVERY record carries regime {tag,opex,event} (v10.53 C)');
ok(day.feat.SPY.every(r=>r.resolved===true && (r.hit===0||r.hit===1)), '1h every record is resolved with a 0/1 hit');

// ---------------- 2. THE BASELINE the whole thing is judged against ----------
let bn=0,bu=0,bd=0;
day.snaps.SPY.forEach(s=>{ bn++; if(s.out10.mfe>=DIR_PTS) bu++; if(s.out10.mae<=-DIR_PTS) bd++; });
const upPct=Math.round(100*bu/bn), dnPct=Math.round(100*bd/bn);
ok(upPct===30 && dnPct===70, '2a the planted tape is a DOWN day: baseline up 30% / dn 70%', upPct+'/'+dnPct);
ok(upPct+dnPct===100, '2b every bar resolves exactly one way — the baseline is well-formed');

// ---------------- helper: the same arithmetic dirFactorStats does -----------
function stat(key){
  const rows=day.feat.SPY.filter(r=>r.key===key);
  let n=0,hit=0,up=0,dn=0,tn=0,th=0,cn=0,ch=0;
  rows.forEach(r=>{
    if(r.hit==null) return;
    n++; if(r.hit) hit++;
    if(r.rec.vote>0) up++; else if(r.rec.vote<0) dn++;
    const tg=r.rec.regime.tag;
    if(tg==='trend'){ tn++; if(r.hit) th++; } else if(tg==='chop'){ cn++; if(r.hit) ch++; }
  });
  const rate=Math.round(100*hit/n);
  const expected=Math.round((up*upPct + dn*dnPct)/(up+dn));   // baseline re-weighted by the row's OWN vote mix
  return { n, rate, up, dn, expected, lift:rate-expected,
           trendRate:tn?Math.round(100*th/tn):null, trendN:tn,
           chopRate:cn?Math.round(100*ch/cn):null, chopN:cn,
           oneSided:Math.max(up,dn)/(up+dn) };
}
const E=stat('synth.edge'), T=stat('synth.trap'), R=stat('synth.regime');

// ---------------- 3. (a) THE PLANTED TRUE EDGE ------------------------------
ok(E.n===120, '3a the edge factor voted on every bar', E.n);
ok(E.rate>=73 && E.rate<=77, '3b PLANTED EDGE: ~75% hit rate', E.rate+'%');
ok(E.up===60 && E.dn===60, '3c ...on BALANCED votes (60 up / 60 down) — no one-way artifact', E.up+'/'+E.dn);
ok(E.expected>=48 && E.expected<=52, '3d ...so the re-weighted baseline is ~50%', E.expected+'%');
ok(E.lift>=20, '3e ...and the LIFT is real (>= +20)', '+'+E.lift);
ok(Math.abs(E.trendRate-E.chopRate)<=5, '3f the edge is regime-NEUTRAL (trend ~= chop)', E.trendRate+'% / '+E.chopRate+'%');

// ---------------- 4. (b) THE PLANTED 1-WAY TRAP -----------------------------
ok(T.oneSided>=0.90, '4a PLANTED TRAP: >=90% of its votes point ONE direction', Math.round(100*T.oneSided)+'%');
ok(T.dn>T.up && T.dn>=108, '4b ...and that direction is DOWN, matching the down day', T.dn+' down votes');
ok(T.rate>=60, '4c ...so its RAW rate looks respectable', T.rate+'%');
ok(Math.abs(T.lift)<=5, '4d ...but the re-weighted LIFT is ~zero — the rate is the PERIOD, not the factor', T.lift);
ok(T.rate-E.rate<0 && T.lift<E.lift-15, '4e the trap must never out-rank the true edge on lift', T.lift+' vs +'+E.lift);

// ---------------- 5. (c) THE PLANTED REGIME SPLIT ---------------------------
ok(R.trendN>=50 && R.chopN>=50, '5a both regimes carry a real sample', R.trendN+'/'+R.chopN);
ok(R.trendRate>=75 && R.trendRate<=85, '5b PLANTED SPLIT: ~80% on trend-tagged bars', R.trendRate+'%');
ok(R.chopRate>=25 && R.chopRate<=35, '5c ...and ~30% on chop-tagged bars', R.chopRate+'%');
ok(R.trendRate-R.chopRate>=40, '5d ...a gap no pooled number can survive', (R.trendRate-R.chopRate)+'pts');
ok(R.rate>=50 && R.rate<=60, '5e ...while the POOLED rate averages to a meaningless ~55%', R.rate+'%');
ok(Math.abs(R.lift)<=6, '5f ...with ~no pooled lift: the split IS the finding', R.lift);

// ---------------- 6. determinism + the acceptance doc -----------------------
const a=JSON.stringify(SYN.build()), b=JSON.stringify(SYN.build());
ok(a===b, '6a the generator is deterministic — no RNG, no clock');
const tmp=path.join(os.tmpdir(), 'gex_selftest_'+process.pid+'.json');
const wrote=(function(){ const argv=process.argv; process.argv=[argv[0],argv[1],tmp]; const p=SYN.main(); process.argv=argv; return p; })();
ok(fs.existsSync(wrote), '6b main() writes the day file to the requested path');
const onDisk=JSON.parse(fs.readFileSync(wrote,'utf8'));
ok(onDisk.snaps.SPY.length===120 && onDisk.feat.SPY.length===480, '6c ...and it round-trips through JSON intact');
try{ fs.unlinkSync(wrote); }catch(e){}

const acc=fs.readFileSync('./docs/REVIEW-ACCEPTANCE.md','utf8');
ok(/synth\.edge/.test(acc) && /synth\.trap/.test(acc) && /synth\.regime/.test(acc),
   '6d the acceptance doc names all three planted factors');
ok(/1-way/.test(acc) && /lift/.test(acc), '6e ...states the 1-way flag and the lift criterion');
ok(/80/.test(acc) && /30/.test(acc) && /75/.test(acc), '6f ...and quotes the same numbers this test just re-derived');
ok(/clearsBar/.test(acc), '6g ...and forbids a proposal clearing the bar off one synthetic day');

// ---------------- 7. (v10.54) THE PANEL RUNS THE SAME TEST, IN-PAGE ---------
// The generator above is a node script: a trader on the Skylit page could never run it,
// so "is the scorer working?" was a question only a developer could answer. v10.54
// embeds the same deterministic day in the userscript and scores it with the panel's OWN
// arithmetic behind a button in the Testing tab. If those two ever disagree, one of them
// is lying — so this file asserts they agree.
const fs2=require('fs'); const src=fs2.readFileSync('./v10.js','utf8');
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
global.DIR_PTS=0.5;
eval([ex('selfTestDay'), ex('selfTestRun')].join('\n'));

const R7=selfTestRun();
ok(R7.baseline.up===upPct && R7.baseline.dn===dnPct,
   '7a the in-page synthetic day plants the SAME baseline as tools/synth_day.js', R7.baseline.up+'/'+R7.baseline.dn);
ok(R7.checks.length===3, '7b it scores the same three planted properties', R7.checks.length);
ok(R7.checks[0].id==='edge' && R7.checks[0].pass===true, '7c the panel FINDS the planted edge', R7.checks[0].got);
ok(R7.checks[1].id==='trap' && R7.checks[1].pass===true, '7d ...FLAGS the 1-way trap', R7.checks[1].got);
ok(R7.checks[2].id==='regime' && R7.checks[2].pass===true, '7e ...and SPLITS the regime-dependent rule', R7.checks[2].got);
ok(R7.ok===true, '7f ...so the button reports the scorer OK');
ok(JSON.stringify(selfTestRun())===JSON.stringify(selfTestRun()), '7g the in-page generator is deterministic too');

// ---------------- 8. (v10.54, audit 10) THE 1-WAY RULE IS NUMERIC -----------
// The trap is the whole point: on a one-way day a factor that simply voted with the tape
// scores well for free. Up to v10.53 the flag fired only on `up===0 || dn===0`, so THIS
// trap — which carries a handful of contrary votes precisely because a real one does —
// would NOT have been flagged. It is a ratio now: >=90% one way on >=10 votes.
const TRAP=(function(){ const e=stat('synth.trap'); return e; })();
ok(TRAP.oneSided>=0.90, '8a the planted trap is >=90% one-way', Math.round(100*TRAP.oneSided)+'%');
ok(TRAP.up>0 && TRAP.dn>0, '8b ...but it is NOT 100% one-way — it has contrary votes, like a real one', TRAP.up+'/'+TRAP.dn);
ok(!(TRAP.up===0 || TRAP.dn===0), '8c ...so the pre-10.54 absolute-zero rule would have MISSED it entirely');
const R8=selfTestRun().checks[1];
ok(R8.pass===true, '8d the >=0.90 ratio rule catches it', R8.got);
ok(/% of votes are one way/.test(R8.got) && /lift/.test(R8.got), '8e ...and the reported line names both the one-way share and the lift', R8.got);
// the panel's live table must use the same rule
ok(/b\.oneSided=\(vn>=10 && b\.oneWayShare>=0\.90\)/.test(src),
   '8f dirFactorStats flags 1-way on the same >=0.90 ratio the self-test uses');
ok(/Math\.max\(up,dn\)\/vn\)>=0\.90/.test(ex('selfTestRun')), '8g ...and the self-test is not using a private rule of its own');

// ---------------- 9. (v10.54) IT IS REACHABLE FROM THE TESTING TAB ----------
ok(/data-gselftest/.test(src), '9a the self-test is behind a button, not a console call');
ok(/'⑤','SELF-TEST'/.test(src), '9b ...in section ⑤ of the Testing tab');
ok(/SELFTEST_LAST=selfTestRun\(\)/.test(src), '9c clicking it runs the scorer');
ok(/finds the planted edge/.test(src) && /flags the 1-way trap/.test(src) && /splits the regime-dependent rule/.test(src),
   '9d the three checks are named in plain language, not by key');
ok(/want:/.test(ex('selfTestHtml')), '9e ...and each states what it WANTED, so a failure is diagnosable');
ok(!/fetch|XMLHttpRequest/.test(ex('selfTestDay')), '9f the generator needs no network');
ok(!/Math\.random|Date\.now/.test(ex('selfTestDay')), '9g ...no RNG and no clock');

console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
