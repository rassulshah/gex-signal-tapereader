// (v10.54, audit 5) THE AUTO EXPORT IS THE FULL FILE, AND THE FLAG IS EVIDENCE.
//
// Two silent failures shipped together up to v10.53:
//
//  (1) repoExportDay — the AUTO exporter that runs at the close, whose output is the ONLY
//      thing git pushes and therefore the only thing the nightly review can ever read —
//      wrote {version, date, exportedAt, snaps, events}. It did NOT carry feat[] (the
//      resolved outcomes with hit / mfe / mae), act[] (the operator's takes and passes),
//      flrCeilHist, the rules the day was scored under, or the question queue. Everything
//      the manual "Save & prep review" button produced was missing from the file that
//      actually left the machine, so the review had nothing to measure.
//
//  (2) repoAutoExportTick wrote gpts_last_export BEFORE attempting the export ("set
//      first: never double-fire"), so a write that threw left the flag behind and the
//      panel reported a day as saved that had never reached disk.
//
// Also: the export version string was frozen at '10.15' for 39 releases.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m+(g!==undefined?' -> '+g:''));} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+g:''));} };

// ---------------- mocks ----------------
let LS={};
global.localStorage={ getItem:k=>(k in LS?LS[k]:null), setItem:(k,v)=>{LS[k]=String(v);}, removeItem:k=>{delete LS[k];} };
global.window={ __gptsDebug:{} };
global.TODAY='2026-08-18';
global.RECORDER_SYMS=['SPY','QQQ'];
global.FEAT_FWD=10;
global.GM_info={ script:{ version:'10.55' } };
global.FEATURES=[{key:'dir',label:'Direction grade',phase:'dashboard',fwd:10,rule:{id:'dir'}}];
global.registerCoreFeatures=function(){ return FEATURES; };
global.seedQuestions=function(){ return [{id:'dir_A_follow', feature:'dir'}]; };
global.rulesLoad=function(){ return { 'dir':{ id:'dir', tier:'hand' } }; };
global.rulesDoc=function(){ return { asOf:'2026-08-17', schema:'gex-rules/v2' }; };
global.modelStamp=function(){ return { rulesAsOf:'2026-08-17', weightsHash:'abc123' }; };
global.projScorecard=function(){ return { days:1, covN:0, etaErrs:[], succN:0, pinN:0, reachN:0 }; };
global.projRecs=function(){ return []; };
global.fcHistOf=function(s){ return [{ t:1, flr:772, ceil:778 }]; };
// the day the recorder holds: snaps + the four things the auto export was dropping
const DAY={
  snaps:{ SPY:[{ t:1, bar:0, px:773, h:773.4, l:772.6 }] },
  events:{ SPY:[{ t:1, kind:'defl' }] },
  feat:{ SPY:[{ key:'dir', t:1, bar:0, n:1, px:773, session:'morning',
                rec:{ grade:'B', verdict:'UP', tgt:776, inval:772 },
                hit:1, drift:1, frame:{ tgtHit:true, invalHit:false, first:'tgt', rr:3 },
                mfe:0.9, mae:-0.1, resolved:true, partial:false }] },
  act:{ SPY:[{ t:1, sym:'SPY', k:773, cell:'B×A', action:'take' }] }
};
global.recorderLoad=function(){ return { days:{ '2026-08-18':DAY } }; };

eval(['VERSION_STR','buildDayExport'].map(ex).join('\n'));

// ================= 1. buildDayExport CARRIES EVERYTHING =====================
const P=buildDayExport('2026-08-18');
ok(P.schema==='gex-day-export/v1', '1a it is a day export', P.schema);
ok(P.date==='2026-08-18', '1b for the requested date', P.date);
ok(P.snaps && P.snaps.SPY && P.snaps.SPY.length===1, '1c snaps');
ok(P.events && P.events.SPY, '1d events');
ok(P.feat && P.feat.SPY && P.feat.SPY.length===1, '1e feat[] — the resolved outcome queue');
const fr=P.feat.SPY[0];
ok(fr.hit===1 && fr.mfe===0.9 && fr.mae===-0.1 && fr.resolved===true,
   '1f ...with hit / mfe / mae / resolved on every record, which is what makes it measurable');
ok(fr.frame && fr.frame.first==='tgt' && fr.frame.rr===3,
   '1g ...and the v10.54 FRAME outcome (target before invalidation, with its R:R)');
ok(fr.drift===1, '1h ...beside the 0.5-pt drift hit, not instead of it');
ok(P.act && P.act.SPY && P.act.SPY.length===1, '1i act[] — the operator’s takes and passes');
ok(P.flrCeilHist && P.flrCeilHist.SPY && P.flrCeilHist.QQQ, '1j flrCeilHist for every recorded symbol');
ok(Array.isArray(P.features) && P.features.length===1, '1k the feature registry, so a consumer knows what the keys mean');
ok(P.rules && P.rules['dir'], '1l the rule map the day was scored under');
ok(Array.isArray(P.questions), '1m the question queue');
ok(P.rulesAsOf==='2026-08-17', '1n (v10.54) WHICH rules document — a file with no asOf cannot be audited', P.rulesAsOf);
ok(P.model && P.model.weightsHash, '1o (v10.54) and the model stamp {rulesAsOf, weightsHash}');
ok(P.effN && P.effN.fwd===10 && /effective observations/.test(P.effN.note),
   '1p (v10.54) the forward window, so a consumer can re-derive effective n rather than counting bars');

// ================= 2. THE VERSION STRING IS REAL ============================
ok(P.version==='10.55', '2a the export version is the running version, not the frozen "10.15"', P.version);
ok(!/version:'10\.15'/.test(src), '2b the hardcoded 10.15 is gone from source');
ok(/version:VERSION_STR\(\)/.test(ex('buildDayExport')), '2c ...replaced by VERSION_STR()');

// ================= 3. THE AUTO EXPORTER WRITES THAT FILE ====================
const RED=ex('repoExportDay');
ok(/payload=buildDayExport\(date\)/.test(RED), '3a repoExportDay builds the FULL file');
ok(/payload\.snaps=day\.snaps/.test(RED), '3b ...then overlays the richer IndexedDB snaps (unbounded, all symbols)');
ok(/payload\.rulesAsOf=/.test(RED) && /payload\.model=/.test(RED), '3c ...and stamps which model wrote it');
ok(!/var payload=\{version:VERSION_STR\(\), date:date, exportedAt/.test(RED),
   '3d the old snaps-and-events-only payload is no longer the primary path');
ok(/catch\(eB\)/.test(RED), '3e ...but a throw still falls back to a minimal file rather than exporting nothing');

// ================= 4. THE FLAG IS SET ONLY ON SUCCESS =======================
const TICK=ex('repoAutoExportTick');
ok(!/localStorage\.setItem\('gpts_last_export', TODAY\)/.test(TICK),
   '4a repoAutoExportTick NO LONGER writes the flag before attempting the export');
ok(/REPO_AUTO_TRIED===TODAY/.test(TICK) && /REPO_AUTO_TRIED=TODAY/.test(TICK),
   '4b the one-attempt guard is in MEMORY, so a failure is not remembered as a success');
// the only writers of the flag are the two success callbacks
const writers=(src.match(/localStorage\.setItem\('gpts_last_export'/g)||[]).length;
ok(writers===2, '4c exactly two writers of gpts_last_export remain: the folder-write and the download callbacks', writers);
ok(/w\.close\(\); \}\); \}\)\.then\(function\(\)\{ REPO_LAST_SAVE=[\s\S]{0,120}gpts_last_export/.test(src),
   '4d the folder write sets it inside .then() — after the write resolved');
ok(/a\.click\(\)[\s\S]{0,200}gpts_last_export/.test(src), '4e the download sets it after the click');
// and saveState turns an unbacked flag into pending / failed rather than "saved"
const SS=ex('saveState');
ok(/code:\(pend\?'pending':'failed'\)/.test(SS),
   '4f a flag with no confirming pipeNoteSave record reads PENDING (in flight) or FAILED — never "saved"');

// ================= 5. THE FEATURE CAP CANNOT EAT THE MORNING ================
// (audit 16) With 18 enrolled features a flat 1200-RECORD cap was ~66 bars: on any normal
// session the recorder silently discarded the whole morning, and every "since the open"
// statistic in the export was computed on an afternoon-only sample.
const FE=ex('featEnqueue');
ok(!/arr\.length>1200/.test(FE), '5a the flat 1200-record cap is gone');
ok(/FEAT_KEEP_BARS/.test(FE) && /barList\.length-FEAT_KEEP_BARS/.test(FE) && !/maxBar-FEAT_KEEP_BARS/.test(FE), '5b the cap is by DISTINCT BAR VALUES (v11.1.2: bar is a ms timestamp, so maxBar-160 was 160 ms)');
ok(/var FEAT_KEEP_BARS = 160/.test(src), '5c ...160 bars, which is a full session for EVERY feature');
ok(/typeof r\.bar!=='number' \|\| r\.bar>=cutoff/.test(FE), '5d ...and records with no bar index are kept, not silently dropped');

// ================= 6. TREND_LAST SURVIVES A RELOAD ==========================
// (audit 15) TREND_LAST is the ONLY memory the five-state trend machine has, and it lived
// in RAM alone: any page reload wiped it, so a genuinely BROKEN uptrend came back as plain
// 'flat' and the whole up-broken / dn-broken distinction — the mandatory middle step
// before a reversal — silently vanished mid-session.
global.TREND_LAST={ SPY:null, QQQ:null };
global.TRENDLAST_KEY='gpts_trendlast_v1';
global.ctTodayStr=function(){ return '2026-08-18'; };
eval([ex('trendLastSave'), ex('trendLastLoad')].join('\n'));
LS={};
TREND_LAST.SPY='up'; TREND_LAST.QQQ='dn';
trendLastSave();
ok(!!LS['gpts_trendlast_v1'], '6a the confirmed-trend memory is persisted');
ok(/gpts_trendlast_v1/.test(src), '6b ...under the one NEW key this release adds');
TREND_LAST={ SPY:null, QQQ:null };
ok(trendLastLoad()===true, '6c a fresh boot rehydrates it');
ok(TREND_LAST.SPY==='up' && TREND_LAST.QQQ==='dn', '6d ...so a BROKEN trend is still recognised as broken, not as flat', TREND_LAST.SPY+'/'+TREND_LAST.QQQ);
// a NEW day must start with no memory: yesterday's trend must never leak in
TREND_LAST={ SPY:null, QQQ:null };
global.ctTodayStr=function(){ return '2026-08-19'; };
ok(trendLastLoad()===false, '6e a NEW trading day starts with no memory');
ok(TREND_LAST.SPY===null, '6f ...yesterday’s confirmed trend does not leak into today', ''+TREND_LAST.SPY);
// only the two legal values are accepted
global.ctTodayStr=function(){ return '2026-08-18'; };
LS['gpts_trendlast_v1']=JSON.stringify({ date:'2026-08-18', last:{ SPY:'sideways', QQQ:'up' } });
TREND_LAST={ SPY:null, QQQ:null };
trendLastLoad();
ok(TREND_LAST.SPY===null && TREND_LAST.QQQ==='up', '6g a corrupt value is refused, a valid one is taken', TREND_LAST.SPY+'/'+TREND_LAST.QQQ);
LS['gpts_trendlast_v1']='{not json';
TREND_LAST={ SPY:null, QQQ:null };
ok(trendLastLoad()===false && TREND_LAST.SPY===null, '6h ...and a corrupt store fails soft, it never throws');
// it is written when the state CONFIRMS, and rehydrated at boot
ok(/TREND_LAST\[sym\]='up'; trendLastSave\(\)/.test(src), '6i trendVerdict persists a newly confirmed trend');
ok(/trendLastLoad\(\);\s*\/\/ \(v10\.54\)/.test(src), '6j boot() rehydrates it');

console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
