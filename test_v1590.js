// test_v1590.js — (v15.90) THE LEARNING PROCESS, HARDENED + THE LIST ATLAS DRAWS.
// Operator, 2026-09-09: "yes, fix everything and make sure it all makes sense and is integrated and working together and
// aligned to my objectives/purpose. make sure you fix any and all issues." · on the ES1 chart's "7658 — 100%" the file did
// not carry: "we need to match atlas … FRONT".
// §1 the D rows (the merged derived list Atlas draws, FRONT window) · §2 the sweep corpus appends · §3 the registry is the
// machine's (needs / have / ETA, machine-written results, the level-name collapse, the cuts) · §4 the docs and records.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);if(!m)throw new Error('no fn '+n);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}

ok(/@version\s+15\.9\d/.test(src) && /var GPTS_VERSION='15\.9\d';/.test(src), '0a v15.90 or later in both spots');

// ---------- 1. D-KING · D2..D5 — the list Atlas draws on the futures chart ----------
{
  ok(/^var LASTFUTDER_FRONT = \{\};/m.test(src), '1a LASTFUTDER_FRONT declared bare');
  global.LASTSPXW=null; global.LASTFUTDER={}; global.LASTFUTDER_FRONT={}; global.SYM_SEEN={}; global.LASTDISP={}; global.LASTVEX={}; global.LASTFEED={}; global.FEED_REJECTS={SPY:{n:0},QQQ:{n:0}};
  global.FUTDER_STALE_MS=300000;
  eval(['feedNewestT','futDerNewestT','futDerIsFront','futDerFrontList','onFeed'].map(ex).join('\n'));
  // the 2026-09-09 shape: ES has no book; derived[] per source; snapshot.slices = ONE merged list per expiration
  const front=(t)=>({ success:true, levels:[], level_count:0, expirations:['2026-09-09'], strike_interval:2,
    derived:[
      { source:'SPY',  ratio:10.0264, levels:[{ t:t, s:7655.55, l:[{k:7650.1,v:19860000,d:-1,net:-19860000},{k:7660.2,v:14300000,d:-1,net:-14300000},{k:7670.2,v:13200000,d:1,net:13200000}] }] },
      { source:'SPXW', ratio:1.00071, levels:[{ t:t, s:7655.61, l:[{k:7655.4,v:25878000,d:-1,net:-25878000},{k:7660.4,v:8500000,d:-1,net:-8500000}] }] } ],
    snapshot:{ t:t, s:7655.6, pc:7681, slices:[{ exp:'2026-09-09', l:[
      {k:7658,v:34000000,d:-1,net:-34000000},{k:7650,v:19860000,d:-1,net:-19860000},{k:7660,v:14300000,d:-1,net:-14300000},{k:7670,v:13200000,d:1,net:13200000},{k:7640,v:6000000,d:1,net:6000000},{k:7630,v:1000000,d:1,net:1000000} ] }] } });
  const next2=(t)=>Object.assign(front(t), { expirations:['2026-09-09','2026-09-10'], snapshot:{ t:t, s:7655.6, slices:[{exp:'2026-09-09',l:[{k:7658,v:1,d:1,net:1}]},{exp:'2026-09-10',l:[{k:7700,v:99,d:1,net:99}]}] } });
  onFeed('ES1','gamma',next2(1788963300));
  ok(LASTFUTDER.ES1 && !LASTFUTDER_FRONT.ES1, '1b a two-expiration payload (the app\'s next-2 request) is kept for the ratio, NOT as the front list');
  onFeed('ES1','gamma',front(1788963360));
  ok(LASTFUTDER_FRONT.ES1 && LASTFUTDER_FRONT.ES1.j.expirations.length===1, '1c the one-expiration payload (FRONT, his selector) is the front list');
  onFeed('ES1','gamma',front(1788960000));
  ok(LASTFUTDER_FRONT.ES1.j.snapshot.t===1788963360, '1d a historical front payload never overwrites live');
  const noExp=(t)=>{ const j=front(t); delete j.expirations; return j; };
  onFeed('ES1','gamma',noExp(1788963420), true);
  ok(LASTFUTDER_FRONT.ES1.j.snapshot.t===1788963420, '1e no expirations field: one slice, or the panel\'s own self-fetch (current/1), counts as front');
  const D=futDerFrontList('ES1');
  ok(D.rows.length===6 && D.rows[0].k===7658 && D.rows[0].pct===-100 && D.rows[1].k===7650 && D.rows[1].pct===-58 && D.rows[2].k===7660 && D.rows[2].pct===-42 && D.rows[3].k===7670 && D.rows[3].pct===39,
     '1f the merged slice ranked by |dollars|, % against its own largest, signed — 7658 −100 · 7650 −58 · 7660 −42 · 7670 +39 (his chart, 2026-09-09)', D.rows.slice(0,4));
  ok(/snapshot slice 2026-09-09/.test(D.why), '1g …and says which slice it read', D.why);
  const noSl=(t)=>{ const j=front(t); delete j.snapshot; return j; };
  LASTFUTDER_FRONT.ES1={ j:noSl(1788963480), ts:Date.now() };
  const P=futDerFrontList('ES1');
  ok(P.rows.length===5 && P.rows[0].k===7655.4 && P.rows[0].pct===-100 && P.rows[0].src==='SPX' && P.rows[1].k===7650.1 && P.rows[1].src==='SPY' && /pooled/.test(P.why), '1h no slices: the derived books pooled (each already at the chart\'s price), ranked together', P.rows.slice(0,2));
  LASTFUTDER_FRONT.ES1.ts=Date.now()-301000;
  ok(futDerFrontList('ES1').rows.length===0 && futDerFrontList('ES1').stale===true, '1i a front payload older than five minutes → no rows (the export holds the day\'s last)');
  ok(futDerFrontList('NQ1').rows.length===0 && /no front-window payload/.test(futDerFrontList('NQ1').why), '1j no payload → no rows, said');
  // the export block
  const csv=ex('irtBuildCsv');
  const iD=csv.indexOf("2d) D-KING"), iS=csv.indexOf("IRT_LAST.sWhy=sWhy"), iT=csv.indexOf('ES/ETF target expansion');
  ok(iD>iS && iD<iT && /var DL=futDerFrontList\(SKY_ES\);/.test(csv) && /var kept=futDerDedupe\(DL\.rows\.slice\(0,5\), have\);/.test(csv), '1k the ES D rows come after the SG rows and before the target expansion, read from the FRONT list (v15.91: deduped against the rows already there)');
  ok(/col:futDerCol\(g, isK\), w:\(isK\?2:1\), style:0, dOnly:true/.test(csv) && /dking: irtColor\(255,255,255\)/.test(src) && /dnode: irtColor\(190,198,208\)/.test(src), '1l a KING row width 2, the rest width 1, solid, futures-only; the colours by the book\'s polarity (v15.91), greys when no book resolved');
  ok(/if\(R2\.dOnly && !\(T2\.mul>1\)\) return;/.test(csv), '1m a merged ES row is never written for an ETF target (no ETF-space price exists)');
  ok(/irtGLatch\(ds\.map\(function\(g\)\{ return \{ k:g\.k, lbl:g\.lbl, spx:g\.k, pct:g\.pct \}; \}\), 'DE'\)/.test(csv) && /HD=irtGHeld\('DE'\)/.test(csv) && /\^D-\[A-Z\]\+ KING\$\|\^D-\[A-Z\]\+\[2-9\]\$\|\^D\[1-9\]\$/.test(ex('irtGHeld')), '1n held day-scoped under DE like the G rows; the hold accepts the D labels (v15.91: D-<BOOK> KING · D-<BOOK>n · Dn)');
  ok(/var DQ=futDerFrontList\(SKY_NQ\);/.test(csv) && /out\.push\(irtCsvRow\(nqSym, g\.k, g\.lbl\+pctTag\(g\.pct\), futDerCol\(g, isK\), \(isK\?2:1\), 0\)\); nqN\+\+;/.test(csv) && /irtGHeld\('DQ'\)/.test(csv) && /var keptQ=futDerDedupe\(DQ\.rows\.slice\(0,5\), haveQ\);/.test(csv), '1o the NQ chart gets its own D rows from NQ1\'s front list, deduped against the QQQ rows, held under DQ');
  ok(/IRT_LAST\.dWhy=dWhy/.test(csv) && /IRT_LAST\.dqWhy=dqWhy/.test(csv), '1p the preview says why (dWhy / dqWhy)');
  ok(/window\.__gptsDebug\.futDerRows=function\(sym\)/.test(src) && /o\.frontSlices=/.test(src) && /o\.frontExpirations=/.test(src), '1q __gptsDebug.futDerRows(sym): the payload\'s rows, slices and expirations — the numbers to hold against his chart');
  ok(/futDerIsFront\(j, viaSelf\)/.test(ex('onFeed')) && /LASTFUTDER_FRONT\[sym\]=\{ j:j, ts:Date\.now\(\) \}/.test(ex('onFeed')), '1r the observer keeps the front payload beside the freshest-any one');
}

// ---------- 2. the sweep corpus appends ----------
{
  const { execSync } = require('child_process');
  const run=(cmd)=>{ try{ return execSync(cmd,{encoding:'utf8',stdio:['ignore','pipe','pipe']}); }catch(e){ return 'FAILED '+(e.stdout||'')+(e.stderr||''); } };
  ok(/append-futures selftest ok/.test(run('python3 tools/append-futures.py --selftest')), '2a append-futures: the nights keyed by the session day, the post-close dropped, Sunday → Monday, idempotent (selftest)');
  ok(/study-sweeps selftest ok/.test(run('python3 tools/study-sweeps.py --selftest')), '2b study-sweeps: vendor + every CSV, one source per day, the gap rule both ways (selftest)');
  const ap=fs.readFileSync('./tools/append-futures.py','utf8'), sw=fs.readFileSync('./tools/study-sweeps.py','utf8'), hl=fs.readFileSync('./tools/study-hodlod.py','utf8'), rp=fs.readFileSync('./tools/nightly/run.py','utf8');
  ok(/def session_day_of\(dt\)/.test(ap) && /def harvest\(paths, night=False\)/.test(ap) && /suffix='-night'/.test(ap), '2c the night harvest and the -night file');
  ok(/def sources\(market='ES'\)/.test(sw) && /glob\.glob\(os\.path\.join\('data', 'futures', market, '\*\.csv'\)\)/.test(sw) && /def load_all\(paths\)/.test(sw) && /gap_ok = \(prev_d is not None\) and/.test(sw) && /days <= 4/.test(sw), '2d sources(), load_all over many files, the four-day gap rule');
  ok(/if not p\.endswith\('-night\.csv'\)/.test(hl), '2e study-hodlod skips the night files (its sessions are RTH)');
  ok(/srcs = sw\.sources\('ES'\)/.test(rp) && /res = sw\.run\(srcs\)/.test(rp) && /ap\.harvest\(paths, night=True\)/.test(rp) && /suffix='-night'/.test(rp), '2f the nightly reads every source for H7 and refreshes SWEEPS.json from them; the nights are appended after the RTH');
  { const iF=rp.indexOf('futures = refresh_futures(days)'), iJ=rp.indexOf("verdicts = [judge(H, eps, defl, null['p95'], since=frm)"), iS=rp.indexOf('if write: refresh_sweeps()'), iC=rp.indexOf('_cv.write(ROOT)');
    ok(iF>0 && iF<iJ && iJ<iS && iS<iC, '2f2 the ORDER: the corpus appends before any judge reads it, the sweep tables after the append, the coverage count last (his first v15.90 run read 284 because the sweeps ran before the append)', [iF,iJ,iS,iC]); }
  const SW=JSON.parse(fs.readFileSync('./data/es-1min/SWEEPS.json','utf8'));
  ok(SW.corpus.sessions>=290 && SW.corpus.sources && SW.corpus.sources.vendor===284 && SW.corpus.sources.yahoo>=6 && SW.corpus.yahooFirst==='2026-08-31' && SW.corpus.last>='2026-09-08', '2g the committed SWEEPS.json: 290+ sessions = 284 vendor + the couriered nights since 08-31', SW.corpus);
  ok(fs.existsSync('./data/futures/ES/2026-09-08-night.csv') && fs.readFileSync('./data/futures/ES/2026-09-04-night.csv','utf8').split('\n').length>900, '2h the night files exist (09-04: a full night, 900+ minutes)');
  // H7 withdrawn, H5 judged by its join
  ok(/if H\.get\('withdrawn'\):/.test(rp) && /verdict='withdrawn'/.test(rp) && /def judge_h5\(H, days, sym='SPY'\)/.test(rp) && /judge_h5\(H, days\) if \(H\.get\('pick'\) == 'defl' and H\.get\('blocked'\) and not H\.get\('withdrawn'\)\)/.test(rp), '2i the nightly: a withdrawn row is said, never counted; H5 is judged by its join after the day files');
  const REG=JSON.parse(fs.readFileSync('./learning/register.json','utf8'));
  const H7=REG.hypotheses.find(h=>h.id==='H7'), H5=REG.hypotheses.find(h=>h.id==='H5');
  ok(H7 && H7.withdrawn && H7.withdrawn.on==='2026-09-09' && /F-23/.test(H7.withdrawn.why) && H7.predict==='printed > 24%' && H7.since==='2026-08-21', '2j register.json: H7 withdrawn with the date and the reason — its claim, prediction and since untouched');
  ok(/id:'H7'[^\n]*withdrawn:\{ on:'2026-09-09'/.test(src), '2k PREREG_SEED carries the withdrawal (one register, not two)');
  const LOG=JSON.parse(fs.readFileSync('./learning/log/2026-09-08.json','utf8'));
  const v7=(LOG.hypotheses||[]).find(h=>h.id==='H7'), v5=(LOG.hypotheses||[]).find(h=>h.id==='H5');
  ok(v7 && v7.verdict==='withdrawn' && v5 && v5.verdict==='blocked' && typeof v5.extremes==='number' && /coincide with a ledger tap/.test(v5.bar), '2l the committed log: H7 WITHDRAWN, H5 BLOCKED with the join\'s own n (extremes, at a node), never "ready"', [v7&&v7.verdict, v5&&v5.bar]);
  ok(LOG.feats && LOG.feats['dir.kingRoll'] && LOG.feats['dir.kingRoll'].n>=170 && /def feat_outcomes\(days, keys, sym='SPY'\)/.test(rp), '2m the log carries the feature outcomes (dir.kingRoll, gateHour) for the registry');
  // the panel's register row
  const reg=ex('preregHtml');
  ok(/else if\(H\.withdrawn\)\{ n=0; status='WITHDRAWN'/.test(reg) && /else if\(H\.blocked\)\{ n=\(hv&&hv\.n!=null\)\?hv\.n:0; status='BLOCKED'/.test(reg) && !/the join can be run/.test(reg), '2n Testing ②: WITHDRAWN rendered; a blocked row shows the JOIN\'s n from the nightly, never the ledger\'s size as "ready"');
}

// ---------- 3. the registry is the machine's output ----------
{
  const ST=JSON.parse(fs.readFileSync('./learning/studies.json','utf8'));
  const flat=ST.subjects.flatMap(sj=>sj.subsections.flatMap(ss=>ss.studies));
  ok(flat.length===184 && flat.every(x=>x.needs && typeof x.needs.corpus==='string' && typeof x.needs.n==='number'), '3a 184 studies (195 − 13 level names + H2.L + H2.10), every one with a machine-readable need');
  const bs=ST.counts.byStatus;
  ok(!bs['READ NEXT'] && (bs['OPEN']||0)<=1 && bs['WAITING']>=100 && bs['READY']>=10 && bs['NULL']===2 && bs['CUT']===2, '3b the statuses are the machine\'s: WAITING / READY replace OPEN / READ NEXT (one live-only row stays OPEN)', bs);
  const L=flat.find(x=>x.id==='H2.L');
  ok(L && L.status==='NULL' && /collapsed v15\.90/.test(L.was) && /^NO: every named level/.test(L.result) && /named levels: /.test(L.nightly) && /produce by chance/.test(L.nightly) && L.machine.read==='levelTable', '3c H2.L: the thirteen level-name studies are one NULL row — the review\'s reason as its result, the machine\'s table under it');
  ok(!flat.some(x=>/^H2\.(1|2|3|10[bcdehij]|10j2|10j3)$/.test(x.id)), '3d …and the thirteen are gone from the registry');
  const pdc=flat.find(x=>x.id==='H2.10');
  ok(pdc && pdc.status==='READ' && /^PDC- 29% n=119 vs 21% \(\+8pp, clears the control\)/.test(pdc.result) && pdc.by==='nightly', '3e PDC keeps its own row — the one name whose CI clears the control, machine-written');
  const h24=flat.find(x=>x.id==='H2.4'), h25=flat.find(x=>x.id==='H2.5');
  ok(h24.by==='nightly' && /^ON\/PD sweep-reclaims by the clock: 08:30-09:00 21% n=25\d vs 18%/.test(h24.result) && !/\+5 to \+15pp/.test(h24.result), '3f H2.4\'s sentence is the file\'s, not the withdrawn +5 to +15pp');
  ok(h25.by==='nightly' && /^deep \(> 8 pts\) 35% n=9\d vs 23%/.test(h25.result) && !/40% n=86/.test(h25.result), '3g H2.5\'s sentence is the file\'s, not the withdrawn 40% n=86');
  ok(['S1.4','S7.3'].every(id=>{ const x=flat.find(y=>y.id===id); return x && x.status==='CUT' && /nodeType/.test(x.result); }), '3h S1.4 / S7.3 CUT — a field no feed carries');
  const d33=flat.find(x=>x.id==='D3.3');
  ok(d33.status==='READ' && d33.by==='nightly' && /^dir\.kingRoll right within 10 bars: \d+ \/ \d+ = \d+% \(low \d+%\) on \d+ sessions — per-bar rows/.test(d33.result), '3i D3.3 read by the machine from the day files\' feature rows');
  const w=flat.find(x=>x.id==='K1.4'), r=flat.find(x=>x.id==='K4.1'), b=flat.find(x=>x.id==='K2.4');
  ok(w.status==='WAITING' && /the tap record: 0 of 40 taps — not recorded yet \(v15\.91\)/.test(w.nightly) && r.status==='READY' && /kingroll: \d+ rows on hand since 2026-08-19 — no reader yet/.test(r.nightly) && b.status==='WAITING' && /book: 13 of 30 sessions · ~17 sessions at 1\.0\/session/.test(b.nightly) && b.result==='11 sessions', '3j WAITING with have / need / ETA, READY with "no reader yet" — the review\'s sentence kept beside them');
  const h13=flat.find(x=>x.id==='H1.3'), h28=flat.find(x=>x.id==='H2.8'), h27=flat.find(x=>x.id==='H2.7');
  ok(h13.status==='REGISTERED' && /^H5 blocked: n=0 of 50/.test(h13.nightly) && !/the join can be run/.test(h13.result) && h28.status==='NULL' && /WITHDRAWN 2026-09-09/.test(h28.result) && h27.status==='REGISTERED', '3k H1.3 / H2.7 / H2.8 say what the register says (H5 blocked by its join, H6 registered, H7 withdrawn)');
  const CV=JSON.parse(fs.readFileSync('./learning/coverage.json','utf8'));
  ok(CV.counts && CV.counts.price.have>=295 && CV.counts.sweeps.have>=290 && CV.counts.book.have>=13 && CV.counts.ledger.have>=140 && CV.counts.kingroll.have>=170 && CV.counts.tap.have===0 && /not recorded yet/.test(CV.counts.tap.note) && CV.counts.vix.have===0, '3l coverage.json counts every corpus the registry names (tap 0 — not recorded yet; vix 0 — no file)', Object.keys(CV.counts));
  ok(CV.studies.byCorpus.tap && CV.studies.byCorpus.tap.studies===104 && CV.studies.byCorpus.tap.waiting>=85, '3m …and groups the studies by their need (104 on the tap record, 85+ WAITING; the rest THIN on the ledger or REGISTERED meanwhile)', CV.studies.byCorpus.tap);
  const rs=fs.readFileSync('./tools/nightly/results.py','utf8');
  ok(/def sweep_results\(studies, sw, asof\)/.test(rs) && /def feat_results\(studies, log, asof\)/.test(rs) && /def corpus_status\(studies, counts, asof, answered=\(\)\)/.test(rs) && /keepResult/.test(rs) && /import coverage as _cov/.test(rs), '3n results.py: sweep sentences, feature reads, corpus statuses, keepResult');
  const { execSync } = require('child_process');
  let rso=''; try{ rso=execSync('python3 tools/nightly/results.py --selftest',{encoding:'utf8'}); }catch(e){ rso='FAILED'; }
  ok(/results\.py selftest ok/.test(rso) && /the machine.s statuses/.test(rso), '3o results.py selftest covers the new machinery');
  const seed=fs.readFileSync('./tools/studies-seed.py','utf8');
  ok(/def needs_of\(corpus, status\)/.test(seed) && /machine=None, needs=None/.test(seed) && /st\("H2\.L"/.test(seed) && /"CUT", "CUT 2026-09-09/.test(seed), '3p the seed derives needs from the corpus sentence; H2.L and the cuts live in the seed (never the JSON by hand)');
  // the panel
  ok(/'WAITING':'#8b98a5', 'READY':'#7cc7ff', 'NULL':'#6c7889', 'CUT':'#6c7889', 'WITHDRAWN':'#6c7889'/.test(src), '3q the Analysis tab colours the machine\'s statuses');
  const dn=ex('dataNeeds');
  ok(/var K=\(CV&&CV\.counts\)\|\|\{\}; var ST=null; try\{ ST=studiesLoad\(\); \}/.test(dn) && /x\.needs&&x\.needs\.corpus/.test(dn) && /waiting · ready · read/.test(dn) && /~'\+Math\.ceil\(\(g\.need-c\.have\)\/c\.perSession\)\+' sessions'/.test(dn), '3r the Data tab ④ reads the rows\' needs and the file\'s counts — have · need · ETA · waiting · ready · read');
}

console.log('test_v1590: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
