global.GPTS_VERSION='11.0';
// (v10.52) PIPELINE INDICATOR — the end-to-end path a day's data travels:
//   recorded -> saved into the repo folder -> pushed to GitHub -> nightly review back.
//
// What these tests pin:
//   1. saveState() classifies saved / download / none, and compares against the
//      CHICAGO trading date. A UTC clock that has already rolled to tomorrow must
//      still read today's CT export as `saved` — the exact false "not exported"
//      reading observed on 2026-08-17.
//   2. pipeCheck() does at most ONE round of remote checks per 10 minutes (cached in
//      gpts_pipe_v1), skips a hidden tab, and FAILS SOFT: a rejected fetch leaves the
//      stage 'unknown' and never throws.
//   3. The four stage states map to the four colours (green/amber/red/grey).
//   4. The rendered indicator is ONE line and carries all four stage keys/labels.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m+(g!==undefined?' -> '+g:''));} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+g:''));} };

// ---------------- mocked globals the extracted functions reference ----------------
global.PAL={ longAccent:'#2ec27e', shortAccent:'#f0616d', amber:'#f2b45a', sub:'#8b98a9',
             line:'#1e2530', ink:'#e6edf3', card:'#12161f', blue:'#4a90d9', gold:'#e3c341' };
global.mul=function(a,b){ return a/(1/b); };
global.PIPE_RAW_BASE='https://raw.githubusercontent.com/rassulshah/gex-signal-tapereader/main';
global.PIPE_KEY='gpts_pipe_v1';
global.PIPE_TTL_MS=600000;
global.PIPE_REC_MS=360000;
global.REPO_LAST_SAVE=null;
global.RECORDER={ _lastSnapBar:{}, _lastSnapT:null };
global.ANALYSIS_REVIEW=null;
global.LASTVEX={ SPY:null };
global.LASTFEED={ SPY:null };
global.LASTDISP={ SPY:null };
global.FEED_STALE_MS=12000;
global.TODAY=null;
global.tapeMap=function(){ return null; };
let RENDERS=0;
global.render=function(){ RENDERS++; };
global.document={ visibilityState:'visible' };

// in-memory localStorage
let LS={};
global.localStorage={ getItem:k=>(k in LS?LS[k]:null), setItem:(k,v)=>{LS[k]=String(v);}, removeItem:k=>{delete LS[k];} };

// controllable clock: FAKE_NOW is the real UTC instant "now" is pinned to.
let FAKE_NOW=null;
const RealDate=Date;
global.Date=class extends RealDate{
  constructor(...a){ if(a.length===0 && FAKE_NOW!==null) super(FAKE_NOW); else super(...a); }
  static now(){ return FAKE_NOW!==null?FAKE_NOW:RealDate.now(); }
};

// fetch is swapped per-test
let FETCHES=[];
let FETCH_IMPL=()=>Promise.reject(new Error('no impl'));
global.fetch=function(u,o){ FETCHES.push(u); return FETCH_IMPL(u,o); };
global.window={ __gptsDebug:{} };

function two(n){ return (n<10?'0':'')+n; }
global.two=two;

eval(['ctNow','ctDateStr','ctTodayStr','pipeLoad','pipeSave','pipeRender','pipeNoteSave',
      'saveState','lastTradingDay','pipeFetch','pipeReviewLine','pipeReviewTry','pipeCheck',
      'pipeColor','pipeStages','feedStatusHtml'].map(ex).join('\n'));

function reset(){ LS={}; FETCHES=[]; RENDERS=0; global.REPO_LAST_SAVE=null; global.ANALYSIS_REVIEW=null; global.REPO_AUTO_TRIED=null; }
const tickMicro=()=>new Promise(r=>setTimeout(r,0));

// ============================================================================
// 1. saveState() CLASSIFICATION
// ============================================================================
// Anchor the clock at 2026-08-17 11:00 CT (16:00 UTC): CT date == UTC date here, so
// this block isolates classification from the timezone question.
FAKE_NOW=RealDate.parse('2026-08-17T16:00:00Z');
reset();
ok(ctTodayStr()==='2026-08-17', '1a CT trading date is 2026-08-17', ctTodayStr());

var s1=saveState();
ok(s1.code==='none', '1b nothing exported -> none', s1.code);

LS['gpts_last_export']='2026-08-16';
ok(saveState().code==='none', '1c yesterday’s export does not count as today', saveState().code);

// (v10.54, audit 5) THE ORPHAN FLAG. gpts_last_export alone is INTENT, not evidence:
// before 10.54 repoAutoExportTick wrote it BEFORE attempting the export "so it can never
// double-fire", so a write that threw left the flag behind and the panel reported a day
// as saved that had never reached disk. A flag with no confirming pipeNoteSave record is
// now FAILED (red), or PENDING (amber) while this session's attempt is still in flight.
LS['gpts_last_export']='2026-08-17';
var s2=saveState();
ok(s2.code==='failed', '1d a flagged day with NO confirmed write -> failed, never "saved"', s2.code);
ok(s2.name==='2026-08-17.json', '1d2 ...and still names the file it should have been', s2.name);
global.REPO_AUTO_TRIED='2026-08-17';
ok(saveState().code==='pending', '1d3 ...while THIS session\'s attempt is in flight it is pending', saveState().code);
global.REPO_AUTO_TRIED=null;
// a flagged day WITH a confirming repo-folder record is the real "saved"
pipeNoteSave('2026-08-17','repo folder');
var s2b=saveState();
ok(s2b.code==='saved' && s2b.name==='2026-08-17.json', '1d4 a flag backed by a confirmed folder write -> saved', s2b.code);

// download-only: exported, but it never reaches the repo, so the review cannot see it
reset();
FAKE_NOW=RealDate.parse('2026-08-17T16:00:00Z');
LS['gpts_last_export']='2026-08-17';
pipeNoteSave('2026-08-17','download');
var s3=saveState();
ok(s3.code==='download', '1e download-only export -> download (NOT saved)', s3.code);
ok(s3.t!=null, '1f download state carries the export time');

// this session's own write is the most specific evidence and wins
reset();
FAKE_NOW=RealDate.parse('2026-08-17T16:00:00Z');
global.REPO_LAST_SAVE={ t:FAKE_NOW, how:'repo folder', name:'2026-08-17.json' };
ok(saveState().code==='saved', '1g in-session repo-folder write -> saved', saveState().code);
global.REPO_LAST_SAVE={ t:FAKE_NOW, how:'download', name:'2026-08-17.json' };
ok(saveState().code==='download', '1h in-session download -> download', saveState().code);
// a folder PICK is not an export
global.REPO_LAST_SAVE={ t:FAKE_NOW, how:'folder set' };
ok(saveState().code==='none', '1i picking the folder is not an export', saveState().code);
// a save carried over from a previous day must not claim today
global.REPO_LAST_SAVE={ t:FAKE_NOW, how:'repo folder', name:'2026-08-14.json' };
ok(saveState().code==='none', '1j stale REPO_LAST_SAVE from another day -> none', saveState().code);

// ============================================================================
// 2. THE CT-vs-UTC BUG (2026-08-17)
// ============================================================================
// 2026-08-18 01:30 UTC == 2026-08-17 20:30 CT. The UTC calendar has ALREADY rolled to
// tomorrow while the Chicago trading day is still the 17th. Comparing gpts_last_export
// against the UTC date reports a day that WAS exported as "not exported".
reset();
FAKE_NOW=RealDate.parse('2026-08-18T01:30:00Z');
var utcDate=new RealDate(FAKE_NOW).toISOString().slice(0,10);
ok(utcDate==='2026-08-18', '2a precondition: the UTC clock has rolled to tomorrow', utcDate);
ok(ctTodayStr()==='2026-08-17', '2b but the CT trading date is still today', ctTodayStr());
ok(ctTodayStr()!==utcDate, '2c the two dates genuinely disagree at this instant');
LS['gpts_last_export']='2026-08-17';
pipeNoteSave('2026-08-17','repo folder');
var s4=saveState();
ok(s4.code==='saved', '2d today’s CT export still reads SAVED under a UTC-tomorrow clock', s4.code);
ok(s4.code!=='none', '2e regression guard: never the false "not exported" reading');
// and the same instant must NOT accept a file dated with the UTC day
reset(); FAKE_NOW=RealDate.parse('2026-08-18T01:30:00Z');
LS['gpts_last_export']='2026-08-18';
ok(saveState().code==='none', '2f a UTC-dated export is not today’s CT export', saveState().code);
// the source proves the comparison is CT, not UTC
ok(/function saveState\(\)[\s\S]{0,400}ctTodayStr\(\)/.test(src), '2g saveState() derives today from ctTodayStr (CT)');
ok(!/function saveState\(\)[\s\S]{0,600}toISOString\(\)\.slice/.test(src), '2h saveState() never slices a UTC ISO date');

// ============================================================================
// 3. lastTradingDay()
// ============================================================================
FAKE_NOW=RealDate.parse('2026-08-17T16:00:00Z');   // Monday 2026-08-17
ok(lastTradingDay(0)==='2026-08-17', '3a Monday -> Monday', lastTradingDay(0));
ok(lastTradingDay(1)==='2026-08-14', '3b one step back from Monday skips the weekend to Friday', lastTradingDay(1));
FAKE_NOW=RealDate.parse('2026-08-16T16:00:00Z');   // Sunday
ok(lastTradingDay(0)==='2026-08-14', '3c Sunday -> the previous Friday', lastTradingDay(0));
FAKE_NOW=RealDate.parse('2026-08-19T16:00:00Z');   // Wednesday
ok(lastTradingDay(1)==='2026-08-18', '3d Wednesday -> Tuesday', lastTradingDay(1));

// ============================================================================
// 4. pipeCheck() CACHING — at most one round of remote checks per 10 minutes
// ============================================================================
(async function(){
  reset();
  FAKE_NOW=RealDate.parse('2026-08-17T16:00:00Z');
  FETCH_IMPL=function(u){
    if(/\/data\//.test(u))   return Promise.resolve({ ok:true, status:200 });
    if(/\/review\//.test(u)) return Promise.resolve({ ok:true, status:200, json:()=>Promise.resolve({ headline:'King held 775 all session', why:'because' }) });
    return Promise.reject(new Error('unexpected'));
  };
  pipeCheck();
  await tickMicro();
  var n1=FETCHES.length;
  ok(n1>=2, '4a first call fetches both the data and the review file', n1);
  ok(FETCHES.some(u=>u.indexOf('/data/2026-08-17.json')>=0), '4b checks data/<CT-today>.json');
  ok(FETCHES.some(u=>u.indexOf('/review/2026-08-17.json')>=0), '4c checks review/<lastTradingDay>.json');
  ok(FETCHES.every(u=>u.indexOf('https://raw.githubusercontent.com/rassulshah/gex-signal-tapereader/main')===0), '4d uses the public raw base');

  // AUTO READ-BACK: a 200 review is parsed straight into ANALYSIS_REVIEW
  ok(global.ANALYSIS_REVIEW && global.ANALYSIS_REVIEW.headline==='King held 775 all session',
     '4e a 200 review is parsed into ANALYSIS_REVIEW (auto read-back)', global.ANALYSIS_REVIEW&&global.ANALYSIS_REVIEW.headline);
  ok(RENDERS>0, '4f the read-back triggers a re-render', RENDERS);

  // cached: a second call 1 minute later must NOT refetch
  FAKE_NOW+=60000;
  pipeCheck();
  await tickMicro();
  ok(FETCHES.length===n1, '4g second call within 10 min does NOT refetch', FETCHES.length-n1);
  FAKE_NOW+=8*60000;                       // 9 min in total
  pipeCheck(); await tickMicro();
  ok(FETCHES.length===n1, '4h still cached at 9 min', FETCHES.length-n1);
  FAKE_NOW+=2*60000;                       // 11 min: the cache has expired
  pipeCheck(); await tickMicro();
  ok(FETCHES.length>n1, '4i the check runs again after 10 min', FETCHES.length-n1);

  // the cache lives in gpts_pipe_v1 (a NEW key — nothing was renamed)
  var P=JSON.parse(LS['gpts_pipe_v1']);
  ok(typeof P.t==='number', '4j gpts_pipe_v1 carries the check timestamp');
  ok(P.pushed==='yes' && P.pushedDay==='2026-08-17', '4k pushed recorded for today', P.pushed);
  ok(P.review==='yes' && P.reviewDay==='2026-08-17', '4l review recorded', P.review);
  ok(/King held 775/.test(P.reviewLine||''), '4m the review headline is cached for the hover', P.reviewLine);

  // hidden tab: no network at all
  reset(); FAKE_NOW=RealDate.parse('2026-08-17T16:00:00Z');
  global.document={ visibilityState:'hidden' };
  pipeCheck(); await tickMicro();
  ok(FETCHES.length===0, '4n a hidden tab is never checked', FETCHES.length);
  global.document={ visibilityState:'visible' };

  // ==========================================================================
  // 5. FAIL SOFT — a rejected fetch leaves the stage unknown and never throws
  // ==========================================================================
  reset(); FAKE_NOW=RealDate.parse('2026-08-17T16:00:00Z');
  FETCH_IMPL=function(){ return Promise.reject(new TypeError('Failed to fetch')); };
  var threw=false;
  try{ pipeCheck(); }catch(e){ threw=true; }
  ok(!threw, '5a a rejecting fetch does not throw out of pipeCheck');
  await tickMicro(); await tickMicro();
  var P2=JSON.parse(LS['gpts_pipe_v1']);
  ok(P2.pushed==='unknown', '5b network error -> pushed unknown', P2.pushed);
  ok(P2.review==='unknown', '5c network error -> review unknown', P2.review);
  ok(global.ANALYSIS_REVIEW===null, '5d a failed check never invents a review');
  ok(pipeColor('grey')===PAL.sub, '5e unknown renders grey, not red');
  // and a stage set is still produced (render is never blocked)
  var stErr=pipeStages();
  ok(stErr.length===4, '5f the indicator still renders four stages after a failure', stErr.length);

  // a hard throw from fetch itself (not a rejection) is also soft
  reset(); FAKE_NOW=RealDate.parse('2026-08-17T16:00:00Z');
  global.fetch=function(){ throw new Error('blocked by extension'); };
  var threw2=false; try{ pipeCheck(); }catch(e){ threw2=true; }
  ok(!threw2, '5g a synchronously throwing fetch is caught too');
  await tickMicro(); await tickMicro();
  ok(JSON.parse(LS['gpts_pipe_v1']).pushed==='unknown', '5h -> unknown');
  global.fetch=function(u,o){ FETCHES.push(u); return FETCH_IMPL(u,o); };

  // a 404 review falls back to the prior weekday ONCE
  reset(); FAKE_NOW=RealDate.parse('2026-08-17T16:00:00Z');
  FETCH_IMPL=function(u){
    if(/\/data\//.test(u)) return Promise.resolve({ ok:false, status:404 });
    if(/review\/2026-08-17/.test(u)) return Promise.resolve({ ok:false, status:404 });
    if(/review\/2026-08-14/.test(u)) return Promise.resolve({ ok:true, status:200, json:()=>Promise.resolve({ headline:'Friday review' }) });
    return Promise.reject(new Error('x'));
  };
  pipeCheck(); await tickMicro(); await tickMicro(); await tickMicro();
  ok(FETCHES.some(u=>/review\/2026-08-14/.test(u)), '5i a 404 review retries the prior weekday once');
  ok(global.ANALYSIS_REVIEW && global.ANALYSIS_REVIEW.headline==='Friday review', '5j the prior weekday’s review loads');
  ok(JSON.parse(LS['gpts_pipe_v1']).pushed==='no', '5k a 404 on today’s data = not pushed (red), not unknown');

  // ==========================================================================
  // 6. THE FOUR STAGE COLOURS
  // ==========================================================================
  ok(pipeColor('green')===PAL.longAccent,  '6a green  = ok',      pipeColor('green'));
  ok(pipeColor('amber')===PAL.amber,       '6b amber  = warn',    pipeColor('amber'));
  ok(pipeColor('red')===PAL.shortAccent,   '6c red    = bad',     pipeColor('red'));
  ok(pipeColor('grey')===PAL.sub,          '6d grey   = unknown', pipeColor('grey'));
  ok(pipeColor(undefined)===PAL.sub,       '6e anything unrecognised falls back to grey');

  function stagesWith(o){
    reset(); FAKE_NOW=RealDate.parse('2026-08-17T16:00:00Z');
    global.RECORDER={ _lastSnapBar:{}, _lastSnapT:o.recT===undefined?(FAKE_NOW-60000):o.recT };
    if(o.lastExport) LS['gpts_last_export']=o.lastExport;
    if(o.how) pipeNoteSave(o.lastExport, o.how);
    if(o.pipe){ var P0=pipeLoad(); Object.keys(o.pipe).forEach(k=>P0[k]=o.pipe[k]); pipeSave(P0); }
    global.ANALYSIS_REVIEW=o.review||null;
    var st=pipeStages(); var m={}; st.forEach(x=>m[x.key]=x); m._list=st; return m;
  }
  // -- rec
  var A=stagesWith({ recT:FAKE_NOW-60000 });
  ok(A.rec.state==='green', '6f rec: a bar 1m ago = green', A.rec.state);
  var B=stagesWith({ recT:RealDate.parse('2026-08-17T16:00:00Z')-20*60000 });
  ok(B.rec.state==='amber', '6g rec: a bar 20m ago = amber (recording has stopped)', B.rec.state);
  var C=stagesWith({ recT:null });
  ok(C.rec.state==='amber', '6h rec: nothing recorded yet = amber', C.rec.state);
  // -- saved
  var D=stagesWith({ lastExport:'2026-08-17', how:'repo folder' });
  ok(D.saved.state==='green' && D.saved.label==='saved', '6i saved: a CONFIRMED repo write = green "saved"', D.saved.label);
  // (v10.54, audit 5) the two new states
  var Df=stagesWith({ lastExport:'2026-08-17' });                      // flagged, never confirmed
  ok(Df.saved.state==='red' && Df.saved.label==='FAILED', '6i2 saved: a flag with no confirmed write = red "FAILED"', Df.saved.label);
  ok(/FAILED/.test(Df.saved.tip) && /NOT in the repo/.test(Df.saved.tip),
     '6i3 ...and the hover says plainly that the file is not in the repo', Df.saved.tip.slice(0,80));
  reset(); FAKE_NOW=RealDate.parse('2026-08-17T16:00:00Z');
  global.RECORDER={ _lastSnapBar:{}, _lastSnapT:FAKE_NOW-60000 };
  LS['gpts_last_export']='2026-08-17'; global.REPO_AUTO_TRIED='2026-08-17';
  var Dp={}; pipeStages().forEach(x=>Dp[x.key]=x);
  ok(Dp.saved.state==='amber' && Dp.saved.label==='pending', '6i4 saved: an in-flight export = amber "pending"', Dp.saved.label);
  ok(/PENDING/.test(Dp.saved.tip), '6i5 ...with a hover that says what pending means');
  global.REPO_AUTO_TRIED=null;
  var E=stagesWith({ lastExport:'2026-08-17', how:'download' });
  ok(E.saved.state==='amber' && E.saved.label==='dl', '6j saved: download-only = amber "dl"', E.saved.label);
  ok(/DOWNLOAD only/i.test(E.saved.tip) && /review will not see it/i.test(E.saved.tip),
     '6k the dl hover states plainly that the review will not see it');
  var F=stagesWith({});
  ok(F.saved.state==='red' && F.saved.label==='none', '6l saved: nothing exported = red "none"', F.saved.label);
  // -- pushed
  var G=stagesWith({ pipe:{ pushed:'yes', pushedDay:'2026-08-17' } });
  ok(G.pushed.state==='green', '6m pushed: on GitHub = green', G.pushed.state);
  var H=stagesWith({ pipe:{ pushed:'no', pushedDay:'2026-08-17' } });
  ok(H.pushed.state==='red', '6n pushed: 404 = red', H.pushed.state);
  var I=stagesWith({ pipe:{ pushed:'unknown', pushedDay:'2026-08-17' } });
  ok(I.pushed.state==='grey', '6o pushed: network error = grey unknown', I.pushed.state);
  var J=stagesWith({});
  ok(J.pushed.state==='grey', '6p pushed: never checked = grey', J.pushed.state);
  var K=stagesWith({ pipe:{ pushed:'yes', pushedDay:'2026-08-14' } });
  ok(K.pushed.state==='grey', '6q a result for another day is not today’s answer', K.pushed.state);
  // -- review
  var L=stagesWith({ pipe:{ review:'yes', reviewDay:'2026-08-14', reviewLine:'King held' }, review:{ headline:'King held' } });
  ok(L.review.state==='green', '6r review: exists AND loaded = green', L.review.state);
  ok(/King held/.test(L.review.tip), '6s the review hover cites its headline');
  var M=stagesWith({ pipe:{ review:'no', reviewDay:'2026-08-14' } });
  ok(M.review.state==='amber', '6t review: missing = amber', M.review.state);
  var N=stagesWith({});
  ok(N.review.state==='grey', '6u review: unchecked = grey', N.review.state);
  // question-first hovers
  ok(/^Is it recording\?/.test(A.rec.tip),                 '6v rec hover is question-first');
  ok(/^Is today’s data written where git can see it\?/.test(D.saved.tip), '6w saved hover is question-first');
  ok(/^Has today’s data reached GitHub/.test(G.pushed.tip), '6x pushed hover is question-first');
  ok(/^Did the nightly review run and come back\?/.test(L.review.tip), '6y review hover is question-first');

  // ==========================================================================
  // 7. THE RENDERED INDICATOR — one line, all four stages, controls kept
  // ==========================================================================
  reset(); FAKE_NOW=RealDate.parse('2026-08-17T16:00:00Z');
  global.RECORDER={ _lastSnapBar:{}, _lastSnapT:FAKE_NOW-60000 };
  LS['gpts_last_export']='2026-08-17';
  pipeNoteSave('2026-08-17','repo folder');
  var P3=pipeLoad(); P3.pushed='yes'; P3.pushedDay='2026-08-17'; P3.review='yes'; P3.reviewDay='2026-08-14'; pipeSave(P3);
  global.ANALYSIS_REVIEW={ headline:'King held 775' };
  global.LASTFEED={ SPY:{ feed:'gamma', ts:FAKE_NOW } };
  var html=feedStatusHtml();

  ok(typeof html==='string' && html.length>0, '7a the footer renders');
  ['rec','saved','pushed','review'].forEach(function(k){
    ok(html.indexOf('data-pipe="'+k+'"')>=0, '7b stage present: '+k);
  });
  ok(/>rec</.test(html) && />saved</.test(html) && />pushed</.test(html) && />review</.test(html),
     '7c all four stage LABELS are visible text');
  // ONE line: a single top-level element, nowrap, no breaks or block children
  ok((html.match(/^<div /)||[]).length===1 && /<\/div>$/.test(html), '7d exactly one top-level element');
  ok(html.indexOf('<br')<0, '7e no line breaks');
  ok(/white-space:nowrap/.test(html), '7f the strip is nowrap (cannot wrap at 250px)');
  ok(!/flex-wrap:\s*wrap/.test(html), '7g the flex row never wraps');
  ok(!/display:\s*block/.test(html), '7h no block-level children to force a second line');
  ok((html.match(/<div/g)||[]).length===1, '7i only one <div> in the whole footer', (html.match(/<div/g)||[]).length);
  // the capabilities that must survive the redesign
  ok(html.indexOf('💾')>=0, '7j the 💾 export button is kept');
  ok(html.indexOf('📁')>=0, '7k the 📁 folder-pick button is kept');
  ok(/>v11\.0<\/span>/.test(html), '7l the version is kept, at the right');
  ok(/margin-left:auto/.test(html), '7m the version pins right so the strip stays left');
  // the three v10.50 dots are gone, replaced by the pipeline
  ok(!/>feed</.test(html) && !/>vex</.test(html), '7n the old feed/vex dots are replaced');
  ok(/Feed is live/.test(html), '7o feed liveness survives inside the rec hover (no signal lost)');
  // colours actually reach the markup
  ok(html.indexOf(PAL.longAccent)>=0, '7p a green dot is painted');
  // a 250px budget sanity check: the visible text is short enough for one line
  var visible=html.replace(/<[^>]*>/g,'');
  ok(visible.length<=44, '7q visible footer text stays compact for 250px', visible.length+' chars: '+visible);

  // red / grey render too, without changing the line count
  reset(); FAKE_NOW=RealDate.parse('2026-08-17T16:00:00Z');
  global.RECORDER={ _lastSnapBar:{}, _lastSnapT:null };
  var html2=feedStatusHtml();
  ok((html2.match(/<div/g)||[]).length===1, '7r still one line with nothing exported and nothing checked');
  ok(html2.indexOf('data-pipe="saved"')>=0 && />none</.test(html2), '7s an unsaved day says "none" out loud');
  ok(html2.indexOf(PAL.shortAccent)>=0, '7t and paints it red');

  // ==========================================================================
  // 8. SOURCE-LEVEL CONTRACTS
  // ==========================================================================
  ok(/gpts_pipe_v1/.test(src), '8a the new cache key gpts_pipe_v1 exists');
  ok(/localStorage\.getItem\('gpts_last_export'\)/.test(src), '8b gpts_last_export is READ, not renamed');
  ok(/__gptsDebug\.setReview=function/.test(src),  '8c the console setReview still exists');
  ok(/__gptsDebug\.loadReview=function/.test(src), '8d the console loadReview still exists');
  ok(/ANALYSIS_REVIEW=j;/.test(src), '8e the auto read-back writes the SAME variable setReview writes');
  ok(/PIPE_TTL_MS=600000/.test(src), '8f the remote check is throttled to 10 minutes');
  ok(/visibilityState/.test(ex('pipeCheck')), '8g pipeCheck skips a hidden tab');
  ok(/pipeCheck\(\);/.test(ex('tick')), '8h the render loop drives pipeCheck');
  ok(/@version\s+14.32/.test(src) && /v'\+GPTS_VERSION\+' part1 loaded/.test(src) && />v'\+GPTS_VERSION\+'<\/span>/.test(src),
     '8i version 10.56 in all three spots');
  ok(/pipeReviewLine\(RV\)/.test(ex('briefLine')), '8j the pre-open brief may cite one review line');

  console.log('\n'+pass+' passed, '+fail+' failed');
  process.exit(fail?1:0);
})();
