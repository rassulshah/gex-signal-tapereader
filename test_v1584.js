// test_v1584.js — (v15.84) ONE WINDOW, ONE BREADTH — F-22. The panel's own SPY / QQQ book (LASTFEED, the book the whole
// read / record pipeline and the IRT S rows stand on) is self-fetched by copying the LAST gex/levels URL seen — and that
// URL was the app's (three rotating windows on an ES chart with the projection on: nodes=5 current/1 · nodes=5 next_n/2 ·
// nodes=60 next_n/4) or the panel's own (the expiry sets, nodes=500 in the 0DTE / week / next_n-6 windows, whose
// responses come back through the same fetch hook). Measured 2026-09-08: LASTFEED.SPY at 15:38 CT = 3 expirations, FIVE
// rows, 766 at −$49M (Skylit's 0DTE: 766 at +$849M); SPY KING 760 in the 15:21 file, 766 at 15:23; the recorded node
// count per bar 30 · 3 · 12 · 4 · 5 · 4 · 11 · 12 · 5 · 4 · 11 · 4 · 28 · 5 · 13. Fix: the self-fetch pins
// exp_mode=current&exp_count=1&nodes=500; onFeed drops a multi-expiration gamma payload once a 0DTE book is held.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
function v(n){ return src.match(new RegExp('var '+n+'=[\\s\\S]*?;\\n'))[0]; }
function q(url, k){ var m=new RegExp('[?&]'+k+'=([^&]*)').exec(url); return m?m[1]:null; }
function count(url, k){ return (url.match(new RegExp('[?&]'+k+'=','g'))||[]).length; }

// ---------- 0. the version ----------
ok(/@version\s+15\.(8[4-9]|9\d)/.test(src) && /var GPTS_VERSION='15\.(8[4-9]|9\d)';/.test(src), '0a v15.84 or later in both spots');

// ---------- 1. the self-fetch pins the window and the breadth ----------
{
  global.window={__gptsDebug:{}}; global.console=console;
  const FETCHED=[]; global.fetch=(u,o)=>{ FETCHED.push(u); return Promise.resolve({ ok:false, status:503 }); };
  global.LASTAUTH='Bearer x'; global.onFeed=()=>{};
  eval(src.match(/var SELF_MIN_MS\s*=\s*\d+;/)[0]); eval('var SELF_LAST={};');
  eval(ex('selfFetch'));
  const APP1='https://app.skylit.ai/tv/api/gex/levels?symbol=ES1&data_type=gamma&nodes=5&exp_mode=current&exp_count=1&extended=false&include_derived=true&with_snapshot=true&with_slices=true&v=1700000000000';
  const APP2='https://app.skylit.ai/tv/api/gex/levels?symbol=ES1&data_type=gamma&nodes=5&exp_mode=next_n&exp_count=2&extended=false&include_derived=true&v=1700000000000';
  const APP4='https://app.skylit.ai/tv/api/gex/levels?symbol=ES1&data_type=gamma&nodes=60&exp_mode=next_n&exp_count=4&extended=false&include_derived=true&v=1700000000000';
  const OWN_WEEK='https://app.skylit.ai/tv/api/gex/levels?symbol=SPY&data_type=gamma&nodes=500&exp_mode=week&exp_count=1&extended=false&include_derived=true&v=1700000000001';
  const OWN_WK7='https://app.skylit.ai/tv/api/gex/levels?symbol=SPY&data_type=gamma&nodes=500&exp_mode=next_n&exp_count=6&extended=false&v=1700000000002';
  const FUT='https://app.skylit.ai/tv/api/gex/levels?symbol=NQ1&data_type=gamma&nodes=60&exp_mode=current&exp_count=1&include_derived=true&v=1700000000003';
  const BARE='https://app.skylit.ai/tv/api/gex/levels?symbol=SPY&data_type=gamma&v=1700000000004';
  function fetchFrom(base, sym, type){ SELF_LAST={}; global.LASTFEEDURL=base; FETCHED.length=0; selfFetch(sym, type); return FETCHED[0]||null; }
  const u1=fetchFrom(APP1,'SPY','gamma');
  ok(u1 && q(u1,'symbol')==='SPY' && q(u1,'data_type')==='gamma' && q(u1,'exp_mode')==='current' && q(u1,'exp_count')==='1' && q(u1,'nodes')==='500', '1a from the app\'s 0DTE window: symbol / data_type swapped, exp_mode=current&exp_count=1&nodes=500', u1);
  ok(u1 && q(u1,'v')!=='1700000000000' && /^\d{13}$/.test(q(u1,'v')||''), '1b …the v= cache-buster refreshed');
  ok(u1 && q(u1,'include_derived')==='true' && q(u1,'with_snapshot')==='true' && q(u1,'extended')==='false', '1c …every other parameter of the app\'s URL kept (its auth shape)');
  const u2=fetchFrom(APP2,'SPY','gamma');
  ok(u2 && q(u2,'exp_mode')==='current' && q(u2,'exp_count')==='1' && q(u2,'nodes')==='500', '1d from the app\'s next_n/2 basket (the projection): still the 0DTE chain, every strike', u2);
  const u4=fetchFrom(APP4,'QQQ','gamma');
  ok(u4 && q(u4,'symbol')==='QQQ' && q(u4,'exp_mode')==='current' && q(u4,'exp_count')==='1' && q(u4,'nodes')==='500', '1e from the app\'s nodes=60 next_n/4 basket, for QQQ: the same pin', u4);
  const u5=fetchFrom(OWN_WEEK,'SPY','gamma');
  ok(u5 && q(u5,'exp_mode')==='current' && q(u5,'exp_count')==='1' && q(u5,'nodes')==='500', '1f from the panel\'s OWN week expiry-set URL (the last URL seen is often ours): the 0DTE chain', u5);
  const u6=fetchFrom(OWN_WK7,'SPY','vanna');
  ok(u6 && q(u6,'data_type')==='vanna' && q(u6,'exp_mode')==='current' && q(u6,'exp_count')==='1' && q(u6,'nodes')==='500', '1g the vanna self-fetch gets the same window and breadth (one book, two Greeks)', u6);
  const u7=fetchFrom(FUT,'SPY','gamma');
  ok(u7 && q(u7,'symbol')==='SPY' && q(u7,'exp_mode')==='current' && q(u7,'nodes')==='500', '1h from the futures self-fetch\'s URL (symbol=NQ1): SPY, the 0DTE chain', u7);
  const u8=fetchFrom(BARE,'SPY','gamma');
  ok(u8 && q(u8,'exp_mode')==='current' && q(u8,'exp_count')==='1' && q(u8,'nodes')==='500' && count(u8,'exp_mode')===1 && count(u8,'exp_count')===1 && count(u8,'nodes')===1, '1i a URL without the three parameters gets them APPENDED, once each', u8);
  ok(u1 && count(u1,'exp_mode')===1 && count(u1,'exp_count')===1 && count(u1,'nodes')===1 && count(u1,'symbol')===1, '1j …and a URL that has them is not given them twice');
  ok(u1 && !/exp_mode=next_n|exp_count=[24]|nodes=(5|60)(&|$)/.test(u1) && u2 && !/exp_mode=next_n/.test(u2) && u4 && !/nodes=60/.test(u4), '1k no window or breadth of the app\'s survives into the panel\'s own request');
  SELF_LAST={}; global.LASTFEEDURL=null; FETCHED.length=0; selfFetch('SPY','gamma');
  ok(FETCHED.length===0, '1l no URL seen yet → no request (unchanged)');
}

// ---------- 2. the window guard in onFeed ----------
{
  global.window={__gptsDebug:{}};
  global.LASTDISP={SPY:null,QQQ:null}; global.LASTFEED={SPY:null,QQQ:null}; global.LASTVEX={SPY:null,QQQ:null};
  global.observeFeedCadence=()=>{}; global.SYM_SEEN={}; global.LASTFUTDER={};
  eval(v('FEED_REJECTS'));
  eval(['feedNewestT','onFeed'].map(ex).join('\n'));
  const NOW=1788897540;   // 2026-09-08 15:19:00 CT — the newest snapshot in the payload held at 15:38
  // the 15:21–15:38 CT fixture: the app's next_n/2 basket for SPY (three expirations, five rows, 766 −$49M) …
  const BASKET={ expirations:['2026-09-08','2026-09-09','2026-09-10'], levels:[{ t:NOW, s:766.09, l:[{k:766,v:-49e6,d:-1},{k:765,v:39e6,d:1},{k:768,v:33e6,d:1},{k:767,v:30e6,d:1},{k:770,v:-25e6,d:-1}] }] };
  // … and the 0DTE chain the same minute (one expiration, the whole strip, 766 +$849M — what Skylit's derived orb showed)
  const DTE0={ expirations:['2026-09-08'], levels:[{ t:NOW, s:766.09, l:[{k:766,v:849e6,d:1},{k:767,v:520e6,d:1},{k:765,v:410e6,d:1},{k:770,v:-380e6,d:-1},{k:768,v:200e6,d:1},{k:760,v:90e6,d:1},{k:775,v:-40e6,d:-1}] }] };
  const WEEK={ expirations:['2026-09-08','2026-09-09','2026-09-10','2026-09-11'], levels:[{ t:NOW, s:766.09, l:[{k:766,v:-20e6,d:-1},{k:770,v:-900e6,d:-1}] }] };
  function top(j){ var l=j.levels[j.levels.length-1].l.slice().sort((a,b)=>Math.abs(b.v)-Math.abs(a.v))[0]; return l.k+':'+Math.round(l.v/1e6); }
  onFeed('SPY','gamma',BASKET,true);
  ok(LASTFEED.SPY && LASTFEED.SPY.j===BASKET, '2a nothing held → even a three-expiration basket is accepted (the panel is never blind at boot)');
  onFeed('SPY','gamma',DTE0,true);
  ok(LASTFEED.SPY && LASTFEED.SPY.j===DTE0 && top(LASTFEED.SPY.j)==='766:849', '2b the 0DTE chain arrives → it replaces the basket (766 +849M)');
  onFeed('SPY','gamma',BASKET,true);
  ok(LASTFEED.SPY && LASTFEED.SPY.j===DTE0, '2c the basket arrives again → DROPPED: the 0DTE book is kept (766 +849M, not −49M)', top(LASTFEED.SPY.j));
  ok(FEED_REJECTS.SPY && FEED_REJECTS.SPY.win===1 && typeof FEED_REJECTS.SPY.winT==='number' && FEED_REJECTS.SPY.winT>0, '2d …counted: FEED_REJECTS.SPY.win=1 with its time', FEED_REJECTS.SPY);
  ok(FEED_REJECTS.SPY.n===0, '2e …and NOT counted as a history rejection (a different reason, a different counter)', FEED_REJECTS.SPY);
  onFeed('SPY','gamma',WEEK,false);
  ok(LASTFEED.SPY.j===DTE0 && FEED_REJECTS.SPY.win===2, '2f the panel\'s own week set (four expirations, back through the hook) → dropped and counted too', FEED_REJECTS.SPY);
  const DTE0b={ expirations:['2026-09-08'], levels:[{ t:NOW+60, s:766.2, l:[{k:766,v:851e6,d:1},{k:767,v:522e6,d:1}] }] };
  onFeed('SPY','gamma',DTE0b,true);
  ok(LASTFEED.SPY.j===DTE0b, '2g a fresher 0DTE payload still replaces normally (the guard is about the window, not freshness)');
  const HIST={ expirations:['2026-09-08'], levels:[{ t:NOW-6*86400, s:777, l:[{k:778,v:9e8,d:1}] }] };
  onFeed('SPY','gamma',HIST,false);
  ok(LASTFEED.SPY.j===DTE0b && FEED_REJECTS.SPY.n===1, '2h the never-history-over-live guard still stands behind it (a 6-day-old 0DTE payload → history rejection)', FEED_REJECTS.SPY);
  const NOEXP={ levels:[{ t:NOW+120, s:766.3, l:[{k:766,v:852e6,d:1}] }] };
  onFeed('SPY','gamma',NOEXP,true);
  ok(LASTFEED.SPY.j===NOEXP, '2i a payload that names no expirations at all is accepted (degrade: the guard needs the field on both sides)');
  const BASKET2={ expirations:BASKET.expirations, levels:[{ t:NOW+150, s:766.1, l:BASKET.levels[0].l }] };
  onFeed('SPY','gamma',BASKET2,true);
  ok(LASTFEED.SPY.j===BASKET2, '2j …and with an expiration-less book held the guard stands down (it never guesses)');
  onFeed('QQQ','gamma',{ expirations:['2026-09-08'], levels:[{ t:NOW, s:590, l:[{k:590,v:3e8,d:1}] }] },true);
  onFeed('QQQ','gamma',{ expirations:['2026-09-08','2026-09-09'], levels:[{ t:NOW+30, s:590, l:[{k:590,v:-3e7,d:-1}] }] },true);
  ok(LASTFEED.QQQ.j.expirations.length===1 && FEED_REJECTS.QQQ.win===1 && FEED_REJECTS.SPY.win===2, '2k per symbol: QQQ guarded on its own counter, SPY\'s untouched', { q:FEED_REJECTS.QQQ, s:FEED_REJECTS.SPY });
  // the vanna book is not window-guarded here (its self-fetch is pinned; nothing else feeds it)
  onFeed('SPY','vanna',DTE0,true); onFeed('SPY','vanna',BASKET,true);
  ok(LASTVEX.SPY.j===BASKET, '2l the vanna capture is untouched by the gamma guard (pinned at the source instead)');
}

// ---------- 3. the debug hook and the words ----------
{
  ok(/window\.__gptsDebug\.feedRejects=function\(\)\{ return FEED_REJECTS; \};/.test(src), '3a __gptsDebug.feedRejects() reports the window rejections alongside the history ones');
  ok(/RW\.win===1\|\|RW\.win%50===0/.test(ex('onFeed')) && /-expiration window IGNORED/.test(ex('onFeed')), '3b the console says so once, then every 50th time');
  ok(/exp_mode=current&exp_count=1&nodes=500/.test(ex('selfFetch')) && /F-22/.test(ex('selfFetch')), '3c the comment above the pin names F-22 and the one window');
}

console.log('test_v1584: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
