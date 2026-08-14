// ==UserScript==
// @name         Gex Signal Tapereader
// @namespace    gpts
// @version    10.29
// @description  Feed-driven GEX signal state machine for SPY on Skylit Atlas (trend slope, T1/T2 target ladder, structural read, accumulation, vertical grid, Phase-1 recorder)
// @match        https://app.skylit.ai/atlas*
// @grant        none
// @run-at       document-idle
// ==/UserScript==
(function(){
'use strict';

function mul(a,b){ return a/(1/b); }
function two(x){ x=''+x; return x.length<2?'0'+x:x; }

var POLL_MS = 15000;
var CANDLE_MS = 180000;
var CANDLE_S = 180;
var STAGE_MAX_MS = 1800000;
var ES_RATIO = 10.0676;
var TODAY = null;
var FEED_BASE = 'https://app.skylit.ai/tv/api/gex/levels';
var MIN_STRENGTH = 20;
var FEED_STALE_MS = 60000;

var LOG_KEY   = 'gpts_inplay_v7';
var SLICE_KEY = 'gpts_slices_v7';
var POS_KEY   = 'gpts_panelpos_v7';
var SIZE_KEY  = 'gpts_panelsize_v7';
var STATE_KEY = 'gpts_state_v7';
var CFG_KEY     = 'gpts_cfg_v8';
var CFG_KEY_OLD = 'gpts_cfg_v7';   // migrated forward on first v8 load
var STATS_KEY = 'gpts_stats_v7';
var RECORDER_KEY = 'gpts_recorder_v7';   // DATA layer: node snapshots + outcome events for LLM analytics

var TREND_WINDOW = 20;
var TREND_DOM = 16;   // (v10.18) 16 of 20 bars (>=75%) = confirmed directional trend
// (v10.27) BREAKOUT QUALITY GATE: a BO only fires if the breakout bar also prints a
// new N-bar EXTREME \u2014 a 14-bar HIGH for upside breakouts / 14-bar LOW for downside
// breakdowns (window INCLUDES the breakout bar). Filters weak/noise pokes through a
// node that aren't backed by genuine range expansion. Symmetric (longs + shorts).
var BO_HL_LOOKBACK = 14;   // closed 3m bars in the high/low window (tunable)
var TREND_LAST = { SPY:null, QQQ:null };   // last CONFIRMED trend dir ('up'|'dn') for broken-state detection
var SMA_CONT_FLAG = { SPY:false, QQQ:false };  // (v10.23 C) set when the continuous SMA failed sanity and we fell back
var TREND_RUNMIN = 3;
var TREND_BANDX = 0.25;
var TREND_SLOPEX = 0.15;
var ACC_WINDOW = 10;        // intraday lookback in 3m bars (~30 min) for accumulation
var ACC_SAMPLES = 8;        // max points sampled from the window
var ACC_SAMPLE_STEP = 1;    // sample every bar for a dense series (dip logic needs density)
var ACC_FLOOR_PCT = 20;     // ignore nodes weaker than this %King (noise floor)
var ACC_NET = 8;            // min %King net move for legacy paths
var ACC_ROWS = 4;
// --- Accumulation trend detector (absolute-value based, dip-tolerant) ---
var ACC_DROP_BUDGET = 0.28; // a node may give back up to 28% of its recent peak and still count as building
var ACC_CONFIRM_DOWN = 2;   // consecutive down-samples required to flip Building -> Fading
var ACC_BUILD_MIN = 0.15;   // abs value must have grown >=15% peak-vs-start to call Building
var ACC_RAPID_ROC = 0.20;   // >=20% abs growth over the last 2 samples = Rapid Accumulation flag
// --- Node history strip (%King samples read from the tape) ---
// We trade off a 3m chart and buy pullbacks lasting ~12-30 min, so the strip
// and the Building/Steady/Dissipating JUDGMENT are aligned to 3m bar closes
// (8 closes = ~24 min, covering a full pullback). The RAPID flag still reads
// the LIVE %King between closes so a fast load can fire mid-bar.
var HIST_SAMPLE_MS = 180000; // one strip sample per 3m bar close
var HIST_MAX = 12;           // STORE up to 12 closes; display length = CFG.stripLen
var HIST_STEADY_BAND = 6;    // <= this net %-pt move over the window = Steady
var HIST_RAPID_STEP = 10;    // >= this jump over the last 2 samples = Rapid
var HIST_SHARP_STEP = 8;     // a sample-over-sample %King change >= this is SHARP, colored
// --- Session peak/low per node (context beyond the visible strip) ---
// A node that built 20->60 over 40 min then flattened reads 'Steady' on the
// strip but is a heavily-loaded wall. We track today's peak/low %King per node
// so a fading-but-large node can show a '↓peak' marker and feed target scoring.
var PEAK_OFF_MARK = 8;       // show '↓peak' only when current is >= this many pts below session peak
// --- Adaptive proximity (relative trap read #2) ---
// 'Not far away' overhead/underfoot resistance is judged against how far a
// normal 12-30 min move actually travels NOW, derived from recent 3m bar range,
// clamped so it never misbehaves on a freak bar.
var PROX_BARS = 10;          // lookback bars for average 3m range
var PROX_MOVE_BARS = 6;      // a typical bounce spans ~this many 3m bars
var PROX_MIN_STRIKES = 1;    // floor: never consider resistance closer than this irrelevant
var PROX_MAX_STRIKES = 5;    // ceiling: never flag resistance beyond this as 'in reach'
// --- #1 target scoring weights (confluence) ---
var TS_W_STRENGTH = 1.0;     // weight on %King strength
var TS_W_VELOCITY = 1.4;     // weight on accumulation velocity (build rate)
var TS_W_PROXIMITY = 0.8;    // weight on nearness to price
var TS_W_STRUCT = 1.2;       // structural bonus for King / Gatekeeper roles
var TS_MIN_PCT = 20;         // a strike must be at least this %King to be a target candidate
var GK_MAX_DIST = 5;
var STATS_DAYS = 5;
var READ_NEARX = 0.25;
// --- Recorder (DATA layer feeding LLM analytics / prediction) ---
var RECORDER_DAYS = 10;          // rolling trading days kept in localStorage
var RECORDER_MAX_SNAPS = 200;    // hard cap of node-snapshots per symbol per day
var RECORDER_MAX_EVENTS = 300;   // hard cap of outcome events per symbol per day
var RECORDER_SYMS = ['SPY','QQQ'];
var READ_APPRX = 0.75;

var CFG = {
  ftReq: true, boPb: true, dir: 'both', nodeThresh: 20, voidBackN: 2,
  trendOn: true, trendMA: { SPY:50, QQQ:50 },
  smaShort: { SPY:9, QQQ:9 }, smaLong: { SPY:21, QQQ:21 },
  hideGO: true, cfgOpen: true, showSPY: true, showQQQ: true,
  // --- Display (#5) ---
  compact: false,          // compact node cells (single line) vs expanded
  stripLen: 8,             // growth-strip points (3m closes) shown, 4..12
  // --- Alerts (#10) --- per event: on + visual + sound. Conservative defaults.
  alerts: {
    inplayAccum: { on:true,  vis:true,  snd:false },
    dissipate:   { on:false, vis:true,  snd:false },
    kingRoll:    { on:true,  vis:true,  snd:true  },
    trap:        { on:true,  vis:true,  snd:false },
    absorption:  { on:false, vis:true,  snd:false },
    feedStale:   { on:true,  vis:true,  snd:true  }
  }
};

function loadCfg(){
  try{
    var raw=localStorage.getItem(CFG_KEY);
    if(!raw){ // migrate v7 -> v8 so existing settings carry over
      var old=localStorage.getItem(CFG_KEY_OLD);
      if(old){ raw=old; }
    }
    if(!raw) return;
    var o=JSON.parse(raw);
    if(o && typeof o==='object'){
      if(typeof o.ftReq==='boolean') CFG.ftReq=o.ftReq;
      if(typeof o.boPb==='boolean') CFG.boPb=o.boPb;
      if(o.dir==='both'||o.dir==='longs'||o.dir==='shorts') CFG.dir=o.dir;
      if(typeof o.nodeThresh==='number' && o.nodeThresh>=20 && o.nodeThresh<=100) CFG.nodeThresh=o.nodeThresh;
      if(typeof o.voidBackN==='number' && o.voidBackN>=1 && o.voidBackN<=10) CFG.voidBackN=o.voidBackN;
      if(typeof o.trendOn==='boolean') CFG.trendOn=o.trendOn;
      if(o.trendMA){ if(o.trendMA.SPY) CFG.trendMA.SPY=o.trendMA.SPY; if(o.trendMA.QQQ) CFG.trendMA.QQQ=o.trendMA.QQQ; }
      if(typeof o.hideGO==='boolean') CFG.hideGO=o.hideGO;
      if(o.smaShort){ if(o.smaShort.SPY) CFG.smaShort.SPY=o.smaShort.SPY; if(o.smaShort.QQQ) CFG.smaShort.QQQ=o.smaShort.QQQ; }
      if(o.smaLong){ if(o.smaLong.SPY) CFG.smaLong.SPY=o.smaLong.SPY; if(o.smaLong.QQQ) CFG.smaLong.QQQ=o.smaLong.QQQ; }
      if(typeof o.cfgOpen==='boolean') CFG.cfgOpen=o.cfgOpen;
      if(typeof o.showSPY==='boolean') CFG.showSPY=o.showSPY;
      if(typeof o.showQQQ==='boolean') CFG.showQQQ=o.showQQQ;
      // #5 display
      if(typeof o.compact==='boolean') CFG.compact=o.compact;
      if(typeof o.stripLen==='number' && o.stripLen>=4 && o.stripLen<=12) CFG.stripLen=o.stripLen;
      // #10 alerts (guarded-merge each event so new events keep defaults)
      if(o.alerts && typeof o.alerts==='object'){
        for(var ev in CFG.alerts){
          if(o.alerts[ev] && typeof o.alerts[ev]==='object'){
            if(typeof o.alerts[ev].on==='boolean')  CFG.alerts[ev].on=o.alerts[ev].on;
            if(typeof o.alerts[ev].vis==='boolean') CFG.alerts[ev].vis=o.alerts[ev].vis;
            if(typeof o.alerts[ev].snd==='boolean') CFG.alerts[ev].snd=o.alerts[ev].snd;
          }
        }
      }
    }
    MIN_STRENGTH = CFG.nodeThresh;
    saveCfg(); // persist under v8 key after any migration
  }catch(e){}
}
function saveCfg(){ try{ localStorage.setItem(CFG_KEY, JSON.stringify(CFG)); }catch(e){} }

var LAST_OK = {SPY:0, QQQ:0};
var STATE = {
  SPY: { price:null, king:null, walls:[], candles:[], cur:null, setups:{}, lastClosedB:0 },
  QQQ: { price:null, king:null, walls:[], candles:[], cur:null, setups:{}, lastClosedB:0 }
};
var FUT = { ES:null, NQ:null };
var LASTFEED = { SPY:null, QQQ:null };
var LASTVEX = { SPY:null, QQQ:null };
var LASTNODEMAP = { SPY:null, QQQ:null };   // (v10.24) last emitted Node Map model (effectiveness capture)

var RESHUFFLE = { SPY:false, QQQ:false };
var PREVWALLKEYS = { SPY:null, QQQ:null };
var PREVKING = { SPY:null, QQQ:null };

function ctNow(){ return new Date(new Date().toLocaleString('en-US',{timeZone:'America/Chicago'})); }
function ctDateStr(d){ d=d||ctNow(); return d.getFullYear()+'-'+two(d.getMonth()+1)+'-'+two(d.getDate()); }
function ctMinutesSinceMidnight(){ var d=ctNow(); return mul(d.getHours(),60)+d.getMinutes(); }
function pastReset(){ return ctMinutesSinceMidnight() >= mul(8,60)+30; }
function ctTodayStr(){ return ctDateStr(); }
function ctNowSecOfDay(){ var d=ctNow(); return mul(d.getHours(),3600)+mul(d.getMinutes(),60)+d.getSeconds(); }
function ctOffsetSec(){
  var d=new Date();
  var c=new Date(d.toLocaleString('en-US',{timeZone:'America/Chicago'}));
  var u=new Date(d.toLocaleString('en-US',{timeZone:'UTC'}));
  return Math.round((u-c)/1000);
}
function naiveDayStr(t){ var d=new Date(mul(t,1000)); return d.getUTCFullYear()+'-'+two(d.getUTCMonth()+1)+'-'+two(d.getUTCDate()); }
function naiveSecOfDay(t){ var d=new Date(mul(t,1000)); return mul(d.getUTCHours(),3600)+mul(d.getUTCMinutes(),60)+d.getUTCSeconds(); }
function fmtClock(ts){
  var d=new Date(ts);
  var s=d.toLocaleTimeString('en-US',{timeZone:'America/Chicago',hour:'numeric',minute:'2-digit',hour12:true});
  return s.toLowerCase().replace(/\s/g,' ');
}
function feedTypeFromUrl(u){
  if(u.indexOf('data_type=combined')!==-1) return 'combined';
  if(u.indexOf('data_type=gamma')!==-1) return 'gamma';
  if(u.indexOf('data_type=vanna')!==-1) return 'vanna';
  return 'other';
}
function symFromUrl(u){ return (u.indexOf('symbol=QQQ')!==-1) ? 'QQQ' : 'SPY'; }

function installFeedObserver(){
  if(window.__gptsFeedHook) return;
  window.__gptsFeedHook = true;
  // ---- Hook fetch() ----
  if(typeof window.fetch==='function'){
    var orig = window.fetch;
    window.fetch = function(input, init){
      var url = (typeof input==='string') ? input : (input && input.url) || '';
      var p = orig.apply(this, arguments);
      if(url.indexOf('gex/levels')!==-1){
        p.then(function(resp){
          try{
            if(resp && resp.status===200){
              var clone = resp.clone();
              clone.json().then(function(j){ onFeed(symFromUrl(url), feedTypeFromUrl(url), j); }).catch(function(){});
            }
          }catch(e){}
        }).catch(function(){});
      }
      return p;
    };
  }
  // ---- Hook XMLHttpRequest() too ----
  // Skylit may deliver gex/levels over XHR (or axios-on-XHR), which the fetch
  // hook would miss. Capture the URL at open() and parse responseText at load.
  try{
    var XP = window.XMLHttpRequest && window.XMLHttpRequest.prototype;
    if(XP && !XP.__gptsHooked){
      XP.__gptsHooked = true;
      var origOpen = XP.open, origSend = XP.send;
      XP.open = function(method, url){
        try{ this.__gptsUrl = (typeof url==='string') ? url : ''; }catch(e){}
        return origOpen.apply(this, arguments);
      };
      XP.send = function(){
        try{
          var xhr=this, u=xhr.__gptsUrl||'';
          if(u.indexOf('gex/levels')!==-1){
            xhr.addEventListener('load', function(){
              try{
                if(xhr.status===200 && xhr.responseText){
                  var j=JSON.parse(xhr.responseText);
                  onFeed(symFromUrl(u), feedTypeFromUrl(u), j);
                }
              }catch(e){}
            });
          }
        }catch(e){}
        return origSend.apply(this, arguments);
      };
    }
  }catch(e){}
}
function onFeed(sym, feed, j){
  if(sym!=='SPY' && sym!=='QQQ') return;
  if(!j || !j.levels || !j.levels.length) return;
  if(feed==='vanna'){ LASTVEX[sym] = { j:j, ts:Date.now() }; return; }
  if(feed==='gamma' || feed==='combined'){ var _ts=Date.now(); LASTFEED[sym] = { j:j, feed:feed, ts:_ts }; observeFeedCadence(sym, _ts); }
}

console.log('[GPTS] v10.29 part1 loaded');

function fiberKeyOf(el){
  var ks=Object.keys(el);
  for(var i=0;i<ks.length;i++){ if(ks[i].indexOf('__reactFiber$')===0) return ks[i]; }
  return null;
}
function isCandleArr(v){
  if(!v || !v.length || v.length<3) return false;
  var x=v[0];
  return x && typeof x==='object' && ('open' in x) && ('high' in x) && ('low' in x) && ('close' in x) && ('time' in x);
}
function readFiberCandles(sym){
  var canvases=document.querySelectorAll('canvas');
  for(var c=0;c<canvases.length;c++){
    var fk=fiberKeyOf(canvases[c]);
    if(!fk) continue;
    var node=canvases[c][fk];
    var d=0;
    while(node && d<10){
      var p=node.memoizedProps;
      if(p && typeof p==='object'){
        try{ if(p.symbol===sym && isCandleArr(p.candles)){ return p.candles; } }catch(e){}
      }
      node=node.return; d++;
    }
  }
  return null;
}
function lastCloseOf(sym){
  var raw=readFiberCandles(sym);
  if(!raw || !raw.length) return null;
  var lastC=raw[raw.length-1];
  return (lastC && typeof lastC.close==='number') ? lastC.close : null;
}
function convertFiberCandles(raw){
  var todayStr=ctTodayStr();
  var openSec=mul(8,3600)+mul(30,60);
  var offMs=mul(ctOffsetSec(),1000);
  var out=[];
  for(var i=0;i<raw.length;i++){
    var x=raw[i];
    var t=x.time;
    if(typeof t!=='number') continue;
    if(naiveDayStr(t)!==todayStr) continue;
    var so=naiveSecOfDay(t);
    if(so<openSec) continue;
    var naiveMs=mul(t,1000);
    var realMs=naiveMs+offMs;
    out.push({ b:realMs, t:realMs, o:x.open, h:x.high, l:x.low, c:x.close, so:so });
  }
  out.sort(function(a,b){ return a.b-b.b; });
  return out;
}
// (v10.23 Issue C) CONTINUOUS close series across ALL sessions (RTH only),
// so our SMA-50 matches Skylit's rolling chart SMA instead of resetting each day.
// Same RTH filter as convertFiberCandles but WITHOUT the today-only gate; keeps
// prior sessions' closes so a full 50-bar SMA exists from the session open.
function convertFiberCandlesCont(raw){
  var openSec=mul(8,3600)+mul(30,60);
  var offMs=mul(ctOffsetSec(),1000);
  var out=[];
  for(var i=0;i<raw.length;i++){
    var x=raw[i];
    var t=x.time;
    if(typeof t!=='number') continue;
    var so=naiveSecOfDay(t);
    if(so<openSec) continue;        // keep RTH only, across every day
    var naiveMs=mul(t,1000);
    var realMs=naiveMs+offMs;
    out.push({ b:realMs, t:realMs, c:x.close, so:so, day:naiveDayStr(t) });
  }
  out.sort(function(a,b){ return a.b-b.b; });
  if(out.length>1200) out=out.slice(out.length-1200);   // ~cap memory
  return out;
}
function applyCandles(sym, arr){
  var S=STATE[sym];
  if(!arr.length) return;
  var nowBucket=Math.floor(ctNowSecOfDay()/CANDLE_S)*CANDLE_S;
  var closed=[], cur=null;
  arr.forEach(function(c){
    if(c.so < nowBucket) closed.push(c);
    else cur=c;
  });
  if(closed.length>400) closed = closed.slice(closed.length-400);
  S.candles = closed;
  S.cur = cur;
  // (v10.23 Issue C) how many of the continuous closes belong to a PRIOR session
  // (used so the continuous SMA aligns with today's closed-bar count).
  if(S.contCloses && S.contCloses.length){
    var todayStr=ctTodayStr(), pc=0;
    for(var ci=0;ci<S.contCloses.length;ci++){ if(S.contCloses[ci].day!==todayStr) pc++; }
    S.contPriorCount = pc;
  }
  if(closed.length){
    var lastPrice = cur ? cur.c : closed[closed.length-1].c;
    S.price = (S.price==null) ? lastPrice : S.price;
  }
}
function nearestWall(walls, price, above){
  var best=null;
  for(var i=0;i<walls.length;i++){
    var w=walls[i];
    if(above && w.k>price){ if(!best || w.k<best.k) best=w; }
    if(!above && w.k<price){ if(!best || w.k>best.k) best=w; }
  }
  return best;
}

function captureGuards(sym, walls, king){
  try{
    var keys = walls.map(function(w){ return w.k.toFixed(2); }).sort().join(',');
    var prev = PREVWALLKEYS[sym];
    if(prev!=null){
      var prevSet = prev.split(',');
      var nowSet = keys.split(',');
      var common=0;
      nowSet.forEach(function(k){ if(prevSet.indexOf(k)!==-1) common++; });
      var union = {}; prevSet.concat(nowSet).forEach(function(k){ union[k]=1; });
      var uCount = Object.keys(union).length || 1;
      RESHUFFLE[sym] = (common / uCount) < 0.5;
    }
    PREVWALLKEYS[sym]=keys;
    if(king!=null) PREVKING[sym]=king;
  }catch(e){}
}

function extractWalls(j){
  var lv = j.levels || [];
  if(!lv.length) return {price:null, king:null, walls:[]};
  var last = lv[lv.length-1];
  var price = last.s;
  var native = [];
  (last.l||[]).forEach(function(n){ native.push({k:n.k, v:n.v, d:n.d, net:(typeof n.net==='number'?n.net:null)}); });
  // (v10.25 KING BUG FIX) The King is the STRIKE with the largest ABSOLUTE dealer
  // exposure \u2014 NOT the magnitude value. The old code did `king = max(n.v)` and
  // then RETURNED that magnitude as the King (leaking e.g. 6.6e8 into S.king, which
  // consumers treated as a strike \u2192 nonsense King disconnected from the tape).
  // Track magnitude (for %-normalization) and the strike separately; use |v| so a
  // dominant NEGATIVE-gamma node (e.g. the tape's \u221285%) can be King.
  var kingMag = 0, kingK = null;
  native.forEach(function(n){ var a=Math.abs(n.v); if(a>kingMag){ kingMag=a; kingK=n.k; } });
  if(kingMag<=0 || kingK==null) return {price:price, king:null, walls:[]};
  var byK = {};
  native.forEach(function(n){
    var pct = Math.round(mul(100, Math.abs(n.v)/kingMag));
    if(pct>=MIN_STRENGTH){ byK[n.k.toFixed(2)] = {k:n.k, pct:pct, abs:n.v, pos:(n.d>0), derived:false, net:n.net}; }
  });
  var haveDerived = j.derived && j.derived.length;
  if(haveDerived){
    (j.derived||[]).forEach(function(dd){
      if(!dd.levels || !dd.levels.length) return;
      var dlast = dd.levels[dd.levels.length-1];
      (dlast.l||[]).forEach(function(n){
        var snapped = Math.round(mul(n.k,2))/2;
        var pct = Math.round(mul(100, Math.abs(n.v)/kingMag));
        if(pct<MIN_STRENGTH) return;
        var isInt = Math.abs(snapped - Math.round(snapped)) < 0.001;
        if(isInt && byK[Math.round(snapped).toFixed(2)]) return;
        var key = snapped.toFixed(2);
        if(byK[key] && !byK[key].derived) return;
        if(!byK[key] || (byK[key].derived && pct>byK[key].pct)){
          byK[key] = {k:snapped, pct:pct, abs:n.v, pos:(n.d>0), derived:true, net:(typeof n.net==='number'?n.net:null)};
        }
      });
    });
  } else {
    synthDerived(native, kingMag, byK);
  }
  var walls = [];
  for(var kk in byK){ walls.push(byK[kk]); }
  walls.sort(function(a,b){ return a.k-b.k; });
  return {price:price, king:kingK, walls:walls};
}
function synthDerived(native, king, byK){
  var pts = native.slice().sort(function(a,b){ return a.k-b.k; });
  for(var i=0;i<pts.length-1;i++){
    var a=pts[i], b=pts[i+1];
    var gap=b.k-a.k;
    if(gap<=0.6) continue;
    var mid = Math.round(mul((a.k+b.k),0.5)/0.5)*0.5;
    var isInt = Math.abs(mid - Math.round(mid)) < 0.001;
    if(isInt) continue;
    var vAvg = mul((a.v+b.v),0.5);
    var pct = Math.round(mul(100, Math.abs(vAvg)/king));
    if(pct<MIN_STRENGTH) continue;
    var key = mid.toFixed(2);
    if(byK[key]) continue;
    var netA = (typeof a.net==='number')?a.net:0;
    var netB = (typeof b.net==='number')?b.net:0;
    var netMid = mul((netA+netB),0.5);
    var posMid = (a.d>0 && b.d>0) ? true : (a.d<0 && b.d<0 ? false : (netMid>0));
    byK[key] = {k:mid, pct:pct, abs:vAvg, pos:posMid, derived:true, net:(typeof a.net==='number'||typeof b.net==='number')?netMid:null};
  }
}

console.log('[GPTS] v9.1 part2 loaded');

function closedCandles(sym){ return STATE[sym].candles; }
function atr(sym){
  var c=closedCandles(sym); if(c.length<2) return 0.1;
  var n=Math.min(14,c.length-1); var sum=0;
  for(var i=c.length-n;i<c.length;i++){
    var cur=c[i], prev=c[i-1];
    var tr=Math.max(cur.h-cur.l, Math.abs(cur.h-prev.c), Math.abs(cur.l-prev.c));
    sum+=tr;
  }
  return sum/n || 0.1;
}
// (v10.23 Issue C) CONTINUOUS SMA over the multi-session close series.
// Returns the SMA of the last `period` continuous closes (matches the chart's
// rolling SMA-50). Falls back to the today-only series if no continuous data yet.
function contSMA(sym, period){
  var S=STATE[sym]||{};
  var cc=S.contCloses;
  if(cc && cc.length>=period){
    var sum=0;
    for(var i=cc.length-period;i<cc.length;i++){ sum+=cc[i].c; }
    return sum/period;
  }
  return null;
}
// SMA of the continuous series evaluated at the point where today's closed-bar
// index `k` sits (k = index into closedCandles). Maps to the continuous array via
// contPriorCount so the 20-bar trend window can walk today's bars with a proper
// rolling continuous SMA at each step. Returns null if not enough history.
function contSMAAtTodayIdx(sym, period, k){
  var S=STATE[sym]||{};
  var cc=S.contCloses;
  if(!cc || !cc.length) return null;
  var prior = S.contPriorCount||0;
  var end = prior + k;                 // continuous index of today's k-th closed bar
  if(end < period-1) return null;      // not enough continuous history yet
  if(end > cc.length-1) end = cc.length-1;
  var sum=0;
  for(var i=end-period+1;i<=end;i++){ sum+=cc[i].c; }
  return sum/period;
}
// (v10.23 Issue C) primary SMA accessor. Continuous first (chart-aligned);
// falls back to today-only if the continuous series is unavailable OR fails a
// sanity check (per Option-A: verify; if the continuous value doesn't make sense,
// fall back). Sanity = SMA must sit within a sane band of the current price
// (a corrupt/misaligned continuous series would drift absurdly far from spot).
function todayOnlySMA(sym, period){
  var c=closedCandles(sym);
  if(c.length<period) return null;
  var sum=0;
  for(var i=c.length-period;i<c.length;i++){ sum+=c[i].c; }
  return sum/period;
}
function smaVal(sym, period){
  var cs=contSMA(sym, period);
  if(cs!=null){
    var px=STATE[sym]?STATE[sym].price:null;
    // one-shot sanity: an SMA-50 should not be more than ~5% away from spot on a
    // liquid index intraday; if it is, the continuous series is suspect -> fallback.
    if(px==null || Math.abs(cs-px) <= mul(px,0.05)) return cs;
    var fb=todayOnlySMA(sym, period);
    if(fb!=null){ SMA_CONT_FLAG[sym]=true; return fb; }
    return cs;
  }
  return todayOnlySMA(sym, period);
}
// (v10.18) Trend-GATED breakouts, per spec. Only surface breakouts aligned with
// the trend picture, and require the bar to close BEYOND the 50-MA:
//   • upside break (long)  allowed when trend is Uptrend OR Downtrend-broken, AND close > MA
//   • downside break (short) allowed when trend is Downtrend OR Uptrend-broken, AND close < MA
// A confirmed-trend break = high conviction; a broken-trend break = low/early
// (still allowed, since a broken uptrend is where the first downside breaks appear).
function trendOkFor(sym, dir){
  if(!CFG.trendOn) return true;
  var mp=parseInt(CFG.trendMA[sym],10);
  if(isNaN(mp)||mp<1){ mp=50; }
  var ma=smaVal(sym, mp);
  var px=STATE[sym]?STATE[sym].price:null;
  // 50-MA close filter (hard requirement when we have an MA + price)
  if(ma!=null && px!=null){
    if(dir==='long' && !(px>ma)) return false;
    if(dir==='short' && !(px<ma)) return false;
  }
  // trend-state permission
  var st=trendVerdict(sym).state;
  if(dir==='long')  return (st==='up' || st==='dn-broken');   // uptrend, or downtrend breaking up
  /* short */       return (st==='dn' || st==='up-broken');   // downtrend, or uptrend breaking down
}
// conviction tag for a breakout given the current trend state (used by BO labels)
function breakoutConviction(sym, dir){
  var st=trendVerdict(sym).state;
  if(dir==='long')  return st==='up' ? 'high' : (st==='dn-broken'?'early':'none');
  return st==='dn' ? 'high' : (st==='up-broken'?'early':'none');
}

// (v10.18) FIVE-STATE trend machine per spec:
//   'up'      = confirmed uptrend (>=16/20 bars above MA+band)
//   'up-broken'  = was uptrend, dominance lost, downtrend not yet confirmed (amber caution)
//   'dn'      = confirmed downtrend (>=16/20 below MA-band)
//   'dn-broken'  = was downtrend, dominance lost, uptrend not yet confirmed (amber caution)
//   'flat'    = never-confirmed / genuine chop
//   'na'      = insufficient bars (<mp+1)
// "Broken" is the MANDATORY middle step: uptrend -> up-broken -> downtrend (and inverse),
// signalling the momentum pause that precedes a reversal. Uses TREND_LAST memory of the
// last CONFIRMED direction so we can tell a break from plain chop.
function trendVerdict(sym){
  var mp=parseInt(CFG.trendMA[sym],10);
  if(isNaN(mp)||mp<1) mp=50;
  var c=closedCandles(sym);
  var S=STATE[sym]||{};
  // (v10.23 Issue C) CONTINUOUS SMA: no longer requires mp+1 TODAY bars. We only
  // need (a) at least a couple of today closed bars to form a window, and (b)
  // enough CONTINUOUS history for a full-period SMA (checked per-bar below).
  if(c.length<2) return { state:'na', up:0, dn:0, win:0, ma:null, slope:0 };
  var haveCont = !!(S.contCloses && S.contCloses.length >= mp);
  // If we have neither continuous history NOR mp+1 today bars, we truly can't judge.
  if(!haveCont && c.length<mp+1) return { state:'na', up:0, dn:0, win:0, ma:null, slope:0 };
  var win=Math.min(TREND_WINDOW, haveCont ? c.length : (c.length-mp));
  if(win<1) return { state:'na', up:0, dn:0, win:0, ma:null, slope:0 };
  var band=mul(atr(sym), TREND_BANDX);
  var up=0, dn=0, lastMa=null, firstMa=null, counted=0;
  for(var i=c.length-win;i<c.length;i++){
    var ma;
    if(haveCont){
      ma=contSMAAtTodayIdx(sym, mp, i);
      if(ma==null){
        // continuous SMA not defined this far back -> fall back to today-only inline
        if(i-mp+1 < 0) continue;
        var s2=0; for(var j2=i-mp+1;j2<=i;j2++){ s2+=c[j2].c; } ma=s2/mp;
      }
    } else {
      var sum=0;
      for(var j=i-mp+1;j<=i;j++){ sum+=c[j].c; }
      ma=sum/mp;
    }
    if(ma==null) continue;
    if(firstMa==null) firstMa=ma;
    lastMa=ma;
    counted++;
    var px=c[i].c;
    if(px > ma+band) up++;
    else if(px < ma-band) dn++;
  }
  if(counted<1) return { state:'na', up:0, dn:0, win:0, ma:null, slope:0 };
  win=counted;   // report the number of bars actually scored
  var slope = (lastMa!=null && firstMa!=null) ? (lastMa-firstMa) : 0;
  // 16/20 dominance over ALL 20 bars (per user: count vs the full window).
  var confUp = up>=TREND_DOM;
  var confDn = dn>=TREND_DOM;
  var prior = TREND_LAST[sym];   // 'up' | 'dn' | null
  var state;
  if(confUp){ state='up'; TREND_LAST[sym]='up'; }
  else if(confDn){ state='dn'; TREND_LAST[sym]='dn'; }
  else if(prior==='up'){ state='up-broken'; }   // was up, lost 16/20, dn not yet 16 -> broken
  else if(prior==='dn'){ state='dn-broken'; }
  else { state='flat'; }
  var dom = (state==='dn'||state==='dn-broken') ? dn : up;
  return { state:state, up:up, dn:dn, dom:dom, win:win, ma:lastMa, slope:slope };
}
// helpers so the rest of the app reads the machine consistently
function trendIsUpish(s){ return s==='up'; }
function trendIsDnish(s){ return s==='dn'; }
function trendWordOf(state){
  return state==='up'?'Uptrend':state==='dn'?'Downtrend':
         state==='up-broken'?'Uptrend broken':state==='dn-broken'?'Downtrend broken':
         state==='flat'?'No trend':'\u2013';
}
function trendColorOf(state){
  if(state==='up') return PAL.longAccent;
  if(state==='dn') return PAL.shortAccent;
  if(state==='up-broken'||state==='dn-broken') return PAL.amber;   // caution
  return PAL.sub;
}
// (v10.23 Issue G) compact state code for the trend badge.
function trendCodeOf(state){
  return state==='up'?'UP':state==='dn'?'DN':
         state==='up-broken'?'UP-BRK':state==='dn-broken'?'DN-BRK':
         state==='flat'?'SIDE':'\u2013';
}
// (v10.23 Issue G) TREND BADGE — matches the King badge chrome (same height + pill),
// stacked two-line: TOP = state code (colored by state); BOTTOM = dominant-side
// bar-count with a state-colored arrow (\u2191 above SMA / \u2193 below). A slope tick sits
// to the RIGHT of the stack, vertically centered (parallel to the King node's offset
// arrow), colored by slope DIRECTION (green rising / red falling / grey flat).
function trendBadgeHtml(sym){
  var tv = trendVerdict(sym);
  var st = tv.state;
  var col = trendColorOf(st);
  var period = (function(){ var mp=parseInt(CFG.trendMA[sym],10); return (isNaN(mp)||mp<1)?50:mp; })();
  var code = trendCodeOf(st);
  // dominant-side count + arrow (\u2191 = bars ABOVE SMA, \u2193 = below). Arrow colored by state.
  var upish = (st==='up'||st==='up-broken');
  var dnish = (st==='dn'||st==='dn-broken');
  var cntArrow = upish?'\u2191':(dnish?'\u2193':'');
  var cnt = upish?(tv.up+'/'+tv.win):(dnish?(tv.dn+'/'+tv.win):'\u2013');
  // slope tick: \u2197 rising / \u2198 falling / \u2192 flat, colored by direction
  var slopeGlyph = tv.slope>0?'\u2197':(tv.slope<0?'\u2198':'\u2192');
  var slopeCol = tv.slope>0?PAL.longAccent:(tv.slope<0?PAL.shortAccent:PAL.sub);
  var bottom;
  if(st==='na'){
    // warming up: no numbers (should be rare once continuous SMA / Issue C lands)
    code='warming up'; col=PAL.sub;
    bottom='<span style="font-size:11px;font-weight:700;color:'+PAL.sub+'">\u2013</span>';
    slopeGlyph=''; 
  } else {
    bottom='<span style="font-weight:800;color:'+col+'">'+cntArrow+'</span>'+
           '<span style="color:'+PAL.sub+';font-weight:700;font-variant-numeric:tabular-nums">'+cnt+'</span>';
  }
  var tip=('Trend: '+ (st==='na'?'warming up (need continuous SMA).':
           (tv.up>=tv.dn?tv.up:tv.dn)+'/'+tv.win+' bars '+(upish?'above':(dnish?'below':'around'))+' SMA'+period+' (\u00b10.25 ATR band); '+
           'SMA '+(tv.slope>0?'rising':(tv.slope<0?'falling':'flat'))+' (slope '+(tv.slope||0).toFixed(3)+'). '+
           'Broken = dominance lost, opposite not yet confirmed.')).replace(/"/g,'');
  var slopeHtml = slopeGlyph ? ('<span style="font-size:14px;font-weight:800;line-height:1;color:'+slopeCol+'">'+slopeGlyph+'</span>') : '';
  return '<span title="'+tip+'" style="display:inline-flex;align-items:center;gap:7px;padding:3px 12px;height:34px;box-sizing:border-box;border:1.5px solid '+col+';border-radius:16px;background:'+PAL.card+'">'+
    '<span style="display:inline-flex;flex-direction:column;align-items:center;line-height:1.05">'+
      '<span style="font-weight:800;font-size:13px;letter-spacing:.3px;color:'+col+'">'+code+'</span>'+
      '<span style="width:100%;height:1px;background:'+PAL.line+';margin:1px 0"></span>'+
      '<span style="font-size:11px;display:inline-flex;gap:3px;align-items:center">'+bottom+'</span>'+
    '</span>'+ slopeHtml +
  '</span>';
}

function saveLog(){ }
var LOG = null;

function stashSlice(sym, j){
  try{
    var raw=localStorage.getItem(SLICE_KEY); var o=raw?JSON.parse(raw):{};
    if(o.date!==TODAY) o={date:TODAY, SPY:[], QQQ:[]};
    var lv=j.levels||[]; if(!lv.length) return;
    var last=lv[lv.length-1];
    var arr=o[sym]||[];
    var snap={t:last.t, s:last.s, l:(last.l||[]).map(function(n){return {k:n.k,v:n.v,d:n.d,net:(typeof n.net==='number'?n.net:null)};})};
    if(!arr.length || arr[arr.length-1].t!==last.t){
      arr.push(snap);
      if(arr.length>500) arr.shift();
    } else {
      // Same 3m bar still forming: overwrite the current slice in place so the
      // newest stored slice always mirrors the LIVE tape (matches extractWalls).
      arr[arr.length-1]=snap;
    }
    o[sym]=arr; localStorage.setItem(SLICE_KEY, JSON.stringify(o));
  }catch(e){}
}

function slicesFor(sym){
  try{
    var raw=localStorage.getItem(SLICE_KEY); if(!raw) return [];
    var o=JSON.parse(raw); if(!o || o.date!==TODAY) return [];
    return o[sym]||[];
  }catch(e){ return []; }
}

function rawAccumMap(sym){
  var slices=slicesFor(sym);
  if(!slices.length) return {};
  var take=Math.min(ACC_WINDOW, slices.length);
  var window=slices.slice(slices.length-take);
  var snaps=[];
  var step=ACC_SAMPLE_STEP;
  for(var i=window.length-1; i>=0 && snaps.length<ACC_SAMPLES; i-=step){ snaps.unshift(window[i]); }
  if(!snaps.length) snaps=[window[window.length-1]];

  function kingOf(s){
    var k=0;
    (s.l||[]).forEach(function(n){ if(n.v>k) k=n.v; });
    return k;
  }

  var byK={};
  snaps.forEach(function(s){
    var kg=kingOf(s) || STATE[sym].king;
    (s.l||[]).forEach(function(n){
      var pct=Math.round(mul(100, n.v/kg));
      var key=n.k.toFixed(2);
      if(!byK[key]) byK[key]={k:n.k, seq:[], absSeq:[], net:(typeof n.net==='number'?n.net:null), pos:(n.d>0)};
      byK[key].seq.push(pct);
      byK[key].absSeq.push(n.v);
      if(typeof n.net==='number') byK[key].net=n.net;
      byK[key].pos=(n.d>0);
    });
  });

  for(var key in byK){
    var r=byK[key];
    if(r.seq.length){
      r.first=r.seq[0];
      r.last=r.seq[r.seq.length-1];
      r.delta=r.last-r.first;
    } else {
      r.first=null; r.last=null; r.delta=null;
    }
    if(r.absSeq.length){
      r.absFirst=r.absSeq[0];
      r.absLast=r.absSeq[r.absSeq.length-1];
      r.absDelta=r.absLast-r.absFirst;
    } else {
      r.absFirst=null; r.absLast=null; r.absDelta=null;
    }
  }
  return byK;
}
function rawAccumNode(sym, k){
  if(typeof k!=='number') return null;
  var map=rawAccumMap(sym);
  return map[k.toFixed(2)] || null;
}

function accumData(sym){
  var slices=slicesFor(sym);
  if(!slices.length) return [];
  var take=Math.min(ACC_WINDOW, slices.length);
  var window=slices.slice(slices.length-take);
  var snaps=[];
  var step=ACC_SAMPLE_STEP;
  for(var i=window.length-1; i>=0 && snaps.length<ACC_SAMPLES; i-=step){ snaps.unshift(window[i]); }
  if(!snaps.length) snaps=[window[window.length-1]];
  function kingOf(s){ var k=0; (s.l||[]).forEach(function(n){ if(n.v>k) k=n.v; }); return k; }
  var byK={};
  snaps.forEach(function(s){
    var kg=kingOf(s) || STATE[sym].king; if(kg<=0) return;
    (s.l||[]).forEach(function(n){
      var pct=Math.round(mul(100, n.v/kg));
      var key=n.k.toFixed(2);
      if(!byK[key]) byK[key]={k:n.k, seq:[], absSeq:[], net:(typeof n.net==='number'?n.net:null), pos:(n.d>0)};
      byK[key].seq.push(pct);
      byK[key].absSeq.push(n.v);
      if(typeof n.net==='number') byK[key].net=n.net;
      byK[key].pos=(n.d>0);
    });
  });
  var rows=[];
  for(var key in byK){
    var r=byK[key];
    if(r.seq.length<2) continue;
    r.last=r.seq[r.seq.length-1];
    r.first=r.seq[0];
    r.delta=r.last-r.first;
    if(r.absSeq.length){
      r.absLast=r.absSeq[r.absSeq.length-1];
      r.absFirst=r.absSeq[0];
      r.absDelta=r.absLast-r.absFirst;
    }
    if(r.last<ACC_FLOOR_PCT) continue;
    if(r.delta<ACC_NET) continue;
    r.hero=(r.delta>=15 && r.last>=40);
    rows.push(r);
  }
  rows.sort(function(a,b){ return b.delta-a.delta; });
  return rows.slice(0, ACC_ROWS);
}
function livePctAt(sym, k){
  // Prefer the value Skylit actually renders on its heatmap tape, so ACM %s
  // match the tape exactly. Fall back to the script's own derived walls.
  var tp = tapeMap(sym);
  if(tp && tp.pct){
    var key=k.toFixed(2);
    if(typeof tp.pct[key]==='number') return tp.pct[key];
  }
  var walls=STATE[sym].walls||[];
  for(var i=0;i<walls.length;i++){
    if(Math.abs(walls[i].k - k) < 0.001) return walls[i].pct;
  }
  return null;
}

// ---- Read Skylit's rendered heatmap tape straight from the DOM ----
// To be fully consistent with the tape (percentages AND which strike is King)
// we read the numbers Skylit already renders instead of re-deriving them. The
// heatmap sidebar is the <table> whose header row contains "Strike". Each data
// row is one strike; the King row is the one carrying a "$...K" dollar value.
// We read ONLY the nearest-expiry column (the first data column), which is the
// %King strength you read when trading. Keyed off the data pattern + the
// "Strike" header, not CSS classes, so it survives Skylit markup changes.
var TAPE_CACHE = { SPY:{t:0, data:null}, QQQ:{t:0, data:null} };
function tapeMap(sym){
  try{
    var now=Date.now();
    var c=TAPE_CACHE[sym];
    if(c && c.data && (now-c.t)<1000) return c.data;
    var data=readTapeFromDOM(sym);
    if(data && data.count>=5){ TAPE_CACHE[sym]={t:now, data:data}; return data; }
    return c ? c.data : null;
  }catch(e){ return null; }
}
// Matches Skylit's King dollar-exposure cell, e.g. "$189,852K" or "-$189,852K"
// (Skylit uses a real minus OR the unicode minus sign \u2212).
var KING_DOLLAR_RE = /^[+\-\u2212]?\$[\d,]+K$/;
// v10.22 FIX: the FLOW BUCKET / "Top contracts" popup ALSO carries a "Strike"
// header and a "$...K" cell, and sits EARLIER in the DOM, so the old
// first-match logic latched onto it and crowned the wrong strike (e.g. 775
// from the flow popup instead of the true GEX King 773 on the ladder). We now
// (a) hard-reject the flow-bucket popup, (b) require the GEX-ladder fingerprint
// (ISO expiry-date column headers OR a deep strike list), and (c) among
// survivors pick the one with the MOST strike rows (the real ladder, never a
// small popup). Keyed off rendered data, not CSS classes.
var TAPE_REJECT_RE = /FLOW BUCKET|Top contracts|Pick range end/i;   // flow popup markers
var ISO_DATE_RE = /\b20\d{2}-\d{2}-\d{2}\b/;                          // expiry column header
function tapeStrikeRowCount(txt){
  // count distinct strike-looking tokens "NNN.0" / "NNN" (>=50) as a size proxy
  var m=(''+txt).match(/\b\d{2,5}(?:\.\d+)?\b/g); if(!m) return 0;
  var seen={}, c=0;
  for(var i=0;i<m.length;i++){ var v=parseFloat(m[i]); if(v>=50 && !seen[m[i]]){ seen[m[i]]=1; c++; } }
  return c;
}
function findTapeTable(){
  function ok(txt){
    if(txt.indexOf('Strike')===-1) return false;
    if(!/[+\-\u2212]?\$[\d,]+K/.test(txt)) return false;   // needs a King $K cell
    if(TAPE_REJECT_RE.test(txt)) return false;              // NOT the flow popup
    // real ladder: has expiry-date headers OR is a deep strike list
    return ISO_DATE_RE.test(txt) || tapeStrikeRowCount(txt) >= 15;
  }
  var best=null, bestRows=-1;
  var tables=document.querySelectorAll('table');
  for(var i=0;i<tables.length;i++){
    var head=(tables[i].textContent||'');
    if(ok(head)){ var r=tapeStrikeRowCount(head); if(r>bestRows){ bestRows=r; best=tables[i]; } }
  }
  if(best) return best;
  // Fallback: a container matching the same fingerprint, smallest subtree wins ties.
  var all=document.querySelectorAll('div,section');
  for(var j=0;j<all.length;j++){
    var t=all[j].textContent||'';
    if(ok(t) && all[j].querySelectorAll('*').length<1200){
      var r2=tapeStrikeRowCount(t); if(r2>bestRows){ bestRows=r2; best=all[j]; }
    }
  }
  return best;
}
// Grid cells of the tape, in document order, each collapsed to its LEADING
// value token. Skylit renders a column value like "98%" as the cell's own
// text with the change delta ("+3%") as a NESTED child, so the cell's leading
// token (own text, which precedes the child) is the real nearest-expiry value
// and the nested chip must be ignored. We therefore pick the TOP-MOST elements
// whose leading token is a strike / percent / $K value, and never descend into
// them (so a nested +3% chip is never read as its own cell).
var TAPE_TOK_RE = /^\s*([+\-\u2212]?\$[\d,]+K|[+\-\u2212]?\d{1,3}%|\d{2,5}(?:\.\d+)?)/;
function leadTok(el){
  var t=(el.textContent||'').replace(/\s+/g,'');
  var m=t.match(TAPE_TOK_RE);
  return m ? m[1] : null;
}
function tapeCells(table){
  var out=[];
  (function walk(node){
    var kids=node.children;
    for(var i=0;i<kids.length;i++){
      var el=kids[i];
      var tok=leadTok(el);
      if(tok!=null){ out.push(tok); }      // top-most value cell: stop here
      else { walk(el); }                    // structural wrapper: descend
    }
  })(table);
  return out;
}
function readTapeFromDOM(sym){
  var table=findTapeTable();
  if(!table) return null;
  var pct={}, kingK=null, count=0;
  // ---- Path A: a real <tr>/<td> table (legacy / forward-compatible). ----
  var trs=table.querySelectorAll('tr');
  if(trs.length){
    for(var i=0;i<trs.length;i++){
      var cells=trs[i].children;
      if(!cells || cells.length<2) continue;
      var strike=parseFloat((cells[0].textContent||'').replace(/[^0-9.]/g,''));
      if(!isFinite(strike) || strike<50) continue;
      var rowTxt=(trs[i].textContent||'');
      var v=firstStrengthPct(cells[1]!=null?cells[1].textContent:'');
      if(v==null && cells.length>2) v=firstStrengthPct(cells[2].textContent);
      var isKingRow=/[+\-\u2212]?\$[\d,]+K/.test(rowTxt);
      // v10.22: DON'T clobber the King row's real %King with 100; keep the
      // parsed strength (King is 96% here, not 100). Only mark the $K strike.
      if(v!=null){ pct[strike.toFixed(2)]=v; count++; }
      else if(isKingRow){ pct[strike.toFixed(2)]=100; count++; }   // no readable % -> fallback
      if(isKingRow) kingK=strike;
    }
    if(count>=5) return kingResolve(pct, kingK, count);
  }
  // ---- Path B: div/grid table (current Skylit). Grid cells arrive in
  // document order as strike-then-values; each value cell is collapsed to its
  // leading token (nested change chips ignored). Assign the FIRST value cell
  // after a strike as that strike's nearest-expiry %King (sign kept). The $K
  // King dollar cell marks the King row. ----
  pct={}; kingK=null; count=0;
  var gcells=tapeCells(table);
  var curStrike=null, haveVal=false;
  var strikeRe=/^(\d{2,5}(?:\.\d+)?)$/;      // "776.0" (or "776")
  for(var L=0;L<gcells.length;L++){
    var txt=gcells[L];
    var sm=txt.match(strikeRe);
    if(sm){
      var sv=parseFloat(sm[1]);
      // v10.22: reject a bare 4-digit YEAR from an expiry column header
      // (e.g. "2026") so it can't be mistaken for a strike and pollute the map.
      var isYear=/^(19|20)\d{2}$/.test(txt);
      if(isFinite(sv) && sv>=50 && !isYear){ curStrike=sv; haveVal=false; }
      continue;
    }
    if(curStrike==null) continue;
    var key=curStrike.toFixed(2);
    if(KING_DOLLAR_RE.test(txt)){
      kingK=curStrike;
      // v10.22: the %King value cell precedes the $K cell in grid order, so a
      // real strength (e.g. 96) is usually already set. Keep it; only fill 100
      // as a last resort when no % was readable for this strike.
      if(typeof pct[key]!=='number'){ pct[key]=100; count++; }
      haveVal=true;
      continue;
    }
    if(!haveVal){
      var v2=leadSignedPct(txt);
      if(v2!=null){ pct[key]=v2; haveVal=true; count++; }
    }
  }
  if(count>=5) return kingResolve(pct, kingK, count);
  return null;
}
// v10.22 CROSS-CHECK: docs say King = largest ABSOLUTE dealer exposure. The $K
// dollar tag is Skylit's own marker, but if it ever disagrees with the strike
// carrying the largest parsed |%King|, we trust the DATA (the tape values) and
// flag the disagreement so downstream can caveat it. This makes a single
// mis-tagged/misplaced $K cell unable to silently crown the wrong strike.
function kingResolve(pct, kingK, count){
  var maxK=null, maxV=-1;
  for(var k in pct){ if(!pct.hasOwnProperty(k)) continue;
    var a=Math.abs(pct[k]); if(a>maxV){ maxV=a; maxK=parseFloat(k); } }
  var out={ pct:pct, king:kingK, count:count, kingSrc:'dollar', kingConflict:false };
  if(kingK==null){                       // no $K tag seen -> fall back to data
    out.king=maxK; out.kingSrc='maxpct'; return out;
  }
  var tagV=Math.abs(pct[kingK.toFixed(2)]||0);
  // If the largest-|value| strike is a DIFFERENT strike AND meaningfully bigger
  // than the $K-tagged one, the tag is untrustworthy -> trust the data.
  if(maxK!=null && maxK!==kingK && maxV >= tagV + 5){
    out.king=maxK; out.kingSrc='maxpct-override'; out.kingConflict=true;
    out.kingTagged=kingK;   // keep what the $K said, for diagnostics
  }
  return out;
}
// Leading signed %King from a single leaf cell, e.g. "-39%" -> -39, "25%" -> 25.
function leadSignedPct(txt){
  var m=(''+txt).replace(/\s+/g,'').match(/^([+\-\u2212]?)(\d{1,3})%$/);
  if(!m) return null;
  var v=parseInt(m[2],10);
  if(m[1]==='-'||m[1]==='\u2212') v=-v;
  return v;
}
// From a cell's text, return the unsigned %King strength (ignore signed change).
function firstStrengthPct(txt){
  if(!txt) return null;
  txt=(''+txt).replace(/\s+/g,' ').trim();
  var re=/([+\-\u2212]?)(\d{1,3})%/g, m, val=null;
  while((m=re.exec(txt))!==null){ if(m[1]==='') val=parseInt(m[2],10); }
  return val;
}
// Tape's King strike (what Skylit marks with the $K cell), for classification.
function tapeKingStrike(sym){
  var tp=tapeMap(sym);
  return (tp && typeof tp.king==='number') ? tp.king : null;
}

// ---- Per-node 1-minute %King history (read from the tape) ----
// HIST[sym][strikeKey] = { last:ts, seq:[{t,v}, ...] } keeping the last HIST_MAX
// minute-samples. This is the trajectory the strip renders AND the badge is
// judged on, so the tag and the visible history can never disagree.
var HIST = { SPY:{}, QQQ:{} };
// Session peak/low %King per node, keyed by strike, reset per trading day.
var PEAK = { SPY:{}, QQQ:{} };
// Live (between-close) %King per node, so the RAPID flag can read the current
// tape value even though the strip only advances on 3m closes.
var LIVEPCT = { SPY:{}, QQQ:{} };
function todayKey(){ var d=new Date(); return d.getFullYear()+'-'+(d.getMonth()+1)+'-'+d.getDate(); }
function sampleTapeHistory(sym){
  var tp=tapeMap(sym);
  if(!tp || !tp.pct) return;
  var now=Date.now();
  var store=HIST[sym] || (HIST[sym]={});
  var pk=PEAK[sym] || (PEAK[sym]={});
  var lv=LIVEPCT[sym] || (LIVEPCT[sym]={});
  var dk=todayKey();
  for(var key in tp.pct){
    var v=tp.pct[key];
    if(typeof v!=='number') continue;
    lv[key]=v; // live value drives the rapid flag between 3m closes
    var p=pk[key];
    if(!p || p.day!==dk){ p=pk[key]={peak:v, peakT:now, low:v, lowT:now, day:dk}; }
    if(v>p.peak){ p.peak=v; p.peakT=now; }
    if(v<p.low){ p.low=v; p.lowT=now; }
    var rec=store[key] || (store[key]={last:0, seq:[]});
    // Advance the strip once per 3m bar close; between closes update the last
    // point in place so it tracks the live tape without adding a point.
    if(now - rec.last >= HIST_SAMPLE_MS || !rec.seq.length){
      rec.seq.push({t:now, v:v});
      rec.last=now;
      if(rec.seq.length>HIST_MAX) rec.seq.shift();
    } else {
      rec.seq[rec.seq.length-1]={t:now, v:v};
    }
  }
  // Drop history for strikes no longer on the tape (keep the store small).
  for(var k2 in store){ if(!(k2 in tp.pct)){ /* keep briefly */ } }
  // Track the King strike over time to detect ROLLING, and record the confirmed
  // daily journey (persisted).
  if(typeof tp.king==='number'){
    var kh=KINGHIST[sym] || (KINGHIST[sym]={last:0, seq:[]});
    if(now - kh.last >= HIST_SAMPLE_MS || !kh.seq.length){
      kh.seq.push({t:now, k:tp.king}); kh.last=now;
      if(kh.seq.length>HIST_MAX) kh.seq.shift();
    } else {
      kh.seq[kh.seq.length-1]={t:now, k:tp.king};
    }
    updateKingJourney(sym, tp.king); // #3 / King persistence
  }
}
// King-strike history, for rolling detection.
var KINGHIST = { SPY:{last:0,seq:[]}, QQQ:{last:0,seq:[]} };
// Direction the King has ROLLED over the recent window: +1 up (bullish), -1
// down (bearish), 0 none. A floor rolling to a higher strike is bullish; a
// ceiling rolling to a lower strike is bearish (Skylit).
function kingRoll(sym){
  var kh=KINGHIST[sym]; if(!kh || kh.seq.length<3) return 0;
  var first=kh.seq[0].k, last=kh.seq[kh.seq.length-1].k;
  if(last - first >= 1) return 1;   // King moved up >=1 strike over the window
  if(first - last >= 1) return -1;  // King moved down >=1 strike
  return 0;
}

// ===== King daily journey (persisted per trading day) #3 + King persistence =====
// KINGDAY[sym] = { day, cur, moves:[{k,dir,t}], count }. A move is recorded only
// on a CONFIRMED change to a genuinely different strike (guarded against a
// one-poll flicker), so the count is trustworthy. Persisted to localStorage so a
// reload mid-session does not reset 'Moved N× today'. Resets on a new day.
var KINGDAY = { SPY:null, QQQ:null };
var KINGDAY_KEY = 'gpts_kingday_v1';
var KING_CONFIRM = { SPY:{k:null,n:0}, QQQ:{k:null,n:0} }; // flicker guard
var KING_CONFIRM_N = 2; // require this many consecutive polls on a new strike
function loadKingDay(){
  try{
    var raw=localStorage.getItem(KINGDAY_KEY);
    if(!raw) return;
    var o=JSON.parse(raw);
    var dk=todayKey();
    ['SPY','QQQ'].forEach(function(s){
      if(o[s] && o[s].day===dk) KINGDAY[s]=o[s]; // only rehydrate today's journey
    });
  }catch(e){}
}
function saveKingDay(){ try{ localStorage.setItem(KINGDAY_KEY, JSON.stringify(KINGDAY)); }catch(e){} }
function updateKingJourney(sym, king){
  if(typeof king!=='number') return;
  var dk=todayKey();
  var kd=KINGDAY[sym];
  if(!kd || kd.day!==dk){ // new day (or first ever) -> fresh journey seeded at current King
    kd=KINGDAY[sym]={ day:dk, cur:king, moves:[{k:king, dir:0, t:Date.now()}], count:0 };
    KING_CONFIRM[sym]={k:null,n:0};
    saveKingDay();
    return;
  }
  if(Math.abs(king-kd.cur)<0.001){ KING_CONFIRM[sym]={k:null,n:0}; return; } // unchanged
  // Candidate change: require KING_CONFIRM_N consecutive polls on the SAME new
  // strike before recording, so a flicker cannot inflate the count.
  var cf=KING_CONFIRM[sym];
  if(cf.k!=null && Math.abs(cf.k-king)<0.001){ cf.n++; }
  else { cf.k=king; cf.n=1; }
  if(cf.n>=KING_CONFIRM_N){
    var dir = king>kd.cur ? 1 : -1;
    kd.moves.push({k:king, dir:dir, t:Date.now()});
    if(kd.moves.length>30) kd.moves.shift();
    kd.cur=king; kd.count=(kd.count||0)+1;
    KING_CONFIRM[sym]={k:null,n:0};
    saveKingDay();
  }
}
function kingDay(sym){ return KINGDAY[sym]; }
// #3 ex-King check: was this strike the King earlier today but is not now? If so
// the signal anchored to it is built on a premise that has changed. Returns the
// {from,to,t} of the roll that took King off this strike, or null.
function exKingInfo(sym, strike){
  var kd=kingDay(sym); if(!kd || !kd.moves || !kd.moves.length) return null;
  var curKing=kd.cur;
  if(Math.abs(curKing-strike)<0.001) return null; // still King, not ex-King
  var wasKing=false, whenLeft=null, toK=null;
  for(var i=0;i<kd.moves.length;i++){
    if(Math.abs(kd.moves[i].k-strike)<0.001) wasKing=true;
    else if(wasKing && whenLeft==null){ whenLeft=kd.moves[i].t; toK=kd.moves[i].k; }
  }
  if(!wasKing) return null;
  return { from:strike, to:(toK!=null?toK:curKing), t:whenLeft };
}
function nodeHistory(sym, k){
  var store=HIST[sym]; if(!store) return null;
  var rec=store[k.toFixed(2)];
  if(!rec || !rec.seq.length) return null;
  var arr=rec.seq.map(function(p){ return p.v; });
  // Display/judge only the last CFG.stripLen closes (storage keeps more).
  var n=(CFG&&CFG.stripLen)?CFG.stripLen:8;
  return arr.length>n ? arr.slice(arr.length-n) : arr;
}
// Session peak/low for a strike (context beyond the visible strip). #2
function nodePeak(sym, k){
  var pk=PEAK[sym]; if(!pk) return null;
  var p=pk[k.toFixed(2)];
  return p || null;
}
// Live %King for a strike (between 3m closes), used by the rapid flag. #2
function liveNodePct(sym, k){
  var lv=LIVEPCT[sym]; if(!lv) return null;
  var v=lv[k.toFixed(2)];
  return (typeof v==='number') ? v : null;
}
// Build RATE for a node over the visible window: net %King change per sample.
// Positive = accumulating, negative = dissipating. Used by #1/#2/#8. #2
function nodeBuildRate(sym, k){
  var h=nodeHistory(sym, k);
  if(!h || h.length<2) return 0;
  return (h[h.length-1]-h[0]);
}

// ===== S/R BATTLE (v10.14) =====
// A DISSIPATION-DOMINANT support-vs-resistance force model, validated against
// the user's real recorder tape (2026-08-11). The design lesson from that data:
// naive "count building nodes" stays green all the way down a grind because SPY
// support keeps REBUILDING one strike lower. What actually marks the turn is the
// NEAREST level dissipating while price sits on it. So:
//   resistanceForce = Σ over ABOVE nodes of prox*net (+ heavy penalty if the
//                     nearest ceiling is Fading \u2014 that ceiling giving way is bullish)
//   supportForce    = Σ over BELOW nodes of prox*net (+ heavy penalty if the
//                     nearest floor is Fading \u2014 the floor giving way is bearish)
// prox = 1/(1+dist) so nearer strikes dominate. A Fading NEAREST level (|k-px|<=1)
// gets an extra rate*1.5 (rate is negative) so a crumbling in-play level swings
// its own side sharply negative \u2014 exactly the bear-pullback-high / bounce-low tell.
// Symmetric: pullback HIGH => resistance-dominant; bounce LOW => support-dominant.
// The module-level SRB_PREV lets us detect the CROSSOVER (the tradeable event).
var SRB_PREV = { SPY:null, QQQ:null };
// (v10.23 Issue D) STABILIZED imbalance state, committed on 3m bar close.
// SRB_STATE holds the last COMMITTED dom side + the bar bucket it was committed in,
// so intra-bar jitter never flips the badge. SRB_BAND = dead-band; a new side must
// clear SRB_FLIP_MULT * band (asymmetric hysteresis) OR hold across bar closes.
var SRB_STATE = { SPY:{dom:'balanced', bucket:-1, cand:null, candBuckets:0}, QQQ:{dom:'balanced', bucket:-1, cand:null, candBuckets:0} };
var SRB_BAND = 0.015;        // |imbalance| below this reads 'balanced' (relative scale)
var SRB_FLIP_MULT = 1.8;     // opposite side must exceed band*this to flip immediately
var SRB_HOLD_BARS = 2;       // ...or persist this many bar-closes to flip
// Per-render cache: srBattle is called by BOTH the S/R bar and the confluence
// engine in one render. Without caching, the 2nd call would see the 1st call's
// SRB_PREV write and never detect a crossover. RENDER_SEQ is bumped once per
// render(); srBattle returns the memoized result within the same tick.
var RENDER_SEQ = 0;
var SRB_CACHE = { SPY:{seq:-1,val:null}, QQQ:{seq:-1,val:null} };
function srBattle(sym){
  var c=SRB_CACHE[sym];
  if(c && c.seq===RENDER_SEQ) return c.val;
  var S=STATE[sym]||{};
  var px=S.price;
  if(px==null || !S.walls || !S.walls.length){ if(c){c.seq=RENDER_SEQ;c.val=null;} return null; }
  function sideForce(above){
    var f=0, nearestFade=null, nearestDist=1e9, nearestK=null, nearestSt=null;
    S.walls.forEach(function(w){
      if(w.k==null) return;
      var isAbove=(w.k>px);
      if(isAbove!==above) return;
      var dist=Math.abs(w.k-px);
      if(dist<0.001) return;
      var prox=1/(1+dist);
      var rate=nodeBuildRate(sym, w.k);         // signed %King net over the window
      var st=accumulationStateFor(sym, w.k);
      var label=(st&&st.label)?st.label:'Steady';
      var c=prox*rate;
      // extra penalty when the NEAREST level on this side is Fading (giving way)
      if(label==='Fading' && dist<=1.0){ c += rate*1.5; }
      f += c;
      if(dist<nearestDist){ nearestDist=dist; nearestFade=(label==='Fading'); nearestK=w.k; nearestSt=label; }
    });
    return { force:f, nearestK:nearestK, nearestSt:nearestSt, nearestFade:nearestFade };
  }
  var res=sideForce(true), sup=sideForce(false);
  // total for a 0..100 split (shift by min so negatives render sensibly)
  var lo=Math.min(sup.force,res.force,0);
  var s2=sup.force-lo, r2=res.force-lo, tot=s2+r2;
  var supPct = tot>0 ? Math.round(s2/tot*100) : 50;

  // ---- (v10.23 Issue D) IMBALANCE = divergence of the two sides' BUILD-RATES,
  // weighted by size+nearness, gated to the near-price window. This is the DYNAMIC
  // reframe: not 'who's bigger' but 'who is GAINING while the other FADES'.
  var imb = imbalanceMetric(sym, px);   // {val, gain, fade, bearish, forming}
  var rawDom;
  if(imb.forming || Math.abs(imb.val) < SRB_BAND) rawDom='balanced';
  else rawDom = imb.val < 0 ? 'resistance' : 'support';   // val<0 => resistance gaining => bearish

  // ---- COMMIT on 3m bar close with dead-band + asymmetric hysteresis ----
  var st=SRB_STATE[sym] || (SRB_STATE[sym]={dom:'balanced', bucket:-1, cand:null, candBuckets:0});
  var bucket = Math.floor(ctNowSecOfDay()/CANDLE_S)*CANDLE_S;
  var mag = Math.abs(imb.val);
  if(bucket!==st.bucket){
    // a new bar has closed -> re-evaluate the committed side
    if(rawDom==='balanced'){ st.dom='balanced'; st.cand=null; st.candBuckets=0; }
    else if(rawDom===st.dom){ st.cand=null; st.candBuckets=0; }   // reaffirm current side
    else {
      // opposite side wants to take over: flip immediately if it clears the wide
      // threshold, else require it to persist SRB_HOLD_BARS closes.
      if(mag >= SRB_BAND*SRB_FLIP_MULT){ st.dom=rawDom; st.cand=null; st.candBuckets=0; }
      else if(st.cand===rawDom){ st.candBuckets++; if(st.candBuckets>=SRB_HOLD_BARS){ st.dom=rawDom; st.cand=null; st.candBuckets=0; } }
      else { st.cand=rawDom; st.candBuckets=1; }
    }
    st.bucket=bucket;
  }
  var dom = st.dom;   // COMMITTED side (badge reads this; intra-bar jitter never shows)

  // crossover derived from the COMMITTED state (stability-first, fire later)
  var prev=SRB_PREV[sym];
  var cross='';
  if(prev && prev.dom && dom!=='balanced' && prev.dom!==dom){
    if(prev.dom==='support' && dom==='resistance') cross='bears';
    else if(prev.dom==='resistance' && dom==='support') cross='bulls';
  }
  var out={ dom:dom, cross:cross, supForce:+sup.force.toFixed(1), resForce:+res.force.toFixed(1),
            supPct:supPct, resPct:100-supPct,
            imb:+imb.val.toFixed(4), forming:imb.forming,
            gain:imb.gain, fade:imb.fade, bearish:imb.bearish,
            nearFloor:{k:sup.nearestK, st:sup.nearestSt, fading:sup.nearestFade},
            nearCeil:{k:res.nearestK, st:res.nearestSt, fading:res.nearestFade} };
  SRB_PREV[sym]={ dom:dom };
  if(c){ c.seq=RENDER_SEQ; c.val=out; }
  return out;
}
// (v10.23 Issue D) IMBALANCE metric: proximity+size-weighted divergence of the
// two sides' BUILD-RATES within the adaptive near-price window. Returns the signed
// value (negative = resistance gaining => bearish; positive = support gaining =>
// bullish), plus the specific GAINING and FADING strikes for the mechanism wording,
// and a `forming` flag when sample is too thin to assert.
function imbalanceMetric(sym, px){
  var S=STATE[sym]||{};
  var out={ val:0, gain:null, fade:null, bearish:false, forming:true };
  if(px==null || !S.walls || !S.walls.length) return out;
  var winStrikes = adaptiveProxStrikes(sym);
  var resSum=0, supSum=0, samples=0;
  var bestResBuild=null, bestSupBuild=null, bestResFade=null, bestSupFade=null;
  S.walls.forEach(function(w){
    if(w.k==null) return;
    var dist=Math.abs(w.k-px);
    if(dist<0.001) return;
    if(dist>winStrikes) return;                 // near-price window gate (Q4)
    var rate=nodeBuildRate(sym, w.k);            // signed %King net over window
    var h=nodeHistory(sym, w.k);
    if(h && h.length>=2) samples++;
    var prox=1/(1+dist);
    var sizeW = 1 + (w.pct!=null ? mul(w.pct,0.01) : 0);   // bigger walls weigh more
    var contrib = rate*prox*sizeW;
    // fade AT the in-play level weighted heaviest (Q4)
    if(dist<=1.0) contrib = mul(contrib,1.5);
    if(w.k>px){
      resSum += contrib;
      if(rate>0 && (!bestResBuild || rate>bestResBuild.rate)) bestResBuild={k:w.k,rate:rate};
      if(rate<0 && (!bestResFade  || rate<bestResFade.rate )) bestResFade ={k:w.k,rate:rate};
    } else {
      supSum += contrib;
      if(rate>0 && (!bestSupBuild || rate>bestSupBuild.rate)) bestSupBuild={k:w.k,rate:rate};
      if(rate<0 && (!bestSupFade  || rate<bestSupFade.rate )) bestSupFade ={k:w.k,rate:rate};
    }
  });
  // IMBALANCE = support-side gain minus resistance-side gain (divergence of deltas).
  // >0 => support gaining (bullish); <0 => resistance gaining (bearish).
  var val = supSum - resSum;
  // normalize to a relative scale so SRB_BAND is meaningful across symbols/regimes
  var scale = Math.max(Math.abs(supSum), Math.abs(resSum), 1);
  out.val = val/scale;
  out.forming = (samples < 3);                  // honest low-sample gate
  out.bearish = out.val < 0;
  // mechanism strikes: the gaining side's biggest builder + the other side's biggest fader
  if(out.val < 0){ out.gain = bestResBuild ? bestResBuild.k : (res_nearest(S,px,true)); out.fade = bestSupFade ? bestSupFade.k : null; }
  else            { out.gain = bestSupBuild ? bestSupBuild.k : null; out.fade = bestResFade ? bestResFade.k : null; }
  return out;
}
function res_nearest(S,px,above){ var best=null; (S.walls||[]).forEach(function(w){ if(w.k==null)return; var isA=w.k>px; if(isA!==above)return; if(!best||Math.abs(w.k-px)<Math.abs(best-px))best=w.k; }); return best; }
// Adaptive proximity: how many strikes a normal 12-30 min move covers NOW,
// derived from recent 3m bar range, clamped to [MIN,MAX]. #2
function adaptiveProxStrikes(sym){
  var S=STATE[sym]||{};
  var bars=(S.bars3m||S.candles||S.fiber||[]);
  var rng=0, cnt=0;
  if(bars && bars.length){
    for(var i=Math.max(0,bars.length-PROX_BARS); i<bars.length; i++){
      var b=bars[i]; if(!b) continue;
      var hi=(b.h!=null?b.h:b.high), lo=(b.l!=null?b.l:b.low);
      if(hi!=null && lo!=null){ rng+=(hi-lo); cnt++; }
    }
  }
  var avg = cnt ? (rng/cnt) : 0;
  var reach = avg * PROX_MOVE_BARS; // price points a typical bounce travels
  var strikes = Math.round(reach);  // SPY strikes are ~1 point
  if(!isFinite(strikes) || strikes<PROX_MIN_STRIKES) strikes=PROX_MIN_STRIKES;
  if(strikes>PROX_MAX_STRIKES) strikes=PROX_MAX_STRIKES;
  return strikes;
}
// RELATIVE trap read (#2): compare the nearest building support below price vs
// the nearest building resistance above (within adaptive proximity) and return
// a Long-friendly / Mixed / Trap verdict, symmetric for both directions.
function relativeRead(sym){
  var S=STATE[sym]||{};
  var px=S.price;
  if(px==null || !S.walls || !S.walls.length) return null;
  var prox=adaptiveProxStrikes(sym);
  var supBelow=null, resAbove=null;
  S.walls.forEach(function(w){
    if(w.k==null) return;
    var dist=Math.abs(w.k-px);
    if(dist>prox || dist<0.001) return;
    var rate=nodeBuildRate(sym, w.k);
    var cand={ k:w.k, pct:w.pct, rate:rate, dist:dist };
    if(w.k<px){ if(!supBelow || w.k>supBelow.k) supBelow=cand; }
    else       { if(!resAbove || w.k<resAbove.k) resAbove=cand; }
  });
  var supRate=supBelow?supBelow.rate:0;
  var resRate=resAbove?resAbove.rate:0;
  // Long view: support building beats overhead resistance building.
  var longScore = supRate - resRate;
  var verdict, cls;
  if(longScore >= HIST_STEADY_BAND){ verdict='Long-friendly'; cls='long'; }
  else if(longScore <= -HIST_STEADY_BAND){ verdict='Trap risk'; cls='trap'; }
  else { verdict='Mixed'; cls='mixed'; }
  return { verdict:verdict, cls:cls, prox:prox,
           sup:supBelow, res:resAbove, supRate:supRate, resRate:resRate,
           longScore:longScore };
}

// ===== #6 ABSORPTION (rejection) =====
// Price tested this node's strike (came within tolerance) over the recent bars
// and got turned back, while the node's %King did NOT meaningfully decline.
// That is ABSORPTION = the level HELD (a rejection, scoped level-to-level). It
// is NOT a reversal on its own. Rising %King into the test escalates to 'strong'
// (active defense); a %King that FELL as price broke through is disqualified
// (that is give-way, not absorption).
var ABS_TEST_BARS = 4;      // look back this many closed 3m bars for a test
var ABS_TOUCH_TOL = 0.20;   // a bar wick within this many strikes = a touch/test
var ABS_REJECT_MIN = 0.15;  // price must have pulled back >= this many strikes from the test
var ABS_DECLINE_TOL = 4;    // %King may only decline up to this and still 'hold'
function absorptionAt(sym, k){
  var S=STATE[sym]||{};
  var cs=S.candles||[];
  var px=S.price;
  if(px==null || cs.length<2) return null;
  var n=cs.length;
  var from=Math.max(0, n-ABS_TEST_BARS);
  var touched=false, testExtreme=null, testIdx=-1;
  for(var i=from;i<n;i++){
    var b=cs[i]; if(!b) continue;
    // A test = a wick reaching the strike from either side.
    if(b.l!=null && b.h!=null && b.l<=k+ABS_TOUCH_TOL && b.h>=k-ABS_TOUCH_TOL){
      touched=true; testIdx=i;
      testExtreme = (k>=px) ? b.h : b.l; // how far price pushed into the level
    }
  }
  if(!touched) return null;
  // Rejection = price has since pulled away from the strike by a real amount.
  var away = Math.abs(px - k);
  if(away < ABS_REJECT_MIN) return null; // still sitting on it, not yet rejected
  // Did the node HOLD? Compare %King now vs at/around the test.
  var h=nodeHistory(sym, k);
  var declined = 0;
  if(h && h.length>=2){ declined = h[0]-h[h.length-1]; } // positive = it fell
  if(declined > ABS_DECLINE_TOL) return null; // %King fell as tested -> give-way, not absorption
  var rate=nodeBuildRate(sym, k);
  var tier = (rate >= HIST_STEADY_BAND) ? 'strong' : 'held';
  var side = (k>=px) ? 'above' : 'below';
  return { k:k, side:side, tier:tier, rate:rate, away:away,
           // rejection direction: resistance above rejects DOWN, support below rejects UP
           rejectDir: (side==='above'?-1:1) };
}

// ===== #8 NET POSITIONING =====
// Aggregate accumulation ABOVE price vs BELOW to gauge the board's directional
// tilt. Momentum-leaning: build RATE is primary, static %King size secondary,
// nearer nodes weighted heavier.
var NET_SIZE_W = 0.35;      // static %King contribution
var NET_RATE_W = 1.0;       // build-rate contribution (primary)
var NET_RATIO_STRONG = 1.6; // ratio past this = a decisive tilt
function netPositioning(sym){
  var S=STATE[sym]||{};
  var px=S.price;
  if(px==null || !S.walls || !S.walls.length) return null;
  var above=0, below=0;
  S.walls.forEach(function(w){
    if(w.k==null) return;
    var dist=Math.abs(w.k-px);
    if(dist<0.001) return;
    var wgt = 1/(1+dist);                    // nearer weighted heavier
    var rate=Math.max(0, nodeBuildRate(sym, w.k)); // only building sides add tilt
    var contrib = wgt * (NET_RATE_W*rate + NET_SIZE_W*(w.pct||0));
    if(w.k>px) above+=contrib; else below+=contrib;
  });
  var tot=above+below;
  if(tot<=0) return { bias:'balanced', ratio:1, above:above, below:below };
  var ratio, bias, dir;
  if(below>=above){ ratio = above>0 ? below/above : 99; bias='support-heavy'; dir=1; }
  else            { ratio = below>0 ? above/below : 99; bias='resistance-heavy'; dir=-1; }
  var decisive = ratio>=NET_RATIO_STRONG;
  return { bias:bias, dir:dir, ratio:ratio, decisive:decisive, above:above, below:below };
}

// ===== #9 SETUP GRADE =====
// One letter (A-D) on the in-play pullback, a weighted composite of the factors
// already computed: node accumulation + relative trap read (heaviest), net tilt,
// target confluence, reach, minus an absorption-against penalty. Direction is
// the current trade bias; the opposite side's grade is exposed in the tooltip.
function setupGrade(sym){
  var S=STATE[sym]||{};
  var px=S.price;
  var v=trendVerdict(sym);
  if(px==null || (v.state!=='up' && v.state!=='dn')) return null;   // (v10.18) only act on CONFIRMED trend
  var longSide = v.state==='up';
  var rel=relativeRead(sym);
  var net=netPositioning(sym);
  var lad=targetLadder();
  var score=0, parts=[];
  // Relative trap read (heaviest): aligned with our direction?
  if(rel){
    var favor = longSide ? rel.longScore : -rel.longScore;
    if(favor>=HIST_STEADY_BAND){ score+=35; parts.push('local read favors '+(longSide?'long':'short')+' (+35)'); }
    else if(favor<=-HIST_STEADY_BAND){ score-=25; parts.push('local read AGAINST ('+'-25)'); }
    else parts.push('local read mixed (0)');
  }
  // Net board tilt aligned?
  if(net && net.bias!=='balanced'){
    var tiltFavor = longSide ? (net.dir>0) : (net.dir<0);
    if(tiltFavor){ var add=net.decisive?25:15; score+=add; parts.push('board '+net.bias+' aligned (+'+add+')'); }
    else { score-=15; parts.push('board '+net.bias+' against (-15)'); }
  }
  // Target confluence present?
  if(lad && lad.t1star){ score+=15; parts.push('T1 confluence \u2605 (+15)'); }
  // Reach: is T1 comfortably reachable?
  if(lad && lad.t1!=null){
    var prox=adaptiveProxStrikes(sym);
    var d1=Math.abs(lad.t1-px);
    if(d1<=prox){ score+=15; parts.push('T1 in reach (+15)'); }
    else parts.push('T1 a stretch (0)');
  }
  // In-play node itself building in our favor?
  var anchor=resolveAnchor(sym);
  if(anchor && anchor.ok && anchor.active){
    var h=nodeHistory(sym, anchor.active.k);
    var ft = (h&&h.length>=2)?histTrend(h, liveNodePct(sym,anchor.active.k)):null;
    if(ft && ft.label==='Building'){ score+=10; parts.push('in-play node building (+10)'); }
    else if(ft && ft.label==='Fading'){ score-=10; parts.push('in-play node fading (-10)'); }
  }
  // clamp 0-100
  if(score<0) score=0; if(score>100) score=100;
  var letter = score>=75?'A':(score>=55?'B':(score>=35?'C':'D'));
  return { letter:letter, score:Math.round(score), dir:(longSide?'Long':'Short'),
           parts:parts, oppLetter:null };
}
// Judge accumulation from the VISIBLE 1-minute %King series (what the strip
// shows), so tag and strip always agree. Dip-tolerant via the drop budget.
function histTrend(seq, liveVal){
  var n=seq?seq.length:0;
  if(n<2) return { label:'Steady', rapid:false, arrow:'\u2192', net:0 };
  var start=seq[0], last=seq[n-1];
  var peak=start, peakIdx=0;
  for(var i=0;i<n;i++){ if(seq[i]>peak){ peak=seq[i]; peakIdx=i; } }
  var net=last-start;
  var fromPeak=peak>0?(peak-last)/peak:0;
  var downRun=0; for(var j=n-1;j>0;j--){ if(seq[j]<seq[j-1]) downRun++; else break; }
  var slope = seqTrendSlope(seq);
  var label='Steady';
  // Building: net rise clears the steady band AND overall slope is up, and we
  // haven't given back more than the drop budget from the peak (pullback ok).
  if(net > HIST_STEADY_BAND && slope > 0 && fromPeak <= ACC_DROP_BUDGET){
    label='Building';
  } else if((net < -HIST_STEADY_BAND && slope < 0) || (fromPeak > ACC_DROP_BUDGET && downRun >= ACC_CONFIRM_DOWN)){
    label='Fading';
  }
  var arrow = label==='Building' ? '\u2197' : (label==='Fading' ? '\u2198' : '\u2192');
  // RAPID signal (separate from the slow badge). On a 3m-close strip the slow
  // label lags, so 'rapid' must react FAST: if a live between-close value is
  // supplied, measure the move from the last close to the live tape; else fall
  // back to the last two closed samples. +HIST_RAPID_STEP = Rapid Accumulation,
  // -HIST_RAPID_STEP = Rapid Unwinding, 0 = neutral.
  var recentMove;
  if(typeof liveVal==='number'){
    recentMove = liveVal - seq[n-1];                       // live vs last close
    if(Math.abs(recentMove) < HIST_RAPID_STEP && n>=2){    // fold in prior close too
      var twoStep = liveVal - seq[n-2];
      if(Math.abs(twoStep) > Math.abs(recentMove)) recentMove = twoStep;
    }
  } else {
    recentMove = (n>=3) ? (seq[n-1]-seq[n-3]) : (seq[n-1]-seq[n-2]);
  }
  var rapidDir = 0;
  if(recentMove >= HIST_RAPID_STEP) rapidDir = 1;
  else if(recentMove <= -HIST_RAPID_STEP) rapidDir = -1;
  return { label:label, rapid:(rapidDir!==0), rapidDir:rapidDir, recentMove:recentMove, arrow:arrow, net:net, seq:seq };
}

function persistState(){
  try{
    var snap={date:TODAY,
      SPY:{setups:STATE.SPY.setups, lastClosedB:STATE.SPY.lastClosedB},
      QQQ:{setups:STATE.QQQ.setups, lastClosedB:STATE.QQQ.lastClosedB}};
    localStorage.setItem(STATE_KEY, JSON.stringify(snap));
  }catch(e){}
}
function restoreState(){
  try{
    var snap=JSON.parse(localStorage.getItem(STATE_KEY)||'null');
    if(!snap || snap.date!==TODAY) return;
    ['SPY','QQQ'].forEach(function(sym){
      if(snap[sym]){
        STATE[sym].setups=snap[sym].setups||{};
        STATE[sym].lastClosedB=snap[sym].lastClosedB||0;
      }
    });
    syncLog('SPY'); syncLog('QQQ');
  }catch(e){}
}

function recordSession(){
  try{
    var raw=localStorage.getItem(STATS_KEY);
    var db = raw ? JSON.parse(raw) : { days:{} };
    if(!db.days) db.days={};
    var day = db.days[TODAY] || { date:TODAY, px:[], setups:{} };
    var px = STATE.SPY.price;
    if(px!=null){
      var bucket = Math.floor(ctNowSecOfDay()/CANDLE_S)*CANDLE_S;
      var lastPx = day.px.length ? day.px[day.px.length-1] : null;
      if(!lastPx || lastPx.b!==bucket){ day.px.push({b:bucket, s:px}); }
      else { lastPx.s = px; }
      if(day.px.length>200) day.px = day.px.slice(day.px.length-200);
    }
    for(var key in STATE.SPY.setups){
      var s=STATE.SPY.setups[key];
      if(s.voided || s.stage==='GO'){
        day.setups[key] = {
          strike:s.strike, dir:s.dir, attempt:s.attempt, stage:s.stage,
          voided:!!s.voided, boBar:s.boBar, ftBar:s.ftBar, testBar:s.testBar,
          confBar:s.confBar, goBar:s.goBar, voidBar:s.voidBar,
          boPct:(typeof s.boPct==='number'?s.boPct:null), targets:s.targets||null
        };
      }
    }
    db.days[TODAY]=day;
    var dates=Object.keys(db.days).sort();
    while(dates.length>STATS_DAYS){ delete db.days[dates.shift()]; }
    try{ localStorage.setItem(STATS_KEY, JSON.stringify(db)); }
    catch(qe){
      var d2=Object.keys(db.days).sort();
      if(d2.length){ delete db.days[d2[0]]; try{ localStorage.setItem(STATS_KEY, JSON.stringify(db)); }catch(e2){} }
    }
  }catch(e){}
}

// ============================================================================
// RECORDER (DATA layer for LLM analytics / prediction)
// ----------------------------------------------------------------------------
// Two streams, both keyed by trading day, kept for RECORDER_DAYS rolling days:
//   snaps[sym]  = once-per-closed-3m-bar snapshots of the whole node picture
//                 (price, King, and every tracked node's strike/role/side/%King/
//                  state/rapid/short-history), so an LLM can learn what node
//                  CONDITIONS precede a setup succeeding or failing.
//   events[sym] = one row each time a setup RESOLVES (T1/T2/FAILED/EXPIRED),
//                 with the setup facts + the node picture at resolution, so the
//                 pre-conditions can be joined to the outcome downstream.
// Purely ADDITIVE: its own storage key, never touches the state machine. All
// writes are quota-guarded and hard-capped so it can never break the panel.
// ============================================================================
function recorderLoad(){
  try{ var raw=localStorage.getItem(RECORDER_KEY); var db=raw?JSON.parse(raw):null;
    if(!db||typeof db!=='object') db={days:{}}; if(!db.days) db.days={}; return db;
  }catch(e){ return {days:{}}; }
}
function recorderSave(db){
  try{
    var dates=Object.keys(db.days).sort();
    while(dates.length>RECORDER_DAYS){ delete db.days[dates.shift()]; }
    localStorage.setItem(RECORDER_KEY, JSON.stringify(db));
  }catch(e){
    // On quota, drop the oldest day and retry once; never throw into render.
    try{ var d2=Object.keys(db.days).sort(); if(d2.length){ delete db.days[d2[0]]; localStorage.setItem(RECORDER_KEY, JSON.stringify(db)); } }catch(e2){}
  }
}
function recorderDay(db){
  var day=db.days[TODAY];
  if(!day){ day={date:TODAY, snaps:{SPY:[],QQQ:[]}, events:{SPY:[],QQQ:[]}}; db.days[TODAY]=day; }
  if(!day.snaps) day.snaps={SPY:[],QQQ:[]};
  if(!day.events) day.events={SPY:[],QQQ:[]};
  return day;
}
// Compact one node to the fields analytics needs (short keys to save space).
function recNode(r){
  return {
    k:r.k,
    role:r.role||null,
    side:r.side||null,                                   // 'above'=resistance, 'below'=support
    pct:(typeof r.pct==='number')?r.pct:null,            // %King (feed/wall)
    tp:(function(){ var v=livePctAt('SPY', r.k); return (typeof v==='number')?v:null; })(), // tape %King (signed) — best-effort
    st:(r.state&&r.state.label)||null,                   // Building/Steady/Fading
    net:(r.state&&typeof r.state.net==='number')?r.state.net:null,
    rapid:!!(r.state&&r.state.rapid),
    rdir:(r.state&&r.state.rapidDir)||0,
    roll:r.roll||0,
    hist:(r.hist&&r.hist.length)?r.hist.slice(-8):(r.seq&&r.seq.length?r.seq.slice(-8):null)
  };
}
// Snapshot the whole node picture for a symbol, at most once per closed 3m bar.
function recordNodeSnapshot(sym){
  try{
    if(RECORDER_SYMS.indexOf(sym)<0 || !TODAY) return;
    var S=STATE[sym]; if(!S) return;
    var bar=S.lastClosedB||0;
    if(!bar) return;
    if(!RECORDER._lastSnapBar) RECORDER._lastSnapBar={};
    if(RECORDER._lastSnapBar[sym]===bar) return;         // throttle: one per closed bar
    var fs=(sym==='SPY')?futureStructureSummary('SPY'):futureStructureSummary(sym);
    if(!fs) return;
    var nodes=[];
    (fs.above||[]).forEach(function(r){ nodes.push(recNode(r)); });
    (fs.below||[]).forEach(function(r){ nodes.push(recNode(r)); });
    // ---- SIGNAL VECTOR (v10.15 DATA layer): capture the full derived read at
    // this closed bar so the end-of-day LLM can score prediction quality of
    // EVERY signal, not just node structure. Each is wrapped in try/guard so a
    // single failing signal never blocks the whole snapshot.
    var sig=null;
    try{
      var tv=trendVerdict(sym);
      var srb=srBattle(sym);
      var nb=nodeBreadth(sym);
      var cf=confluence(sym);
      var mvNow=(kingDay(sym)&&kingDay(sym).moves)?kingDay(sym).moves:[];
      var kv=kingVerdict(mvNow,(typeof S.king==='number')?S.king:null,
                         (typeof S.price==='number')?S.price:null,Date.now());
      sig={
        trend: tv?{state:tv.state,up:tv.up,win:tv.win,ma:tv.ma,slope:+(tv.slope||0).toFixed(4)}:null,
        king:  kv?{cls:kv.cls,word:kv.word,drift:kv.drift,score:+(kv.score||0).toFixed(2),
                   magnet:kv.magnet,offK:((typeof S.king==='number'&&typeof S.price==='number')?Math.round(S.king-S.price):null)}:null,
        srb:   srb?{dom:srb.dom,cross:srb.cross,supF:srb.supForce,resF:srb.resForce,supPct:srb.supPct,
                    floorK:(srb.nearFloor&&srb.nearFloor.k),floorFade:(srb.nearFloor&&srb.nearFloor.fading),
                    ceilK:(srb.nearCeil&&srb.nearCeil.k),ceilFade:(srb.nearCeil&&srb.nearCeil.fading)}:null,
        breadth: nb?{net:nb.net,dir:nb.dir,mag:+(nb.mag||0).toFixed(2)}:null,
        conf:  cf?{dir:cf.dir,word:cf.word,score:+(cf.score||0).toFixed(2),aligned:cf.aligned,
                   bull:cf.bull,bear:cf.bear,declared:cf.declared}:null
      };
      // (v10.24 Issue I5) NODE MAP + DETECTOR effectiveness capture. Record the
      // emitted map STATE + each detector so per-verdict/per-pattern hit-rate is
      // computable later (mandate: log from day one, even before the Analysis tab).
      try{
        var nm=nodeMapModel(sym);
        if(nm && nm.ok){
          sig.nodemap={
            emphasis:nm.emphasis, againstKing:nm.againstKing, kingK:nm.kingK,
            regime: nm.regime?{label:nm.regime.label,dir:nm.regime.dir,skew:nm.regime.skew,conf:nm.regime.conf,target:nm.regime.target}:null,
            gk: (nm.gatekeeper&&nm.gatekeeper.ok)?{k:nm.gatekeeper.k,ratio:nm.gatekeeper.ratio,strong:nm.gatekeeper.strong,side:nm.gatekeeper.side}:null,
            rug:(nm.rug&&nm.rug.ok)?{type:nm.rug.type,ceilK:nm.rug.ceilK,floorK:nm.rug.floorK,targets:nm.rug.targets,shown:nm.rug.shown,dn:nm.rug.confirm.downGrowing,lowUp:nm.rug.confirm.lowUpside}:null,
            strongSup: nm.strongSup?nm.strongSup.k:null,
            strongRes: nm.strongRes?nm.strongRes.k:null,
            // per-level verdicts (the thing whose forward accuracy we measure)
            levels: nm.levels.map(function(L){ return {k:L.k,side:L.side,pct:L.pct,st:L.state,
              v:L.verdict,conf:L.conf,dist:L.dist,strong:!!L.isStrong,king:!!L.isKing,
              gk:!!L.isGatekeeper,forming:!!L.forming,emph:!!L.onEmphasis}; })
          };
        }
      }catch(eNM){}
    }catch(eSig){ sig=null; }
    var snap={
      t:Date.now(),
      bar:bar,
      px:(typeof S.price==='number')?S.price:null,
      king:(typeof S.king==='number')?S.king:null,
      tking:tapeKingStrike(sym),
      inplay:(fs.inPlay?{k:fs.inPlay.k, role:fs.inPlay.role, side:fs.inPlay.side,
              st:(fs.inPlay.state&&fs.inPlay.state.label)||null}:null),
      nodes:nodes,
      sig:sig,
      // Forward-outcome slots, back-filled by labelForwardOutcomes() once N bars
      // elapse. null = not yet resolved. Stored for BOTH horizons (#3).
      out5:null,   // {mfe,mae,net,pxEnd,hitKing,revUp,revDn} over next 5 bars (15m)
      out10:null   // ...over next 10 bars (30m)
    };
    var db=recorderLoad(); var day=recorderDay(db);
    var arr=day.snaps[sym]||(day.snaps[sym]=[]);
    arr.push(snap);
    if(arr.length>RECORDER_MAX_SNAPS) day.snaps[sym]=arr.slice(arr.length-RECORDER_MAX_SNAPS);
    RECORDER._lastSnapBar[sym]=bar;
    // Back-fill forward outcomes on older snapshots now that a fresh bar landed.
    labelForwardOutcomes(sym, day.snaps[sym]);
    recorderSave(db);
  }catch(e){}
}

// ---- FORWARD-OUTCOME AUTO-LABELER (v10.15 DATA layer) ------------------------
// For each snapshot, once >=H newer bars exist, stamp what price actually did
// over the next H bars: MFE (max favorable excursion up), MAE (max adverse down),
// net move, ending price, whether price reached the King strike, and whether a
// reversal formed (higher-high then lower, or vice-versa). Runs for BOTH 5-bar
// (15m) and 10-bar (30m) horizons (#3). Idempotent: only fills null slots.
function _fwdStats(base, fwd){
  // base = snapshot being labeled; fwd = array of the next H snapshots.
  var px0=(typeof base.px==='number')?base.px:null;
  if(px0==null || !fwd.length) return null;
  var kingK=(typeof base.king==='number')?base.king:null;
  var hi=px0, lo=px0, hitKing=false, last=px0;
  for(var i=0;i<fwd.length;i++){
    var p=(typeof fwd[i].px==='number')?fwd[i].px:null; if(p==null) continue;
    if(p>hi) hi=p; if(p<lo) lo=p; last=p;
    if(kingK!=null && Math.abs(p-kingK)<=0.25) hitKing=true;
  }
  var mfe=+(hi-px0).toFixed(2);     // best up-move from entry
  var mae=+(lo-px0).toFixed(2);     // worst down-move (<=0)
  var net=+(last-px0).toFixed(2);
  // crude reversal tag: did price make an extreme then retrace >50% of it?
  var revDn = (mfe>0.15 && (hi-last) > 0.5*mfe);   // ran up then rolled over
  var revUp = (mae<-0.15 && (last-lo) > 0.5*(-mae)); // dropped then bounced
  return { mfe:mfe, mae:mae, net:net, pxEnd:+last.toFixed(2),
           hitKing:hitKing, revUp:revUp, revDn:revDn, n:fwd.length };
}
function labelForwardOutcomes(sym, snaps){
  try{
    if(!snaps || snaps.length<2) return;
    var H5=5, H10=10;
    for(var i=0;i<snaps.length;i++){
      var s=snaps[i];
      if(s.out5==null && (snaps.length-1-i)>=H5){
        s.out5=_fwdStats(s, snaps.slice(i+1, i+1+H5));
      }
      if(s.out10==null && (snaps.length-1-i)>=H10){
        s.out10=_fwdStats(s, snaps.slice(i+1, i+1+H10));
      }
    }
  }catch(e){}
}
// Record a setup resolution (T1/T2/FAILED/EXPIRED) with node context at resolve.
function recordOutcomeEvent(sym, s){
  try{
    if(RECORDER_SYMS.indexOf(sym)<0 || !TODAY || !s) return;
    var fs=futureStructureSummary(sym);
    var ctx=null;
    if(fs){
      // node nearest the setup strike, for the resolution's local structure
      var near=null, best=1e9, all=(fs.above||[]).concat(fs.below||[]);
      for(var i=0;i<all.length;i++){ var d=Math.abs(all[i].k-s.strike); if(d<best){best=d; near=all[i];} }
      ctx={ inplay:(fs.inPlay?{k:fs.inPlay.k,role:fs.inPlay.role,side:fs.inPlay.side}:null),
            nearStrikeNode:(near?recNode(near):null) };
    }
    var ev={
      t:Date.now(),
      strike:s.strike, dir:s.dir, attempt:s.attempt||null,
      outcome:s.outcome||null,
      goBar:s.goBar||null, resolvedBar:s.resolvedBar||null,
      durationMs:(typeof s.durationMs==='number')?s.durationMs:null,
      boPct:(typeof s.boPct==='number')?s.boPct:null,
      targets:s.targets||null,
      t1Bar:s.t1Bar||null, t2Bar:s.t2Bar||null,
      px:(typeof STATE[sym].price==='number')?STATE[sym].price:null,
      king:(typeof STATE[sym].king==='number')?STATE[sym].king:null,
      ctx:ctx
    };
    var db=recorderLoad(); var day=recorderDay(db);
    var arr=day.events[sym]||(day.events[sym]=[]);
    arr.push(ev);
    if(arr.length>RECORDER_MAX_EVENTS) day.events[sym]=arr.slice(arr.length-RECORDER_MAX_EVENTS);
    recorderSave(db);
  }catch(e){}
}
var RECORDER = { _lastSnapBar:{} };

function clearSignalsAll(){
  STATE.SPY.setups={}; STATE.QQQ.setups={};
  STATE.SPY.lastClosedB=0; STATE.QQQ.lastClosedB=0;
  LOGCACHE={SPY:[],QQQ:[]};
  try{ localStorage.removeItem(STATE_KEY); localStorage.removeItem(LOG_KEY); }catch(e){}
  render();
}
function clearSignalsSym(sym){
  STATE[sym].setups={};
  STATE[sym].lastClosedB=0;
  LOGCACHE[sym]=[];
  persistState();
  render();
}

function keyFor(sym,k,dir,attempt){ return sym+':'+k+':'+dir+':'+attempt; }
function attemptCount(S,k,dir){
  var n=0;
  for(var key in S.setups){ var s=S.setups[key];
    if(s.strike===k && s.dir===dir) n=Math.max(n, s.attempt); }
  return n;
}
function liveSetup(S,k,dir){
  for(var key in S.setups){ var s=S.setups[key];
    if(s.strike===k && s.dir===dir && !s.voided && s.stage!=='GO') return s; }
  return null;
}

function newSetup(sym,S,k,dir,boBar){
  var attempt=attemptCount(S,k,dir)+1;
  var key=keyFor(sym,k,dir,attempt);
  var boPct = livePctAt(sym, k);
  var s={ sym:sym, strike:k, dir:dir, attempt:attempt, stage:'BO',
          boBar:boBar.t, ftBar:null, testBar:null, confBar:null, goBar:null, goFired:false,
          voided:false, backCount:0, voidBar:null, tokens:['BO'], ts:boBar.t, updated:Date.now(),
          boPct:boPct };
  S.setups[key]=s;
  return s;
}

function addToken(s,tok){
  if(tok==='GO'){ if(s.goFired) return; s.goFired=true; }
  if(s.tokens.length && s.tokens[s.tokens.length-1]===tok && tok!=='T1' && tok!=='T2' && tok!=='T3') return;
  s.tokens.push(tok); s.updated=Date.now();
}
function stageAnchor(s){
  if(s.stage==='CONF') return s.confBar;
  if(s.stage==='TST') return s.testBar;
  if(s.stage==='FT') return s.ftBar;
  return s.ts;
}


function runOutcome(sym, last){
  var S=STATE[sym];
  if(!S || !last) return;
  var eod = (typeof last.so==='number' && last.so>=54000);
  for(var key in S.setups){
    var s=S.setups[key];
    if(!s || s.voided || !s.goFired || !s.goBar) continue;
    if(s.outcome==='FAILED' || s.outcome==='EXPIRED' || s.outcome==='T2') continue;
    var t1 = (s.targets && s.targets.length>0) ? s.targets[0] : null;
    var t2 = (s.targets && s.targets.length>1) ? s.targets[1] : null;
    var hitT1 = false, hitT2 = false;

    if(t1!=null){
      hitT1 = (s.dir==='long') ? (last.h>=t1) : (last.l<=t1);
    }
    if(t2!=null){
      hitT2 = (s.dir==='long') ? (last.h>=t2) : (last.l<=t2);
    }

    if(hitT2 && !s.t1Bar && t1!=null){
      s.t1Bar = last.t;
      addToken(s,'T1');
    }
    if(hitT1 && !s.t1Bar){
      s.t1Bar = last.t;
      addToken(s,'T1');
    }
    if(hitT2 && !s.t2Bar){
      s.t2Bar = last.t;
      addToken(s,'T2');
    }

    if(s.t2Bar){
      s.outcome='T2';
      s.resolvedBar=s.t2Bar;
      s.durationMs = s.goBar ? (s.resolvedBar - s.goBar) : null;
      continue;
    }
    if(s.t1Bar){
      s.outcome='T1';
      s.resolvedBar=s.t1Bar;
      s.durationMs = s.goBar ? (s.resolvedBar - s.goBar) : null;
      continue;
    }

    var failed = (s.dir==='long') ? (last.c < s.strike) : (last.c > s.strike);
    if(failed){
      s.outcome='FAILED';
      s.failBar=last.t;
      s.resolvedBar=last.t;
      s.durationMs = s.goBar ? (s.resolvedBar - s.goBar) : null;
      continue;
    }

    if(eod){
      s.outcome='EXPIRED';
      s.expiredBar=last.t;
      s.resolvedBar=last.t;
      s.durationMs = s.goBar ? (s.resolvedBar - s.goBar) : null;
    }
  }
  // DATA layer: record each setup ONCE, the first pass it carries a resolved
  // outcome. _recorded guards against re-logging on later ticks.
  for(var rk in S.setups){
    var rs=S.setups[rk];
    if(!rs || rs._recorded) continue;
    if(rs.outcome==='T1' || rs.outcome==='T2' || rs.outcome==='FAILED' || rs.outcome==='EXPIRED'){
      recordOutcomeEvent(sym, rs);
      rs._recorded=true;
    }
  }
}

// (v10.27) Does the LAST closed bar make a new N-bar extreme (high for long, low for
// short), INCLUSIVE of itself? Needs a full window of N bars; if fewer exist we can't
// confirm range expansion -> return false (no BO). cs is oldest..newest.
function isNBarExtreme(cs, dir, n){
  if(!cs || cs.length < n) return false;
  var win = cs.slice(cs.length - n);
  var last = win[win.length - 1];
  if(dir==='long'){
    var hi = -Infinity;
    for(var i=0;i<win.length;i++){ if(win[i].h>hi) hi=win[i].h; }
    return last.h >= hi;   // last bar's high is the highest in the window
  } else {
    var lo = Infinity;
    for(var j=0;j<win.length;j++){ if(win[j].l<lo) lo=win[j].l; }
    return last.l <= lo;   // last bar's low is the lowest in the window
  }
}
function runMachine(sym){
  var S=STATE[sym];
  var cs=closedCandles(sym);
  if(cs.length<3) return;
  var last=cs[cs.length-1];
  if(last.b <= S.lastClosedB) return;
  S.lastClosedB = last.b;
  var prev=cs[cs.length-2];
  var a=atr(sym);
  var tol=Math.max(mul(0.2,a),0.05);
  S.walls.forEach(function(w){
    var k=w.k;
    if(last.c>k && prev.c<=k){
      var liveL=liveSetup(S,k,'long');
      var lastAttemptL=null;
      for(var key in S.setups){ var s=S.setups[key]; if(s.strike===k&&s.dir==='long'){ if(!lastAttemptL||s.attempt>lastAttemptL.attempt) lastAttemptL=s; } }
      var doneL = (!lastAttemptL || lastAttemptL.voided || lastAttemptL.stage==='GO');
      // (v10.27) BO quality gate: require a 14-bar HIGH on the breakout bar.
      if(!liveL && doneL && trendOkFor(sym,'long') && isNBarExtreme(cs,'long',BO_HL_LOOKBACK)){ newSetup(sym,S,k,'long',last); }
    }
    if(last.c<k && prev.c>=k){
      var liveS=liveSetup(S,k,'short');
      var lastAttemptS=null;
      for(var key2 in S.setups){ var s2=S.setups[key2]; if(s2.strike===k&&s2.dir==='short'){ if(!lastAttemptS||s2.attempt>lastAttemptS.attempt) lastAttemptS=s2; } }
      var doneS = (!lastAttemptS || lastAttemptS.voided || lastAttemptS.stage==='GO');
      // (v10.27) BO quality gate: require a 14-bar LOW on the breakdown bar.
      if(!liveS && doneS && trendOkFor(sym,'short') && isNBarExtreme(cs,'short',BO_HL_LOOKBACK)){ newSetup(sym,S,k,'short',last); }
    }
  });
  for(var key in S.setups){
    var s=S.setups[key];
    if(s.voided || s.goFired) continue;
    var k=s.strike, dir=s.dir;
    var anchorPre=stageAnchor(s);
    if(anchorPre && (last.t - anchorPre) > STAGE_MAX_MS){
      s.voided=true; s.voidBar=last.t; addToken(s,'VOID');
      continue;
    }
    var isBackBar = (dir==='long') ? (last.c<k) : (last.c>k);
    if(last.t > s.boBar){
      if(isBackBar){ s.backCount = (s.backCount||0) + 1; }
      else { s.backCount = 0; }
    }
    var needN = parseInt(CFG.voidBackN,10);
    if(isNaN(needN)||needN<1) needN=2;
    if(s.stage!=='GO' && (s.backCount||0) >= needN){
      s.voided=true; s.voidBar=last.t; addToken(s,'VOID');
      continue;
    }
    var holdBeyond = (dir==='long') ? (last.l>k) : (last.h<k);
    var barLaterThanBO = last.t > s.boBar;
    if(s.stage==='BO' && barLaterThanBO && holdBeyond){
      s.stage='FT'; s.ftBar=last.t; addToken(s,'FT');
    }
    var wickTouch = (dir==='long') ? (last.l<=k+tol) : (last.h>=k-tol);
    if((s.stage==='FT'||s.stage==='BO') && s.ftBar && last.t>s.ftBar && wickTouch){
      s.stage='TST'; s.testBar=last.t; addToken(s,'TST');
    }
    var confOk = (dir==='long') ? (last.c>last.o) : (last.c<last.o);
    if(s.stage==='TST' && s.testBar && last.t>=s.testBar && confOk){
      s.stage='CONF'; s.confBar=last.t; addToken(s,'CONF');
    }
    if(s.stage==='CONF' && s.confBar && last.t>s.confBar){
      var confCandle=null;
      for(var i=cs.length-1;i>=0;i--){ if(cs[i].t===s.confBar){ confCandle=cs[i]; break; } }
      if(confCandle){
        var go = (dir==='long') ? (last.c>confCandle.h) : (last.c<confCandle.l);
        if(go){ s.stage='GO'; s.goBar=last.t; addToken(s,'GO'); assignTargets(s,S); }
      }
    }
  }
  runOutcome(sym, last);
  syncLog(sym);
  persistState();
}

function countBarsSince(cs, boBarT){
  var idx=-1; for(var i=0;i<cs.length;i++){ if(cs[i].t===boBarT){ idx=i; break; } }
  if(idx<0) return 0; return (cs.length-1)-idx;
}
function assignTargets(s,S){
  var beyond=S.walls.filter(function(w){ return s.dir==='long' ? w.k>s.strike : w.k<s.strike; });
  beyond.sort(function(a,b){ return s.dir==='long' ? a.k-b.k : b.k-a.k; });
  s.targets=beyond.slice(0,2).map(function(w){return w.k;});
}

// ---------------- Engine layer ----------------

function nearestOpposingWall(walls, price, activeK, dir){
  if(!walls || !walls.length || activeK==null) return null;
  var best=null;
  for(var i=0;i<walls.length;i++){
    var w=walls[i];
    if(Math.abs(w.k-activeK)<0.001) continue;
    if(dir==='long'){ if(w.k>activeK){ if(!best || w.k<best.k) best=w; } }
    else if(dir==='short'){ if(w.k<activeK){ if(!best || w.k>best.k) best=w; } }
  }
  return best;
}

function resolveAnchor(sym){
  var S = STATE[sym];

  var diag = {
    pricePresent:false, wallCount:0, trendState:null,
    liveSetupCount:0, fallbackDir:null, hadPullbackSideWall:null
  };

  function none(reason){
    return {
      ok:false,
      reason:reason,
      sym:sym,
      active:null,
      opposing:null,
      dir:null,
      price:null,
      source:null,
      diag:diag
    };
  }

  if(!S) return none('no state for symbol');

  var price = S.price;
  var walls = S.walls || [];
  diag.pricePresent = (price!=null);
  diag.wallCount = walls.length;

  if(price==null) return none('no price');
  if(!walls.length) return none('no walls');

  var v = trendVerdict(sym);
  var a = atr(sym);
  diag.trendState = v.state;

  var chosen=null, liveCount=0;
  for(var key in S.setups){
    var s=S.setups[key];
    if(!s || s.voided || s.stage==='GO' || s.goFired) continue;
    liveCount++;
    if(!chosen || (s.updated||0) > (chosen.updated||0)) chosen=s;
  }
  diag.liveSetupCount = liveCount;

  var activeK=null, dir=null, activeWall=null, srcTag=null;

  if(chosen){
    activeK = chosen.strike;
    dir = chosen.dir;
    srcTag = 'setup';
  } else {
    if(v.state==='up') dir='long';
    else if(v.state==='dn') dir='short';
    else dir=null;
    diag.fallbackDir = dir;
    if(dir==null) return none('no directional idea (flat trend)');
    var pbWall = nearestWall(walls, price, dir==='short');
    diag.hadPullbackSideWall = !!pbWall;
    if(!pbWall) return none('no pullback-side wall');
    activeK = pbWall.k;
    srcTag = 'nearest';
  }

  for(var i=0;i<walls.length;i++){
    if(Math.abs(walls[i].k-activeK)<0.001){ activeWall=walls[i]; break; }
  }

  var opposing = nearestOpposingWall(walls, price, activeK, dir);

  return {
    ok:true,
    reason:null,
    sym:sym,
    dir:dir,
    price:price,
    atr:a,
    trend:v.state,
    source:srcTag,
    setupKey: chosen ? (sym+':'+chosen.strike+':'+chosen.dir+':'+chosen.attempt) : null,
    active:{
      k:activeK,
      stage: chosen ? chosen.stage : null,
      wall: activeWall,
      dist: (activeWall!=null || activeK!=null) ? Math.abs(price-activeK) : null
    },
    hasOpposing: !!opposing,
    opposingNote: opposing ? null : 'no opposing wall yet',
    opposing: opposing ? {
      k:opposing.k,
      wall:opposing,
      dist:Math.abs(opposing.k-activeK),
      distFromPrice:Math.abs(opposing.k-price)
    } : null,
    diag: diag
  };
}

function absAccumStateFor(sym, k){
  var r = rawAccumNode(sym, k);
  if(r && typeof r.absDelta==='number' && r.absSeq && r.absSeq.length>=2){
    return {
      available:true,
      absDelta:r.absDelta,
      absSeq:r.absSeq || null,
      delta:(typeof r.delta==='number'?r.delta:null),
      seq:r.seq || null
    };
  }
  return { available:false, absDelta:null, absSeq:null, delta:null, seq:null };
}

function nodeQuality(sym){
  var none = { ok:false, reason:'insufficient structure', sym:sym,
               cls:null, factors:null, gate:null, active:null };

  var anc = resolveAnchor(sym);
  if(!anc || !anc.ok) return none;

  var active = anc.active;
  var wall = active ? active.wall : null;
  if(!wall || typeof wall.abs!=='number'){
    return { ok:false, reason:'active node has no live strength', sym:sym,
             cls:null, factors:null, gate:null, active:active };
  }

  var S = STATE[sym];
  var king = (S && typeof S.king==='number') ? S.king : null;
  var dir  = anc.dir;

  var f = {};

  var absShare = (king && king>0) ? (wall.abs/king) : null;
  f.absStrength = { value:wall.abs, absShareOfKing:absShare,
                    band:(absShare==null)?'unknown':(absShare>=0.75?'top':(absShare>=0.40?'mid':'low')) };

  var role = (anc.price!=null) ? (anc.price>=active.k ? 'support' : 'resistance') : 'unknown';
  f.role = { value:role, nodeSideDir:(wall.pos===true?'pos':(wall.pos===false?'neg':'unknown')) };

  var acc = absAccumStateFor(sym, active.k);
  f.accumulation = acc.available
    ? { available:true, absDelta:acc.absDelta,
        state:(acc.absDelta>0?'building':(acc.absDelta<0?'bleeding':'flat')) }
    : { available:false, state:'unavailable' };

  var opp = anc.opposing;
  var nearBand = (typeof anc.atr==='number') ? anc.atr : null;
  var oppDominant = false, oppState='none';
  if(opp && opp.wall && typeof opp.wall.abs==='number'){
    var closeEnough = (nearBand!=null) ? (opp.distFromPrice <= nearBand) : false;
    var stronger = (opp.wall.abs >= wall.abs);
    oppDominant = closeEnough && stronger;
    oppState = oppDominant ? 'dominant-near' : 'present';
  }
  f.nearbyOpposition = { available:!!opp, state:oppState,
                         k:opp?opp.k:null,
                         distFromPrice:opp?opp.distFromPrice:null,
                         dominant:oppDominant };

  f.freshness  = { available:false, state:'unavailable', note:'no touch counter yet' };
  f.confluence = { available:false, state:'unavailable' };
  f.regimeFit  = { available:false, state:'unavailable' };
  f.topology   = { available:false, state:'unavailable' };
  f.divergence = { available:false, state:'unavailable' };

  var gate = null;
  if(RESHUFFLE[sym]===true){ gate = 'reshuffle'; }

  var trail = [];
  var cls;

  if(gate==='reshuffle'){
    cls = 'compromised';
    trail.push('gate: reshuffle');
  } else {
    if(f.absStrength.band==='top'){ cls='solid'; trail.push('absolute strong'); }
    else if(f.absStrength.band==='mid'){ cls='usable-but-contested'; trail.push('absolute mid'); }
    else if(f.absStrength.band==='low'){ cls='fragile'; trail.push('absolute low'); }
    else { cls='fragile'; trail.push('absolute unknown'); }

    var order = ['solid','usable-but-contested','fragile'];
    function demote(reason){
      var idx=order.indexOf(cls);
      if(idx>=0 && idx<order.length-1){ cls=order[idx+1]; }
      trail.push('demote: '+reason);
    }
    if(f.accumulation.available && f.accumulation.state==='bleeding'){ demote('accum bleeding (abs)'); }
    if(f.nearbyOpposition.dominant){ demote('dominant nearby opposition'); }
    if(f.accumulation.available && f.accumulation.state==='building'){ trail.push('confirm: accum building (abs)'); }
  }

  return {
    ok:true, reason:null, sym:sym,
    cls:cls,
    gate:gate,
    dir:dir,
    active:{ k:active.k, stage:active.stage, abs:wall.abs, pct:wall.pct,
             absShareOfKing:absShare, role:role },
    factors:f,
    trail:trail,
    supported:['absStrength','role','accumulation(abs)','nearbyOpposition'],
    unavailable:['freshness','confluence','regimeFit','topology','divergence'],
    note:'classes limited to {solid, usable-but-contested, fragile, compromised(=reshuffle gate)} from currently-supported factors; elite/exhausted require freshness/breach detection not yet present.'
  };
}

function pathQuality(sym){
  var anc = resolveAnchor(sym);
  if(!anc || !anc.ok){
    return {
      ok:false,
      reason:(anc && anc.reason) ? anc.reason : 'insufficient structure',
      sym:sym,
      cls:'insufficient-structure',
      factors:null,
      supported:null,
      unavailable:null
    };
  }

  var S = STATE[sym];
  var walls = (S && S.walls) ? S.walls : [];
  var dir   = anc.dir;
  var a     = (typeof anc.atr==='number') ? anc.atr : atr(sym);

  var f = {};
  f.activeAnchor = { available:true, k:anc.active.k, dir:dir };
  f.hasOpposing  = { available:true, value:!!anc.hasOpposing };

  var trail = [];
  var cls;

  if(!anc.hasOpposing || !anc.opposing || !anc.opposing.wall){
    var ahead=0;
    for(var i=0;i<walls.length;i++){
      var w=walls[i];
      if(dir==='long'){ if(w.k>anc.active.k) ahead++; }
      else if(dir==='short'){ if(w.k<anc.active.k) ahead++; }
    }

    f.opposing = { available:false, note:'no opposing wall yet' };
    f.pathSparsity = { available:true, wallsAhead:ahead };

    if(ahead===0){
      cls='open-path';
      trail.push('no walls ahead in direction');
    } else {
      cls='mostly-open';
      trail.push('no opposing wall; '+ahead+' minor wall(s) ahead');
    }
    trail.push('opposing: none yet');

    return finalizePath(sym, cls, dir, anc, f, trail,
      ['activeAnchor','hasOpposing','pathSparsity'],
      ['stackedClusters','compressionTrend','topology','targetLadder','asymmetry','regimeFit','confluence','airPocketSpans']);
  }

  var opp = anc.opposing;
  var oppWall = opp.wall;
  var activeWall = anc.active.wall;

  var distPrice = (typeof opp.distFromPrice==='number') ? opp.distFromPrice : null;
  var distActive = (typeof opp.dist==='number') ? opp.dist : null;

  var band='unknown';
  if(distPrice!=null && a>0){
    if(distPrice <= a) band='near';
    else if(distPrice <= mul(a,2)) band='mid';
    else band='far';
  }

  var oppAbs = (oppWall && typeof oppWall.abs==='number') ? oppWall.abs : null;
  var actAbs = (activeWall && typeof activeWall.abs==='number') ? activeWall.abs : null;
  var rel='unknown';
  if(oppAbs!=null && actAbs!=null && actAbs>0){
    if(oppAbs >= actAbs) rel='stronger-equal';
    else rel='weaker';
  } else if(oppAbs!=null && actAbs==null){
    rel='opposing-only';
  }

  f.opposing = {
    available:true,
    k:opp.k,
    distFromPrice:distPrice,
    distFromActive:distActive,
    band:band
  };
  f.relStrength = {
    available:true,
    opposingAbs:oppAbs,
    activeAbs:actAbs,
    rel:rel
  };

  var strong = (rel==='stronger-equal' || rel==='opposing-only');
  if(strong && band==='near'){
    cls='heavy-overhead';
    trail.push('opposing stronger/equal and near (<=1 ATR)');
  } else if((strong && band==='mid') || (!strong && band==='near')){
    cls='contested';
    trail.push('opposing '+rel+' at '+band+' distance');
  } else {
    cls='mostly-open';
    trail.push('opposing '+rel+' at '+band+' distance');
  }

  return finalizePath(sym, cls, dir, anc, f, trail,
    ['activeAnchor','hasOpposing','opposing','relStrength'],
    ['stackedClusters','compressionTrend','topology','targetLadder','asymmetry','regimeFit','confluence','airPocketSpans']);
}

function finalizePath(sym, cls, dir, anc, factors, trail, supported, unavailable){
  return {
    ok:true,
    reason:null,
    sym:sym,
    cls:cls,
    dir:dir,
    active:{ k:anc.active.k },
    opposing: anc.opposing ? { k:anc.opposing.k } : null,
    factors:factors,
    trail:trail,
    supported:supported,
    unavailable:unavailable,
    note:'classes limited to {open-path, mostly-open, contested, heavy-overhead, insufficient-structure} from distance + relative absolute strength only; compression/reversal-risk require trend/cluster inputs not yet present.'
  };
}

function setupHealth(sym){
  var anc = resolveAnchor(sym);
  if(!anc || !anc.ok){
    return { ok:false, reason:(anc && anc.reason) ? anc.reason : 'insufficient structure',
             sym:sym, cls:'insufficient-structure', factors:null,
             supported:null, unavailable:null };
  }

  var nq = nodeQuality(sym);
  var pq = pathQuality(sym);

  if(!nq || !nq.ok){
    return { ok:false, reason:(nq && nq.reason) ? nq.reason : 'node quality unavailable',
             sym:sym, cls:'insufficient-structure', factors:null,
             supported:null, unavailable:null };
  }

  var liveStage=null;
  var S = STATE[sym];
  if(S && S.setups){
    var chosen=null;
    for(var key in S.setups){
      var s=S.setups[key];
      if(!s || s.voided || s.stage==='GO' || s.goFired) continue;
      if(!chosen || (s.updated||0) > (chosen.updated||0)) chosen=s;
    }
    if(chosen) liveStage=chosen.stage;
  }

  var f = {};
  f.anchor       = { available:true, k:anc.active.k, dir:anc.dir };
  f.reshuffle    = { available:true, active:(RESHUFFLE[sym]===true) };
  f.nodeQuality  = { available:true, cls:nq.cls, gate:nq.gate||null };
  f.pathQuality  = pq && pq.ok ? { available:true, cls:pq.cls }
                               : { available:false, cls:(pq?pq.cls:null), reason:(pq?pq.reason:'unavailable') };
  f.liveStage    = { available:(liveStage!=null), stage:liveStage, note:'context only' };

  var trail = [];
  var cls;

  if(RESHUFFLE[sym]===true || nq.gate==='reshuffle'){
    cls='failing-structurally'; trail.push('gate: reshuffle active');
  }
  else if(nq.cls==='exhausted'){
    cls='failing-structurally'; trail.push('node exhausted');
  }
  else if(nq.cls==='compromised'){
    cls='weakening'; trail.push('node compromised');
  }
  else if(nq.cls==='fragile'){
    cls='weakening'; trail.push('node fragile');
    if(pq && pq.ok && pq.cls==='heavy-overhead'){ trail.push('path heavy-overhead reinforces weakening'); }
  }
  else if(pq && pq.ok && pq.cls==='heavy-overhead'){
    cls='weakening'; trail.push('path heavy-overhead against a non-fragile node');
  }
  else if(pq && pq.ok && pq.cls==='contested'){
    cls='crowded'; trail.push('path contested');
  }
  else {
    cls='stable';
    trail.push('node '+nq.cls+(pq&&pq.ok?(' + path '+pq.cls):' (path insufficient)'));
  }

  return {
    ok:true, reason:null, sym:sym,
    cls:cls, dir:anc.dir,
    active:{ k:anc.active.k },
    liveStage:liveStage,
    factors:f, trail:trail,
    supported:['anchor','reshuffleGate','nodeQuality.cls/gate','pathQuality.cls','liveStage(context)'],
    unavailable:['nodeDelta','pathDelta','touchTrend','divergence','regimeFit','transitionLog','timeOfDay','momentum','confidence'],
    note:'health is orthogonal to the BO/FT/TST/CONF/GO stage machine and derived from current state only; improving/late and trend-based re-grades require delta history not yet present.'
  };
}

function reader(sym){
  var anc = resolveAnchor(sym);

  if(!anc || !anc.ok){
    var why = (anc && anc.reason) ? anc.reason : 'insufficient structure';
    var fb;
    if(why.indexOf('flat trend')!==-1) fb='No directional idea right now — trend is flat and there is no live setup to anchor.';
    else if(why==='no price' || why==='no walls') fb='Waiting on structure — no live map to read yet.';
    else fb='Limited structure to read right now.';
    return { ok:false, reason:why, sym:sym, text:fb,
             slots:null, supported:null, unavailable:null };
  }

  var nq = nodeQuality(sym);
  var pq = pathQuality(sym);
  var sh = setupHealth(sym);

  var dir = anc.dir;
  var role = (anc.price!=null && anc.active) ? (anc.price>=anc.active.k ? 'support' : 'resistance') : null;

  var slots = {};

  if(anc.active && anc.active.k!=null){
    slots.activeNode = (role ? role+' node ' : 'node ') + fmtNum(anc.active.k);
  }
  if(nq && nq.ok && nq.cls){
    slots.nodeState = nq.cls;
  }
  if(pq && pq.ok && pq.cls){
    slots.pathState = pq.cls;
  }
  if(sh && sh.ok && sh.cls){
    slots.healthState = sh.cls;
  }
  if(anc.hasOpposing && anc.opposing && anc.opposing.k!=null){
    slots.opposing = fmtNum(anc.opposing.k);
  }

  var sentences = [];

  if(slots.activeNode){
    var s1 = 'Active ' + slots.activeNode;
    if(slots.nodeState){ s1 += ', graded ' + slots.nodeState; }
    if(slots.opposing){ s1 += '; nearest opposing structure at ' + slots.opposing; }
    s1 += '.';
    sentences.push(s1);
  }

  if(slots.pathState){
    var pathPhrase = {
      'open-path':'The path ahead is open',
      'mostly-open':'The path ahead is mostly open',
      'contested':'The path ahead is contested',
      'heavy-overhead':'There is heavy overhead structure ahead',
      'insufficient-structure':null
    }[slots.pathState];
    if(pathPhrase){ sentences.push(pathPhrase + '.'); }
  }

  if(slots.healthState && slots.healthState!=='insufficient-structure'){
    var healthPhrase = {
      'stable':'The setup is structurally stable',
      'crowded':'The setup looks crowded',
      'weakening':'The setup is weakening structurally',
      'failing-structurally':'The setup is failing structurally'
    }[slots.healthState];
    if(healthPhrase){ sentences.push(healthPhrase + '.'); }
  }

  if(!sentences.length){ sentences.push('Limited structure to read right now.'); }
  if(sentences.length>3) sentences = sentences.slice(0,3);

  return {
    ok:true, reason:null, sym:sym,
    text: sentences.join(' '),
    slots: slots,
    dir: dir,
    supported:['activeNode','nodeState','pathState','healthState','opposing'],
    unavailable:['probability','confidence','entry','stop','size','targetAdvice','divergence','regimeFit','LLMStyling'],
    note:'descriptive-only narrative assembled from present engine slots; absent slots suppressed; no advice or confidence.'
  };
}

window.__gptsDebug = window.__gptsDebug || {};
window.__gptsDebug.resolveAnchor = resolveAnchor;
window.__gptsDebug.accumData = accumData;
window.__gptsDebug.nodeQuality = nodeQuality;
window.__gptsDebug.pathQuality = pathQuality;
window.__gptsDebug.setupHealth = setupHealth;
window.__gptsDebug.reader = reader;
window.__gptsDebug.tape = function(sym){ return readTapeFromDOM(sym||'SPY'); };
window.__gptsDebug.tapeKing = function(sym){ return tapeKingStrike(sym||'SPY'); };
window.__gptsDebug.STATE = STATE;
// DATA layer export: recorder DB (node snapshots + outcome events).
window.__gptsDebug.dumpRecorder = function(){
  var db=recorderLoad();
  try{
    var days=Object.keys(db.days).sort();
    var sum=days.map(function(d){ var y=db.days[d];
      var ss=(y.snaps&&y.snaps.SPY?y.snaps.SPY.length:0)+(y.snaps&&y.snaps.QQQ?y.snaps.QQQ.length:0);
      var ee=(y.events&&y.events.SPY?y.events.SPY.length:0)+(y.events&&y.events.QQQ?y.events.QQQ.length:0);
      return d+': '+ss+' snaps, '+ee+' events'; });
    console.log('[GPTS recorder] '+days.length+' day(s):\n'+sum.join('\n'));
  }catch(e){}
  return db;
};
window.__gptsDebug.dumpRecorderJSON = function(){ return JSON.stringify(recorderLoad()); };
window.__gptsDebug.clearRecorder = function(){ try{ localStorage.removeItem(RECORDER_KEY); }catch(e){} return 'recorder cleared'; };

// ---- DAILY ARCHIVE EXPORT (v10.15) -----------------------------------------
// Build a self-describing export payload for ONE trading day (default today):
// the labeled snapshot vectors + outcome events + a small header so the
// end-of-day LLM review knows the schema without guessing. This is what gets
// dropped into AI Drive /GEX-Signal-Tapereader/daily-data/ for the scheduled
// review workflow to consume.
function buildDayExport(dateKey){
  var db=recorderLoad();
  var dk=dateKey||TODAY;
  var day=(db.days&&db.days[dk])?db.days[dk]:{snaps:{},events:{}};
  return {
    schema:'gex-day-export/v1',
    version:'10.15',
    date:dk,
    generatedAt:new Date().toISOString(),
    horizons:{ out5:'5 bars = 15 min forward', out10:'10 bars = 30 min forward' },
    legend:{
      sig:'per-bar derived signal read: trend{state,up,win,ma,slope}, king{cls,word,drift,score,magnet,offK}, srb{dom,cross,supF,resF,supPct,floorK,floorFade,ceilK,ceilFade}, breadth{net,dir,mag}, conf{dir,word,score,aligned,bull,bear,declared}',
      out:'forward outcome {mfe:max up, mae:max down(<=0), net, pxEnd, hitKing, revUp, revDn, n}'
    },
    syms:RECORDER_SYMS,
    snaps:day.snaps||{},
    events:day.events||{}
  };
}
var SAVED_TODAY=null;   // (v10.17) date-key of the day exported this session; drives the Analysis-tab banner
function saveDayToFile(dateKey){
  try{
    var payload=buildDayExport(dateKey);
    var name='gex_'+payload.date+'.json';
    var blob=new Blob([JSON.stringify(payload)],{type:'application/json'});
    var url=URL.createObjectURL(blob);
    var a=document.createElement('a');
    a.href=url; a.download=name; document.body.appendChild(a); a.click();
    setTimeout(function(){ document.body.removeChild(a); URL.revokeObjectURL(url); },1500);
    SAVED_TODAY=payload.date;
    if(typeof render==='function') render();   // refresh banner to "saved" state
    return name;
  }catch(e){ console.warn('[GPTS] saveDayToFile failed',e); return null; }
}
window.__gptsDebug.buildDayExport = buildDayExport;
window.__gptsDebug.saveDayToFile = saveDayToFile;
window.__gptsDebug.LASTFEED = LASTFEED;
window.__gptsDebug.refreshSym = refreshSym;

var LOGCACHE={ SPY:[], QQQ:[] };
function syncLog(sym){
  var S=STATE[sym];
  var rows=[];
  for(var key in S.setups){ var s=S.setups[key]; rows.push(s); }
  rows.sort(function(a,b){ return b.updated-a.updated; });
  LOGCACHE[sym]=rows;
  render();
}

console.log('[GPTS] v9.1 part3 loaded');

var PANEL=null, elBody=null, elCfg=null;
function css(el,obj){ for(var k in obj){ el.style[k]=obj[k]; } }

var PAL={
  bg:'#0b0e14', card:'#12161f', line:'#1e2530',
  longAccent:'#2ec27e', longSoft:'rgba(46,194,126,0.10)', longHdr:'rgba(46,194,126,0.16)',
  shortAccent:'#f0616d', shortSoft:'rgba(240,97,109,0.10)', shortHdr:'rgba(240,97,109,0.16)',
  ink:'#e6edf3', sub:'#8b98a9', time:'#f3f6fa', amber:'#f2b45a', gold:'#e3c341', blue:'#4a90d9'
};

function tokenLabel(tok, dir){
  if(tok==='TST') return 'PB';
  if(tok==='CONF') return (dir==='long') ? 'Long' : 'Short';
  return tok;
}
function stageEpoch(s, tok){
  if(tok==='BO') return s.ts || s.boBar || null;
  if(tok==='FT') return s.ftBar || null;
  if(tok==='TST') return s.testBar || null;
  if(tok==='CONF') return s.confBar || null;
  if(tok==='GO') return s.goBar || null;
  if(tok==='VOID') return s.voidBar || s.updated || null;
  return null;
}
function stageTimeline(s){
  var parts=[];
  for(var i=0;i<s.tokens.length;i++){
    var t=s.tokens[i];
    if(t==='VOID'){ parts.push('VOID'); continue; }
    if(t==='T1'||t==='T2'||t==='T3'){ parts.push(t); continue; }
    var lbl=tokenLabel(t, s.dir);
    var ep=stageEpoch(s, t);
    if(ep){ parts.push(lbl+' '+fmtClock(ep)); }
    else { parts.push(lbl); }
  }
  var head=(s.dir==='long'?'LONG ':'SHORT ')+fmtNum(s.strike);
  return head+' | '+parts.join(' -> ');
}

function buildPanel(){
  if(document.getElementById('gpts-panel')) return;
  PANEL=document.createElement('div');
  PANEL.id='gpts-panel';
  css(PANEL,{position:'fixed', top:'60px', left:'', right:'12px', width:'300px',
    background:PAL.bg, color:PAL.ink, font:'12px/1.4 Inter,Arial,sans-serif',
    border:'1px solid '+PAL.line, borderRadius:'10px', zIndex:'999999',
    boxShadow:'0 8px 28px rgba(0,0,0,0.6)', userSelect:'none', overflow:'visible'});
  var hdr=document.createElement('div');
  hdr.id='gpts-hdr';
  css(hdr,{padding:'5px 11px', background:'#0f131b', borderBottom:'1px solid '+PAL.line,
    borderRadius:'10px 10px 0 0', cursor:'move', display:'flex',
    justifyContent:'space-between', alignItems:'center'});
  var ttl=document.createElement('span');
  ttl.textContent='Tapereader';
  css(ttl,{color:PAL.ink, fontSize:'12px', fontWeight:'800', letterSpacing:'0.4px',
    borderLeft:'3px solid '+PAL.blue, paddingLeft:'7px', whiteSpace:'nowrap'});
  hdr.appendChild(ttl);
  var right=document.createElement('span');
  css(right,{display:'flex', alignItems:'center', gap:'6px'});
  var grade=document.createElement('span');
  grade.id='gpts-grade';
  grade.title='Setup grade for the in-play direction.';
  css(grade,{fontSize:'11px', fontWeight:'800', padding:'1px 7px', borderRadius:'6px', display:'none'});
  right.appendChild(grade);
  var fresh=document.createElement('span');
  fresh.id='gpts-fresh';
  fresh.title='Feed freshness.';
  css(fresh,{fontSize:'9px', fontWeight:'700'});
  right.appendChild(fresh);
  var handle=document.createElement('span');
  handle.id='gpts-draghandle';
  handle.innerHTML='&#10303;';
  handle.title='Drag to move (you can also drag from any empty area of the panel).';
  css(handle,{cursor:'grab', color:PAL.sub, fontSize:'14px', lineHeight:'1', padding:'0 2px'});
  right.appendChild(handle);
  var gear=document.createElement('span');
  gear.id='gpts-gear';
  gear.innerHTML='&#9881;';
  gear.title='Config: open BO Pullback settings (node thresh, follow-through, signal type, trend).';
  css(gear,{cursor:'pointer', color:PAL.sub, fontSize:'15px', lineHeight:'1', padding:'0 2px'});
  gear.addEventListener('mousedown', function(e){ e.stopPropagation(); });
  gear.addEventListener('click', function(e){ e.stopPropagation(); toggleCfg(); });
  right.appendChild(gear);
  var clr=document.createElement('span');
  clr.id='gpts-clear';
  clr.textContent='Clear';
  clr.title='Erase every tracked setups and reset the state machine.';
  css(clr,{cursor:'pointer', color:PAL.amber, fontWeight:'600', fontSize:'10px',
    padding:'1px 7px', border:'1px solid '+PAL.line, borderRadius:'20px'});
  clr.addEventListener('mousedown', function(e){ e.stopPropagation(); });
  clr.addEventListener('click', function(e){ e.stopPropagation(); clearSignalsAll(); });
  right.appendChild(clr);
  hdr.appendChild(right);
  PANEL.appendChild(hdr);

  elCfg=document.createElement('div');
  elCfg.id='gpts-cfg';
  css(elCfg,{display:'none', position:'absolute', top:'36px', right:'8px', width:'240px',
    background:PAL.card, border:'1px solid '+PAL.line, borderRadius:'8px', zIndex:'1000001',
    boxShadow:'0 6px 20px rgba(0,0,0,0.6)', padding:'6px 8px 8px 8px'});
  PANEL.appendChild(elCfg);

  elBody=document.createElement('div');
  elBody.id='gpts-body';
  css(elBody,{padding:'9px 10px', cursor:'move', position:'relative'});
  PANEL.appendChild(elBody);

  // (v10.25) 5-step methodology popover (created once, positioned near the clicked icon)
  var pop=document.createElement('div');
  pop.id='gpts-step-pop';
  pop.innerHTML='<span class="gs-x">\u2715</span><div id="gpts-step-pop-body"></div>';
  pop.addEventListener('mousedown', function(e){ e.stopPropagation(); });   // don't drag the panel
  elBody.appendChild(pop);
  pop.querySelector('.gs-x').addEventListener('click', function(e){ e.stopPropagation(); closeStepPop(); });

  var grip=document.createElement('div');
  grip.id='gpts-grip';
  css(grip,{position:'absolute', right:'0', bottom:'0', width:'16px', height:'16px',
    cursor:'nwse-resize', background:'linear-gradient(135deg, transparent 50%, #3a4150 50%, #3a4150 60%, transparent 60%, transparent 70%, #3a4150 70%, #3a4150 80%, transparent 80%)'});
  PANEL.appendChild(grip);

  document.body.appendChild(PANEL);
  restorePos();
  restoreSize();
  makeDraggable([hdr, elBody]);   // drag from the header OR anywhere in the body
  makeResizable(grip);
}

function toggleCfg(){
  if(!elCfg) return;
  if(elCfg.style.display==='none'){ renderCfg(); elCfg.style.display='block'; }
  else { elCfg.style.display='none'; }
}

// Full-body drag: you can grab ANY empty area of the panel to move it, not
// just the top bar. A press-and-move is treated as a drag; a plain click or
// text-selection still works because we ignore mousedowns that land on
// interactive elements and only start dragging once the pointer actually moves.
function isInteractiveTarget(t){
  while(t && t!==PANEL){
    var tag=(t.tagName||'').toLowerCase();
    if(tag==='input'||tag==='select'||tag==='textarea'||tag==='button'||tag==='a') return true;
    if(t.id==='gpts-gear'||t.id==='gpts-clear'||t.id==='gpts-cfg'||t.id==='gpts-grip') return true;
    if(t.classList && (t.classList.contains('gpts-clr-sym'))) return true;
    t=t.parentNode;
  }
  return false;
}
function makeDraggable(dragEls){
  if(!Array.isArray(dragEls)) dragEls=[dragEls];
  var dragging=false, armed=false, sx=0, sy=0, ox=0, oy=0;
  var THRESH=4; // px of movement before a press becomes a drag (lets clicks/selection through)
  function onDown(e){
    if(e.button!==0) return;                 // left button only
    if(isInteractiveTarget(e.target)) return; // let buttons/inputs/links work
    armed=true; dragging=false; sx=e.clientX; sy=e.clientY;
    var r=PANEL.getBoundingClientRect(); ox=r.left; oy=r.top;
  }
  dragEls.forEach(function(el){ if(el) el.addEventListener('mousedown', onDown); });
  document.addEventListener('mousemove', function(e){
    if(!armed) return;
    if(!dragging){
      if(Math.abs(e.clientX-sx)<THRESH && Math.abs(e.clientY-sy)<THRESH) return;
      dragging=true;
      PANEL.style.right=''; PANEL.style.left=ox+'px'; PANEL.style.top=oy+'px';
    }
    var nx=ox+(e.clientX-sx), ny=oy+(e.clientY-sy);
    PANEL.style.left=nx+'px'; PANEL.style.top=ny+'px';
    e.preventDefault();
  });
  document.addEventListener('mouseup', function(){
    armed=false;
    if(!dragging) return; dragging=false;
    try{ localStorage.setItem(POS_KEY, JSON.stringify({left:PANEL.style.left, top:PANEL.style.top})); }catch(e){}
  });
}
function makeResizable(grip){
  var rz=false, sx=0, sy=0, ow=0, oh=0;
  grip.addEventListener('mousedown', function(e){
    rz=true; sx=e.clientX; sy=e.clientY;
    var r=PANEL.getBoundingClientRect(); ow=r.width; oh=r.height;
    e.preventDefault(); e.stopPropagation();
  });
  document.addEventListener('mousemove', function(e){
    if(!rz) return;
    var nw=ow+(e.clientX-sx), nh=oh+(e.clientY-sy);
    if(nw<240) nw=240; if(nw>560) nw=560;
    if(nh<160) nh=160; if(nh>900) nh=900;
    PANEL.style.width=nw+'px'; PANEL.style.height=nh+'px';
    render();
  });
  document.addEventListener('mouseup', function(){
    if(!rz) return; rz=false;
    try{ localStorage.setItem(SIZE_KEY, JSON.stringify({w:PANEL.style.width, h:PANEL.style.height})); }catch(e){}
  });
}
function restorePos(){
  try{ var p=JSON.parse(localStorage.getItem(POS_KEY)||'null');
    if(p&&p.left){ PANEL.style.right=''; PANEL.style.left=p.left; PANEL.style.top=p.top; }
  }catch(e){}
}
function restoreSize(){
  try{ var s=JSON.parse(localStorage.getItem(SIZE_KEY)||'null');
    if(s&&s.w){ PANEL.style.width=s.w; if(s.h) PANEL.style.height=s.h; }
  }catch(e){}
}

function dirColor(dir){ return dir==='long' ? PAL.longAccent : PAL.shortAccent; }
function fmtNum(x){ return (Math.round(mul(x,100))/100).toString(); }
function fmtFut(x){ return (Math.round(x)).toString(); }

// ============================================================================
// 5-STEP INFO ICONS (v10.25) — the Skylit 'How to Read & Use Heatseeker' method,
// quoted faithfully behind small circular ①②③④⑤ icons. CLICK opens a popover (text
// is long). Step 2 = EOD ONLY (day-trading tool). Step 4 keeps the doc rule of
// thumb verbatim even though our impl uses the soft booster.
// ============================================================================
var STEP_TEXT = {
  1:{t:'Step 1 — Identify the Magnets', h:
    '<div class="gs-sub">Concept of Magnets</div>'+
    '<p>Every node acts like a <b>magnet</b>. Price is attracted to these zones due to dealer positioning \u2014 yet the same areas can act as <b>walls that repel</b> price and create reversals.</p>'+
    '<ul><li><b>Larger node values = stronger magnets.</b></li><li>The <b>closer price drifts</b>, the <b>stronger the pull</b>.</li></ul>'+
    '<div class="gs-sub">Magnetic Behavior</div>'+
    '<ul><li>Farther from a high-value node \u2192 pull <b>weakens</b>.</li><li>Approaching one \u2192 pull <b>strengthens</b>.</li><li>Direct interaction can cause a <b>deflection/repulsion</b>.</li></ul>'},
  2:{t:'Step 2 — Spot the King Node', h:
    '<p>The <b>King Node</b> is the most influential node \u2014 where Market Makers prefer to <b>settle price</b> by the <b>end of day (EOD)</b>. This is a day-trading tool, so the King is your <b>EOD settlement anchor</b>.</p>'+
    '<div class="gs-sub">How to Use It</div>'+
    '<ul><li>Your <b>primary destination target</b> for the session.</li><li>Mark it as your <b>EOD anchor</b>.</li><li>Watch early-day <b>drive-offs</b> or late-day <b>pinning</b>.</li></ul>'},
  3:{t:'Step 3 — Define the Range', h:
    '<p>Ranges provide some of the <b>highest-probability setups</b> and best risk-to-reward.</p>'+
    '<ul><li>Find where price <b>consistently reverses</b> \u2014 these are the boundaries.</li><li>At the <b>extreme end</b>, <b>fade the edge</b> \u2014 don\u2019t chase.</li><li>Avoid <b>midpoints</b> \u2014 poor R:R, uncertain direction.</li></ul>'},
  4:{t:'Step 4 — Watch Gatekeeper Nodes', h:
    '<p><b>Gatekeeper Nodes</b> are defensive levels that often <b>block price</b> from reaching the King Node. They act as <b>strong rejection zones</b>, and failed tests can <b>trigger trend shifts</b>.</p>'+
    '<div class="gs-sub">How to Read Them</div>'+
    '<ul><li>Rejection \u2192 potential <b>map reshuffling</b> or <b>directional change</b>.</li><li><b>Early-day rejections</b> often mark <b>powerful reversals</b>.</li><li>Align <b>SPX, SPY, QQQ</b> \u2014 confluence increases probability; mixed lowers it.</li></ul>'+
    '<div class="gs-sub">Rule of thumb</div><p>SPX, SPY, and QQQ must agree \u2014 if one diverges, stand aside.</p>'},
  5:{t:'Step 5 — Map the Flow', h:
    '<p>Heatseeker updates <b>dynamically</b> as dealer positioning shifts. Each <b>reshuffle</b> is a change in Market Maker risk and can reshape your bias.</p>'+
    '<div class="gs-sub">Watch for</div>'+
    '<ul><li><b>Accumulation (Acm):</b> dealers building \u2192 <b>stronger</b> magnets.</li><li><b>Dissipation (Diss):</b> dealers closing \u2192 <b>weakening</b> nodes.</li><li><b>Reshuffling:</b> rapid changes \u2192 new structure.</li></ul>'+
    '<p>By playing <b>direct deflections off key nodes</b>, you\u2019re trading <b>ahead of the reshuffle</b>.</p>'+
    '<div class="gs-sub">Our rule</div><p>Accumulation only <b>attracts</b> price to a node. We make <b>no</b> deflect/break call \u2014 the breakout tracker resolves the outcome (broke \u2191 / broke \u2193 / held / false break).</p>'}
};
// small circular icon; data-gstep drives the delegated click handler in the panel.
function stepIcon(n, extraStyle){
  return '<span class="gs-ico" data-gstep="'+n+'" title="Step '+n+' \u2014 click for the method" '+
    'style="width:16px;height:16px;border-radius:50%;border:1px solid '+PAL.sub+';color:'+PAL.sub+';'+
    'font-size:9.5px;font-weight:800;display:inline-flex;align-items:center;justify-content:center;cursor:pointer;'+(extraStyle||'')+'">'+n+'</span>';
}
var STEP_POP_OPEN = 0;   // which step's popover is open (0 = none)
function ensureStepPop(){
  if(!elBody) return;
  if(elBody.querySelector('#gpts-step-pop')) return;   // still there
  var pop=document.createElement('div');
  pop.id='gpts-step-pop';
  pop.innerHTML='<span class="gs-x">\u2715</span><div id="gpts-step-pop-body"></div>';
  pop.addEventListener('mousedown', function(e){ e.stopPropagation(); });
  elBody.appendChild(pop);
  pop.querySelector('.gs-x').addEventListener('click', function(e){ e.stopPropagation(); closeStepPop(); });
  STEP_POP_OPEN=0;
}
function closeStepPop(){
  STEP_POP_OPEN=0;
  var pop=document.getElementById('gpts-step-pop');
  if(pop) pop.classList.remove('gs-show');
  var ons=document.querySelectorAll('#gpts-panel .gs-ico.gs-on');
  for(var i=0;i<ons.length;i++) ons[i].classList.remove('gs-on');
}
function openStepPop(n, iconEl){
  var pop=document.getElementById('gpts-step-pop'), body=document.getElementById('gpts-step-pop-body');
  if(!pop||!body||!STEP_TEXT[n]) return;
  if(STEP_POP_OPEN===n){ closeStepPop(); return; }   // toggle off
  closeStepPop();
  STEP_POP_OPEN=n;
  body.innerHTML='<div class="gs-ttl">'+STEP_TEXT[n].t+'</div>'+STEP_TEXT[n].h;
  // position just under the clicked icon (relative to elBody)
  if(iconEl && elBody){
    var br=elBody.getBoundingClientRect(), ir=iconEl.getBoundingClientRect();
    pop.style.top=(ir.bottom-br.top+6)+'px';
  } else { pop.style.top='40px'; }
  pop.classList.add('gs-show');
  if(iconEl) iconEl.classList.add('gs-on');
}
// delegated: wire every .gs-ico in the body (called after each render)
function wireStepIcons(){
  if(!elBody) return;
  var icos=elBody.querySelectorAll('.gs-ico');
  for(var i=0;i<icos.length;i++){
    (function(el){
      el.addEventListener('mousedown', function(e){ e.stopPropagation(); });   // don't start a panel drag
      el.addEventListener('click', function(e){ e.stopPropagation(); openStepPop(parseInt(el.getAttribute('data-gstep'),10), el); });
    })(icos[i]);
  }
}

function sectionHdr(text){
  return '<div style="background:'+PAL.card+';color:'+PAL.ink+';font-size:9px;font-weight:800;'+
    'letter-spacing:0.5px;padding:3px 9px;margin:4px 0 2px 0;border-left:3px solid '+PAL.blue+';'+
    'border-radius:6px">'+text+'</div>';
}
// Section header with a right-aligned add-on (badges/pills) on the SAME row.
// Optional `tip` adds a hover description for the whole section.
function sectionHdrRight(text, rightHtml, tip){
  var t = tip ? (' title="'+(''+tip).replace(/"/g,'')+'"') : '';
  return '<div'+t+' style="background:'+PAL.card+';color:'+PAL.ink+';font-size:9px;font-weight:800;'+
    'letter-spacing:0.5px;padding:3px 9px;margin:4px 0 2px 0;border-left:3px solid '+PAL.blue+';'+
    'border-radius:6px;display:flex;justify-content:space-between;align-items:center;gap:6px">'+
    '<span>'+text+'</span>'+
    '<span style="display:flex;gap:5px;align-items:center;font-weight:700">'+(rightHtml||'')+'</span>'+
  '</div>';
}
function symSignalsHdr(sym){
  var tip='SIGNALS: breakout-pullback setups tracked per node. Columns are individual attempts (L=long/green, S=short/red), rows trace the lifecycle: BO (first close beyond the node) \u2192 FT (a later bar holds fully beyond it) \u2192 Void (time it failed, if it did). Newest setup is left-most.';
  return '<div title="'+tip.replace(/"/g,'')+'" style="background:'+PAL.card+';color:'+PAL.ink+';font-size:9px;font-weight:800;'+
    'letter-spacing:0.5px;padding:3px 9px;margin:4px 0 2px 0;border-left:3px solid '+PAL.blue+';'+
    'border-radius:6px;display:flex;justify-content:space-between;align-items:center">'+
    '<span title="BO = breakout-pullback setup tracker for '+sym+'.">BO</span>'+
    '<span class="gpts-clr-sym" data-sym="'+sym+'" title="Erase tracked setups for '+sym+' only." '+
    'style="cursor:pointer;color:'+PAL.amber+';font-weight:600;font-size:10px;padding:0 6px;border:1px solid '+PAL.line+';border-radius:20px">Clear</span>'+
    '</div>';
}
function typeHdr(text){
  return '<div style="color:'+PAL.sub+';font-size:10px;font-weight:700;letter-spacing:0.3px;'+
    'padding:3px 4px;margin:2px 0 1px 0">'+text+'</div>';
}
function sep(){ return '<div style="border-top:1px solid '+PAL.line+';margin:6px 0"></div>'; }

function segBtn(val, label, tip){
  var on = CFG.dir===val;
  var bg = on ? '#2a3550' : '#14161c';
  var col = on ? PAL.ink : PAL.sub;
  var bd = on ? PAL.blue : PAL.line;
  return '<span class="gpts-seg" data-val="'+val+'" title="'+tip+'" '+
    'style="cursor:pointer;flex:1;text-align:center;padding:2px 0;background:'+bg+';color:'+col+';'+
    'border:1px solid '+bd+';border-radius:4px;font-size:10px;font-weight:600">'+label+'</span>';
}
function cfgHtml(){
  var html='';
  html+='<div style="color:'+PAL.ink+';font-size:12px;font-weight:700;padding:1px 2px 5px 2px;border-bottom:1px solid '+PAL.line+';margin-bottom:4px">BO Pullback Config</div>';
  var boChk = CFG.boPb ? 'checked' : '';
  html+='<div style="display:flex;align-items:center;justify-content:space-between;padding:2px 4px" '+
    'title="BO Pullback: show the breakout-pullback signal type.">'+
    '<span style="font-weight:600">BO Pullback</span>'+
    '<input type="checkbox" class="gpts-bopb" '+boChk+' style="cursor:pointer"></div>';
  html+='<div style="padding:2px 4px" title="BO Node Thresh: minimum node strength (percent of king).">'+
    '<div style="display:flex;justify-content:space-between"><span>1. BO Node Thresh (% King)</span><span class="gpts-nt-val">'+CFG.nodeThresh+'%</span></div>'+
    '<input type="range" class="gpts-nt" min="20" max="100" step="5" value="'+CFG.nodeThresh+'"></div>';
  var ftChk = CFG.ftReq ? 'checked' : '';
  html+='<div style="display:flex;align-items:center;justify-content:space-between;padding:2px 4px" '+
    'title="BO Followthrough Req: only show a signal once it posts a follow-through bar.">'+
    '<span>2. BO Followthrough Req</span>'+
    '<input type="checkbox" class="gpts-ftreq" '+ftChk+' style="cursor:pointer"></div>';
  html+='<div style="padding:2px 4px" title="Signal Type: filter which side of signals to display.">'+
    '<div style="margin-bottom:2px">3. Signal Type</div>'+
    '<div style="display:flex;gap:3px">'+
      segBtn('both','Both','Both: show long and short signals.')+
      segBtn('longs','Longs','Longs: show only long signals.')+
      segBtn('shorts','Shorts','Shorts: show only short signals.')+
    '</div></div>';
  var trChk = CFG.trendOn ? 'checked' : '';
  html+='<div style="display:flex;align-items:center;justify-content:space-between;padding:2px 4px" '+
    'title="Trend: only create new breakouts that agree with the trend MA and slope.">'+
    '<span>4. Trend</span>'+
    '<input type="checkbox" class="gpts-trend" '+trChk+' style="cursor:pointer"></div>';
  var slbl='Trend MA period. Longs need price above this MA and rising slope; shorts below and falling. Fallback 50.';
  html+='<div style="display:flex;gap:12px;align-items:center;padding:2px 4px" title="'+slbl+'">'+
    '<label style="display:flex;align-items:center;gap:4px;font-size:10px;color:'+PAL.sub+'">Trend MA'+
    '<input type="text" maxlength="3" class="gpts-trend-ma" value="'+CFG.trendMA.SPY+'" style="width:42px;text-align:center;background:#14161c;color:'+PAL.ink+';border:1px solid '+PAL.line+';border-radius:3px;box-sizing:border-box"></label>'+
    '</div>';

  // ===== DISPLAY group (#5) =====
  html+=cfgGroup('Display');
  var cmpChk = CFG.compact ? 'checked' : '';
  html+='<div style="display:flex;align-items:center;justify-content:space-between;padding:2px 4px" title="Compact node cells: single-line rows (strike, role\u00b7%King, badge). Off = full sparkline+growth view.">'+
    '<span>Compact node cells</span>'+
    '<input type="checkbox" class="gpts-compact" '+cmpChk+' style="cursor:pointer"></div>';
  html+='<div style="padding:2px 4px" title="Growth strip length: how many 3m closes the strip shows (each = 3 min).">'+
    '<div style="display:flex;justify-content:space-between"><span>Growth strip length</span><span class="gpts-sl-val">'+CFG.stripLen+' \u00b7 '+(CFG.stripLen*3)+'m</span></div>'+
    '<input type="range" class="gpts-nt gpts-striplen" min="4" max="12" step="1" value="'+CFG.stripLen+'"></div>';

  // ===== ALERTS group (#10) =====
  html+=cfgGroup('Alerts');
  html+='<div style="display:grid;grid-template-columns:1fr auto auto;gap:6px;font-size:8.5px;color:'+PAL.sub+';font-weight:700;padding:1px 4px;letter-spacing:.4px"><span>event</span><span>vis</span><span>snd</span></div>';
  html+=alertRow('inplayAccum','In-play \u2192 Accumulating');
  html+=alertRow('dissipate','Node \u2192 Dissipating');
  html+=alertRow('kingRoll','King rolls');
  html+=alertRow('trap','Trap / clean forms');
  html+=alertRow('absorption','Absorption confirms');
  html+=alertRow('feedStale','Feed goes stale');

  html+='<div style="color:'+PAL.sub+';font-size:8.5px;padding:5px 4px 1px;line-height:1.4">Existing settings (thresholds, FT, trend, MA) carry over automatically \u2014 config migrated v7\u2192v8.</div>';
  return html;
}
function cfgGroup(name){
  return '<div style="margin:8px 0 3px;color:'+PAL.sub+';font-size:9px;font-weight:800;letter-spacing:.6px;text-transform:uppercase;border-top:1px solid '+PAL.line+';padding-top:5px">'+name+' <span style="color:'+PAL.longAccent+';font-size:7.5px">NEW</span></div>';
}
function alertRow(ev, label){
  var a=CFG.alerts[ev]||{on:false,vis:false,snd:false};
  var onChk=a.on?'checked':'', visChk=a.vis?'checked':'', sndChk=a.snd?'checked':'';
  return '<div style="display:grid;grid-template-columns:1fr auto auto;gap:6px;align-items:center;padding:2px 4px;font-size:10px" data-ev="'+ev+'">'+
    '<label style="display:flex;align-items:center;gap:5px;cursor:pointer"><input type="checkbox" class="gpts-al-on" data-ev="'+ev+'" '+onChk+' style="cursor:pointer">'+label+'</label>'+
    '<input type="checkbox" class="gpts-al-vis" data-ev="'+ev+'" '+visChk+' style="cursor:pointer">'+
    '<input type="checkbox" class="gpts-al-snd" data-ev="'+ev+'" '+sndChk+' style="cursor:pointer"></div>';
}
function renderCfg(){ if(!elCfg) return; elCfg.innerHTML=cfgHtml(); wireConfig(); }

function injectSliderCss(){
  if(document.getElementById('gpts-slider-css')) return;
  var st=document.createElement('style');
  st.id='gpts-slider-css';
  st.textContent =
    '#gpts-panel input.gpts-nt{ -webkit-appearance:none; appearance:none; width:100%; height:6px; border-radius:4px; background:#5a6270; outline:none; cursor:pointer; margin-top:4px; }'+
    '#gpts-panel input.gpts-nt::-webkit-slider-thumb{ -webkit-appearance:none; appearance:none; width:14px; height:14px; border-radius:50%; background:#cfd6e0; border:1px solid #8a93a3; cursor:pointer; }'+
    '#gpts-panel input.gpts-nt::-moz-range-thumb{ width:14px; height:14px; border-radius:50%; background:#cfd6e0; border:1px solid #8a93a3; cursor:pointer; }'+
    '#gpts-panel input.gpts-nt::-moz-range-track{ height:6px; border-radius:4px; background:#5a6270; }'+
    '#gpts-panel .gpts-gridwrap{ overflow-x:auto; }'+
    '#gpts-panel .gpts-gridwrap::-webkit-scrollbar{ height:7px; }'+
    '#gpts-panel .gpts-gridwrap::-webkit-scrollbar-thumb{ background:#3a4150; border-radius:4px; }'+
    '#gpts-panel .gpts-gridwrap::-webkit-scrollbar-track{ background:#0f131b; }'+
    '#gpts-panel .gpts-analysis-scroll::-webkit-scrollbar{ width:8px; }'+
    '#gpts-panel .gpts-analysis-scroll::-webkit-scrollbar-thumb{ background:#3a4150; border-radius:5px; }'+
    '#gpts-panel .gpts-analysis-scroll::-webkit-scrollbar-track{ background:#0f131b; }'+
    '#gpts-panel table.gpts-grid{ border-collapse:collapse; }'+
    '#gpts-panel table.gpts-grid td{ padding:2px 10px; font-size:11px; white-space:nowrap; text-align:right; cursor:default; color:'+PAL.ink+'; }'+
    '#gpts-panel table.gpts-grid td.g-lbl{ position:sticky; left:0; z-index:2; text-align:left; color:'+PAL.sub+'; font-weight:700; border-right:2px solid '+PAL.line+'; }'+
    '#gpts-panel table.gpts-grid td.g-sep{ border-left:1px solid '+PAL.line+'; }'+
    '#gpts-panel table.gpts-grid td.g-time{ color:'+PAL.time+'; }'+
    '#gpts-panel table.gpts-grid td.g-na{ color:'+PAL.amber+'; }'+
    '#gpts-panel table.gpts-grid td.g-void{ color:'+PAL.amber+'; }'+
    // #7 S/R vertical scroll box: thin dark theme.
    '#gpts-panel .gpts-sr-scroll::-webkit-scrollbar{ width:7px; }'+
    '#gpts-panel .gpts-sr-scroll::-webkit-scrollbar-thumb{ background:#3a4150; border-radius:4px; }'+
    '#gpts-panel .gpts-sr-scroll::-webkit-scrollbar-track{ background:#0f131b; }'+
    // (v10.25) 5-step info icons + popover
    '#gpts-panel .gs-ico:hover{ border-color:'+PAL.blue+' !important; color:'+PAL.blue+' !important; }'+
    '#gpts-panel .gs-ico.gs-on{ background:'+PAL.blue+'; border-color:'+PAL.blue+'; color:#fff; }'+
    '#gpts-step-pop{ position:absolute; z-index:40; left:8px; right:8px; background:#0d1017; border:1px solid '+PAL.blue+'; border-radius:10px; padding:11px 30px 11px 13px; box-shadow:0 8px 30px rgba(0,0,0,.6); display:none; }'+
    '#gpts-step-pop.gs-show{ display:block; }'+
    '#gpts-step-pop .gs-ttl{ font-size:12px; font-weight:800; color:'+PAL.blue+'; margin:0 0 6px; }'+
    '#gpts-step-pop .gs-x{ position:absolute; top:7px; right:10px; color:'+PAL.sub+'; cursor:pointer; font-weight:800; font-size:13px; }'+
    '#gpts-step-pop .gs-x:hover{ color:'+PAL.ink+'; }'+
    '#gpts-step-pop p{ margin:0 0 7px; font-size:11px; line-height:1.5; color:'+PAL.sub+'; }'+
    '#gpts-step-pop b{ color:'+PAL.ink+'; }'+
    '#gpts-step-pop .gs-sub{ font-size:10px; font-weight:800; color:'+PAL.ink+'; margin:6px 0 3px; }'+
    '#gpts-step-pop ul{ margin:2px 0 7px; padding-left:16px; }'+
    '#gpts-step-pop li{ font-size:11px; line-height:1.5; color:'+PAL.sub+'; margin:2px 0; }';
  document.head.appendChild(st);
}

function wireConfig(){
  if(!elCfg) return;
  var bo=elCfg.querySelector('.gpts-bopb');
  if(bo) bo.addEventListener('change', function(){ CFG.boPb=bo.checked; saveCfg(); render(); });
  var ft=elCfg.querySelector('.gpts-ftreq');
  if(ft) ft.addEventListener('change', function(){ CFG.ftReq=ft.checked; saveCfg(); render(); });
  var segs=elCfg.querySelectorAll('.gpts-seg');
  for(var i=0;i<segs.length;i++){
    (function(el){ el.addEventListener('click', function(){ CFG.dir=el.getAttribute('data-val'); saveCfg(); renderCfg(); render(); }); })(segs[i]);
  }
  var nt=elCfg.querySelector('.gpts-nt');
  var ntv=elCfg.querySelector('.gpts-nt-val');
  if(nt){
    nt.addEventListener('input', function(){ if(ntv) ntv.textContent=nt.value+'%'; });
    nt.addEventListener('change', function(){
      var v=parseInt(nt.value,10);
      if(isNaN(v)||v<20) v=20; if(v>100) v=100;
      CFG.nodeThresh=v; MIN_STRENGTH=v; saveCfg(); render();
    });
  }
  var tr=elCfg.querySelector('.gpts-trend');
  if(tr) tr.addEventListener('change', function(){ CFG.trendOn=tr.checked; saveCfg(); render(); });
  var tma=elCfg.querySelector('.gpts-trend-ma');
  function commitTrendMa(){
    var mv=parseInt(tma&&tma.value,10);
    if(isNaN(mv)||mv<1) mv=50;
    CFG.trendMA.SPY=mv; CFG.trendMA.QQQ=mv;
    if(tma) tma.value=mv;
    saveCfg(); render();
  }
  function digitsOnly(el){ if(!el) return; el.addEventListener('input', function(){ var v=el.value.replace(/[^0-9]/g,''); if(v.length>3) v=v.slice(0,3); if(v!==el.value) el.value=v; }); }
  digitsOnly(tma);
  if(tma) tma.addEventListener('change', commitTrendMa);
  // #5 display controls
  var cmp=elCfg.querySelector('.gpts-compact');
  if(cmp) cmp.addEventListener('change', function(){ CFG.compact=cmp.checked; saveCfg(); render(); });
  var sl=elCfg.querySelector('.gpts-striplen');
  var slv=elCfg.querySelector('.gpts-sl-val');
  if(sl){
    sl.addEventListener('input', function(){ if(slv) slv.textContent=sl.value+' \u00b7 '+(parseInt(sl.value,10)*3)+'m'; });
    sl.addEventListener('change', function(){ var v=parseInt(sl.value,10); if(isNaN(v)||v<4)v=4; if(v>12)v=12; CFG.stripLen=v; saveCfg(); render(); });
  }
  // #10 alert controls
  function wireAlert(cls, field){
    var els=elCfg.querySelectorAll(cls);
    for(var i=0;i<els.length;i++){
      (function(el){ el.addEventListener('change', function(){ var ev=el.getAttribute('data-ev'); if(CFG.alerts[ev]){ CFG.alerts[ev][field]=el.checked; saveCfg(); } }); })(els[i]);
    }
  }
  wireAlert('.gpts-al-on','on');
  wireAlert('.gpts-al-vis','vis');
  wireAlert('.gpts-al-snd','snd');
}

console.log('[GPTS] v9.1 part4 loaded');

function gridFor(dir, rows){
  var setups=rows.filter(function(s){ return s.dir===dir && passesFilters(s); });
  var label = dir==='long' ? 'Long' : 'Short';
  var accent = dir==='long' ? PAL.longAccent : PAL.shortAccent;
  var hdrbg = dir==='long' ? PAL.longHdr : PAL.shortHdr;
  var confRow = dir==='long' ? 'LONG' : 'SHORT';
  var confLabel = dir==='long' ? 'Long' : 'Short';
  var head='<div style="color:'+accent+';background:'+hdrbg+';font-size:10px;font-weight:800;'+
    'letter-spacing:0.4px;padding:2px 8px;margin:3px 0 0 0;border-radius:5px 5px 0 0">'+label+'</div>';
  if(!setups.length){
    return head+'<div style="color:'+PAL.sub+';padding:3px 8px;font-size:11px;background:'+PAL.card+';border-radius:0 0 5px 5px">none</div>';
  }
  var rowDefs=[
    {key:'STRIKE', lbl:'Strike'},
    {key:'PCT',    lbl:'%King'},
    {key:'BO',     lbl:'BO'},
    {key:'FT',     lbl:'FT'},
    {key:'PB',     lbl:'PB'},
    {key:confRow,  lbl:confLabel},
    {key:'GO',     lbl:'Go'},
    {key:'VOID',   lbl:'Void'}
  ];
  var html='<div class="gpts-gridwrap" style="background:'+PAL.card+';border-radius:0 0 6px 6px;padding-bottom:2px">';
  html+='<table class="gpts-grid"><tbody>';
  rowDefs.forEach(function(rd){
    var underStrike = (rd.key==='STRIKE') ? ';border-bottom:2px solid '+accent : '';
    var lblTip;
    if(rd.key==='STRIKE') lblTip='Node price each setup is anchored at.';
    else if(rd.key==='PCT') lblTip='Node strength as % of King (breakout to current).';
    else if(rd.key==='VOID') lblTip='If a setup failed, the time it voided.';
    else if(rd.key===confRow) lblTip=(dir==='long'?'Long':'Short')+' confirmation bar time.';
    else if(rd.key==='BO') lblTip='Breakout: first close beyond the node.';
    else if(rd.key==='FT') lblTip='Follow-through: a later bar holds fully beyond the node.';
    else if(rd.key==='PB') lblTip='Pullback: price wicked back to retest the node after follow-through.';
    else lblTip=rd.lbl;
    html+='<tr>';
    html+='<td class="g-lbl" title="'+lblTip.replace(/"/g,'')+'" style="background:'+PAL.card+underStrike+'">'+rd.lbl+'</td>';
    setups.forEach(function(s, ci){
      var sepCls = ci>0 ? ' g-sep' : '';
      var extra = (rd.key==='STRIKE') ? ';border-bottom:2px solid '+accent : '';
      var cellCls='', cellTxt='', cellTip='';
      if(rd.key==='STRIKE'){
        cellCls=sepCls; cellTxt='<span style="color:'+accent+';font-weight:800">'+fmtNum(s.strike)+'</span>';
        cellTip=strikeCellTip(dir, s.strike);
        html+='<td class="'+cellCls.trim()+'" title="'+cellTip+'" style="'+extra+'">'+cellTxt+'</td>';
        return;
      }
      if(rd.key==='PCT'){
        var cur=livePctAt('SPY', s.strike);
        var bo=(typeof s.boPct==='number')?s.boPct:null;
        if(bo!=null && cur!=null){ cellTxt=bo+'%–'+cur+'%'; cellCls=sepCls; }
        else if(cur!=null){ cellTxt=cur+'%'; cellCls=sepCls; }
        else { cellTxt='NA'; cellCls=sepCls+' g-na'; }
        cellTip=kingCellTip(s.strike);
        html+='<td class="'+cellCls.trim()+'" title="'+cellTip+'">'+cellTxt+'</td>';
        return;
      }
      var eps=setupStageEpochs(s);
      var ep=eps[rd.key];
      var isVoidRow=(rd.key==='VOID');
      if(ep!=null){
        cellCls=sepCls+(isVoidRow?' g-void':' g-time');
        cellTxt=fmtClock(ep);
      } else {
        cellCls=sepCls;
        cellTxt='<span style="color:'+PAL.line+'">·</span>';
      }
      cellTip=stageCellTip(rd.key, dir, s.strike);
      html+='<td class="'+cellCls.trim()+'" title="'+cellTip+'">'+cellTxt+'</td>';
    });
    html+='</tr>';
  });
  html+='</tbody></table></div>';
  return head+html;
}
function signalGrid(){
  var rows=LOGCACHE.SPY||[];
  if(CFG.boPb!==true){
    return '<div style="color:'+PAL.sub+';padding:2px 6px;font-size:11px">BO Pullback signals off</div>';
  }
  var html=combinedGrid(rows);
  return html;
}

// Combined Long+Short grid: one table, no per-direction heading bars, each
// setup is a column tinted by its direction (green=long, red=short) with an
// L/S tag on the strike. Saves the vertical space the two stacked sections
// (plus their "none" rows) used to cost.
function combinedGrid(rows){
  var showLong = CFG.dir!=='shorts';
  var showShort = CFG.dir!=='longs';
  var setups=[];
  rows.forEach(function(s){
    if(!passesFilters(s)) return;
    if(s.dir==='long' && !showLong) return;
    if(s.dir==='short' && !showShort) return;
    setups.push(s);
  });
  // Newest first: sort by most-recent activity across BOTH directions so the
  // latest signal is the left-most column, regardless of long/short.
  function recencyOf(s){
    return Math.max(
      s.updated||0, s.voidBar||0, s.goBar||0, s.confBar||0,
      s.testBar||0, s.ftBar||0, s.boBar||0, s.ts||0
    );
  }
  setups.sort(function(a,b){ return recencyOf(b)-recencyOf(a); });
  if(!setups.length){
    return '<div style="color:'+PAL.sub+';padding:3px 8px;font-size:11px;background:'+PAL.card+';border-radius:0 0 6px 6px">No active setups</div>';
  }
  // Trimmed to the essentials: Strike, %King, BO, FT. (PB, Conf, Go, and the
  // Void ROW removed to save space — a voided signal shows a ⊘ icon next to its
  // strike instead; an ex-King anchor shows a ⚠ flag. #3 / v2 layout)
  var rowDefs=[
    {key:'STRIKE', lbl:'Strike'},
    {key:'PCT',    lbl:'%King'},
    {key:'BO',     lbl:'BO'},
    {key:'FT',     lbl:'FT'}
  ];
  var html='<div class="gpts-gridwrap" style="background:'+PAL.card+';border-radius:0 0 6px 6px;padding:2px 0">';
  html+='<table class="gpts-grid"><tbody>';
  rowDefs.forEach(function(rd){
    var lblTip;
    if(rd.key==='STRIKE') lblTip='Node price each setup is anchored at. Green = long, red = short.';
    else if(rd.key==='PCT') lblTip='Node strength as % of King (breakout to current).';
    else if(rd.key==='VOID') lblTip='If a setup failed, the time it voided.';
    else if(rd.key==='CONF') lblTip='Confirmation bar time (long: bullish close off retest; short: bearish close off retest).';
    else if(rd.key==='BO') lblTip='Breakout: first close beyond the node.';
    else if(rd.key==='FT') lblTip='Follow-through: a later bar holds fully beyond the node.';
    else if(rd.key==='PB') lblTip='Pullback: price wicked back to retest the node after follow-through.';
    else lblTip=rd.lbl;
    html+='<tr>';
    var underRow = (rd.key==='STRIKE') ? ';border-bottom:2px solid '+PAL.line : '';
    html+='<td class="g-lbl" title="'+lblTip.replace(/"/g,'')+'" style="background:'+PAL.card+underRow+'">'+rd.lbl+'</td>';
    setups.forEach(function(s, ci){
      var accent = s.dir==='long' ? PAL.longAccent : PAL.shortAccent;
      var confRow = s.dir==='long' ? 'LONG' : 'SHORT';
      var sepCls = ci>0 ? ' g-sep' : '';
      var extra = (rd.key==='STRIKE') ? ';border-bottom:2px solid '+accent : '';
      if(rd.key==='STRIKE'){
        var tag = s.dir==='long' ? 'L' : 'S';
        // ⊘ void icon (Void row removed) + ⚠ ex-King flag. #3 / v2 layout
        var voidIc='';
        if(s.voided){
          var vt = s.voidBar ? (' '+fmtClock(s.voidBar)) : '';
          voidIc=' <span title="This setup voided'+vt+'." style="color:'+PAL.shortAccent+';font-size:9px">\u2298</span>';
        }
        var xk=exKingInfo('SPY', s.strike);
        var xkIc='';
        if(xk){
          xkIc=' <span title="Anchored to '+fmtNum(s.strike)+', which was King until '+(xk.t?fmtClock(xk.t):'earlier')+'; King has since rolled to '+fmtNum(xk.to)+'. This setup\u2019s premise (this strike is King) has changed \u2014 scrutinize before trusting it." style="color:'+PAL.amber+';font-size:9px">\u26a0</span>';
        }
        var cellTxt='<span style="color:'+accent+';font-weight:800">'+fmtNum(s.strike)+'</span>'+
          '<span style="color:'+accent+';font-size:8px;font-weight:800;margin-left:3px;border:1px solid '+accent+';border-radius:3px;padding:0 2px">'+tag+'</span>'+xkIc+voidIc;
        html+='<td class="'+sepCls.trim()+'" title="'+strikeCellTip(s.dir, s.strike)+'" style="'+extra+'">'+cellTxt+'</td>';
        return;
      }
      if(rd.key==='PCT'){
        var cur=livePctAt('SPY', s.strike);
        var bo=(typeof s.boPct==='number')?s.boPct:null;
        var cellTxt2, cellCls2=sepCls;
        if(bo!=null && cur!=null){ cellTxt2=bo+'%–'+cur+'%'; }
        else if(cur!=null){ cellTxt2=cur+'%'; }
        else { cellTxt2='NA'; cellCls2=sepCls+' g-na'; }
        html+='<td class="'+cellCls2.trim()+'" title="'+kingCellTip(s.strike)+'">'+cellTxt2+'</td>';
        return;
      }
      var eps=setupStageEpochs(s);
      var epKey = (rd.key==='CONF') ? confRow : rd.key;
      var ep=eps[epKey];
      var isVoidRow=(rd.key==='VOID');
      var cellCls3, cellTxt3;
      if(ep!=null){ cellCls3=sepCls+(isVoidRow?' g-void':' g-time'); cellTxt3=fmtClock(ep); }
      else { cellCls3=sepCls; cellTxt3='<span style="color:'+PAL.line+'">·</span>'; }
      html+='<td class="'+cellCls3.trim()+'" title="'+stageCellTip(rd.key==='CONF'?confRow:rd.key, s.dir, s.strike)+'">'+cellTxt3+'</td>';
    });
    html+='</tr>';
  });
  html+='</tbody></table></div>';
  return html;
}

function passesFilters(s){
  if(CFG.boPb!==true) return false;
  if(CFG.dir==='longs' && s.dir!=='long') return false;
  if(CFG.dir==='shorts' && s.dir!=='short') return false;
  if(CFG.ftReq){
    var reached = s.tokens.indexOf('FT')>=0 || s.stage==='FT' || s.stage==='TST' || s.stage==='CONF' || s.stage==='GO';
    if(!reached) return false;
  }
  return true;
}

function stageCellTip(stageName, dir, strike){
  var m={
    'BO':'BO (Breakout): first 3m close beyond '+fmtNum(strike)+' in the '+(dir==='long'?'long':'short')+' direction. Start of the setup.',
    'FT':'FT (Follow-through): a later bar holds entirely beyond '+fmtNum(strike)+' (no wick back through). Confirms the break stuck.',
    'PB':'PB (Pullback): price wicked back to retest '+fmtNum(strike)+' after follow-through.',
    'LONG':'Long confirm: a bullish bar closed in-direction off the retest of '+fmtNum(strike)+'.',
    'SHORT':'Short confirm: a bearish bar closed in-direction off the retest of '+fmtNum(strike)+'.',
    'GO':'Go: price broke the confirmation bar’s extreme in-direction — the structural trigger completed.',
    'VOID':'Void: the setup failed — price closed back through '+fmtNum(strike)+' too many times, or a stage aged out. Time shown is when it voided.'
  };
  return (m[stageName]||stageName).replace(/"/g,'');
}
function kingCellTip(strike){
  return ('%King for node '+fmtNum(strike)+': its GEX strength as a percent of the day’s King node. Range shows strength at breakout to current. NA means the node is off the current live map or had no reading.').replace(/"/g,'');
}
function strikeCellTip(dir, strike){
  return ((dir==='long'?'Long':'Short')+' setup anchored at node '+fmtNum(strike)+'. Each column is one attempt; rows below trace its lifecycle in Central time.').replace(/"/g,'');
}
function setupStageEpochs(s){
  var confRow = s.dir==='long' ? 'LONG' : 'SHORT';
  var o={ BO:(s.ts||s.boBar||null), FT:(s.ftBar||null), PB:(s.testBar||null), GO:(s.goBar||null), VOID:(s.voidBar||null) };
  o[confRow]=(s.confBar||null);
  return o;
}

function trendStateInfo(){
  var v=trendVerdict('SPY');
  // (v10.18) five-state machine -> label/color via helpers.
  var label, col=trendColorOf(v.state);
  if(v.state==='up') label='Up';
  else if(v.state==='dn') label='Dn';
  else if(v.state==='up-broken') label='Up broken';
  else if(v.state==='dn-broken') label='Dn broken';
  else if(v.state==='na') label='NA';
  else label='No trend';
  return { label:label, color:col, verdict:v, word:trendWordOf(v.state) };
}
function trendBadgeHtml(){
  var info=trendStateInfo();
  var v=info.verdict||{};
  var counter = v.win ? (v.up+'/'+v.win) : '';
  var slopeTxt = (typeof v.slope==='number') ? ((v.slope>0?'+':'')+(Math.round(mul(v.slope,100))/100)) : 'n/a';
  var tip='Trend state from closes vs SMA'+CFG.trendMA.SPY+' over the last '+(v.win||0)+' closed 3m bars plus SMA slope. Counter = closes on the dominant side ('+counter+'). Slope '+slopeTxt+'.';
  return '<span title="'+tip.replace(/"/g,'')+'" style="color:'+info.color+';font-weight:700;font-size:10px;padding:1px 8px;border:1px solid '+info.color+';border-radius:20px;background:rgba(255,255,255,0.02)">'+info.label+'</span>';
}

// #1 CONFLUENCE TARGET SCORING.
// T1/T2 are NOT limited to Gatekeeper+King. Every strike between price and the
// far structural target is scored on strength + accumulation velocity +
// proximity + a structural bonus (King/GK). Targets are then assigned by the
// ORDER price physically reaches the qualifying nodes (nearest first). An
// accumulating node CAN outrank King/GK for the T1 slot, but the King ALWAYS
// keeps a slot (end-of-day magnet). Eligibility uses hysteresis: a strike earns
// candidacy by Building and only loses it when it Dissipates (Steady keeps it).
// A chosen target that is also the strongest-accumulator gets the confluence ★.
function scoreTargetNode(sym, w, px, longSide, kingK, gkK){
  var pct = w.pct||0;
  var rate = nodeBuildRate(sym, w.k);          // %King net over the strip window
  var dist = Math.abs(w.k-px);
  var proxScore = 1/(1+dist);                  // nearer = higher
  var structBonus = 0;
  if(kingK!=null && Math.abs(w.k-kingK)<0.001) structBonus = 1.0;
  else if(gkK!=null && Math.abs(w.k-gkK)<0.001) structBonus = 0.6;
  var velScore = Math.max(0, rate)/20;         // normalize ~ a 20pt build
  return TS_W_STRENGTH*(pct/100)
       + TS_W_VELOCITY*velScore
       + TS_W_PROXIMITY*proxScore
       + TS_W_STRUCT*structBonus;
}
function targetLadder(){
  var S=STATE.SPY;
  var px=S.price;
  var v=trendVerdict('SPY');
  if(px==null || !S.walls.length || (v.state!=='up' && v.state!=='dn')) return { t1:null, t2:null, t1star:false, t2star:false, dir:null };
  var longSide = v.state==='up';
  var dir = longSide ? 'long' : 'short';
  var sym='SPY';
  // Structural anchors (still matter, just no longer the only thing).
  var tk=tapeKingStrike(sym);
  var kingNode=null;
  var beyond=S.walls.filter(function(w){ return longSide ? w.k>px : w.k<px; });
  beyond.sort(function(a,b){ return longSide ? a.k-b.k : b.k-a.k; });
  beyond.forEach(function(w){ if(!kingNode || w.pct>kingNode.pct) kingNode=w; });
  if(!kingNode){ S.walls.forEach(function(w){ if(!kingNode || w.pct>kingNode.pct) kingNode=w; }); }
  var kingK = (tk!=null && (longSide ? tk>px : tk<px)) ? tk : (kingNode?kingNode.k:null);
  var gkNode=null;
  beyond.forEach(function(w){ if(Math.abs(w.k-px)<=GK_MAX_DIST && (kingK==null||Math.abs(w.k-kingK)>=0.001)){ if(!gkNode || w.pct>gkNode.pct) gkNode=w; } });
  var gkK = gkNode?gkNode.k:null;
  // Candidate pool: nodes on the trade side, at/above min strength, whose flow
  // is not Dissipating (hysteresis — Steady keeps the slot). The King is always
  // admitted regardless, so it can never be dropped from the ladder.
  var strongest=strongestAccumulator(sym);
  var strongK = longSide ? (strongest.above?strongest.above.k:null) : (strongest.below?strongest.below.k:null);
  var cands=[];
  beyond.forEach(function(w){
    if(w.k==null) return;
    var isKing = (kingK!=null && Math.abs(w.k-kingK)<0.001);
    if(!isKing){
      if((w.pct||0) < TS_MIN_PCT) return;
      var h=nodeHistory(sym, w.k);
      var flow = (h&&h.length>=2) ? histTrend(h, liveNodePct(sym,w.k)) : null;
      if(flow && flow.label==='Fading') return; // Dissipating loses eligibility
    }
    cands.push({ k:w.k, pct:w.pct, dist:Math.abs(w.k-px),
                 score:scoreTargetNode(sym, w, px, longSide, kingK, gkK),
                 isKing:isKing,
                 star:(strongK!=null && Math.abs(w.k-strongK)<0.001) });
  });
  // Ensure the King is present even if it was filtered above.
  if(kingK!=null && !cands.some(function(c){return c.isKing;})){
    cands.push({ k:kingK, pct:(kingNode?kingNode.pct:100), dist:Math.abs(kingK-px),
                 score:scoreTargetNode(sym, {k:kingK,pct:(kingNode?kingNode.pct:100)}, px, longSide, kingK, gkK),
                 isKing:true, star:(strongK!=null && Math.abs(kingK-strongK)<0.001) });
  }
  if(!cands.length) return { t1:null, t2:null, t1star:false, t2star:false, dir:dir };
  // Keep only meaningful candidates: the top few by score, but ORDER the final
  // ladder by the sequence price reaches them (nearest first).
  cands.sort(function(a,b){ return b.score-a.score; });
  var keep=cands.slice(0, Math.max(2, Math.min(4, cands.length)));
  // The King must survive the score cut.
  if(kingK!=null && !keep.some(function(c){return c.isKing;})){
    var kc=cands.filter(function(c){return c.isKing;})[0];
    if(kc) keep.push(kc);
  }
  keep.sort(function(a,b){ return a.dist-b.dist; }); // path order
  var t1c=keep[0]||null, t2c=keep[1]||null;
  return {
    t1: t1c?t1c.k:null, t2: t2c?t2c.k:null,
    t1star: t1c?!!t1c.star:false, t2star: t2c?!!t2c.star:false,
    t1King: t1c?!!t1c.isKing:false, t2King: t2c?!!t2c.isKing:false,
    kingK: kingK, gkK: gkK, dir: dir
  };
}

// Short role abbreviation for a target strike, so the T1/T2 badges name what
// the level ACTUALLY is (King / Gatekeeper / Floor / Ceiling) rather than
// assuming T1=Gk, T2=King. #1
function targetRoleAbbr(sym, k, isKing){
  if(isKing) return 'K';
  var px=STATE[sym]?STATE[sym].price:null;
  var meta=null;
  var walls=(STATE[sym]&&STATE[sym].walls)?STATE[sym].walls:[];
  var w=null; walls.forEach(function(x){ if(x.k!=null && Math.abs(x.k-k)<0.001) w=x; });
  if(w){
    var role=classifyNodeRole(sym, w, px, targetMetaRaw());
    if(role==='King') return 'K';
    if(role==='Gatekeeper') return 'Gk';
    if(role==='Floor') return 'Fl';
    if(role==='Ceiling') return 'Ce';
    if(role==='Cluster') return 'Cl';
  }
  return (px!=null && k<px) ? 'Fl' : 'Ce';
}
// Raw ladder without labels (avoids recursion when classifying roles).
function targetMetaRaw(){ return targetLadder(); }
function targetMeta(){
  var lad=targetLadder();
  var sym='SPY';
  var t1ab = lad.t1!=null ? targetRoleAbbr(sym, lad.t1, lad.t1King) : null;
  var t2ab = lad.t2!=null ? targetRoleAbbr(sym, lad.t2, lad.t2King) : null;
  return {
    dir: lad.dir,
    t1: lad.t1,
    t2: lad.t2,
    t1star: lad.t1star,
    t2star: lad.t2star,
    t1King: lad.t1King, t2King: lad.t2King,
    kingK: lad.kingK, gkK: lad.gkK,
    t1Label: lad.t1!=null ? ((lad.t1star?'\u2605':'')+fmtNum(lad.t1)+' '+t1ab) : null,
    t2Label: lad.t2!=null ? ((lad.t2star?'\u2605':'')+fmtNum(lad.t2)+' '+t2ab) : null
  };
}
function signalBias(sym){
  var S=STATE[sym]||{};
  var bestLong=null, bestShort=null;
  var rank={BO:1,FT:2,TST:3,CONF:4,GO:5};
  for(var key in (S.setups||{})){
    var s=S.setups[key];
    if(!s || s.voided || s.goFired || s.stage==='GO') continue;
    if(s.dir==='long'){
      if(!bestLong || (rank[s.stage]||0) > (rank[bestLong.stage]||0) || ((rank[s.stage]||0)===(rank[bestLong.stage]||0) && (s.updated||0)>(bestLong.updated||0))) bestLong=s;
    } else if(s.dir==='short'){
      if(!bestShort || (rank[s.stage]||0) > (rank[bestShort.stage]||0) || ((rank[s.stage]||0)===(rank[bestShort.stage]||0) && (s.updated||0)>(bestShort.updated||0))) bestShort=s;
    }
  }
  return { long:bestLong, short:bestShort };
}
// Least-squares slope over a series (per sample step). Used as a smoothed
// direction check so one whimsical print can't set the trend by itself.
function seqTrendSlope(seq){
  var n=seq.length; if(n<2) return 0;
  var sx=0, sy=0, sxx=0, sxy=0;
  for(var i=0;i<n;i++){ sx+=i; sy+=seq[i]; sxx+=i*i; sxy+=i*seq[i]; }
  var denom=(n*sxx - sx*sx); if(denom===0) return 0;
  return (n*sxy - sx*sy)/denom;
}

// Core accumulation detector.
//
// Business goal: flag nodes UNDER ACCUMULATION intraday — dealers actively
// adding positioning at a strike. Those are the reversal levels to trade on a
// pullback, and the walls to target / expect rejection at on an approach.
//
// Per Skylit's own methodology, real accumulation = growth in a node's
// ABSOLUTE exposure over time ("growth = intent"), NOT its %King (which can
// fall just because the King grew). So the BADGE is judged on absSeq (absolute
// value); the DISPLAYED number stays live %King so it matches the tape.
//
// Dip tolerance: price pulling back into a level is exactly when we want to
// trade it, and a node can wobble on that retest. We track the running peak of
// absolute value and let the node give back up to ACC_DROP_BUDGET of that peak
// while still counting as "building"; only ACC_CONFIRM_DOWN consecutive
// down-samples (or a drop beyond budget) flips it to Fading. This keeps the
// signal live through a normal pullback but still catches a real breakdown.
function accumTrend(absSeq){
  var n = absSeq ? absSeq.length : 0;
  if(n < 2) return { label:'Steady', rapid:false, peak:(n?absSeq[0]:0), fromPeak:0, growth:0 };
  var start = absSeq[0];
  var last = absSeq[n-1];
  // Running peak and where we sit relative to it.
  var peak = start, peakIdx = 0;
  for(var i=0;i<n;i++){ if(absSeq[i] > peak){ peak = absSeq[i]; peakIdx = i; } }
  var fromPeak = (peak>0) ? (peak - last)/peak : 0;      // fraction below peak now
  var growth   = (start>0) ? (peak - start)/start : 0;    // peak growth vs window start
  // Count consecutive down-samples at the tail (sample-over-sample declines).
  var downRun = 0;
  for(var j=n-1;j>0;j--){ if(absSeq[j] < absSeq[j-1]) downRun++; else break; }
  var slope = seqTrendSlope(absSeq);
  // Rapid Accumulation: sharp abs growth over the last two samples (magnet).
  var recent = (n>=2 && absSeq[n-2]>0) ? (absSeq[n-1]-absSeq[n-2])/absSeq[n-2] : 0;
  var rapid = recent >= ACC_RAPID_ROC;

  // A decline only counts as "Fading" if it is MEANINGFUL, not micro-noise.
  // Require the drawdown from peak to clear half the drop budget before a
  // confirmed down-run is allowed to flip the badge; and require the window to
  // have shed a real fraction vs its start. This keeps flat/noisy series Steady.
  var meaningfulDD = fromPeak >= (ACC_DROP_BUDGET*0.5);
  var label = 'Steady';
  var builtUp = (growth >= ACC_BUILD_MIN) && (slope > 0 || peakIdx >= n-2);
  if(builtUp && fromPeak <= ACC_DROP_BUDGET && downRun < ACC_CONFIRM_DOWN){
    // Grew meaningfully and is holding within the pullback budget -> still accumulating.
    label = 'Building';
  } else if(fromPeak > ACC_DROP_BUDGET || (downRun >= ACC_CONFIRM_DOWN && slope < 0 && meaningfulDD)){
    // Given back more than the budget, or a CONFIRMED and MEANINGFUL decline -> fading.
    label = 'Fading';
  } else {
    label = 'Steady';
  }
  return { label:label, rapid:rapid, peak:peak, fromPeak:fromPeak, growth:growth, slope:slope, downRun:downRun };
}

function accumulationStateFor(sym, k){
  var r = rawAccumNode(sym, k);
  // Headline strength shown to the user is ALWAYS the live tape %King so the
  // number the user sees matches the tape exactly.
  var livePct = livePctAt(sym, k);
  if(!r || !r.absSeq || r.absSeq.length<2){
    var solo = (typeof livePct==='number') ? livePct : null;
    return { label:'Steady', rapid:false, delta:0, seq:(solo!=null?[solo]:[]),
             available:(solo!=null), pct:solo, pos:(r?r.pos:null), absSeq:(r&&r.absSeq)?r.absSeq:[] };
  }
  // Judge accumulation on ABSOLUTE value (Skylit methodology).
  // (v10.23 Issue H) DE-FLICKER: the LAST absSeq point is the LIVE, still-forming
  // 3m bar and jitters tick-to-tick; judging the whole seq flips the badge intrabar.
  // Fix: commit the label on the CLOSED portion (exclude the live last point). Only
  // a genuinely RAPID move (ACC_RAPID_ROC over the last 2 samples of the FULL seq)
  // is allowed to update the label intrabar, so real fast-accumulation still shows
  // but ordinary tick-jitter cannot flip it.
  var full = r.absSeq || [];
  var closedSeq = full.length>=3 ? full.slice(0, full.length-1) : full;   // drop live last (keep >=2)
  var tClosed = accumTrend(closedSeq);
  var tFull   = accumTrend(full);
  var label = tClosed.label;
  var rapid = tFull.rapid;
  // rapid-override: only when the live tick shows a real fast move do we adopt the
  // full-seq label (which reflects the still-forming bar).
  if(rapid && tFull.label!==label){ label = tFull.label; }
  var t = { label:label, rapid:rapid, slope:tClosed.slope, growth:tClosed.growth, fromPeak:tClosed.fromPeak };
  // Path shown to the user is the %King trajectory (how strength read over
  // time), ending at the live value so the last dot matches the headline.
  var showSeq = (r.seq||[]).slice();
  if(typeof livePct==='number' && showSeq.length) showSeq[showSeq.length-1] = livePct;
  var delta = showSeq.length>1 ? (showSeq[showSeq.length-1]-showSeq[0]) : 0;
  return { label:t.label, rapid:t.rapid, delta:delta, slope:t.slope, seq:showSeq,
           growth:t.growth, fromPeak:t.fromPeak,
           available:true, pct:(typeof livePct==='number'?livePct:showSeq[showSeq.length-1]),
           pos:r.pos, absSeq:r.absSeq||[] };
}
function classifyNodeRole(sym, wall, px, meta){
  if(!wall) return 'Node';
  // Tape is authoritative for the King: the strike Skylit marks with the $K
  // cell IS the King, and nothing else is. Falls back to the feed-derived King
  // (meta.t2) only when the tape can't be read.
  var tk=tapeKingStrike(sym);
  if(tk!=null){
    if(Math.abs(wall.k-tk)<0.001) return 'King';
  } else if(meta && meta.t2!=null && Math.abs(wall.k-meta.t2)<0.001){
    return 'King';
  }
  if(meta && meta.t1!=null && Math.abs(wall.k-meta.t1)<0.001) return 'Gatekeeper';
  var walls=(STATE[sym]&&STATE[sym].walls)||[];
  var above = px!=null ? (wall.k>px) : false;
  // A real "Cluster" = several nodes genuinely bunched together, NOT just one
  // adjacent strike. With whole-dollar SPY strikes almost every node has a
  // neighbor 1.0 away, so the old "1 neighbor within 1.0" test tagged nearly
  // everything Cluster. Require EITHER 2+ same-side neighbors within 1 strike
  // (three stacked levels) OR a neighbor closer than the normal spacing (<=0.5,
  // i.e. an unusually tight pair). Otherwise it's a plain Floor/Ceiling.
  // Only a genuinely TIGHT sub-strike pair (<=0.5 apart, e.g. a whole strike
  // plus a half-strike node) counts as a Cluster. Contiguous whole-dollar
  // strikes are the norm on SPY and are NOT a cluster — they are plain
  // Floors (support below) / Ceilings (resistance above).
  var tightNeighbor=false;
  for(var i=0;i<walls.length;i++){
    var w=walls[i];
    if(Math.abs(w.k-wall.k)<0.001) continue;
    if(above !== (w.k>px)) continue;
    if(Math.abs(w.k-wall.k)<=0.5) tightNeighbor=true;
  }
  if(tightNeighbor) return 'Cluster';
  if(px!=null){
    if(wall.k<px) return 'Floor';
    if(wall.k>px) return 'Ceiling';
  }
  return 'Node';
}
function futureStructureSummary(sym){
  var S=STATE[sym]||{};
  var px=S.price;
  var walls=S.walls||[];
  var meta=targetMeta();
  var bias=signalBias(sym);
  var trend=trendStateInfo();
  var dir = meta.dir;
  if(bias.long && !bias.short) dir='long';
  else if(bias.short && !bias.long) dir='short';
  var above=[], below=[];
  for(var i=0;i<walls.length;i++){
    var w=walls[i];
    var st=accumulationStateFor(sym, w.k);
    // Headline number = Skylit's rendered tape %King when available, else the
    // script's own derived wall pct. This is what makes ACM match the tape.
    var tapePct=livePctAt(sym, w.k);
    // 1-minute %King history read from the tape (the visible strip). The badge
    // is judged on THIS series so the tag and the strip always agree. Fall
    // back to the older window-trend badge until enough minute-samples exist.
    var hist=nodeHistory(sym, w.k);
    // #2: judge on the 3m-close series but pass the LIVE %King so the rapid flag
    // can fire mid-bar between closes.
    var htrend = (hist && hist.length>=2) ? histTrend(hist, liveNodePct(sym, w.k)) : null;
    var pk=nodePeak(sym, w.k); // #2 session peak/low context
    var absorb=absorptionAt(sym, w.k); // #6 absorption (rejection) flag
    var role=classifyNodeRole(sym, w, px, meta);
    // Rolling: tag the King node with the King's roll direction over the window
    // (King up = bullish, King down = bearish).
    var roll = (role==='King') ? kingRoll(sym) : 0;
    var row={
      k:w.k,
      pct:(typeof tapePct==='number'?tapePct:w.pct),
      pos:w.pos,
      role:role,
      side:(px!=null && w.k>px)?'above':'below',
      state:st,
      hist:hist||[],
      htrend:htrend,
      roll:roll,
      peak:pk,
      absorb:absorb,
      abs:w.abs
    };
    if(htrend){
      // Override the badge with the history-based judgement (tag == strip).
      row.state.label = htrend.label;
      row.state.rapid = htrend.rapid;
      row.state.rapidDir = htrend.rapidDir;      // +1 accum, -1 unwind, 0 none
      row.state.recentMove = htrend.recentMove;
      row.state.arrow = htrend.arrow;
      row.state.net   = htrend.net;
    } else {
      row.state.rapidDir = 0;
      row.state.arrow = (row.state.label==='Building'?'\u2197':(row.state.label==='Fading'?'\u2198':'\u2192'));
    }
    row.seq = row.state.seq || [];
    row.delta = row.state.delta;
    if(row.side==='above') above.push(row); else below.push(row);
  }
  above.sort(function(a,b){ return a.k-b.k; });
  below.sort(function(a,b){ return b.k-a.k; });

  var current = resolveAnchor(sym);
  var currentK = (current && current.ok && current.active) ? current.active.k : null;
  var bestLowerSupport=null;
  if(currentK!=null){
    for(var j=0;j<below.length;j++){
      var b=below[j];
      if(b.k < currentK-0.001 && b.state.label==='Building'){
        if(!bestLowerSupport || b.delta>bestLowerSupport.delta || (b.delta===bestLowerSupport.delta && b.pct>bestLowerSupport.pct)) bestLowerSupport=b;
      }
    }
  }

  var nearestAbove = above.length ? above[0] : null;
  var nearestBelow = below.length ? below[0] : null;
  // IN-PLAY = the node price is actually interacting with, i.e. the CLOSEST
  // node to price (regardless of side). Previously this picked the first
  // "Building" node in the trend direction, which wrongly flagged a node price
  // wasn't near (e.g. calling 774 in play while price sits on the 773 King).
  var inPlay = null;
  if(px!=null){
    var allRows = above.concat(below);
    var bestD = null;
    for(var q=0;q<allRows.length;q++){
      var d = Math.abs(allRows[q].k - px);
      if(bestD===null || d < bestD){ bestD = d; inPlay = allRows[q]; }
    }
    // Prefer the resolved anchor node when it exists AND is essentially the
    // node price is on (within ~1 strike), so an active setup's node wins ties.
    if(currentK!=null){
      for(var a2=0;a2<allRows.length;a2++){
        if(Math.abs(allRows[a2].k-currentK)<0.001 && Math.abs(currentK-px)<=1.0){ inPlay=allRows[a2]; break; }
      }
    }
  } else {
    inPlay = nearestBelow || nearestAbove;
  }

  return {
    dir:dir,
    trend:trend.label,
    meta:meta,
    current:current,
    above:above,
    below:below,
    nearestAbove:nearestAbove,
    nearestBelow:nearestBelow,
    bestLowerSupport:bestLowerSupport,
    inPlay:inPlay,
    signal:bias
  };
}

// Legacy read helper retained only as dormant reference; no longer rendered.
function readLine(){
  var S=STATE.SPY;
  var px=S.price;
  var v=trendVerdict('SPY');
  var a=atr('SPY');
  var walls=S.walls||[];
  var acc=accumData('SPY');
  var lad=targetLadder();
  var nearD=mul(a, READ_NEARX);
  var apprD=mul(a, READ_APPRX);
  var msg='', watch='';
  function nearestNode(){
    var best=null;
    walls.forEach(function(w){ var d=Math.abs(w.k-px); if(best==null||d<best.d) best={w:w,d:d}; });
    return best;
  }
  if(px==null || !walls.length){
    msg='Waiting on feed — no live map yet.';
  } else if(RESHUFFLE.SPY){
    msg='Levels just reshuffled — map uncertain, stand by for the new structure to settle.';
  } else {
    var nn=nearestNode();
    var atKing = (lad.t2!=null && Math.abs(lad.t2-px)<=nearD);
    if(atKing){
      msg='At King '+fmtNum(lad.t2)+' — decision point: accept and continue, or reject and fade.';
    } else if(nn && nn.d<=nearD){
      msg='Reacting at '+fmtNum(nn.w.k)+' ('+nn.w.pct+'% King) — watching how price handles it.';
    } else if(nn && nn.d<=apprD){
      msg='Approaching '+fmtNum(nn.w.k)+' ('+nn.w.pct+'% King) from '+(px<nn.w.k?'below':'above')+'.';
    } else if(v.state==='up' && lad.t2!=null){
      msg='In transit — grinding up toward King '+fmtNum(lad.t2)+'.';
    } else if(v.state==='dn' && lad.t2!=null){
      msg='In transit — sliding down toward King '+fmtNum(lad.t2)+'.';
    } else if(v.state==='flat'){
      var below=null, above=null;
      walls.forEach(function(w){ if(w.k<px){ if(!below||w.k>below.k) below=w; } if(w.k>px){ if(!above||w.k<above.k) above=w; } });
      if(below&&above){ msg='Rangebound '+fmtNum(below.k)+'–'+fmtNum(above.k)+' — no trend, fading edges.'; }
      else { msg='Holding '+(v.ma!=null&&px>v.ma?'above':'below')+' the '+(v.slope>=0?'rising':'falling')+' 50MA.'; }
    } else {
      msg='Holding '+(v.ma!=null&&px>v.ma?'above':'below')+' the '+(v.slope>=0?'rising':'falling')+' 50MA.';
    }
  }
  var watchBits=[];
  if(px!=null && walls.length){
    var up2=null, dn2=null;
    walls.forEach(function(w){ if(w.k>px){ if(!up2||w.k<up2.k) up2=w; } if(w.k<px){ if(!dn2||w.k>dn2.k) dn2=w; } });
    if(up2) watchBits.push(fmtNum(up2.k)+' above');
    if(dn2) watchBits.push(fmtNum(dn2.k)+' below');
  }
  if(acc.length){ watchBits.push(fmtNum(acc[0].k)+'★ accumulating'); }
  if(watchBits.length) watch=watchBits.join(' · ');
  var tip='Legacy READ helper retained for fallback reference.';
  return '<div title="'+tip.replace(/"/g,'')+'" style="padding:5px 9px;background:'+PAL.card+';border:1px solid '+PAL.line+';border-radius:8px;margin:2px 0">'+
    '<div style="color:'+PAL.ink+';font-size:11px;line-height:1.35">'+msg+'</div>'+
    (watch?'<div style="color:'+PAL.sub+';font-size:10px;margin-top:2px">Watching: '+watch+'</div>':'')+
    '</div>';
}

function structuralBox(title, body, sub, borderColor){
  borderColor = borderColor || PAL.line;
  return '<div style="padding:5px 9px;background:'+PAL.card+';border:1px solid '+borderColor+';border-radius:8px;margin:2px 0">'+
    '<div style="color:'+PAL.blue+';font-weight:800;font-size:10px;letter-spacing:0.5px;margin-bottom:2px">'+title+'</div>'+
    '<div style="color:'+PAL.ink+';font-size:11px;line-height:1.35">'+body+'</div>'+
    (sub?'<div style="color:'+PAL.sub+';font-size:10px;margin-top:2px">'+sub+'</div>':'')+
    '</div>';
}
function structuralWarn(title, body){
  return '<div style="padding:5px 9px;background:'+PAL.card+';border:1px solid '+PAL.amber+';border-radius:8px;margin:2px 0">'+
    '<div style="color:'+PAL.amber+';font-weight:800;font-size:10px;letter-spacing:0.5px;margin-bottom:2px">'+title+'</div>'+
    '<div style="color:'+PAL.ink+';font-size:11px;line-height:1.35">'+body+'</div>'+
    '</div>';
}
function structuralReadHtml(){
  var fs = futureStructureSummary('SPY');
  var info = trendStateInfo();
  var meta = fs.meta || targetMeta();
  var anc = fs.current;
  // Trend + T1 + T2 badges inline ON the Read header row (not a separate box).
  // T1/T2 colored by GEOMETRY: green if the target is above price, red if below.
  var pxT = STATE.SPY ? STATE.SPY.price : null;
  // #2 (v10.6) TARGET BADGE COLOR \u2014 SAME rule for BOTH T1 and T2:
  //   \u2022 YELLOW if the target is a STRETCH (too far to reach at the recent pace);
  //   \u2022 otherwise GREEN if it is ABOVE price (a long/bullish target),
  //   \u2022 otherwise RED if it is BELOW price (a short/bearish target).
  // (Previously T2 used a reach scale that painted an in-reach downside target
  //  green \u2014 e.g. 769 below price showed green. Fixed: direction wins, stretch
  //  overrides to yellow.) Full distance sentence stays in each badge's tooltip.
  var proxS = adaptiveProxStrikes('SPY');
  function reachWord(k){
    if(pxT==null||k==null) return '\u2013';
    var aStr=Math.abs(k-pxT);
    if(aStr<=proxS)     return 'in reach';
    if(aStr<=proxS*1.8) return 'stretch';
    return 'far';
  }
  function tgtCol(k){
    if(pxT==null||k==null) return PAL.sub;
    var w=reachWord(k);
    if(w==='stretch' || w==='far') return PAL.amber;      // stretch \u2192 yellow, direction-agnostic
    return (k>pxT) ? PAL.longAccent : PAL.shortAccent;     // long target green, short target red
  }
  function tgtTip(k, tag){
    if(pxT==null||k==null) return '';
    var pts=(k-pxT), aStr=Math.abs(k-pxT), w=reachWord(k);
    var dir=(pts>0?'above (long/bullish target)':'below (short/bearish target)');
    var base=tag+' '+fmtNum(k)+' is '+(pts>0?'+':'')+pts.toFixed(1)+' pts ('+aStr.toFixed(0)+' strike'+(aStr>=2?'s':'')+') '+dir+'. At the recent 3m pace (~'+proxS+' strikes/pullback) it is '+w+'.';
    return (base+' Color: green = long target (above), red = short target (below), yellow = stretch (too far for now).').replace(/"/g,'');
  }
  var c1=tgtCol(meta.t1), c2=tgtCol(meta.t2);
  var hdrRight = trendBadgeHtml()+
      (meta.t1Label?'<span title="'+tgtTip(meta.t1,'T1')+'" style="color:'+c1+';font-weight:700;font-size:10px;padding:1px 7px;border:1px solid '+c1+';border-radius:20px">'+meta.t1Label+'</span>':'')+
      (meta.t2Label?'<span title="'+tgtTip(meta.t2,'T2')+'" style="color:'+c2+';font-weight:700;font-size:10px;padding:1px 7px;border:1px solid '+c2+';border-radius:20px">'+meta.t2Label+'</span>':'');
  var html = sectionHdrRight('Trend', hdrRight, 'TREND: the coordinated one-line take on structure right now \u2014 what price is doing relative to the nearest node, the trend (Up/Dn/Side/NA), and the two targets T1/T2. Target color: green = long target (above price), red = short target (below), yellow = stretch (too far to reach at the recent pace). Hover a target for its exact distance.');

  if(!anc || !anc.ok || !anc.active || anc.active.k==null){
    return html + '<div style="padding:6px 9px;background:'+PAL.card+';border:1px solid '+PAL.line+';border-radius:8px;margin:2px 0;color:'+PAL.sub+';font-size:11px;line-height:1.4">Flat market.</div>';
  }

  // #3 (v10.6) Abbreviate Support/Resistance to Sup/Res in the body, and lead
  // with a short TREND clause before the S/R text.
  var roleAb = (anc.price!=null && anc.price>=anc.active.k) ? 'Sup' : 'Res';
  var currentK = fmtNum(anc.active.k);
  var opp = anc.opposing;
  var sentence='';

  if(RESHUFFLE.SPY===true){
    if(opp && opp.k!=null) sentence = fmtNum(opp.k)+' remains overhead while '+currentK+' is still holding below. Direction is tbd.';
    else sentence = roleAb+' at '+currentK+' is active. Direction is tbd.';
  } else if(fs.dir==='long'){
    if(opp && opp.k!=null){
      var oppState = accumulationStateFor('SPY', opp.k);
      sentence = roleAb+' at '+currentK+' is holding while '+fmtNum(opp.k)+' remains overhead Res. ';
      if(oppState.label==='Building') sentence += 'Price is likely moving higher, but '+fmtNum(opp.k)+' is building as Res. ';
      else sentence += 'Price is likely moving higher. ';
      if(meta.t1!=null && meta.t2!=null && Math.abs(meta.t1-opp.k)<0.001) sentence += 'If '+fmtNum(opp.k)+' clears, the next meaningful target is '+fmtNum(meta.t2)+' King.';
      else if(meta.t1!=null && meta.t2!=null) sentence += 'The next meaningful targets are '+fmtNum(meta.t1)+' then '+fmtNum(meta.t2)+' King.';
      else if(meta.t2!=null) sentence += 'The next meaningful target is '+fmtNum(meta.t2)+' King.';
      if(fs.bestLowerSupport) sentence += ' '+fmtNum(fs.bestLowerSupport.k)+' is building as stronger Sup below.';
    } else {
      sentence = roleAb+' at '+currentK+' is holding. Price is likely moving higher.';
    }
  } else if(fs.dir==='short'){
    var lower = fs.nearestBelow;
    if(lower){
      sentence = roleAb+' at '+currentK+' is holding while '+fmtNum(lower.k)+' remains Sup below. Price is likely moving lower. ';
      if(meta.t1!=null && meta.t2!=null && Math.abs(meta.t1-lower.k)<0.001) sentence += 'If '+fmtNum(lower.k)+' breaks, the next meaningful target is '+fmtNum(meta.t2)+' King.';
      else if(meta.t1!=null && meta.t2!=null) sentence += 'The next meaningful targets are '+fmtNum(meta.t1)+' then '+fmtNum(meta.t2)+' King below.';
      else if(meta.t2!=null) sentence += 'The next meaningful target is '+fmtNum(meta.t2)+' King.';
    } else {
      sentence = roleAb+' at '+currentK+' is active. Price is likely moving lower.';
    }
  } else {
    if(opp && opp.k!=null) sentence = roleAb+' at '+currentK+' is holding while '+fmtNum(opp.k)+' remains overhead gatekeeper. Price is still flat.';
    else sentence = roleAb+' at '+currentK+' is active. Price is still flat.';
  }

  // Lead the body with a compact trend clause so the reader sees the trend first,
  // then the S/R structure. Colored to match the Trend badge.
  var ti = trendStateInfo();
  var tv = ti.verdict||{};
  var trendWord = ti.label==='Up'?'Uptrend':(ti.label==='Dn'?'Downtrend':(ti.label==='Side'?'Sideways':'Trend N/A'));
  // Show the bar counter ONLY for a real directional trend (Up/Dn). On Sideways
  // or N/A there is no bias, so the x/y count is noise \u2014 omit it.
  var directional = (ti.label==='Up' || ti.label==='Dn');
  var trendCounter = (directional && tv.win) ? (' ('+tv.up+'/'+tv.win+' bars)') : '';
  var trendClause = '<b style="color:'+ti.color+'">'+trendWord+trendCounter+'.</b> ';

  return html + '<div title="Read = coordinated output from trend, current node, future structure, accumulation, and signal context. Sup = support, Res = resistance." style="padding:6px 9px;background:'+PAL.card+';border:1px solid '+PAL.line+';border-radius:8px;margin:2px 0;color:'+PAL.ink+';font-size:11px;line-height:1.45">'+trendClause+sentence+'</div>';
}

function accTrajHtml(seq){ return seq.map(function(p){ return p+'%'; }).join(' '); }

// Build a %King-history-with-timestamps string for tooltips, e.g.
// "9:41 40 -> 9:42 44 -> 9:43 55(+11) -> ...". Marks sharp steps.
function histDetail(sym, k){
  var store=HIST[sym]; if(!store) return '';
  var rec=store[k.toFixed(2)];
  if(!rec || !rec.seq.length) return '';
  var out=[];
  for(var i=0;i<rec.seq.length;i++){
    var p=rec.seq[i];
    var t=fmtClock(p.t);
    var step=(i>0)?(p.v-rec.seq[i-1].v):0;
    var mark=(step>=HIST_SHARP_STEP)?(' (+'+step+' sharp)'):(step<=-HIST_SHARP_STEP?(' ('+step+' sharp)'):'');
    out.push(t+' '+p.v+'%'+mark);
  }
  return out.join('  ->  ');
}

// Scan a WIDE band around price (not just the 3 shown per side) and find the
// single strongest current accumulator on each side, so a wall forming a few
// strikes out gets surfaced even when it's off the visible ladder.
var STRONGEST_BAND = 6; // strikes each side to scan
function strongestAccumulator(sym){
  var px = STATE[sym] ? STATE[sym].price : null;
  var tp = tapeMap(sym);
  if(px==null || !tp || !tp.pct) return { above:null, below:null };
  var bestAbove=null, bestBelow=null;
  for(var key in tp.pct){
    var k=parseFloat(key);
    if(!isFinite(k)) continue;
    var dist=Math.abs(k-px);
    if(dist>STRONGEST_BAND || dist<0.001) continue;
    var hist=nodeHistory(sym, k);
    if(!hist || hist.length<3) continue;               // need real history
    var t=histTrend(hist, liveNodePct(sym,k));
    if(t.label!=='Building') continue;                  // only genuine builders
    var cand={ k:k, pct:tp.pct[key], net:t.net, rapid:t.rapid, rapidDir:t.rapidDir,
               dist:dist, side:(k>px?'above':'below'), hist:hist };
    if(k>px){ if(!bestAbove || cand.net>bestAbove.net) bestAbove=cand; }
    else    { if(!bestBelow || cand.net>bestBelow.net) bestBelow=cand; }
  }
  return { above:bestAbove, below:bestBelow };
}

// ============================================================================
// GEX-STRUCTURE REGIME (v10.24) — Trend / Whipsaw / Rainbow Road, reverse-engineered
// from the Patternpedia + annotated examples. This is a WHOLE-BOARD regime read on
// the CURRENT node structure (where dealer exposure skews), DISTINCT from and
// COMPLEMENTARY to the SMA price-trend badge (they can disagree — that IS a signal).
// It COLORS how the Node Map + posture are read (regime-as-instruction, Issue L).
// Rules (tunable consts; the effectiveness tracker refines them later):
//   TREND  = prominent |value| mass concentrated ONE side of price, skew >= REGIME_TREND_SKEW.
//   WHIPSAW= ~2 dominant edges + LOW-VALUE middle gap (bimodal, edge/mid >= REGIME_WHIP_EDGEMID).
//   RAINBOW= residual: >= REGIME_RAINBOW_MIN prominent nodes, BOTH signs interleaved,
//            full middle, high dispersion => STAND ASIDE.
var REGIME_TREND_SKEW    = 1.8;   // top-3 one-side / top-3 other-side to call Trend
var REGIME_WHIP_EDGEMID  = 2.0;   // (mean edge value) / (max middle value) to call Whipsaw
var REGIME_RAINBOW_MIN   = 4;     // >= this many prominent nodes for Rainbow
var REGIME_SIG_PCT       = 20;    // |%King| >= this = a 'prominent' node (matches MIN_STRENGTH)
function sum3(arr){ var s=0,n=Math.min(3,arr.length); for(var i=0;i<n;i++) s+=arr[i]; return s; }
function gexRegime(sym){
  var S=STATE[sym]||{};
  var px=S.price, walls=S.walls||[];
  var out={ label:'Forming', dir:0, conf:'low', skew:null, edgeMid:null,
            prominent:0, why:'Not enough node structure yet.', target:null };
  if(px==null || !walls.length) return out;
  // prominent nodes by |%King|
  var prom=[];
  walls.forEach(function(w){
    var v=(typeof w.pct==='number')?Math.abs(w.pct):0;
    if(v>=REGIME_SIG_PCT && w.k!=null) prom.push({k:w.k, v:v, side:(w.k>px?'above':'below'), pos:w.pos, net:w.net});
  });
  out.prominent=prom.length;
  if(prom.length<2){ out.why='Only '+prom.length+' prominent node(s) — forming.'; return out; }
  // --- skew (Trend test): top-3 |value| each side of price ---
  var above=prom.filter(function(x){return x.side==='above';}).map(function(x){return x.v;}).sort(function(a,b){return b-a;});
  var below=prom.filter(function(x){return x.side==='below';}).map(function(x){return x.v;}).sort(function(a,b){return b-a;});
  var upMass=sum3(above), dnMass=sum3(below);
  var heavy=Math.max(upMass,dnMass), light=Math.min(upMass,dnMass)||0.0001;
  var skew=heavy/light; out.skew=+skew.toFixed(2);
  // --- middle-gap (Whipsaw test): strongest node each side = the 'edges'; the
  // strongest node BETWEEN them = the 'middle'. Bimodal when edges >> middle. ---
  var sortedByK=prom.slice().sort(function(a,b){return a.k-b.k;});
  var loEdge=sortedByK[0], hiEdge=sortedByK[sortedByK.length-1];
  var midMax=0;
  sortedByK.forEach(function(x){ if(x.k>loEdge.k+0.001 && x.k<hiEdge.k-0.001) midMax=Math.max(midMax,x.v); });
  var edgeMean=(loEdge.v+hiEdge.v)/2;
  var edgeMid=(midMax>0)?(edgeMean/midMax):Infinity; out.edgeMid=isFinite(edgeMid)?+edgeMid.toFixed(2):null;
  var edgesBothSides = (loEdge.k<px && hiEdge.k>px);
  // --- sign mix (Rainbow test): both polarities interleaved, full middle ---
  var posN=prom.filter(function(x){return x.pos===true;}).length;
  var negN=prom.filter(function(x){return x.pos===false;}).length;
  var bothSigns=(posN>=1 && negN>=1);
  var spread=hiEdge.k-loEdge.k;
  // ---- CLASSIFY (order: Trend -> Whipsaw -> Rainbow -> Mixed) ----
  if(skew>=REGIME_TREND_SKEW){
    var up=(upMass>=dnMass);
    out.label = up?'Trend \u2191':'Trend \u2193'; out.dir = up?1:-1; out.conf = skew>=2.4?'high':'med';
    // target = the heaviest node on the skew side (the King the trend fixates on)
    var side=up?'above':'below';
    var t=null; prom.forEach(function(x){ if(x.side===side && (!t||x.v>t.v)) t=x; });
    out.target = t?t.k:null;
    out.why='Node mass skewed '+(up?'UP':'DOWN')+' '+out.skew+'\u00d7 (top-3 '+Math.round(heavy)+' vs '+Math.round(light)+'). GEX-structure trend'+(out.target!=null?(' toward '+fmtNum(out.target)):'')+' \u2014 enter on pullbacks.';
  } else if(edgesBothSides && edgeMid>=REGIME_WHIP_EDGEMID){
    out.label='Whipsaw'; out.dir=0; out.conf=edgeMid>=3?'high':'med';
    out.why='Two dominant edges '+fmtNum(loEdge.k)+'/'+fmtNum(hiEdge.k)+' with a hollow middle (edge/mid '+out.edgeMid+'\u00d7). Defined range \u2014 fade the edges, avoid the middle.';
  } else if(prom.length>=REGIME_RAINBOW_MIN && bothSigns && midMax>0 && spread>=6){
    out.label='Rainbow Road'; out.dir=0; out.conf='low';
    out.why=prom.length+' prominent nodes, both polarities interleaved across ~'+spread+' strikes with a full middle. Vague positioning \u2014 stand aside.';
  } else {
    out.label='Mixed'; out.dir=0; out.conf='low';
    out.why='No dominant structure (skew '+out.skew+'\u00d7, '+prom.length+' prominent). Low conviction.';
  }
  return out;
}

// ============================================================================
// GATEKEEPER (v10.24) — the nearest high-|value| node lying BETWEEN price and the
// larger node(s)/King beyond it, that price must pass through first. Reverse-
// engineered from Patternpedia + 3 examples: ABSOLUTE value ranks (sign only
// flavors), and SIGNIFICANCE = strength ratio |gatekeeper| / |next MAJOR node
// beyond it toward the target|. Ratio >> 1 => expect STALL/REVERSAL at the
// gatekeeper (feeds the King 'decoy discount': a King gapped behind a strong
// gatekeeper is less reachable). Early-session rejections weight higher.
var GK_RATIO_STRONG = 1.8;   // |gk| / |next-beyond| at/above this => dominant gatekeeper (reversal)
function gatekeeper(sym){
  var S=STATE[sym]||{};
  var px=S.price, walls=S.walls||[];
  var out={ ok:false, k:null, side:null, ratio:null, verdict:null, kingK:null, decoyDiscount:0 };
  if(px==null || !walls.length) return out;
  var tp=tapeMap(sym);
  var kingK=(tp && typeof tp.king==='number')?tp.king:(S.king!=null?S.king:null);
  out.kingK=kingK;
  if(kingK==null || Math.abs(kingK-px)<0.001) return out;   // no target to gatekeep toward
  var toward = (kingK>px)?'above':'below';
  // candidate intervening nodes: strictly between price and King, on the path.
  var between=[];
  walls.forEach(function(w){
    if(w.k==null) return;
    var av=(typeof w.pct==='number')?Math.abs(w.pct):0;
    if(av<REGIME_SIG_PCT) return;                         // must be a high-|value| node
    var onPath = (toward==='above') ? (w.k>px+0.001 && w.k<kingK-0.001)
                                    : (w.k<px-0.001 && w.k>kingK+0.001);
    if(onPath) between.push({k:w.k, v:av, pos:w.pos, dist:Math.abs(w.k-px)});
  });
  if(!between.length) return out;                          // clear path to the King, no gatekeeper
  // (v10.27) GATEKEEPER = the DOMINANT blocker on the path \u2014 the doc's "second-
  // highest node between price and the King" / "compare vs the 2nd highest-value
  // node" / "far in excess of the nodes beyond it". So rank by MAGNITUDE (|%King|),
  // NOT by nearness. Tiebreak on EXACT-equal magnitude only (near-impossible on a
  // live tape) -> the node nearer PRICE, per the doctrine's price-anchored validity
  // (\u2264~5 pts FROM PRICE red flag; "as price approaches that node").
  between.sort(function(a,b){ return (b.v-a.v) || (a.dist-b.dist); });
  var gk=between[0];
  // strength ratio vs the next MAJOR node BEYOND it toward the target (docs:
  // 'compare vs the 2nd-highest node', NOT rigidly vs the King). The King is the
  // TARGET, not the comparator: exclude the King node itself so a gatekeeper that
  // dominates the lesser nodes on the road reads as dominant even when the King
  // beyond it is large. Fall back to the King only if it's the sole node beyond.
  var beyondMax=0, beyondMaxInclKing=0;
  walls.forEach(function(w){
    if(w.k==null) return;
    var av=(typeof w.pct==='number')?Math.abs(w.pct):0;
    var isBeyond = (toward==='above') ? (w.k>gk.k+0.001) : (w.k<gk.k-0.001);
    if(!isBeyond) return;
    var isKingNode = (kingK!=null && Math.abs(w.k-kingK)<0.001);
    beyondMaxInclKing=Math.max(beyondMaxInclKing, av);
    if(!isKingNode) beyondMax=Math.max(beyondMax, av);
  });
  var cmp = (beyondMax>0) ? beyondMax : beyondMaxInclKing;   // prefer non-King comparator
  var ratio = (cmp>0) ? (gk.v/cmp) : Infinity;
  out.ok=true; out.k=gk.k; out.side=toward; out.ratio=isFinite(ratio)?+ratio.toFixed(2):null; out.gkVal=gk.v;
  // early-session weight: rejections early in the day are higher-probability reversals.
  var earlyBoost = isEarlySession() ? 0.3 : 0;
  var strong = (ratio>=GK_RATIO_STRONG) || (ratio>=GK_RATIO_STRONG-0.3 && earlyBoost>0);
  out.verdict = strong ? 'Reversal likely at '+fmtNum(gk.k) : 'Watch '+fmtNum(gk.k)+' (may pass)';
  out.strong = strong;
  // King decoy discount: a King sitting BEHIND a strong gatekeeper is less reachable.
  out.decoyDiscount = strong ? Math.min(1, (ratio||1)/3) : 0;
  return out;
}
function isEarlySession(){
  try{ var s=ctNowSecOfDay(); var openS=mul(8,3600)+mul(30,60); return (s-openS) <= 3600; }  // first hour
  catch(e){ return false; }
}

// ============================================================================
// RUG / REVERSE-RUG (v10.24) — the PROJECT NAMESAKE, the ONE polarity-gated detector.
// RUG (bearish nosedive): a POSITIVE-gamma (yellow, pos:true) node near/above price
// stacked DIRECTLY OVER a strong NEGATIVE-gamma (purple, pos:false) node, with a
// stack of predominantly negative-gamma floors below and NO significant positive
// floor beneath ('no obvious floor'). Mechanism: the yellow ceiling unwinds -> the
// neg-gamma node below ACCELERATES the drop ('gasoline on fire') -> violent nosedive.
// REVERSE-RUG (bullish squeeze) = the mirror: purple ceiling over a yellow floor,
// no significant negative ceiling above.
// Uses per-node polarity `pos` (feed d>0). VERIFY-ONCE caveat: sign(pos) is INFERRED
// to be gamma polarity; RUG_POLARITY_VERIFIED gates the flag until confirmed live
// against the ladder yellow/purple. The detector still computes + exposes its polarity
// evidence so the verification is a glance.
var RUG_POLARITY_VERIFIED = true;    // VERIFIED 2026-08-12 (after-hours): ladder sign(%King) matches feed d/pos.
                                     // Confirmed on SPY 772=−85%(purple/neg), 773/779=+(yellow/pos), 769=−17%(neg);
                                     // QQQ 727=−$13,835K shows the documented yellow-over-purple Rug geometry natively.
                                     // => sign(pos=n.d>0) == ladder color convention; no inversion. Rug flag now live.
var RUG_SIG_PCT = 20;                // node must be this prominent to anchor a rug
// (v10.26-prep) Cluster / Double-Stack thresholds (tunable).
var CLUSTER_SIG_PCT = 25;   // a node must be >= this % of King to count toward a cluster/stack
var CLUSTER_BAND    = 3.0;  // >=3 significant nodes within this strike span => Cluster (pins/chops)
var CLUSTER_MIN_N   = 3;    // min significant nodes for a Cluster
var STACK_GAP       = 1.0;  // two adjacent significant nodes within this span => Double-Stack (bounce shelf)

// (v10.26-prep) CLUSTER DETECTOR: multiple large nodes GROUPED in a tight band -> price
// PINS/CHOPS around the region. Returns the tightest qualifying group per side of price.
// Factual structural read (grouping), NOT an outcome prediction.
function clusterDetect(sym){
  var S=STATE[sym]||{};
  var walls=(S.walls||[]).filter(function(w){ return w.k!=null && typeof w.pct==='number' && Math.abs(w.pct)>=CLUSTER_SIG_PCT; });
  var out={ ok:false, regions:[], memberK:{} };
  if(walls.length<CLUSTER_MIN_N) return out;
  var ks=walls.map(function(w){return w.k;}).sort(function(a,b){return a-b;});
  // sliding window: greedily grow a run while span <= CLUSTER_BAND
  var i=0;
  while(i<ks.length){
    var j=i;
    while(j+1<ks.length && (ks[j+1]-ks[i])<=CLUSTER_BAND) j++;
    var n=(j-i+1);
    if(n>=CLUSTER_MIN_N){
      var members=ks.slice(i,j+1);
      out.regions.push({ lo:members[0], hi:members[members.length-1], n:n, members:members });
      members.forEach(function(k){ out.memberK[k.toFixed(2)]=true; });
      i=j+1;   // non-overlapping regions
    } else {
      i++;
    }
  }
  out.ok=out.regions.length>0;
  return out;
}

// (v10.26-prep) DOUBLE-STACK DETECTOR: two (or more) ADJACENT significant nodes within
// STACK_GAP -> strong-BOUNCE shelf. Distinct from Cluster (which is >=3 spread across a
// wider band and CHOPS). A pair within 1 strike is a shelf price bounces off cleanly.
function doubleStackDetect(sym){
  var S=STATE[sym]||{};
  var walls=(S.walls||[]).filter(function(w){ return w.k!=null && typeof w.pct==='number' && Math.abs(w.pct)>=CLUSTER_SIG_PCT; });
  var out={ ok:false, stacks:[], memberK:{} };
  if(walls.length<2) return out;
  var ks=walls.map(function(w){return w.k;}).sort(function(a,b){return a-b;});
  var i=0;
  while(i<ks.length-1){
    var j=i;
    while(j+1<ks.length && (ks[j+1]-ks[j])<=STACK_GAP) j++;   // chain adjacencies
    var n=(j-i+1);
    if(n>=2){
      var members=ks.slice(i,j+1);
      out.stacks.push({ lo:members[0], hi:members[members.length-1], n:n, members:members });
      members.forEach(function(k){ out.memberK[k.toFixed(2)]=true; });
      i=j+1;
    } else {
      i++;
    }
  }
  out.ok=out.stacks.length>0;
  return out;
}
function rugDetect(sym){
  var S=STATE[sym]||{};
  var px=S.price, walls=S.walls||[];
  var out={ ok:false, type:null, ceilK:null, floorK:null, verified:RUG_POLARITY_VERIFIED,
            targets:[], why:'', confirm:{downGrowing:false, lowUpside:false} };
  if(px==null || walls.length<3) return out;
  var sig=walls.filter(function(w){ return w.k!=null && typeof w.pct==='number' && Math.abs(w.pct)>=RUG_SIG_PCT; });
  if(sig.length<3) return out;
  var above=sig.filter(function(w){return w.k>px;}).sort(function(a,b){return a.k-b.k;}); // nearest-first up
  var below=sig.filter(function(w){return w.k<px;}).sort(function(a,b){return b.k-a.k;}); // nearest-first down
  // --- confirmation signals (reuse build-rate) ---
  function growingDown(){ var g=0; below.forEach(function(w){ var r=nodeBuildRate(sym,w.k); if(r<0) g++; }); return g>=Math.max(1,Math.floor(below.length/2)); }
  function lowUpsideAccum(){ var up=0; above.forEach(function(w){ var r=nodeBuildRate(sym,w.k); if(r>0) up++; }); return up===0; }
  // ---- RUG (bearish): yellow(pos) ceiling over purple(neg) floor, neg stack below, no pos floor ----
  // ceiling candidate: nearest positive node at/above price (or just below within 1 strike).
  var yellowCeil = null;
  above.forEach(function(w){ if(w.pos===true && !yellowCeil) yellowCeil=w; });
  if(!yellowCeil && below.length && below[0].pos===true && Math.abs(below[0].k-px)<=1) yellowCeil=below[0];
  if(yellowCeil){
    // purple node directly below the yellow ceiling
    var purpleBelow=null;
    below.forEach(function(w){ if(w.k<yellowCeil.k && w.pos===false && !purpleBelow) purpleBelow=w; });
    if(purpleBelow){
      var negBelow=below.filter(function(w){ return w.k<=purpleBelow.k && w.pos===false; });
      var posFloor =below.filter(function(w){ return w.k<purpleBelow.k && w.pos===true; });
      var negMass=negBelow.reduce(function(a,w){return a+Math.abs(w.pct);},0);
      var posMass=posFloor.reduce(function(a,w){return a+Math.abs(w.pct);},0);
      var noFloor = posMass < negMass*0.5;   // no significant positive floor beneath the neg cascade
      if(negBelow.length>=1 && noFloor){
        out.ok=true; out.type='Rug'; out.ceilK=yellowCeil.k; out.floorK=purpleBelow.k;
        out.targets=negBelow.map(function(w){return w.k;});
        out.confirm.downGrowing=growingDown(); out.confirm.lowUpside=lowUpsideAccum();
        out.why='Yellow ceiling '+fmtNum(yellowCeil.k)+' over purple '+fmtNum(purpleBelow.k)+', neg-gamma floors below, no positive floor — break accelerates down.';
        return finalizeRug(out);
      }
    }
  }
  // ---- REVERSE-RUG (bullish mirror): purple ceiling over yellow floor, no neg ceiling above ----
  var purpleCeil=null;
  above.forEach(function(w){ if(w.pos===false && !purpleCeil) purpleCeil=w; });
  if(purpleCeil){
    var yellowFloor=null;
    below.forEach(function(w){ if(w.pos===true && !yellowFloor) yellowFloor=w; });
    if(yellowFloor){
      var negCeil=above.filter(function(w){ return w.k>purpleCeil.k && w.pos===false; });
      var negCeilMass=negCeil.reduce(function(a,w){return a+Math.abs(w.pct);},0);
      var posCeilMass=above.filter(function(w){return w.pos===true;}).reduce(function(a,w){return a+Math.abs(w.pct);},0);
      if(negCeilMass < Math.max(posCeilMass,purpleCeil.v||purpleCeil.pct)*0.5){
        out.ok=true; out.type='Reverse-Rug'; out.ceilK=purpleCeil.k; out.floorK=yellowFloor.k;
        out.targets=above.filter(function(w){return w.pos===false;}).map(function(w){return w.k;});
        out.why='Purple ceiling '+fmtNum(purpleCeil.k)+' over yellow floor '+fmtNum(yellowFloor.k)+', no negative ceiling above — break squeezes up.';
        return finalizeRug(out);
      }
    }
  }
  return out;
}
function finalizeRug(out){
  // gate the SHOWN flag on the one-time polarity verification; always keep the
  // computed evidence so the verify is a glance and the recorder can log it.
  out.shown = out.ok && RUG_POLARITY_VERIFIED;
  return out;
}

// ============================================================================
// NODE MAP v1 (v10.24 Issue I) — two-sided, price-anchored dealer-positioning map.
// A CONSUMER of futureStructureSummary rows (NOT a new engine): it orders the
// meaningful levels on BOTH sides of price, marks the strongest floor+ceiling and
// the King, assigns each level a reaction verdict (Bounce / Pullback / Break-
// through) from size + build-state, and highlights the side price is traveling
// toward (trend badge + momentum) WITHOUT hiding the other side. Returns a plain
// model so the renderer AND the recorder (effectiveness capture) share one source.
// ----------------------------------------------------------------------------
var NODEMAP_NEAR_PAD = 2;   // extra strikes beyond the adaptive window to still map
// blended 'strength' score for the ★ headline: size(%King) + build-rate + nearness
function nmStrength(row, dist){
  var size = (typeof row.pct==='number') ? row.pct : 0;            // 0..100
  var rate = (row.state && typeof row.state.net==='number') ? row.state.net : 0; // signed pts
  var near = 1/(1+Math.max(0,dist));                               // 0..1
  // size dominates (it's the wall), rate adds/subtracts intent, nearness scales.
  return (size*0.6 + Math.max(0,rate)*1.5) * (0.5+0.5*near);
}
// per-level reaction verdict from side + build-state (size-only v1; polarity later).
// Bounce   = a floor building below / ceiling building above -> deflects price.
// Break-through = a level FADING in price's path -> gives way (continuation).
// Pullback = a building level in the travel direction that price tags then re-tests.
function nmVerdict(row, emphasisSide){
  var lbl = row.state ? row.state.label : 'Steady';
  var isRes = (row.side==='above');
  var forming = !(row.hist && row.hist.length>=3);
  if(forming) return { tag:'Forming', conf:'low' };
  if(lbl==='Building') return { tag:'Bounce', conf:'med' };      // strengthening wall deflects
  if(lbl==='Fading')   return { tag:'Break-through', conf:'med' }; // giving way -> continuation
  return { tag:'Pullback', conf:'low' };                          // steady: tag & re-test
}
// (v10.25 Step 5) ATTRACTION-ONLY per-node stage + state-machine OUTCOME echo.
// Accumulation ONLY attracts; we make NO deflect/break call. Stages:
//  'attracting' = node is Acm (Building) AND price is being pulled toward it (approaching).
//  'at-node'    = price consolidating ON it (within AT_NODE_STRK) -> hand off to BO tracker.
//  outcome (from the setup state machine, resolved over time): 'up'|'dn'|'held'|'false' | null.
var AT_NODE_STRK = 0.45;   // within this many strikes of the node = 'at node'
function nodeAttraction(sym, k, side, state, px, emphasis){
  var isAcm = (state==='Building');
  var dist = Math.abs(k-px);
  var atNode = dist <= AT_NODE_STRK;
  // approaching = price moving toward this node's side (emphasis points at it)
  var nodeSide = (k>px)?'above':(k<px?'below':'at');
  var approaching = atNode || (emphasis!=null && emphasis===nodeSide);
  var stage=null;
  if(isAcm && approaching) stage = atNode ? 'at-node' : 'attracting';
  return { stage:stage, atNode:atNode };
}
// Read the BO/breakout-pullback state machine for how this node RESOLVED.
// Returns 'up' | 'dn' | 'held' | 'false' | null. Report, not prediction.
function nodeOutcome(sym, k){
  var S=STATE[sym]||{}; if(!S.setups) return null;
  var px=S.price;
  var best=null;
  for(var key in S.setups){
    var s=S.setups[key];
    if(!s || Math.abs(s.strike-k)>0.001) continue;
    if(!best || (s.updated||0)>(best.updated||0)) best=s;
  }
  if(!best) return null;
  // canonical 'broke with follow-through' test (matches the recorder at line ~3298):
  // reached FT or beyond (FT/TST/CONF/GO), by stage OR by token history (catches FT-then-regress).
  var broke = (best.tokens && best.tokens.indexOf('FT')>=0) ||
              best.stage==='FT'||best.stage==='TST'||best.stage==='CONF'||best.stage==='GO'||best.goFired;
  if(best.voided && broke) return 'false';                 // broke then reversed back = trap
  if(best.voided) return 'held';                            // rejected at node, never followed through
  if(broke) return (best.dir==='long')?'up':'dn';           // clean break in its direction
  return null;                                              // still forming (BO only) -> unresolved
}
function outcomeMarker(o){
  if(o==='up')   return {txt:'broke \u2191', col:PAL.longAccent};
  if(o==='dn')   return {txt:'broke \u2193', col:PAL.shortAccent};
  if(o==='held') return {txt:'held', col:PAL.gold};
  if(o==='false')return {txt:'false break', col:PAL.amber};
  return null;
}
function nodeMapModel(sym){
  var S=STATE[sym]||{};
  var px=S.price;
  var out={ ok:false, px:px, levels:[], kingK:null, strongSup:null, strongRes:null,
            emphasis:null, trendState:'na', againstKing:false };
  if(px==null) return out;
  var fs=futureStructureSummary(sym);
  if(!fs) return out;
  var tp=tapeMap(sym);
  var kingK=(tp && typeof tp.king==='number')?tp.king:(S.king!=null?S.king:null);
  out.kingK=kingK;
  var tv=trendVerdict(sym); out.trendState=tv.state;
  out.regime=gexRegime(sym);   // (v10.24) GEX-structure regime colors how the map is read
  out.gatekeeper=gatekeeper(sym);  // (v10.24) intervening blocker on the path to the King
  out.rug=rugDetect(sym);          // (v10.24) polarity-gated Rug / Reverse-Rug (project namesake)
  out.cluster=clusterDetect(sym);  // (v10.26-prep) grouped nodes -> pins/chops region
  out.stack=doubleStackDetect(sym);// (v10.26-prep) adjacent stacked nodes -> strong-bounce shelf
  // travel-emphasis side: trend state primary, else short-term candle momentum.
  var emphasis=null;
  if(tv.state==='up'||tv.state==='up-broken') emphasis='above';
  else if(tv.state==='dn'||tv.state==='dn-broken') emphasis='below';
  else {
    var c=closedCandles(sym);
    if(c.length>=3){ var mo=c[c.length-1].c-c[c.length-3].c; emphasis = mo>0?'above':(mo<0?'below':null); }
  }
  out.emphasis=emphasis;
  // King side vs emphasis => are we moving AGAINST the King (away from the magnet)?
  if(kingK!=null && emphasis){
    var kingSide=(kingK>px)?'above':(kingK<px?'below':null);
    out.againstKing = (kingSide!=null && emphasis!==kingSide);
  }
  // adaptive near-window (both sides) + always include King and the levels to it,
  // plus the strongest wall on the opposite side even if past the near band.
  var win = adaptiveProxStrikes(sym) + NODEMAP_NEAR_PAD;
  function consider(row){
    var dist=Math.abs(row.k-px);
    var inWin = dist<=win;
    var onPathToKing = (kingK!=null) && ((kingK>px && row.k>px && row.k<=kingK) || (kingK<px && row.k<px && row.k>=kingK));
    var isKing = (kingK!=null && Math.abs(row.k-kingK)<0.001);
    return inWin || onPathToKing || isKing;
  }
  var all=(fs.above||[]).concat(fs.below||[]);
  var mapped=[];
  all.forEach(function(row){
    if(!consider(row)) return;
    var dist=Math.abs(row.k-px);
    var v=nmVerdict(row, emphasis);
    mapped.push({
      k:row.k, side:row.side, pct:row.pct, dist:+dist.toFixed(2),
      state:(row.state?row.state.label:'Steady'), net:(row.state?row.state.net:0),
      rapid:!!(row.state&&row.state.rapid), rapidDir:(row.state&&row.state.rapidDir)||0,
      role:row.role, pos:row.pos, isKing:(kingK!=null && Math.abs(row.k-kingK)<0.001),
      onEmphasis:(emphasis && row.side===emphasis),
      verdict:v.tag, conf:v.conf,
      isGatekeeper:(out.gatekeeper && out.gatekeeper.ok && Math.abs(row.k-out.gatekeeper.k)<0.001),
      isRugTarget:(out.rug && out.rug.ok && out.rug.targets && out.rug.targets.some(function(tk){return Math.abs(row.k-tk)<0.001;})),
      // (v10.26-prep) rug ceiling/floor participants (the pattern-forming nodes, not just targets)
      isRugCeil:(out.rug && out.rug.ok && out.rug.ceilK!=null && Math.abs(row.k-out.rug.ceilK)<0.001),
      isRugFloor:(out.rug && out.rug.ok && out.rug.floorK!=null && Math.abs(row.k-out.rug.floorK)<0.001),
      rugType:(out.rug && out.rug.ok)?out.rug.type:null,
      // (v10.26-prep) cluster/double-stack region membership
      isCluster:(out.cluster && out.cluster.ok && out.cluster.memberK[row.k.toFixed(2)]===true),
      isStack:(out.stack && out.stack.ok && out.stack.memberK[row.k.toFixed(2)]===true),
      // (v10.29) live pattern DETAIL for the badge hover (callout lines removed; the
      // per-node badge tooltip now carries geometry/state/targets/span).
      rugDetail:(out.rug && out.rug.ok)?{ceilK:out.rug.ceilK, floorK:out.rug.floorK, targets:(out.rug.targets||[]).slice(), shown:!!out.rug.shown, confirmed:!!(out.rug.confirm&&(out.rug.confirm.downGrowing||out.rug.confirm.lowUpside)), type:out.rug.type}:null,
      stackDetail:(function(){ if(!(out.stack&&out.stack.ok))return null; var f=null; (out.stack.stacks||[]).forEach(function(s){ if(row.k>=s.lo-0.001&&row.k<=s.hi+0.001) f=s; }); return f?{lo:f.lo,hi:f.hi,n:f.n}:null; })(),
      clusterDetail:(function(){ if(!(out.cluster&&out.cluster.ok))return null; var f=null; (out.cluster.regions||[]).forEach(function(r){ if(row.k>=r.lo-0.001&&row.k<=r.hi+0.001) f=r; }); return f?{lo:f.lo,hi:f.hi,n:f.n}:null; })(),
      strength:+nmStrength(row, dist).toFixed(1),
      forming:!(row.hist && row.hist.length>=3),
      touches:(row.peak && row.peak.touches!=null)?row.peak.touches:null,
      // (v10.25 Step 5) attraction stage + resolved outcome echo
      attract:nodeAttraction(sym, row.k, row.side, (row.state?row.state.label:'Steady'), px, emphasis),
      outcome:nodeOutcome(sym, row.k)
    });
  });
  // strongest floor (below) + ceiling (above) by blended strength (headline job).
  mapped.forEach(function(m){
    if(m.side==='below'){ if(!out.strongSup || m.strength>out.strongSup.strength) out.strongSup=m; }
    else               { if(!out.strongRes || m.strength>out.strongRes.strength) out.strongRes=m; }
  });
  if(out.strongSup) out.strongSup.isStrong=true;
  if(out.strongRes) out.strongRes.isStrong=true;
  // DISPLAY order: highest strike at top -> lowest (price divider sits between).
  mapped.sort(function(a,b){ return b.k-a.k; });
  out.levels=mapped;
  out.ok = mapped.length>0;
  return out;
}

// ---- NODE MAP renderer (v10.24) ----
// Two-sided price-anchored ladder: resistance above (highest first) -> SPOT -> support
// below. Strongest floor/ceiling get a ★; the King gets 👑; the emphasis side (travel
// direction) is subtly highlighted. Each row shows strike, %King, state arrow, and
// its reaction verdict. Verdict color = directional meaning (locked rule).
function nmVerdictColor(v, side){
  // Bounce off a floor(below) = bullish(green); off a ceiling(above) = bearish(red).
  // Break-through DOWN through support = bearish(red); UP through resistance = bullish(green).
  if(v==='Bounce')        return side==='below'?PAL.longAccent:PAL.shortAccent;
  if(v==='Break-through') return side==='below'?PAL.shortAccent:PAL.longAccent;
  if(v==='Pullback')      return PAL.blue;
  return PAL.sub;   // Forming
}
// (v10.25 Step 4) white castle-gate (portcullis) SVG icon, stroke tunable.
function gateSvg(stroke){
  return '<svg width="18" height="16" viewBox="0 0 20 18" fill="none" stroke="'+stroke+'" stroke-width="1.4" stroke-linecap="round" style="vertical-align:middle">'+
    '<path d="M3 16 V7 a7 7 0 0 1 14 0 V16"/><line x1="3" y1="16" x2="17" y2="16"/>'+
    '<line x1="7" y1="16" x2="7" y2="6"/><line x1="10" y1="16" x2="10" y2="4.2"/><line x1="13" y1="16" x2="13" y2="6"/>'+
    '<line x1="4.5" y1="10" x2="15.5" y2="10"/></svg>';
}
// (v10.25 Step 4) GATEKEEPER area — own section below the King header. No spelled-out
// title (space). Lists JUST the primary gatekeeper (nearest blocker to the King) with
// strike + strength-ratio + verdict + a tri-index confluence note (soft booster).
function gatekeeperBlock(){
  var sym='SPY';
  var gk = (typeof gatekeeper==='function') ? (function(){ try{return gatekeeper(sym);}catch(e){return null;} })() : null;
  var gate=gateSvg(PAL.ink);
  // (v10.28) \u2463 icon BEFORE the gate icon (was gate+\u2463).
  var hdr='<div style="display:flex;align-items:center;gap:6px;margin:0 2px 5px">'+stepIcon(4)+gate+
    '<span style="font-size:8.5px;color:'+PAL.sub+';font-weight:700;letter-spacing:.3px">gatekeeper</span></div>';
  if(!gk || !gk.ok){
    // clear path to the King (honest positive note) — or nothing to gatekeep toward
    var msg = (gk && gk.kingK!=null) ? ('Clear path to \uD83D\uDC51 '+fmtNum(gk.kingK)+' \u2014 no gatekeeper blocking.') : 'No gatekeeper (King at/na).';
    return hdr+'<div style="font-size:10px;color:'+PAL.sub+';padding:2px 4px">'+msg+'</div>';
  }
  var col = gk.strong?PAL.amber:PAL.sub;
  var ratioTxt = (gk.ratio!=null)?(gk.ratio+'\u00d7'):'\u2013';
  var early = isEarlySession() ? ' <span style="color:'+PAL.amber+';font-weight:700">Early-session \u2014 higher-probability reversal.</span>' : '';
  // tri-index confluence (soft): show a small note. Uses gexRegime dir agreement across symbols if available.
  var conf = triIndexNote(gk);
  var card='<div style="background:'+PAL.card+';border:1px solid '+col+';border-radius:8px;padding:6px 9px">'+
    '<div style="display:flex;align-items:center;gap:7px">'+gateSvg(PAL.ink)+
      '<span style="font-weight:800;font-size:14px;color:'+PAL.ink+';font-variant-numeric:tabular-nums">'+fmtNum(gk.k)+'</span>'+
      '<span style="font-size:10px;font-weight:800;color:'+col+';border:1px solid '+col+';border-radius:20px;padding:0 6px">'+ratioTxt+'</span>'+
      '<span style="font-size:10px;font-weight:800;color:'+col+';margin-left:auto">'+(gk.strong?'Reversal likely':'Watch')+'</span></div>'+
    '<div style="font-size:10px;line-height:1.4;color:'+PAL.sub+';margin-top:4px">Blocks the path to \uD83D\uDC51 '+(gk.kingK!=null?fmtNum(gk.kingK):'?')+'. Ratio '+ratioTxt+' vs the next node beyond'+(gk.strong?' \u2014 strong rejection zone; a failed test can reshuffle the map.':'.')+early+'</div>'+
    (conf?('<div style="font-size:9.5px;color:'+PAL.sub+';margin-top:5px">'+conf+'</div>'):'')+
  '</div>';
  return hdr+card;
}
// tri-index confluence note (SOFT booster, NOT a gate). Compares gexRegime dir across
// SPY/QQQ/SPXW; agreement boosts, divergence is noted, never blocks.
function triIndexNote(gk){
  try{
    var dirs={};
    ['SPY','QQQ','SPXW'].forEach(function(s){ if(STATE[s]){ var r=gexRegime(s); dirs[s]=r?r.dir:0; } });
    var have=Object.keys(dirs); if(have.length<2) return '';
    var gkDir = (gk.side==='above')?1:-1;   // toward-King direction
    var agree=0, tot=0;
    have.forEach(function(s){ tot++; if((dirs[s]>0&&gkDir>0)||(dirs[s]<0&&gkDir<0)) agree++; });
    if(agree===tot && tot>=2) return '<span style="color:'+PAL.longAccent+'">\u25CF</span> '+have.join(' + ')+' aligned \u2014 confluence <b style="color:'+PAL.longAccent+'">boosts</b> confidence.';
    if(agree===0) return '<span style="color:'+PAL.amber+'">\u25CF</span> indices diverge \u2014 lower confluence (noted, not blocking).';
    return '<span style="color:'+PAL.amber+'">\u25CF</span> mixed confluence ('+agree+'/'+tot+' aligned).';
  }catch(e){ return ''; }
}
// (v10.26-prep Step 5) NODE STATUS = the doc's flow read. Building->Acm (green,
// strengthening), Fading->Diss (red, weakening), Steady->grey. RAPID = the doc's
// 'Reshuffling' state: append \uD83D\uDD25 (rapid Acm) / \u2744 (rapid Diss).
function nodeStatusTag(L){
  var lab = L.state==='Building' ? 'Acm' : (L.state==='Fading' ? 'Diss' : 'Steady');
  var col = L.state==='Building' ? PAL.longAccent : (L.state==='Fading' ? PAL.shortAccent : PAL.sub);
  var reshuf='';
  if(L.rapid){
    // rapidDir>0 => rapidly strengthening (fire); <0 => rapidly weakening (snow).
    var ic = (L.rapidDir>0) ? '\uD83D\uDD25' : '\u2744';
    reshuf=' <span title="Reshuffling \u2014 rapid exposure change (new structure forming)." style="font-size:9px">'+ic+'</span>';
  }
  var tip = (lab==='Acm'?'Accumulation \u2014 dealers building \u2192 STRONGER magnet.':
            (lab==='Diss'?'Dissipation \u2014 dealers closing \u2192 WEAKENING node.':
            'Steady \u2014 holding, no net build/decay.'));
  return '<span title="'+tip+'" style="color:'+col+';font-size:8.5px;font-weight:800;border:1px solid '+col+';border-radius:9px;padding:0 5px">'+lab+'</span>'+reshuf;
}
// (v10.26-prep Step 5) NODE TYPE = gamma polarity. +\u03b3 positive-gamma (pinning /
// mean-reverting, yellow); \u2212\u03b3 negative-gamma (accelerant / breakout-prone,
// purple). Same axis that drives Rug detection.
function nodeTypeTag(L){
  if(L.pos===true){
    return '<span title="Positive-gamma node \u2014 dealers dampen moves here (pinning / mean-reverting). Yellow." style="color:'+PAL.gold+';font-size:8.5px;font-weight:800">+\u03b3</span>';
  }
  if(L.pos===false){
    return '<span title="Negative-gamma node \u2014 dealers amplify moves here (accelerant / breakout-prone). Purple." style="color:#b58bff;font-size:8.5px;font-weight:800">\u2212\u03b3</span>';
  }
  return '';
}
// (v10.26-prep) NODE ROLE/SETUP badge \u2014 REPLACES the removed predictive verdict pill
// (Bounce/Pullback/Break-through, which VIOLATED the attraction-only honesty rule).
// This is FACTUAL: what the node IS, never what it might DO. Priority order picks the
// single top badge (King > Gatekeeper > Rug > Floor/Ceiling); secondary role -> tooltip.
// Cluster + Double-Stack are FUTURE detectors (slot reserved in the priority list).
function nodeRoleBadge(L){
  var label=null, col=PAL.sub, tip='';
  if(L.isKing){
    label='King'; col=PAL.gold; tip='King \u2014 dealer settlement target (highest-probability zone).';
  } else if(L.isGatekeeper){
    label='Gatekeeper'; col=PAL.amber; tip='Gatekeeper \u2014 deflection zone where trend shifts often begin; can block the path to the King.';
  } else if(L.isRugCeil){
    var rr=(L.rugType==='Reverse-Rug'); label=rr?'RRugF':'RugC'; col='#b58bff';
    // (v10.29) live geometry + state + targets folded into the hover (callout removed).
    var rd=L.rugDetail||{};
    var geo=(rd.ceilK!=null&&rd.floorK!=null)?(fmtNum(rd.ceilK)+' over '+fmtNum(rd.floorK)):'';
    var st=rd.shown?(rd.confirmed?'forming':'forming (unconfirmed \u2014 flow not yet aligned)'):'candidate \u2014 verify polarity';
    var tg=(rd.targets&&rd.targets.length)?(' \u00b7 targets '+rd.targets.map(fmtNum).join(', ')):'';
    tip=(rr?'Reverse-Rug':'Rug')+' ceiling'+(geo?(' \u00b7 '+geo):'')+' \u00b7 '+st+tg+' \u2014 positive-gamma cap over a negative-gamma floor; '+(rr?'bullish squeeze.':'bearish nosedive risk.');
  } else if(L.isRugFloor){
    var rr2=(L.rugType==='Reverse-Rug'); label=rr2?'RRugC':'RugF'; col='#b58bff';
    var rd2=L.rugDetail||{};
    var geo2=(rd2.ceilK!=null&&rd2.floorK!=null)?(fmtNum(rd2.ceilK)+' over '+fmtNum(rd2.floorK)):'';
    var st2=rd2.shown?(rd2.confirmed?'forming':'forming (unconfirmed \u2014 flow not yet aligned)'):'candidate \u2014 verify polarity';
    var tg2=(rd2.targets&&rd2.targets.length)?(' \u00b7 targets '+rd2.targets.map(fmtNum).join(', ')):'';
    tip=(rr2?'Reverse-Rug':'Rug')+' floor'+(geo2?(' \u00b7 '+geo2):'')+' \u00b7 '+st2+tg2+' \u2014 negative-gamma target below the positive-gamma cap.';
  } else if(L.isStack){
    label='DStk'; col=PAL.longAccent;
    var sd=L.stackDetail;
    var sspan=sd?(sd.lo===sd.hi?fmtNum(sd.lo):(fmtNum(sd.lo)+'\u2013'+fmtNum(sd.hi))):'';
    tip='Double-Stack'+(sspan?(' '+sspan):'')+(sd?(' ('+sd.n+' nodes)'):'')+' \u2014 adjacent stacked nodes forming a strong-BOUNCE shelf; price tends to bounce cleanly off it (fade into it).';
  } else if(L.isCluster){
    label='Clst'; col=PAL.blue;
    var cd=L.clusterDetail;
    var cspan=cd?(fmtNum(cd.lo)+'\u2013'+fmtNum(cd.hi)):'';
    tip='Cluster'+(cspan?(' '+cspan):'')+(cd?(' ('+cd.n+' nodes)'):'')+' \u2014 several large nodes grouped in a tight band; price tends to PIN / CHOP here (poor R:R \u2014 avoid the middle, trade the edges).';
  } else if(L.role==='Floor'){
    label='Flr'; col=PAL.longAccent; tip='Floor \u2014 support node below price (dealers buy as price declines into it).';
  } else if(L.role==='Ceiling'){
    label='Ceil'; col=PAL.shortAccent; tip='Ceiling \u2014 resistance node above price (dealers sell as price rises into it).';
  }
  if(!label) return '';
  // secondary-role note in tooltip (e.g. a King that is also a rug participant)
  var extra=[];
  if(!L.isKing && L.isGatekeeper===false){}
  if(L.isRugTarget && !L.isRugCeil && !L.isRugFloor) extra.push('rug-target');
  var fullTip = tip + (extra.length?(' Also: '+extra.join(', ')+'.'):'');
  return '<span title="'+fullTip.replace(/"/g,'')+'" style="color:'+col+';font-size:8.5px;font-weight:800;border:1px solid '+col+';border-radius:10px;padding:0 6px">'+label+'</span>';
}
function nodeMapBlock(){
  var sym='SPY';
  var m=nodeMapModel(sym);
  var hdrTip=('NODE MAP \u2014 the dealer-positioning levels price will meet on BOTH sides, each tagged with what it should DO (Bounce / Pullback / Break-through). \u2605 = strongest floor/ceiling; \uD83D\uDC51 = King (settlement magnet). The highlighted side is where price is currently heading (trend + momentum); the other side is still shown. Verdicts sharpen once polarity lands.').replace(/"/g,'');
  // (v10.24) REGIME chip in the header = whole-board GEX-structure read, colored by
  // direction; its instruction (fade edges / stand aside / pullbacks) rides below.
  var rg=m.regime||{label:'Forming',dir:0,conf:'low',why:''};
  var rgCol = rg.dir>0?PAL.longAccent:(rg.dir<0?PAL.shortAccent:(rg.label==='Whipsaw'?PAL.amber:PAL.sub));
  var rgChip = '<span title="'+(rg.why||'').replace(/"/g,'')+'" style="color:'+rgCol+';font-size:9px;font-weight:800;padding:1px 7px;border:1px solid '+rgCol+';border-radius:20px">'+rg.label+(rg.skew!=null&&/Trend/.test(rg.label)?(' '+rg.skew+'\u00d7'):'')+'</span>';
  // (v10.28) \u2464 Step-5 icon in the Node Map header (Map the Flow). Rendered inside
  // the header title so it clicks open the Step-5 popover like the other step icons.
  var html=sectionHdrRight(stepIcon(5,'vertical-align:middle;margin-right:4px')+'Node Map', (m.ok?rgChip:''), hdrTip);
  if(!m.ok){ html+='<div style="color:'+PAL.sub+';padding:2px 6px;font-size:11px">Node Map \u2014 waiting on node data\u2026</div>'; return html; }
  // (v10.29) Pattern instruction line REMOVED \u2014 the Pattern BADGE in the header (Trend/
  // Whipsaw/Rainbow Road) already names it, and its stance rides in the badge hover.
  // ('regime' renamed to 'Pattern' per Skylit's Patternpedia vocabulary.)
  // (v10.27) S/R IMBALANCE net-read FOLDED IN here (standalone section removed).
  // This is the Step-5 flow conclusion: the DIVERGENCE in build-RATE between the two
  // sides near price (not who is bigger). srBattle is render-cached, so this matches
  // the same value the old section showed. Below it: the tradeable crossover banner.
  (function(){
    var srb = (typeof srBattle==='function') ? srBattle('SPY') : null;
    if(!srb) return;
    var forming = !!srb.forming;
    var netCol = srb.dom==='support'?PAL.longAccent:(srb.dom==='resistance'?PAL.shortAccent:PAL.sub);
    var head = forming ? '' : (srb.dom==='resistance'?'Bearish imbalance':srb.dom==='support'?'Bullish imbalance':'');
    var mech = '';
    if(!forming && srb.dom==='resistance'){ mech='resistance '+(srb.gain!=null?fmtNum(srb.gain):'\u2013')+' building, support '+(srb.fade!=null?fmtNum(srb.fade):'\u2013')+' fading'; }
    else if(!forming && srb.dom==='support'){ mech='support '+(srb.gain!=null?fmtNum(srb.gain):'\u2013')+' building, resistance '+(srb.fade!=null?fmtNum(srb.fade):'\u2013')+' fading'; }
    var imbTip=('S/R IMBALANCE \u2014 the DIVERGENCE in build-RATE between the two sides near price (not who is bigger). '+(forming?'Forming: not enough rate samples yet.':(srb.dom==='resistance'?('Bearish: '+mech+' \u2014 ceiling strengthening while floor gives way, so a pullback tends to fail and roll down.'):srb.dom==='support'?('Bullish: '+mech+' \u2014 floor strengthening while overhead gives way, so dips tend to hold.'):'Neither side clearly gaining.'))+' Committed on 3m bar close (de-flickered).').replace(/"/g,'');
    if(head || mech){
      html+='<div title="'+imbTip+'" style="font-size:9.5px;line-height:1.35;margin:0 2px 4px">'+(head?('<span style="font-weight:800;color:'+netCol+'">'+head+'</span>'+(mech?' \u2014 ':'')):'')+(mech?('<span style="color:'+PAL.sub+';font-weight:600">'+mech+'</span>'):'')+'</div>';
    }
    if(srb.cross){
      var xBear=(srb.cross==='bears');
      var xCol=xBear?PAL.shortAccent:PAL.longAccent;
      var xTxt=xBear?'\u25bc BEARS TAKING OVER \u2014 floor giving way, pullback-high short trigger':'\u25b2 BULLS TAKING OVER \u2014 overhead giving way, bounce starting';
      html+='<div style="background:'+(xBear?'rgba(240,97,109,0.12)':'rgba(46,194,126,0.12)')+';border:1px solid '+xCol+';border-radius:8px;padding:3px 8px;margin:2px 2px 4px;font-size:9.5px;font-weight:800;color:'+xCol+'">'+xTxt+'</div>';
    }
  })();
  // strongest headline (preserves old PROJ job): strongest Sup + strongest Res.
  function strongChip(c, word, col){
    if(!c) return '<span style="color:'+PAL.sub+';font-size:10px">'+word+' \u2013</span>';
    return '<span title="Strongest '+(word==='Sup'?'floor below':'ceiling above')+' (size+build-rate+nearness blend)" style="display:inline-flex;align-items:center;gap:3px;color:'+col+';font-weight:800;font-size:10px;padding:1px 7px;border:1px solid '+col+';border-radius:20px">\u2605 '+word+' '+fmtNum(c.k)+'<span style="color:'+PAL.sub+';font-weight:600;font-size:9px">'+c.pct+'%</span></span>';
  }
  // (v10.27) Strongest Sup/Res chips REMOVED here \u2014 redundant with the King
  // 3-magnet header (\u2605SUP \u2190 \uD83D\uDC51 \u2192 \u2605RES). Snapback warning retained.
  if(m.againstKing && m.kingK!=null){
    html+='<div style="margin:1px 2px 4px"><span title="Price is moving AWAY from the King \u2014 the magnet behind may pull it back (snapback risk)." style="color:'+PAL.amber+';font-size:9px;font-weight:700">\u21a9 King '+fmtNum(m.kingK)+' behind</span></div>';
  }
  // (v10.24) RUG flag: the polarity-gated nosedive/squeeze. Shows a hard flag once
  // polarity is verified; until then, a dimmed 'candidate (verify polarity)' note.
  // (v10.29) RUG callout line REMOVED \u2014 geometry / forming-state / targets now live in
  // the RugC/RugF (RRugC/RRugF) per-node BADGE HOVER. m.rug still drives those badges.
  // (v10.24) gatekeeper instruction line: the blocker between price and King.
  var gk=m.gatekeeper;
  if(gk && gk.ok){
    var gkCol = gk.strong?PAL.amber:PAL.sub;
    html+='<div title="Gatekeeper: the nearest high-value node between price and the King ('+(gk.kingK!=null?fmtNum(gk.kingK):'?')+'). Strength ratio '+(gk.ratio!=null?gk.ratio+'\u00d7':'\u2013')+' vs the next node beyond it. Ratio >> 1 => stall/reversal here rather than continuation to the King." style="font-size:9.5px;color:'+gkCol+';font-weight:700;margin:0 2px 4px">\uD83D\uDEAA Gatekeeper '+fmtNum(gk.k)+(gk.ratio!=null?(' \u00b7 '+gk.ratio+'\u00d7'):'')+' \u2014 '+gk.verdict+(gk.decoyDiscount>0?' (King less reachable)':'')+'</div>';
  }
  // (v10.29) DOUBLE-STACK + CLUSTER callout lines REMOVED \u2014 fully redundant with the
  // per-node DStk / Clst badges; span + node-count + meaning now ride in the badge HOVER.
  // (v10.27) PER-NODE BO TAG: the breakout-pullback lifecycle now rides on the node
  // it belongs to (standalone BO section removed). Finds the most-advanced non-voided
  // setup at this strike and renders its stage chain (BO / BO\u00b7FT / BO\u00b7FT\u00b7TST\u2026),
  // colored by direction (long=green, short=red). Empty when no live setup here.
  function setupTagForNode(k){
    var S=STATE[sym]; if(!S||!S.setups) return '';
    var STAGES=['BO','FT','TST','CONF','GO'];
    var best=null, bestRank=-1;
    for(var key in S.setups){
      var s=S.setups[key];
      if(!s || s.voided || s.strike!==k) continue;
      var rank=STAGES.indexOf(s.stage);
      if(rank>bestRank){ bestRank=rank; best=s; }
    }
    if(!best || bestRank<0) return '';
    // build the chain up to the current stage from the canonical order (not raw
    // tokens, so VOID/T1/T2 noise never shows); join with a middot.
    var chain=STAGES.slice(0, bestRank+1).join('\u00b7');
    var col = best.dir==='long'?PAL.longAccent:PAL.shortAccent;
    var tip = (best.dir==='long'?'Long':'Short')+' breakout-pullback setup at '+fmtNum(k)+' \u2014 lifecycle '+chain+'. BO=first close beyond the node; FT=a later bar holds fully beyond; TST=wick back to test; CONF=confirming candle; GO=triggered. Requires a '+BO_HL_LOOKBACK+'-bar '+(best.dir==='long'?'high':'low')+' at breakout.';
    return '<span title="'+tip.replace(/"/g,'')+'" style="color:'+col+';font-size:8px;font-weight:800;letter-spacing:.3px;border:1px solid '+col+';border-radius:9px;padding:0 4px;white-space:nowrap">'+chain+'</span>';
  }
  // the two-sided ladder
  function row(L){
    var isPx=false;
    var col=nmVerdictColor(L.verdict, L.side);
    // (v10.26-prep Step 5) node STATUS (Acm/Diss/Steady + reshuffle) and TYPE (+\u03b3/\u2212\u03b3)
    // replace the old bare directional arrow with an explicit, doc-vocabulary identity.
    var statusHtml = nodeStatusTag(L);
    var typeHtml = nodeTypeTag(L);
    var marks=(L.isStrong?'\u2605':'')+(L.isKing?'\uD83D\uDC51':'')+(L.isGatekeeper?'\uD83D\uDEAA':'')+(L.isRugTarget&&m.rug&&m.rug.shown?'\uD83E\uDDF6':'');
    var bg = L.onEmphasis ? (L.side==='above'?'rgba(240,97,109,0.06)':'rgba(46,194,126,0.06)') : 'transparent';
    // (v10.26-prep) ROLE/SETUP badge replaces the removed predictive verdict pill.
    // Factual (what the node IS), never a Bounce/Break prediction. 'forming' fallback
    // only when the node has too little history to classify AND carries no role.
    var roleBadge = nodeRoleBadge(L);
    var vtag = roleBadge ? roleBadge
      : (L.forming ? '<span style="color:'+PAL.sub+';font-size:8px;font-weight:700">forming</span>' : '');
    // (v10.25 Step 5) ATTRACTION-ONLY stage + resolved-OUTCOME echo. NO deflect/break
    // prediction: accumulation only ATTRACTS. Stage says where price is vs the magnet;
    // the outcome (broke/held/false) is a report the BO state machine resolves over time.
    var stageHtml='';
    var om = outcomeMarker(L.outcome);
    if(om){
      // resolved: the node echoes what actually happened (report, not prediction).
      stageHtml='<span style="color:'+om.col+';font-size:8.5px;font-weight:800">'+om.txt+'</span>';
    } else if(L.attract && L.attract.stage==='at-node'){
      stageHtml='<span style="color:'+PAL.gold+';font-size:8.5px;font-weight:800">at node \u00b7 watch BO</span>';
    } else if(L.attract && L.attract.stage==='attracting'){
      stageHtml='<span style="color:'+PAL.blue+';font-size:8.5px;font-weight:700">attracting</span>';
    }
    var stageCell = stageHtml ? ('<span style="width:96px;text-align:right">'+stageHtml+'</span>') : '<span style="width:96px"></span>';
    // (v10.27) per-node BO lifecycle tag (replaces the removed standalone BO section)
    var boTag = setupTagForNode(L.k);
    var boCell = '<span style="width:auto;text-align:right">'+boTag+'</span>';
    return '<div style="display:flex;align-items:center;gap:5px;font-size:10px;padding:2px 5px;border-radius:5px;background:'+bg+'">'+
      '<span style="width:56px;font-weight:800;font-variant-numeric:tabular-nums;color:'+(L.isKing?PAL.gold:PAL.ink)+'">'+marks+fmtNum(L.k)+'</span>'+
      '<span style="width:38px;text-align:right;color:'+PAL.sub+';font-variant-numeric:tabular-nums">'+L.pct+'%</span>'+
      '<span style="width:auto;text-align:left">'+statusHtml+'</span>'+
      '<span style="width:auto;text-align:left">'+typeHtml+'</span>'+
      stageCell+
      boCell+
      '<span style="flex:1;text-align:right">'+vtag+'</span></div>';
  }
  var px=m.px;
  var printed=false;
  m.levels.forEach(function(L, i){
    if(!printed && L.k<=px){
      html+='<div title="Current SPY price." style="display:flex;justify-content:center;align-items:center;height:16px;margin:2px 0;background:'+PAL.card+';border:1px dashed '+PAL.blue+';border-radius:6px;color:'+PAL.blue+';font-size:10px;font-weight:800">\u2014 SPY '+fmtNum(px)+' \u2014</div>';
      printed=true;
    }
    html+=row(L);
  });
  if(!printed){ // price is below every mapped level
    html+='<div style="display:flex;justify-content:center;align-items:center;height:16px;margin:2px 0;background:'+PAL.card+';border:1px dashed '+PAL.blue+';border-radius:6px;color:'+PAL.blue+';font-size:10px;font-weight:800">\u2014 SPY '+fmtNum(px)+' \u2014</div>';
  }
  return html;
}

// ---- King session bounds (CT cash session 8:30\u201315:00) as epoch ms today ----
function sessionBoundsCT(){
  var d=ctNow();
  function atCT(h,m){
    // Build an epoch ms for today's h:m America/Chicago. ctNow() already returns
    // a Date whose fields are CT wall-clock; reconstruct via the same offset.
    var real=new Date();
    var ctMs=ctNow().getTime();
    var offset=real.getTime()-ctMs;              // realUTC - CTwall
    var wall=new Date(d.getFullYear(), d.getMonth(), d.getDate(), h, m, 0, 0).getTime();
    return wall+offset;
  }
  return { start:atCT(8,30), end:atCT(15,0) };
}

// ---- KING PATH SPARKLINE (v10.8) ----
// Stepped SVG of King STRIKE (y) vs real CLOCK TIME (x) across the cash session.
// Honest staircase: hold flat, jump at each confirmed roll's timestamp, flat to
// 'now'. Time axis => a King that held all morning is a long flat left segment;
// a burst of rolls is a tight cluster (correctly signals instability). Faint
// current-price reference line overlaid; gold dot on the current King.
function kingSparkline(mv, kingK, px, now, sess, verdictCol){
  // (v10.23 Issue A) +33% taller so the drift shape + always-on price line are legible.
  var W=236, H=112, padL=4, padR=4, padT=10, padB=10;
  var t0=sess.start, t1=sess.end;
  var firstT = mv.length ? mv[0].t : now;
  var xMin = Math.max(t0, firstT - 3*60000);
  if(xMin > now) xMin = t0;
  var xMax = Math.min(Math.max(now, xMin+60000), (t1>now?t1:now));
  if(xMax<=xMin) xMax=xMin+60000;
  var pts=[];
  for(var i=0;i<mv.length;i++){ var m=mv[i]; if(pts.length && Math.abs(pts[pts.length-1].k-m.k)<0.001) continue; pts.push({t:m.t,k:m.k}); }
  if(!pts.length) pts.push({t:xMin,k:(kingK!=null?kingK:0)});
  if(pts[0].t>xMin) pts.unshift({t:xMin,k:pts[0].k});
  var ks=pts.map(function(p){return p.k;});
  var yLo=Math.min.apply(null,ks), yHi=Math.max.apply(null,ks);
  // (v10.23 Issue A) GUARANTEE price sits inside the axis with real headroom, so the
  // dashed price line NEVER falls outside the padded window and gets dropped. Fold px
  // in BEFORE padding AND force >=1 strike of margin beyond it on the relevant side.
  var pxRef = (px!=null) ? px : (STATE.SPY? STATE.SPY.price : null);   // last-known fallback if null
  var pxStale = (px==null);
  if(pxRef!=null){
    yLo=Math.min(yLo, pxRef-1);
    yHi=Math.max(yHi, pxRef+1);
  }
  if(yHi-yLo<1){ yLo-=1; yHi+=1; }
  var yPad=(yHi-yLo)*0.15; yLo-=yPad; yHi+=yPad;
  function X(t){ return padL+(t-xMin)/(xMax-xMin)*(W-padL-padR); }
  function Y(k){ return padT+(yHi-k)/(yHi-yLo)*(H-padT-padB); }
  var netDir=pts[pts.length-1].k-pts[0].k;
  // (v10.11) PER-SEGMENT coloring: each staircase step is colored by ITS OWN
  // local direction \u2014 green where the King stepped UP, red where it stepped DOWN,
  // gray for the flat baseline holds \u2014 so the line shows that the King was, say,
  // bearish (red) most of the session and only turned flat recently, instead of
  // painting the whole path one current-verdict color. (verdictCol is no longer
  // used for the stroke; kept in the signature for compatibility.)
  function segColor(dv){ return dv>0?PAL.longAccent : (dv<0?PAL.shortAccent : PAL.sub); }
  // (v10.18) HYSTERESIS regime: the LINE color reflects the King's running trend,
  // and only flips when the King reclaims the prior pivot (>=1 strike beyond it),
  // so minor 1-strike wiggles inside a clear trend don't recolor the line. The
  // regime per vertex is precomputed here; segments below use it instead of the
  // raw local jump. (Dots keep their own local up/down color.)
  var BUF=2;   // strikes beyond the prior pivot required to flip regime (>=2 = ignore single-strike wiggles)
  var regimeAt=new Array(pts.length);
  (function(){
    var reg=0, pivotHi=pts[0].k, pivotLo=pts[0].k;
    regimeAt[0]=0;
    for(var q=1;q<pts.length;q++){
      var k=pts[q].k;
      if(k>pivotHi) pivotHi=k;
      if(k<pivotLo) pivotLo=k;
      if(reg>=0 && k <= pivotHi-BUF){ reg=-1; pivotLo=k; pivotHi=k; }        // fell a strike below the run's high -> down regime
      else if(reg<=0 && k >= pivotLo+BUF){ reg=1; pivotHi=k; pivotLo=k; }    // reclaimed a strike above the run's low -> up regime
      else if(reg===0){ reg = k>pts[0].k?1:(k<pts[0].k?-1:0); }
      regimeAt[q]=reg;
    }
  })();
  function regColor(r){ return r>0?PAL.longAccent : (r<0?PAL.shortAccent : PAL.sub); }
  var svg='<svg width="'+W+'" height="'+H+'" viewBox="0 0 '+W+' '+H+'" preserveAspectRatio="xMidYMid meet" style="display:block;width:100%;height:auto">';
  // faint neutral fill under the whole staircase (context only, direction-agnostic)
  var baseY=(H-padB).toFixed(1);
  var fullD='M '+X(pts[0].t).toFixed(1)+' '+Y(pts[0].k).toFixed(1);
  for(var j=1;j<pts.length;j++){ fullD+=' H '+X(pts[j].t).toFixed(1)+' V '+Y(pts[j].k).toFixed(1); }
  fullD+=' H '+X(xMax).toFixed(1);
  var fillD=fullD+' L '+X(xMax).toFixed(1)+' '+baseY+' L '+X(pts[0].t).toFixed(1)+' '+baseY+' Z';
  svg+='<path d="'+fillD+'" fill="'+PAL.sub+'" opacity="0.05" stroke="none"/>';
  // (v10.23 Issue A) ALWAYS draw the price line. Clamp to the axis edge (with a small
  // caret) if price is beyond the padded range; dim it when price is stale (px null).
  var priceLineY=null;
  if(pxRef!=null){
    var clampedPx = Math.max(yLo, Math.min(yHi, pxRef));
    var beyond = (pxRef<yLo) ? -1 : (pxRef>yHi ? 1 : 0);
    var yp = Y(clampedPx);
    priceLineY = yp;
    var lineOp = pxStale ? 0.35 : 0.85;
    // (v10.27) price line made more distinct so it reads separately from the gold
    // King staircase even when they sit within a strike of each other: thicker,
    // longer dash, higher opacity, + a small blue price marker on the right end.
    svg+='<line x1="'+padL+'" y1="'+yp.toFixed(1)+'" x2="'+(W-padR)+'" y2="'+yp.toFixed(1)+'" stroke="'+PAL.blue+'" stroke-width="1.4" stroke-dasharray="4 3" opacity="'+lineOp+'"/>';
    if(beyond===0){ svg+='<circle cx="'+padL+'" cy="'+yp.toFixed(1)+'" r="2.2" fill="'+PAL.blue+'" opacity="'+lineOp+'"/>'; }
    if(beyond!==0){ // caret marking the line is clamped (real price is off-axis)
      var cX=(W-padR-10), cY=yp, dir=(beyond<0?1:-1);
      svg+='<path d="M '+cX+' '+(cY+dir*3).toFixed(1)+' L '+(cX+4)+' '+cY.toFixed(1)+' L '+(cX+8)+' '+(cY+dir*3).toFixed(1)+'" fill="none" stroke="'+PAL.blue+'" stroke-width="1" opacity="'+lineOp+'"/>'; }
  }
  // Draw each segment as its own colored stroke. Segment s (from vertex s-1 to s)
  // = a flat hold at pts[s-1].k then a vertical jump to pts[s].k; color by the
  // jump direction. The final hold (last vertex -> now) keeps the last direction.
  function strokeSeg(x1,y1,x2,y2,col){ return '<path d="M '+x1.toFixed(1)+' '+y1.toFixed(1)+' L '+x2.toFixed(1)+' '+y2.toFixed(1)+'" fill="none" stroke="'+col+'" stroke-width="1.8" stroke-linejoin="round" stroke-linecap="round"/>'; }
  var lastSegCol=PAL.sub;
  for(var s=1;s<pts.length;s++){
    var col=regColor(regimeAt[s]);                // (v10.18) color by hysteresis regime, not local dv
    var xPrev=X(pts[s-1].t), xCur=X(pts[s].t), yPrev=Y(pts[s-1].k), yCur=Y(pts[s].k);
    svg+=strokeSeg(xPrev,yPrev,xCur,yPrev,col);   // flat hold at previous strike
    svg+=strokeSeg(xCur,yPrev,xCur,yCur,col);     // vertical jump to new strike
    lastSegCol=col;
  }
  // trailing flat from the last vertex to 'now', in the last move's color
  var xl=X(pts[pts.length-1].t), yl=Y(pts[pts.length-1].k), xn=X(xMax);
  svg+=strokeSeg(xl,yl,xn,yl,lastSegCol);
  // roll vertices + gold current-King dot
  for(var v=1;v<pts.length;v++){ var cx=X(pts[v].t).toFixed(1), cy=Y(pts[v].k).toFixed(1); var up=pts[v].k>pts[v-1].k;
    svg+='<circle cx="'+cx+'" cy="'+cy+'" r="1.6" fill="'+(up?PAL.longAccent:PAL.shortAccent)+'"/>'; }
  var lx=X(xMax).toFixed(1), ly=Y(pts[pts.length-1].k).toFixed(1);
  svg+='<circle cx="'+lx+'" cy="'+ly+'" r="3" fill="'+PAL.gold+'" stroke="'+PAL.card+'" stroke-width="1"/>';
  // (v10.27) King + price labels: place them so they NEVER collide even when King and
  // price are within a strike of each other (the case that stacked them before).
  //   - King label rides at the gold dot's Y (right-anchored, gold, crown).
  //   - Price label rides at the price line's Y (right-anchored, blue).
  //   - If the two Ys are within a min gap, push them APART vertically (King toward
  //     its own line side, price the other way) so both stay legible and separate.
  var curK = pts[pts.length-1].k;
  function clampY(y){ return Math.max(padT+8, Math.min(H-padB-2, y)); }
  var kingLineY = parseFloat(ly);
  var kingLabelY = clampY(kingLineY);
  var priceLabelY = (priceLineY!=null) ? clampY(priceLineY) : null;
  var MINGAP=11;
  if(priceLabelY!=null && Math.abs(kingLabelY - priceLabelY) < MINGAP){
    // Too close -> separate around their midpoint: whichever sits HIGHER on the axis
    // (smaller Y) gets pinned MINGAP/2 above the midpoint, the other MINGAP/2 below.
    // Guarantees exactly >=MINGAP px between the two labels regardless of order.
    var mid = (kingLineY + priceLineY)/2;
    if(kingLineY <= priceLineY){ kingLabelY = clampY(mid - MINGAP/2); priceLabelY = clampY(mid + MINGAP/2); }
    else                       { priceLabelY = clampY(mid - MINGAP/2); kingLabelY = clampY(mid + MINGAP/2); }
  }
  // King label (gold) at the dot, nudged left so it doesn't run off the right edge.
  var kLx = Math.max(padL+2, parseFloat(lx)-5);
  svg+='<text x="'+kLx.toFixed(1)+'" y="'+kingLabelY.toFixed(1)+'" text-anchor="end" fill="'+PAL.gold+'" style="font-size:9px;font-weight:800;font-variant-numeric:tabular-nums">\uD83D\uDC51'+fmtNum(curK)+'</text>';
  // Price label (blue) at the RIGHT end: value + signed offset vs King, e.g. '777.1 (+1.2)'.
  if(pxRef!=null && priceLabelY!=null){
    var offv = (kingK!=null) ? (pxRef-kingK) : null;   // price relative to King
    var offTxt = (offv!=null) ? (' ('+(offv>=0?'+':'\u2212')+Math.abs(offv).toFixed(1)+')') : '';
    svg+='<text x="'+(W-padR-2)+'" y="'+priceLabelY.toFixed(1)+'" text-anchor="end" fill="'+PAL.blue+'" style="font-size:9px;font-weight:700;font-variant-numeric:tabular-nums">'+(pxStale?'':'')+fmtNum(pxRef)+offTxt+'</text>';
  }
  svg+='</svg>';
  return { svg:svg, yLo:yLo, yHi:yHi, netDir:netDir, firstK:pts[0].k, lastK:pts[pts.length-1].k };
}

// ---- KING VERDICT (v10.10): a bull/bear read from King positioning ----
// Combines the three independent King signals into ONE directional call plus a
// regime qualifier and a projected magnet target. Drift-dominant.
//   1) DRIFT  \u2014 net first\u2192current strike migration. Dealers forced to re-center
//      exposure HIGHER = bullish structural pressure; LOWER = bearish. Primary.
//   2) MAGNET \u2014 King vs price. Price gravitates to the King into expiry, so a
//      King ABOVE price pulls up (bullish lean, King = target above); BELOW
//      pulls down (bearish lean); AT price = pinned/range, no pull. Secondary.
//   3) STABILITY \u2014 pinned long + few rolls = range/mean-revert regime (magnet
//      strong, fade extremes); actively rolling = trending/unstable (go with
//      the drift). Does NOT flip sign \u2014 it scales conviction & names the regime.
// mv:[{k,dir,t}] oldest-first; kingK current strike; px price; now epoch ms.
function kingVerdict(mv, kingK, px, now){
  var firstK = (mv && mv.length) ? mv[0].k : kingK;
  var drift = (firstK!=null && kingK!=null) ? (kingK-firstK) : 0;
  // recent rolls (last 60m, real direction only) + pinned minutes
  var rolls60=0, lastT=null;
  if(mv){ for(var i=0;i<mv.length;i++){ if(mv[i].dir!==0 && mv[i].t>=now-3600000) rolls60++; lastT=mv[i].t; } }
  var pinnedM = (lastT!=null) ? Math.max(0, Math.round((now-lastT)/60000)) : null;
  // magnet side: +1 King above price (pull up), -1 below (pull down), 0 at price
  var gap = (px!=null && kingK!=null) ? (kingK-px) : 0;
  var magnet = Math.abs(gap)<0.5 ? 0 : (gap>0?1:-1);
  // ---- Score (drift-dominant). Each term bounded; sign = bull(+)/bear(-). ----
  var driftTerm = Math.max(-3, Math.min(3, drift));      // \u00b13 cap, 1 pt / strike
  var magnetTerm = magnet * 1.0;                          // lighter magnet lean
  var score = driftTerm*1.6 + magnetTerm;
  // Stability => conviction multiplier & regime word (does not change sign).
  var trending = rolls60>=2;
  var rangePinned = (pinnedM!=null && pinnedM>=20) && rolls60===0 && Math.abs(drift)<=1;
  var conv = trending ? 1.15 : (rangePinned ? 0.6 : 1);
  score *= conv;
  // Drift vs magnet disagreement (e.g. drifting up but sitting below price).
  var mixed = (driftTerm>0 && magnet<0) || (driftTerm<0 && magnet>0);
  var dir, cls, word;
  if(rangePinned && Math.abs(score)<1.2){ dir=0; cls='flat'; word='Neutral'; }
  else if(mixed && Math.abs(score)<1.4){ dir=0; cls='flat'; word='Mixed'; }
  else if(score>=1.2){ dir=1; cls='bull'; word='Bullish'; }
  else if(score<=-1.2){ dir=-1; cls='bear'; word='Bearish'; }
  else { dir=0; cls='flat'; word='Neutral'; }
  var regime = trending ? 'trending' : (rangePinned ? 'range-pinned' : 'settling');
  // Convergence: is price closing on the King (magnet winning) or leaving it?
  var conv2=null;
  if(mv && mv.length>=1 && px!=null){
    // approximate prior gap using the strike before the last roll if present
    conv2 = (Math.abs(gap) < 0.75) ? 'at magnet' : null;
  }
  return { dir:dir, cls:cls, word:word, regime:regime, drift:drift, magnet:magnet,
           mixed:mixed, rolls60:rolls60, pinnedM:pinnedM, gap:gap, score:score,
           firstK:firstK, atMagnet:(Math.abs(gap)<0.75) };
}

// ---- KING tracker section ----
// Shows the current King strike, its %-of-board magnitude behavior, and how it
// has moved through the session (rolling up/down = directional dealer intent).
function kingBlock(){
  var sym='SPY';
  var tp=tapeMap(sym);
  var kingK = (tp && typeof tp.king==='number') ? tp.king : null;
  var px = STATE[sym] ? STATE[sym].price : null;
  var kd=kingDay(sym);
  var mv = (kd && kd.moves) ? kd.moves : [];
  var moveCount = kd ? (kd.count||0) : 0;
  var now=Date.now();

  // ---- Stacked King/price badge (v10.15): one pill, King strike on top with a
  // signed offset vs. price, SPY price below. Replaces the old 3-badge cluster
  // (gold King price + distance chip + net-drift chip). Offset = King - price
  // rounded; color: King above price = resistance (red), below = support
  // (green), equal = gold/neutral \u2014 same convention as the sparkline dots.
  // (v10.19) King header redesign: NO "King" title text, badge CENTERED, crown
  // INSIDE the badge next to the King price. Offset shown as an ARROW only
  // (\u2191 King above price = green, \u2193 below = red), NOTHING when King == price.
  // Color fix: above=green (magnet pulls up), below=red (was inverted before).
  var offRaw = (px!=null && kingK!=null) ? (kingK - px) : null;
  var off = (offRaw!=null) ? Math.round(offRaw) : null;
  var kingPxBadge='';
  if(kingK!=null){
    var arrHtml='';
    if(off!=null && off!==0){
      var above = off>0;
      var arrC = above?PAL.longAccent:PAL.shortAccent;   // above price = GREEN, below = RED
      var arr = above?'\u2191':'\u2193';
      // (v10.23 Issue B) format = SIGN left, number, ARROW right => '+1\u2191' / '-1\u2193'.
      // Sign and arrow always agree (offset axis only \u2014 NOT drift). Drift stays in the King Path.
      var sign = above?'+':'\u2212';   // U+2212 minus for clean tabular alignment
      arrHtml='<span title="King is '+Math.abs(off)+' strike'+(Math.abs(off)===1?'':'s')+' '+(above?'ABOVE price':'BELOW price')+' (offset). + = above, \u2212 = below." style="color:'+arrC+';font-weight:800;font-size:12px;line-height:1;font-variant-numeric:tabular-nums">'+sign+Math.abs(off)+arr+'</span>';
    }
    kingPxBadge=
      '<span title="King strike '+fmtNum(kingK)+' vs SPY '+(px!=null?fmtNum(px):'\u2013')+'. Crown+top = King (settlement magnet), below = current price. Arrow: \u2191 King above price, \u2193 below; none when equal." '+
        'style="display:inline-flex;align-items:center;gap:4px;padding:3px 7px;border:1.5px solid '+PAL.gold+';border-radius:14px;background:'+PAL.card+';flex:0 1 auto;min-width:0">'+
        '<span style="font-size:12px;line-height:1">\uD83D\uDC51</span>'+
        '<span style="display:inline-flex;flex-direction:column;align-items:center;line-height:1.05">'+
          '<span style="color:'+PAL.gold+';font-weight:800;font-size:13px;font-variant-numeric:tabular-nums">'+fmtNum(kingK)+'</span>'+
          '<span style="width:100%;height:1px;background:'+PAL.line+';margin:1px 0"></span>'+
          '<span style="color:'+PAL.sub+';font-weight:700;font-size:10px;font-variant-numeric:tabular-nums">'+(px!=null?fmtNum(px):'\u2013')+'</span>'+
        '</span>'+
        arrHtml+
      '</span>';
  } else {
    kingPxBadge='<span style="color:'+PAL.sub+';font-size:11px">Waiting on tape\u2026</span>';
  }
  // ---- Header (v10.25): centered 3-MAGNET CLUSTER — green ★ strongest SUPPORT (left),
  // 👑 King (middle), red ★ strongest RESISTANCE (right). Trend badge REMOVED (the
  // regime/instruction line in the Node Map carries trend now). Below: the ①②③ icons.
  var nm = (typeof nodeMapModel==='function') ? (function(){ try{return nodeMapModel(sym);}catch(e){return null;} })() : null;
  function sideMagnet(node, isSup){
    var col = isSup?PAL.longAccent:PAL.shortAccent;
    var lbl = isSup?'\u2605 SUP':'\u2605 RES';
    if(!node){ return '<span style="display:inline-flex;flex-direction:column;align-items:center;justify-content:center;padding:3px 6px;min-height:40px;box-sizing:border-box;border-radius:14px;background:'+PAL.card+';border:1.5px solid '+PAL.line+';opacity:.6;flex:0 1 auto;min-width:0"><span style="font-size:7.5px;font-weight:800;line-height:1;color:'+PAL.sub+';letter-spacing:.3px">'+lbl+'</span><span style="width:100%;height:1px;background:'+PAL.line+';margin:2px 0"></span><span style="color:'+PAL.sub+';font-size:10px;line-height:1">\u2013</span></span>'; }
    var tip=(isSup?'Strongest SUPPORT magnet below price':'Strongest RESISTANCE magnet above price')+' \u2014 '+fmtNum(node.k)+' at '+node.pct+'% of King (size+build-rate+nearness blend). A strong magnet attracts price toward it.';
    return '<span title="'+tip.replace(/"/g,'')+'" style="display:inline-flex;flex-direction:column;align-items:center;justify-content:center;padding:3px 6px;min-height:40px;box-sizing:border-box;border-radius:14px;background:'+PAL.card+';border:1.5px solid '+col+';flex:0 1 auto;min-width:0">'+
      '<span style="font-size:7.5px;font-weight:800;line-height:1;color:'+col+';letter-spacing:.3px">'+lbl+'</span>'+
      '<span style="width:100%;height:1px;background:'+PAL.line+';margin:2px 0"></span>'+
      '<span style="line-height:1;white-space:nowrap"><span style="font-weight:800;font-size:12.5px;color:'+col+';font-variant-numeric:tabular-nums">'+fmtNum(node.k)+'</span> <span style="color:'+PAL.sub+';font-size:8.5px;font-weight:600;font-variant-numeric:tabular-nums">'+node.pct+'%</span></span>'+
    '</span>';
  }
  var supBadge = sideMagnet(nm?nm.strongSup:null, true);
  var resBadge = sideMagnet(nm?nm.strongRes:null, false);
  var html='<div style="display:flex;justify-content:space-between;align-items:center;gap:4px;margin:4px 0 3px 0;width:100%;box-sizing:border-box;padding:0 2px">'+
    supBadge+
    '<span title="KING: the strike with the largest absolute dealer exposure \u2014 the day\u2019s EOD settlement magnet." style="display:inline-flex;flex:0 1 auto;min-width:0">'+kingPxBadge+'</span>'+
    resBadge+
  '</div>'+
  // ①②③ info icons (Magnets / King / Range \u2014 the header trio)
  '<div style="display:flex;gap:5px;align-items:center;justify-content:center;margin:0 0 5px">'+
    '<span style="font-size:8.5px;color:'+PAL.sub+';font-weight:700;letter-spacing:.3px;margin-right:2px">5-STEP</span>'+
    stepIcon(1)+stepIcon(2)+stepIcon(3)+
  '</div>';
  if(kingK==null){
    html+='<div style="color:'+PAL.sub+';padding:2px 6px;font-size:11px">Waiting on tape\u2026</div>';
    return html;
  }

  // ---- Derived: pinned time still used for the CURRENT chip's hover only ----
  var lastRollT = mv.length ? mv[mv.length-1].t : null;
  var pinnedMs = (lastRollT!=null) ? (now-lastRollT) : null;
  function mins(ms){ return ms==null?null:Math.max(0,Math.round(ms/60000)); }
  var pinnedM = mins(pinnedMs);
  var pinnedTxt = (pinnedM==null)?'\u2013':(pinnedM>=60?((pinnedM/60).toFixed(1)+'h'):(pinnedM+'m'));

  // ---- KING PATH SPARKLINE (v10.8): stepped King-strike-vs-clock-time chart. ----
  // Replaces the old chip row. The staircase's SHAPE is the day's King trend:
  // climbing = bullish migration, descending = bearish, flat = pinned, sawtooth
  // = chop. X is real time (a 2h hold is a long flat left segment), so pace is
  // honest. Faint dashed line = current price; gold dot = current King.
  var sess=sessionBoundsCT();
  // ---- KING VERDICT (v10.10): bull/bear read from drift + magnet + stability ----
  var kv = kingVerdict(mv, kingK, px, now);
  var vCol = kv.cls==='bull'?PAL.longAccent : (kv.cls==='bear'?PAL.shortAccent : PAL.gold);
  var spark=kingSparkline(mv, kingK, px, now, sess, vCol);
  // Session-drift summary words for the title.
  var driftWord = spark.netDir>0?'up':(spark.netDir<0?'down':'flat');
  var driftCol  = spark.netDir>0?PAL.longAccent:(spark.netDir<0?PAL.shortAccent:PAL.sub);
  var nRolls = moveCount;
  var loStrike = fmtNum(Math.ceil(spark.yLo));
  var hiStrike = fmtNum(Math.floor(spark.yHi));
  // ---- Magnet-target read: the single most actionable King output. ----
  var magWord = kv.magnet>0 ? 'up' : (kv.magnet<0?'down':'');
  var magStr = Math.round(Math.abs(kv.gap));
  var magnetRead = kv.atMagnet
    ? ('Price is AT the King \u2014 pinned; expect it to act as a magnet/range center until the King rolls.')
    : ('King magnet pulls price '+magWord+' toward '+fmtNum(kingK)+' ('+magStr+' strike'+(magStr===1?'':'s')+' '+(kv.magnet>0?'above':'below')+').');
  // Verdict tooltip: spell out how drift + magnet + stability combined.
  var vWhy = (kv.word+' \u00b7 '+kv.regime+'. '+
    'Drift '+(kv.drift>0?'+':'')+kv.drift+' strike'+(Math.abs(kv.drift)===1?'':'s')+' ('+(kv.drift>0?'dealers migrating higher = bullish':kv.drift<0?'dealers migrating lower = bearish':'no migration')+'); '+
    'magnet '+(kv.magnet>0?'above price (pull up)':kv.magnet<0?'below price (pull down)':'at price (no pull)')+'; '+
    (kv.regime==='trending'?'actively rolling ('+kv.rolls60+' in 60m) = momentum regime, go WITH the drift':kv.regime==='range-pinned'?'pinned '+(kv.pinnedM||0)+'m with no drift = range regime, fade the extremes toward the King':'settling')+'.'+
    (kv.mixed?' NOTE: drift and magnet disagree \u2014 migration says one way while the immediate magnet pulls the other; treat as mixed until they align.':'')).replace(/"/g,'');
  var sparkTip=('King path today \u2014 strike vs. clock time across the cash session (8:30\u201315:00 CT). '+
    'Staircase steps at each confirmed roll, holds flat between, so a long flat = pinned and a tight cluster = rapid rolling. '+
    'Net drift '+fmtNum(spark.firstK)+' \u2192 '+fmtNum(spark.lastK)+' ('+driftWord+', '+nRolls+' roll'+(nRolls===1?'':'s')+'). '+
    'Line color = King verdict (green bullish, red bearish, yellow neutral/flat). Gold dot = current King; dashed = current price. '+vWhy).replace(/"/g,'');
  html+='<div id="gpts-kingpath" title="'+sparkTip+'" style="padding:5px 8px;background:'+PAL.card+';border:1px solid '+PAL.line+';border-radius:8px;margin:2px 0">'+
    '<div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:3px">'+
      '<span style="color:'+PAL.sub+';font-size:8px;font-weight:700;letter-spacing:.4px">KING PATH \u00b7 today</span>'+
      '<span style="display:flex;align-items:center;gap:5px">'+
        '<span style="font-size:8px;font-weight:700;color:'+driftCol+'">drift '+driftWord+' \u00b7 '+nRolls+' roll'+(nRolls===1?'':'s')+'</span>'+
        '<span title="'+vWhy+'" style="font-size:9px;font-weight:800;color:'+vCol+';padding:1px 7px;border:1px solid '+vCol+';border-radius:20px">'+kv.word+'</span>'+
      '</span>'+
    '</div>'+
    // y-extent label (top strike) / chart / (bottom strike) so the axis reads.
    '<div style="display:flex;align-items:stretch;gap:4px">'+
      '<div style="display:flex;flex-direction:column;justify-content:space-between;font-size:7.5px;color:'+PAL.sub+';font-variant-numeric:tabular-nums;padding:1px 0">'+
        '<span>'+hiStrike+'</span><span>'+loStrike+'</span>'+
      '</div>'+
      '<div style="flex:1;min-width:0">'+spark.svg+'</div>'+
    '</div>'+
    // x-extent labels: session open (or first-move) time on the left, now on right.
    '<div style="display:flex;justify-content:space-between;font-size:7.5px;color:'+PAL.sub+';margin-top:1px">'+
      '<span>'+fmtClock(spark.firstK!=null && mv.length?mv[0].t:sess.start)+'</span>'+
      '<span>now \u00b7 pinned '+pinnedTxt+'</span>'+
    '</div>'+
    // magnet-target read: the actionable price-prediction line.
    '<div title="Where the King magnet is likely to pull price. Into expiry, price gravitates toward the King strike; distance + side tell you the pull." style="margin-top:4px;padding-top:4px;border-top:1px solid '+PAL.line+';font-size:9.5px;line-height:1.35;color:'+PAL.ink+'">'+
      '<span style="color:'+vCol+';font-weight:800">\u25c9 '+kv.word+'</span> \u00b7 <span style="color:'+PAL.sub+'">'+magnetRead+'</span>'+
    '</div>'+
  '</div>';
  return html;
}

function accumBlock(){
  var fs = futureStructureSummary('SPY');
  // #5 ONLY SHOW STRIKES THAT MATTER. Instead of a fixed 3-above / 3-below
  // ladder padded with Steady placeholders, surface only nodes a trader needs
  // to act on: those actively Accumulating (Building) or Dissipating (Fading),
  // plus the in-play node and the King even if Steady (structural anchors).
  var inPlayK = (fs.inPlay && fs.inPlay.k!=null) ? fs.inPlay.k : null;
  function matters(r){
    if(r.state.label==='Building' || r.state.label==='Fading') return true; // something is happening
    if(r.role==='King') return true;                                        // structural anchor
    if(inPlayK!=null && Math.abs(r.k-inPlayK)<0.001) return true;            // the in-play node
    return false;                                                            // skip idle/Steady filler
  }
  // Cap each side so a very active tape can't blow the panel out; the vertical
  // scrollbar handles overflow beyond this.
  var rowsAbove = fs.above.filter(matters).slice(0,5);
  var rowsBelow = fs.below.filter(matters).slice(0,5);
  // DISPLAY order = price ladder, highest strike at top descending to lowest.
  // Resistance (above) must render highest-first, so reverse it (774,775,776 ->
  // 776,775,774). Support (below) is already descending (773,772,771). The
  // whole panel then reads: 776 775 774 [SPY] 773 772 771.
  rowsAbove = rowsAbove.slice().sort(function(a,b){ return b.k-a.k; });
  rowsBelow = rowsBelow.slice().sort(function(a,b){ return b.k-a.k; });
  var px = STATE.SPY.price;
  // ===== MERGED S/R BIAS BLOCK =====
  // #4 (v10.6) Combined into ONE header: "S/R Bias". The board-tilt headline
  // (Support-heavy / Resistance-heavy N\u00d7) rides on the right of this single
  // header \u2014 the separate "\u2696 Bias" sub-header was removed.
  // BOARD = srBattle() (v10.14): a DISSIPATION-DOMINANT support-vs-resistance
  // FORCE model (validated on the user's real tape). Unlike the old
  // netPositioning (which only summed BUILDING nodes and sat green all the way
  // down a grind), srBattle weights the NEAREST level DISSIPATING heavily \u2014 so
  // the bar flips red exactly at a bear-pullback high (floor giving way) and
  // green at a bounce low (ceiling giving way). It also emits a CROSSOVER flag.
  var srb = srBattle('SPY');
  var haveSrb = !!srb;
  var supPct = haveSrb ? srb.supPct : 50;
  var resPct = haveSrb ? srb.resPct : 50;
  var srDir = !haveSrb ? 0 : (srb.dom==='support'?1:(srb.dom==='resistance'?-1:0));
  var netCol = srDir>0?PAL.longAccent:(srDir<0?PAL.shortAccent:PAL.sub);
  // (v10.27) S/R IMBALANCE standalone section REMOVED. Its net-read ("Bearish/
  // Bullish imbalance \u2014 resistance X building, support Y fading") + the tradeable
  // crossover banner are now FOLDED INTO the Node Map header (see nodeMapBlock),
  // per the Step-5 consolidation. srb/haveSrb kept above for downstream reference.
  var html='';

  // #3 (v10.24 Issue I) NODE MAP replaces the abbreviated PROJ row. The Node Map
  // is the reshaped Projected-S/R: a two-sided price-anchored dealer-positioning
  // map (strongest ★ headline + per-level Bounce/Pullback/Break-through verdicts +
  // King marker + travel-emphasis). strongestAccumulator kept for back-compat refs.
  var haveProj = true;
  html+=nodeMapBlock();
  // (v10.24) EFFECTIVENESS CAPTURE: stash the emitted node-map model for the
  // recorder so predictive accuracy can be measured later (mandate: record from v1).
  try{ LASTNODEMAP.SPY = nodeMapModel('SPY'); }catch(e){}

  return html;   // (v10.27) old two-sided ladder + PROJ block REMOVED — fully superseded by nodeMapBlock() (Step-5 identity). accumBlock is now just the Node Map wrapper.
}

// #7 STALE-FEED GUARD. Freshness state from the age of the last feed update,
// with thresholds ADAPTIVE to the observed cadence (amber ~2x, red ~4x), so it
// self-tunes instead of guessing fixed seconds. Falls back to POLL_MS-based
// thresholds until we have observed a cadence.
var FEED_AGES = { SPY:[], QQQ:[] };
var FEED_LAST_TS = { SPY:0, QQQ:0 };
function observeFeedCadence(sym, ts){
  var prev=FEED_LAST_TS[sym];
  if(prev){ var gap=ts-prev; if(gap>0 && gap<600000){ FEED_AGES[sym].push(gap); if(FEED_AGES[sym].length>10) FEED_AGES[sym].shift(); } }
  FEED_LAST_TS[sym]=ts;
}
function feedCadenceMs(sym){
  var a=FEED_AGES[sym];
  if(!a || !a.length) return POLL_MS;
  var s=0; for(var i=0;i<a.length;i++) s+=a[i];
  return s/a.length;
}
// Returns { state:'live'|'amber'|'red'|'idle', age, col, label }.
function feedFreshness(sym){
  var f=LASTFEED[sym];
  if(!f) return { state:'idle', age:null, col:PAL.sub, label:'idle' };
  var age=Date.now()-f.ts;
  var cad=feedCadenceMs(sym);
  var amber=cad*2, red=cad*4;
  var state, col;
  if(age>=red){ state='red'; col=PAL.shortAccent; }
  else if(age>=amber){ state='amber'; col=PAL.amber; }
  else { state='live'; col=PAL.longAccent; }
  var secs=Math.round(age/1000);
  return { state:state, age:age, col:col, label:(secs<60?(secs+'s'):(Math.round(secs/60)+'m')) };
}
function feedStatusHtml(){
  var now=Date.now();
  var f=LASTFEED.SPY;
  var txt, col;
  var age=f?(now-f.ts):Infinity;
  var feedLive = f && age<=FEED_STALE_MS;
  if(feedLive){ txt='SPY:'+f.feed; col=PAL.longAccent; }
  else {
    // No live network feed — are we running off the visible DOM tape instead?
    var tp=tapeMap('SPY');
    var tapeLive = tp && tp.pct && tp.count>=3;
    if(tapeLive){ txt='SPY:tape'; col=PAL.blue; }        // fallback active, panel populated
    else if(f){ txt='SPY:stale'; col=PAL.amber; }
    else { txt='SPY:idle'; col=PAL.sub; }
  }
  // (v10.17) The 📥 Save Day button moved OFF the dashboard footer and INTO the
  // Analysis tab as an in-tab "Save & prep review" banner (the tab is the trigger).
  return '<div style="display:flex;justify-content:space-between;align-items:center;color:'+PAL.sub+';font-size:9px;letter-spacing:0.3px">'+
    '<span style="color:'+col+'">'+txt+'</span>'+
    '<span>feed v10.29</span>'+
    '</div>';
}

// ===== #10 ALERTS =====
// Fire on CONFIRMED state transitions only, with a per-key cooldown so an
// oscillating value cannot spam. Each event has independent on/visual/sound in
// CFG.alerts. Visual = a brief header pulse; sound = a short WebAudio beep.
var ALERT_PREV = {};                 // last seen state per tracked key
var ALERT_COOLDOWN = {};             // key -> ts of last fire
var ALERT_COOLDOWN_MS = 120000;      // 2 min per key
function beep(){
  try{
    var Ctx=window.AudioContext||window.webkitAudioContext; if(!Ctx) return;
    ALERT_AC = ALERT_AC || new Ctx();
    var o=ALERT_AC.createOscillator(), g=ALERT_AC.createGain();
    o.type='sine'; o.frequency.value=880; o.connect(g); g.connect(ALERT_AC.destination);
    g.gain.setValueAtTime(0.0001, ALERT_AC.currentTime);
    g.gain.exponentialRampToValueAtTime(0.12, ALERT_AC.currentTime+0.01);
    g.gain.exponentialRampToValueAtTime(0.0001, ALERT_AC.currentTime+0.25);
    o.start(); o.stop(ALERT_AC.currentTime+0.26);
  }catch(e){}
}
var ALERT_AC=null;
function pulseHeader(col){
  var p=document.getElementById('gpts-grade'); var host=p?p.parentNode:null;
  if(!host) return;
  try{
    host.style.transition='box-shadow .15s';
    host.style.boxShadow='0 0 0 2px '+col+' inset';
    setTimeout(function(){ host.style.boxShadow='none'; }, 700);
  }catch(e){}
}
function fireAlert(ev, key, col, label){
  var cfg=CFG.alerts[ev]; if(!cfg || !cfg.on) return;
  var ck=ev+':'+key, now=Date.now();
  if(ALERT_COOLDOWN[ck] && (now-ALERT_COOLDOWN[ck])<ALERT_COOLDOWN_MS) return;
  ALERT_COOLDOWN[ck]=now;
  if(cfg.vis) pulseHeader(col);
  if(cfg.snd) beep();
}
function runAlerts(sym, fr){
  if(!CFG.alerts) return;
  // feed stale (state transition to red)
  var prevFeed=ALERT_PREV['feed']; ALERT_PREV['feed']=fr.state;
  if(fr.state==='red' && prevFeed && prevFeed!=='red') fireAlert('feedStale','SPY',PAL.shortAccent);
  // King roll
  var kd=kingDay(sym); var kc=kd?kd.count:0;
  var prevKC=ALERT_PREV['kingCount']; ALERT_PREV['kingCount']=kc;
  if(typeof prevKC==='number' && kc>prevKC) fireAlert('kingRoll','SPY',PAL.gold);
  // in-play node -> Accumulating, and dissipate/absorption on the in-play node
  var anc=resolveAnchor(sym);
  if(anc && anc.ok && anc.active){
    var k=anc.active.k;
    var h=nodeHistory(sym,k);
    var ft=(h&&h.length>=2)?histTrend(h, liveNodePct(sym,k)):null;
    var lbl=ft?ft.label:'Steady';
    var pk='node:'+k.toFixed(2);
    var prevLbl=ALERT_PREV[pk]; ALERT_PREV[pk]=lbl;
    if(prevLbl && prevLbl!=='Building' && lbl==='Building') fireAlert('inplayAccum',k.toFixed(2),PAL.longAccent);
    if(prevLbl && prevLbl!=='Fading' && lbl==='Fading') fireAlert('dissipate',k.toFixed(2),PAL.shortAccent);
    var ab=absorptionAt(sym,k);
    var pkA='absorb:'+k.toFixed(2); var prevAb=ALERT_PREV[pkA]; ALERT_PREV[pkA]=ab?1:0;
    if(ab && !prevAb) fireAlert('absorption',k.toFixed(2),(anc.active.k>=STATE[sym].price?PAL.shortAccent:PAL.longAccent));
  }
  // trap read forms/flips
  var rel=relativeRead(sym);
  if(rel){ var prevRel=ALERT_PREV['trap']; ALERT_PREV['trap']=rel.cls;
    if(prevRel && prevRel!==rel.cls && (rel.cls==='trap'||rel.cls==='long')) fireAlert('trap','SPY',(rel.cls==='trap'?PAL.shortAccent:PAL.longAccent)); }
}

// ===== CONFLUENCE ENGINE (v10.13) =====
// Integrates the four independent reads into ONE directional thesis + an aligned
// count, so the panel tells a single coherent story instead of four scattered
// readouts. Four contributors, each votes -1 (bearish) / +1 (bullish) / 0:
//   \u2460 LEAN    (King)  \u2014 kingVerdict().dir. The structural anchor. weight 1.2
//   \u2461 TRIGGER (Price) \u2014 trendVerdict state + live BO/short setup. price must
//                        confirm the move. weight 1.2
//   \u2462 CONTEXT (Board) \u2014 netPositioning().dir. net board tilt. weight 0.8
//   \u2463 BREADTH (Nodes) \u2014 MULTI-NODE: counts building resistance overhead &
//                        dissipating support below (bearish) vs building support
//                        below & dissipating resistance above (bullish). One node
//                        is weak; several agreeing across strikes is strong.
//                        weight scales with how many nodes agree (up to 1.2).
// Thesis declares a direction only at >=3/4 contributors aligned (weighted);
// otherwise "Mixed / No edge" \u2014 a confluence tool earns trust by staying quiet.
function nodeBreadth(sym){
  // Aggregate ALL nearby nodes into a directional node-bias.
  var fs=futureStructureSummary(sym);
  var above=fs.above||[], below=fs.below||[];
  function isBuild(r){ return r.state && r.state.label==='Building'; }
  function isFade(r){ return r.state && r.state.label==='Fading'; }
  // Bearish node evidence: resistance building overhead + support dissipating below.
  var resBuild=0, supFade=0;
  // Bullish node evidence: support building below + resistance dissipating overhead.
  var supBuild=0, resFade=0;
  above.forEach(function(r){ if(isBuild(r)) resBuild++; else if(isFade(r)) resFade++; });
  below.forEach(function(r){ if(isBuild(r)) supBuild++; else if(isFade(r)) supFade++; });
  var bearScore = resBuild + supFade;   // things pushing price DOWN
  var bullScore = supBuild + resFade;   // things pushing price UP
  var net = bullScore - bearScore;
  var dir = net>0?1:(net<0?-1:0);
  // magnitude 0..1: how lopsided, capped so 3+ agreeing nodes = ~full weight
  var mag = Math.min(1, Math.abs(net)/3);
  return { dir:dir, mag:mag, resBuild:resBuild, supFade:supFade, supBuild:supBuild,
           resFade:resFade, bearScore:bearScore, bullScore:bullScore, net:net };
}
function confluence(sym){
  sym=sym||'SPY';
  var S=STATE[sym]||{};
  var px=S.price;
  // \u2460 LEAN \u2014 King
  var kd=kingDay(sym); var mv=(kd&&kd.moves)?kd.moves:[];
  var tp=tapeMap(sym); var kingK=(tp&&typeof tp.king==='number')?tp.king:null;
  var kv=kingVerdict(mv, kingK, px, Date.now());
  var lean = kv.dir;                                   // -1/0/+1
  // \u2461 TRIGGER \u2014 price trend + live directional setup
  var tv=trendVerdict(sym);
  var trendDir = tv.state==='up'?1:(tv.state==='dn'?-1:0);
  var bias=signalBias(sym);
  var setupDir = (bias.short && !bias.long)?-1:((bias.long && !bias.short)?1:0);
  // price vote: trend, escalated when a live setup agrees; setup alone if flat
  var trigger = trendDir!==0 ? trendDir : setupDir;
  var triggerStrong = (trendDir!==0 && setupDir===trendDir);
  // \u2462 CONTEXT \u2014 board tilt
  var np=netPositioning(sym);
  var board = (np && np.bias!=='balanced') ? np.dir : 0;
  // \u2463 BREADTH \u2014 the S/R BATTLE (v10.14). Dissipation-dominant force model:
  // support-dominant => bull(+1), resistance-dominant => bear(-1). This replaces
  // the crude Building/Fading count for the confluence vote so the whole stack
  // reads the same battle the S/R Bias bar shows. nodeBreadth kept as magnitude.
  var srb=srBattle(sym);
  var nb=nodeBreadth(sym);
  var nodes = srb ? (srb.dom==='support'?1:(srb.dom==='resistance'?-1:0)) : nb.dir;
  // a fresh crossover intensifies the breadth weight (the tradeable moment)
  var breadthMag = srb ? (srb.cross?1.0:0.75) : (0.4+0.6*nb.mag);
  // ---- Weighted vote ----
  var W={ lean:1.2, trigger:1.2, board:0.8, nodes:1.2 };
  var contribs=[
    { key:'lean',    role:'LEAN',    dir:lean,    w:W.lean },
    { key:'trigger', role:'TRIGGER', dir:trigger, w:W.trigger*(triggerStrong?1.15:1) },
    { key:'board',   role:'CONTEXT', dir:board,   w:W.board },
    { key:'nodes',   role:'BREADTH', dir:nodes,   w:W.nodes*breadthMag }
  ];
  var score=0, bull=0, bear=0, voting=0;
  contribs.forEach(function(c){ score+=c.dir*c.w; if(c.dir>0){bull++;voting++;} else if(c.dir<0){bear++;voting++;} });
  // Thesis direction = sign of weighted score; require >=3 of 4 on that side.
  var dir = score>0?1:(score<0?-1:0);
  var agree = dir>0?bull:(dir<0?bear:0);
  var declared = (dir!==0 && agree>=3);
  var thesisDir = declared ? dir : 0;
  var word = thesisDir>0?'BULLISH':(thesisDir<0?'BEARISH':'MIXED / NO EDGE');
  return { dir:thesisDir, declared:declared, word:word, score:score,
           aligned:agree, voting:4, bull:bull, bear:bear,
           contribs:contribs, kv:kv, nb:nb, np:np, tv:tv, bias:bias,
           kingK:kingK, px:px };
}
// ============================================================================
// READ (v10.18) — replaces the old MIXED/NO EDGE confluence strip + 4 badges.
// A plain-language summary of the THREE signals we kept (King, Trend, S/R).
// No vote, no badges, no CONTEXT voter (dropped — proved wrong on 2026-08-11).
// Leads with the dominant/most-directional signal, then names any conflict in
// plain words, and colors the left border by the net lean (green/red/amber).
// ============================================================================
function readBlock(sym){
  sym=sym||'SPY';
  var S=STATE[sym]||{}; var px=S.price;
  // --- the three reads ---
  var kd=kingDay(sym); var mv=(kd&&kd.moves)?kd.moves:[];
  var tp=tapeMap(sym); var kingK=(tp&&typeof tp.king==='number')?tp.king:null;
  var kv=kingVerdict(mv, kingK, px, Date.now());       // King: dir -1/0/+1, word
  var tv=trendVerdict(sym);                            // Trend: 5-state machine
  var srb=srBattle(sym);                               // S/R: dom support/resistance/balanced
  // numeric leans
  var kingDir=kv.dir||0;
  var trendDir = tv.state==='up'?1:(tv.state==='dn'?-1:0);
  var trendBroken = (tv.state==='up-broken'||tv.state==='dn-broken');
  var srDir = srb ? (srb.dom==='support'?1:(srb.dom==='resistance'?-1:0)) : 0;
  // net lean for the border color (simple sum of the three)
  var net = kingDir+trendDir+srDir;
  var leanCol = trendBroken ? PAL.amber : (net>0?PAL.longAccent:(net<0?PAL.shortAccent:PAL.gold));
  // --- headline: lead with the dominant piece ---
  // Priority: a CONFIRMED trend leads; else a broken trend (caution) leads; else
  // the King lean; else S/R; else "no clear edge".
  var head, headCol;
  if(tv.state==='up'||tv.state==='dn'){ head=trendWordOf(tv.state)+' ('+tv.dom+'/'+tv.win+')'; headCol=trendColorOf(tv.state); }
  else if(trendBroken){ head=trendWordOf(tv.state); headCol=PAL.amber; }
  else if(kingDir!==0){ head='King '+(kingDir>0?'bullish':'bearish'); headCol=(kingDir>0?PAL.longAccent:PAL.shortAccent); }
  else if(srDir!==0){ head=(srDir>0?'Support-led':'Resistance-led'); headCol=(srDir>0?PAL.longAccent:PAL.shortAccent); }
  else { head='No clear edge'; headCol=PAL.gold; }
  // --- body sentences (plain language, no jargon badges) ---
  function kingPhrase(){
    if(kingK==null) return 'King unset';
    var off=(px!=null)?Math.round(kingK-px):null;
    var side=off==null?'':(off>0?(Math.abs(off)+' above'):off<0?(Math.abs(off)+' below'):'at price');
    var lean=kingDir>0?'pulling up':kingDir<0?'pulling down':'neutral';
    return 'King '+fmtNum(kingK)+' '+(side?('('+side+') '):'')+lean;
  }
  function srPhrase(){
    if(!srb) return 'S/R forming';
    if(srb.dom==='support') return 'support winning'+(srb.cross==='bulls'?' (just flipped \u2014 bounce)':'');
    if(srb.dom==='resistance') return 'resistance winning'+(srb.cross==='bears'?' (just flipped \u2014 pullback-high)':'');
    return 'S/R balanced';
  }
  function trendPhrase(){
    if(tv.state==='up'||tv.state==='dn') return trendWordOf(tv.state).toLowerCase()+' '+tv.dom+'/'+tv.win+' bars';
    if(trendBroken) return trendWordOf(tv.state).toLowerCase()+' \u2014 momentum paused, watch for reversal';
    if(tv.state==='na') return 'trend N/A (need more bars)';
    return 'no trend (chop)';
  }
  // conflict detection among the directional reads
  var dirs=[kingDir,trendDir,srDir].filter(function(d){return d!==0;});
  var allAgreePos = dirs.length && dirs.every(function(d){return d>0;});
  var allAgreeNeg = dirs.length && dirs.every(function(d){return d<0;});
  var conflict = !(allAgreePos||allAgreeNeg) && dirs.length>=2;
  var body;
  if(trendBroken){
    body='Trend '+trendPhrase()+'. King '+ (kingDir>0?'still leaning up':kingDir<0?'leaning down':'neutral') +', '+srPhrase()+'. Stand aside until the next trend confirms.';
  } else if(allAgreePos){
    body='All three align bullish \u2014 '+trendPhrase()+', '+kingPhrase()+', '+srPhrase()+'. Favor longs on pullbacks into support.';
  } else if(allAgreeNeg){
    body='All three align bearish \u2014 '+trendPhrase()+', '+kingPhrase()+', '+srPhrase()+'. Favor shorts on pullbacks into resistance.';
  } else if(conflict){
    body='Signals mixed \u2014 '+trendPhrase()+', '+kingPhrase()+', '+srPhrase()+'. No clean edge; wait for them to line up.';
  } else {
    body=trendPhrase()+'. '+kingPhrase()+'. '+srPhrase()+'.';
  }
  var tip=('READ \u2014 plain-language synthesis of the three signals we track: Trend (16/20 state machine), King (dealer lean), and S/R battle. No vote; it just tells you what the sections are collectively saying and whether they agree.').replace(/"/g,'');
  return '<div title="'+tip+'" style="background:'+PAL.card+';border:1px solid '+PAL.line+';border-left:3px solid '+leanCol+';border-radius:8px;padding:6px 9px;margin:0 0 3px 0">'+
    '<div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:3px">'+
      '<span style="font-size:12px;font-weight:800;color:'+headCol+';letter-spacing:.3px">READ</span>'+
      '<span style="font-size:9.5px;font-weight:700;color:'+headCol+'">'+head+'</span>'+
    '</div>'+
    '<div style="font-size:9.5px;line-height:1.4;color:'+PAL.ink+'">'+body+'</div>'+
  '</div>';
}

// Human sentence for the confluence thesis (used by the top strip + tooltip).
function confluenceThesis(cf, sym){
  var d=cf.dir;
  function agreeMark(c){ if(c.dir===0) return '\u2013'; return (c.dir===d && d!==0)?'\u2713':(d!==0?'\u2717':'\u2013'); }
  var parts=[];
  cf.contribs.forEach(function(c){
    var w = c.dir>0?'bull':(c.dir<0?'bear':'flat');
    parts.push(c.role+' '+agreeMark(c)+' '+w);
  });
  // action line
  var action='';
  if(cf.declared){
    var kK=cf.kingK!=null?fmtNum(cf.kingK):'King';
    if(d<0){
      var resK=(cf.nb && cf.np) ? null : null;
      action='Short rallies into overhead resistance; magnet target '+kK+'.';
    } else {
      action='Buy dips into support below; magnet target '+kK+'.';
    }
  } else {
    action='Signals conflict \u2014 stand aside until they align.';
  }
  return { parts:parts, action:action };
}
// Top CONFLUENCE strip: one-line thesis + aligned count + action, colored to
// the thesis. This IS the coherence layer \u2014 it states the integrated read the
// three sections below then support in order (King=LEAN, S/R Bias=CONTEXT/
// BREADTH, BO=TRIGGER).
function confluenceStrip(sym){
  var cf=confluence(sym);
  var th=confluenceThesis(cf, sym);
  var col = cf.dir>0?PAL.longAccent:(cf.dir<0?PAL.shortAccent:PAL.gold);
  var arrow = cf.dir>0?'\u25B2':(cf.dir<0?'\u25BC':'\u25C6');
  var alignTxt = cf.declared ? (cf.aligned+'/4 aligned') : ((Math.max(cf.bull,cf.bear))+'/4 \u2014 mixed');
  // contributor chips: role + \u2713/\u2717/\u2013 colored per agreement with the thesis
  var chips='';
  cf.contribs.forEach(function(c){
    var mark = c.dir===0?'\u2013':((c.dir===cf.dir && cf.dir!==0)?'\u2713':(cf.dir!==0?'\u2717':'\u2013'));
    var cc = c.dir===0?PAL.sub : (c.dir>0?PAL.longAccent:PAL.shortAccent);
    // when a contributor disagrees with a declared thesis, show it red-\u2717
    var showCol = (cf.dir!==0 && c.dir!==0 && c.dir!==cf.dir) ? PAL.shortAccent : cc;
    chips+='<span style="font-size:8px;font-weight:700;color:'+showCol+';padding:0 4px;border:1px solid '+showCol+';border-radius:20px;white-space:nowrap">'+c.role+' '+mark+'</span>';
  });
  var tip=('CONFLUENCE \u2014 the integrated read from all four signals. '+
    'LEAN = King dealer lean; TRIGGER = price trend + live BO/short setup; CONTEXT = board support/resistance tilt; BREADTH = multiple nodes (resistance building overhead & support dissipating = bearish, and vice-versa). '+
    '\u2713 = agrees with the thesis, \u2717 = disagrees, \u2013 = neutral. A direction is declared only when \u22653 of 4 align; otherwise "mixed / no edge". '+
    th.action).replace(/"/g,'');
  var html='<div title="'+tip+'" style="background:linear-gradient(180deg,'+PAL.card+','+PAL.bg+');border:1px solid '+col+';border-left:3px solid '+col+';border-radius:8px;padding:5px 9px;margin:0 0 3px 0">'+
    '<div style="display:flex;justify-content:space-between;align-items:center;gap:6px;margin-bottom:3px">'+
      '<span style="font-size:12px;font-weight:800;color:'+col+';letter-spacing:.3px">'+arrow+' '+cf.word+'</span>'+
      '<span style="font-size:8.5px;font-weight:700;color:'+PAL.sub+'">'+alignTxt+'</span>'+
    '</div>'+
    '<div style="display:flex;gap:3px;flex-wrap:wrap;margin-bottom:3px">'+chips+'</div>'+
    '<div style="font-size:9.5px;line-height:1.35;color:'+PAL.ink+'">'+th.action+'</div>'+
  '</div>';
  return html;
}

// ============================================================================
// ANALYSIS TAB (v10.16) — in-app end-of-day review dashboard.
// Reads the recorder's LABELED snapshots (sig vector + out5/out10 forward
// outcomes) and computes the review stats live. If an LLM review file has been
// loaded (ANALYSIS_REVIEW), its narrative fields (why/discoveries/recs) are
// shown; otherwise those panels show an honest "awaiting review" state while
// the numeric panels still render from the raw labeled data.
// ============================================================================
var ANALYSIS_VIEW=false;              // false = Dashboard, true = Analysis tab
var ANALYSIS_SYM='SPY';
var ANALYSIS_REVIEW=null;             // optional loaded LLM review {why,discoveries,recs,grade,...}
window.__gptsDebug=window.__gptsDebug||{};
window.__gptsDebug.setReview=function(obj){ ANALYSIS_REVIEW=obj; if(typeof render==='function') render(); return 'review loaded'; };
window.__gptsDebug.showAnalysis=function(b){ ANALYSIS_VIEW=(b!==false); if(typeof render==='function') render(); };
// (v10.21) LOADER: import a past day's export + structured review so the tab
// renders that day instead of the empty live session. loadDay(json) sets the
// snapshots source; loadReview(obj) sets the narrative. clearLoaded() reverts.
var LOADED_DAY=null;   // parsed gex_YYYY-MM-DD.json to analyze instead of live tape
window.__gptsDebug.loadDay=function(json){ try{ LOADED_DAY=(typeof json==='string')?JSON.parse(json):json; ANALYSIS_VIEW=true; if(typeof render==='function') render(); return 'loaded day '+(LOADED_DAY&&LOADED_DAY.date); }catch(e){ return 'loadDay failed: '+e; } };
window.__gptsDebug.loadReview=function(obj){ ANALYSIS_REVIEW=obj; if(typeof render==='function') render(); return 'review loaded'; };
window.__gptsDebug.clearLoaded=function(){ LOADED_DAY=null; ANALYSIS_REVIEW=null; if(typeof render==='function') render(); return 'cleared'; };

// ============================================================================
// (v10.21) DOC-DERIVED ANALYTICS CORE — pure functions over a day-export object.
// Composes our primitives into the Skylit methodology vocabulary (regimes,
// King behavior, accumulation/dissipation/combined edges). Validated against
// the real 2026-08-11 capture. All defensive: partial data => nulls, never throw.
// Source day = LOADED_DAY if set, else the live buildDayExport().
// ============================================================================
function A_day(){ return LOADED_DAY || (typeof buildDayExport==='function'?buildDayExport():{snaps:{}}); }
function A_num(x){ return (typeof x==='number' && isFinite(x)) ? x : null; }
function A_pct(hit,n){ return n>0 ? Math.round(100*hit/n) : null; }
function A_sideOf(node, px){ var k=A_num(node.k); if(k==null||px==null) return null; if(k<px-0.001) return 'below'; if(k>px+0.001) return 'above'; return null; }

// ---- KING BEHAVIOR: path, rolls (lead/lag), offset posture, reach, pin, converge
function A_kingBehavior(day, sym){
  sym=sym||'SPY';
  var snaps=(day&&day.snaps&&day.snaps[sym])?day.snaps[sym]:[];
  var out={ sym:sym, pts:0, levels:[], distinct:0, firstK:null, lastK:null, netDrift:null,
            rolls:0, rollUp:0, rollDn:0, avgRollSize:null, regimes:[],
            reachRate:null, reachN:0, reachHit:0, avgTimeToReach:null, convergeRate:null,
            offsetAvg:null, aboveBars:0, belowBars:0, atBars:0, pullDir:null,
            pinned:null, pinDist:null, pinTiming:null, closeK:null, closePx:null };
  if(!snaps.length) return out;
  var path=[]; snaps.forEach(function(s,i){ var k=A_num(s.tking), p=A_num(s.px); if(k==null) return; path.push({i:i,bar:s.bar,k:k,px:p}); });
  out.pts=path.length; if(!path.length) return out;
  out.firstK=path[0].k; out.lastK=path[path.length-1].k; out.netDrift=+(out.lastK-out.firstK).toFixed(2);
  var kset={}, dwell={}; path.forEach(function(p){ kset[p.k]=1; dwell[p.k]=(dwell[p.k]||0)+1; });
  out.levels=Object.keys(kset).map(Number).sort(function(a,b){return a-b;}); out.distinct=out.levels.length;
  out.dwell=dwell;
  // CORE = strikes the King actually HELD (>=2 bars), ignoring 1-bar outliers like
  // an EOD pin-roll. Range/tightness for regime classification uses the core.
  var core=out.levels.filter(function(k){ return dwell[k]>=2; }); if(!core.length) core=out.levels.slice();
  out.core=core; out.coreRange=core.length?(core[core.length-1]-core[0]):0; out.coreCount=core.length;
  var rollSizes=[], prev=null;
  path.forEach(function(p){ if(prev!=null && p.k!==prev){ out.rolls++; var d=p.k-prev; rollSizes.push(Math.abs(d)); if(d>0) out.rollUp++; else out.rollDn++; } prev=p.k; });
  out.avgRollSize=rollSizes.length? +(rollSizes.reduce(function(a,b){return a+b;},0)/rollSizes.length).toFixed(2):null;
  var offs=[]; path.forEach(function(p){ if(p.px==null) return; var off=p.k-p.px; offs.push(off); if(off>0.001) out.aboveBars++; else if(off<-0.001) out.belowBars++; else out.atBars++; });
  if(offs.length){ out.offsetAvg=+(offs.reduce(function(a,b){return a+b;},0)/offs.length).toFixed(2); out.pullDir=out.offsetAvg>0.1?'up':(out.offsetAvg<-0.1?'down':'flat'); }
  var regimes=[], cur=null;
  path.forEach(function(p){ if(!cur||p.k!==cur.k){ if(cur) regimes.push(cur); cur={k:p.k,startI:p.i,bars:0,reached:false,reachBars:null,gapStart:null,gapEnd:null}; } cur.bars++; if(p.px!=null){ var gap=Math.abs(p.k-p.px); if(cur.gapStart==null)cur.gapStart=gap; cur.gapEnd=gap; if(gap<=0.25&&!cur.reached){ cur.reached=true; cur.reachBars=cur.bars-1; } } });
  if(cur) regimes.push(cur); out.regimes=regimes;
  var reachN=0,reachHit=0,ttr=[],conv=0,convN=0;
  regimes.forEach(function(r){ if(r.gapStart!=null&&r.gapStart>0.25){ reachN++; if(r.reached){ reachHit++; if(r.reachBars!=null)ttr.push(r.reachBars);} if(r.gapEnd!=null){ convN++; if(r.gapEnd<r.gapStart)conv++; } } });
  out.reachN=reachN; out.reachHit=reachHit; out.reachRate=A_pct(reachHit,reachN);
  out.avgTimeToReach=ttr.length? +(ttr.reduce(function(a,b){return a+b;},0)/ttr.length).toFixed(1):null;
  out.convergeRate=A_pct(conv,convN);
  // PIN: where did price CLOSE vs the day's final King (doc: late reach=pin, early=drive-off)
  var lastWithPx=null; for(var i=path.length-1;i>=0;i--){ if(path[i].px!=null){ lastWithPx=path[i]; break; } }
  if(lastWithPx){ out.closeK=lastWithPx.k; out.closePx=lastWithPx.px; out.pinDist=+Math.abs(lastWithPx.k-lastWithPx.px).toFixed(2);
    out.pinned=out.pinDist<=1.0; // ~0.5-1.0pt SPY zone (docs: ~5-10pt SPX)
    var lastReg=regimes[regimes.length-1]; var frac=lastReg&&path.length? (lastReg.startI/(path.length-1)) : 1;
    out.pinTiming=(frac>=0.66)?'late':(frac<=0.33?'early':'mid');
  }
  return out;
}

// ---- ACCUMULATION / DISSIPATION EDGE (validated on 8/11) ----
function A_accumEdge(day, sym, mode){
  sym=sym||'SPY'; var snaps=(day&&day.snaps&&day.snaps[sym])?day.snaps[sym]:[];
  var B={ sup:{n:0,dh:0,sh:0,mfe:0,mae:0}, res:{n:0,dh:0,sh:0,mfe:0,mae:0} };
  var baseN=0, baseUp=0;
  snaps.forEach(function(s){ var o=s.out10; if(!o) return; var px=A_num(s.px); if(px==null) return; baseN++; if(o.net>0) baseUp++;
    (s.nodes||[]).forEach(function(nd){ var side=A_sideOf(nd,px); if(!side) return; var net=A_num(nd.net); if(net==null) return;
      var want=(mode==='accum')?(net>0):(net<0); if(!want) return;
      var expUp=(mode==='accum')?(side==='below'):(side==='above');
      var b=(side==='below')?B.sup:B.res; b.n++;
      var hit=expUp?(o.net>0):(o.net<0); if(hit) b.dh++;
      var mfe=A_num(o.mfe)||0, mae=A_num(o.mae)||0; b.mfe+=mfe; b.mae+=mae;
      var sw=expUp?(mfe> -mae):(-mae>mfe); if(sw) b.sh++;
    }); });
  function pk(b){ return { n:b.n, dirHit:A_pct(b.dh,b.n), swingHit:A_pct(b.sh,b.n), avgMFE:b.n?+(b.mfe/b.n).toFixed(2):null, avgMAE:b.n?+(b.mae/b.n).toFixed(2):null }; }
  return { mode:mode, baseline:A_pct(baseUp,baseN), baseN:baseN, support:pk(B.sup), resistance:pk(B.res) };
}

// ---- COMBINED / INTERACTION EDGE ----
function A_combinedEdge(day, sym){
  sym=sym||'SPY'; var snaps=(day&&day.snaps&&day.snaps[sym])?day.snaps[sym]:[];
  var trap={n:0,hit:0}, lift={n:0,hit:0}, comp={n:0,range:0}, flow={n:0,hit:0}; var baseN=0,baseUp=0;
  snaps.forEach(function(s){ var o=s.out10; if(!o) return; var px=A_num(s.px); if(px==null) return; baseN++; if(o.net>0) baseUp++;
    var rB=false,rF=false,sB=false,sF=false,pol=0;
    (s.nodes||[]).forEach(function(nd){ var side=A_sideOf(nd,px); var net=A_num(nd.net); if(!side||net==null) return;
      if(side==='above'){ if(net>0){rB=true;pol-=Math.abs(net);} else if(net<0){rF=true;pol+=Math.abs(net);} }
      else { if(net>0){sB=true;pol+=Math.abs(net);} else if(net<0){sF=true;pol-=Math.abs(net);} } });
    if(rB&&sF){ trap.n++; if(o.net<0) trap.hit++; }
    if(sB&&rF){ lift.n++; if(o.net>0) lift.hit++; }
    if(sB&&rB){ comp.n++; if(Math.abs(o.net)<0.15||(o.revUp&&o.revDn)) comp.range++; }
    if(Math.abs(pol)>0.001){ flow.n++; var pu=pol>0; if((pu&&o.net>0)||(!pu&&o.net<0)) flow.hit++; } });
  return { baseline:A_pct(baseUp,baseN), baseN:baseN,
    trapdoor:{n:trap.n,hit:A_pct(trap.hit,trap.n)}, liftoff:{n:lift.n,hit:A_pct(lift.hit,lift.n)},
    compression:{n:comp.n,rangeRate:A_pct(comp.range,comp.n)},
    dualVsSingle:{dualN:trap.n+lift.n,dualHit:A_pct(trap.hit+lift.hit,trap.n+lift.n)},
    netFlow:{n:flow.n,dirHit:A_pct(flow.hit,flow.n)} };
}

// ---- REGIME CLASSIFIER: compose primitives into the docs' day-types.
// Trend / Whipsaw / Rainbow Road (+ tradeability). Returns {label,conf,why}.
function A_regime(day, sym){
  sym=sym||'SPY'; var kb=A_kingBehavior(day,sym); var ce=A_combinedEdge(day,sym);
  var snaps=(day&&day.snaps&&day.snaps[sym])?day.snaps[sym]:[];
  if(kb.pts<6) return { label:'Forming', conf:'low', why:'Not enough King bars yet to classify the day ('+kb.pts+' pts).', kb:kb };
  // node dispersion: are heavy nodes concentrated at two edges (Whipsaw) or scattered (Rainbow)?
  var strikes={}; snaps.forEach(function(s){ (s.nodes||[]).forEach(function(nd){ var k=A_num(nd.k),v=Math.abs(A_num(nd.pct)||0); if(k!=null&&v>0){ strikes[k]=Math.max(strikes[k]||0,v); } }); });
  var arr=Object.keys(strikes).map(function(k){return {k:+k,v:strikes[k]};}).sort(function(a,b){return b.v-a.v;});
  var heavy=arr.filter(function(x){return x.v>=20;}); // significant nodes
  var spread=heavy.length? (Math.max.apply(null,heavy.map(function(x){return x.k;}))-Math.min.apply(null,heavy.map(function(x){return x.k;}))) : 0;
  // TREND: net drift dominates rolls (rolls mostly one way) + leading + flow polarity aligns
  var oneWayRolls = kb.rolls>0 ? Math.abs(kb.rollUp-kb.rollDn)/kb.rolls : 0;
  // King-core tightness: how narrow the band of King strikes actually visited is.
  // A tight core (few distinct strikes over a small range) = a defined range even
  // if many heavy nodes exist elsewhere on the board (Whipsaw, not Rainbow).
  var kingRange = kb.coreRange!=null? kb.coreRange : (kb.levels.length? (kb.levels[kb.levels.length-1]-kb.levels[0]) : 0);
  var tightCore = (kb.coreCount!=null?kb.coreCount:kb.distinct)<=4 && kingRange<=4;
  var trending = Math.abs(kb.netDrift)>=2 && oneWayRolls>=0.5 && kb.rolls>=2;
  // WHIPSAW: flip-flop King (balanced rolls) held to a tight core, small net drift.
  // The heavy-node count no longer gates it \u2014 an oscillating King in a tight band
  // IS a range regardless of how many significant nodes sit around it.
  var whipsaw = kb.rolls>=2 && oneWayRolls<0.6 && tightCore;
  // RAINBOW: genuinely scattered \u2014 wide King wander OR wide heavy spread AND no tight
  // core AND poor reach (the doc's "no clear range, vague positioning").
  var rainbow = !tightCore && (kingRange>=5 || spread>=10) && (kb.reachRate==null||kb.reachRate<40);
  var label, conf, why;
  if(trending){ label=(kb.netDrift<0?'Trend \u2193':'Trend \u2191'); conf=oneWayRolls>=0.7?'high':'medium';
    why='King drifted '+kb.netDrift+' via mostly one-way rolls ('+kb.rollUp+'\u2191/'+kb.rollDn+'\u2193) \u2014 leading relocation, the doc\u2019s stair-step trend. Trade pullbacks.'; }
  else if(whipsaw){ label='Whipsaw'; conf='medium';
    why='King flip-flopped ('+kb.rollUp+'\u2191/'+kb.rollDn+'\u2193) in a tight '+(kb.core&&kb.core.length?(kb.core[0]+'\u2013'+kb.core[kb.core.length-1]):(kb.levels[0]+'\u2013'+kb.levels[kb.levels.length-1]))+' core \u2014 a defined range. Fade the edges, avoid the middle.'; }
  else if(rainbow){ label='Rainbow Road'; conf='low';
    why=heavy.length+' heavy nodes scattered over ~'+spread+' strikes with no clear edges'+(kb.reachRate!=null?(' and low King reach ('+kb.reachRate+'%)'):'')+'. Vague positioning \u2014 stand aside.'; }
  else { label='Mixed'; conf='low'; why='No dominant structure: net drift '+kb.netDrift+', '+kb.rolls+' rolls, '+heavy.length+' heavy nodes. Treat as low-conviction.'; }
  return { label:label, conf:conf, why:why, kb:kb, ce:ce, heavy:heavy.length, spread:spread };
}

// tooltip helper: attaches a coherence explanation (title=) naming the item's
// ROLE in the King\u2192price\u2192S/R\u2192nodes story, not just a definition.
function A_tip(t){ return t? (' title="'+String(t).replace(/"/g,'&quot;')+'"') : ''; }

// ---- pure analytics from labeled snapshots -------------------------------
function _pct(n,d){ return d>0 ? Math.round(100*n/d) : null; }
function _dirOf(sig){
  // net directional lean of a snapshot's read: prefer confluence, else King+trend blend
  if(!sig) return 0;
  if(sig.conf && typeof sig.conf.dir==='number' && sig.conf.dir!==0) return sig.conf.dir>0?1:-1;
  var s=0;
  if(sig.king && typeof sig.king.drift==='number') s+=(sig.king.drift>0?1:(sig.king.drift<0?-1:0));
  if(sig.trend){ if(sig.trend.state==='up')s+=1; else if(sig.trend.state==='down')s-=1; }
  return s>0?1:(s<0?-1:0);
}
function analysisStats(sym){
  var day=A_day(); // loaded day if set, else live
  var snaps=(day && day.snaps && day.snaps[sym]) ? day.snaps[sym] : [];
  var out={ date:day?day.date:TODAY, bars:snaps.length, ready:0,
            dirHit5:null, dirHit10:null, revCatch:null, targetHit:null, targetN:0,
            perSig:{}, lead:{}, matrix:{}, nodes:[] };
  if(!snaps.length) return out;
  // signal keys we score for accuracy
  var keys=['king','trend','srb','conf'];
  var acc={}, leadSum={}, leadN={};
  keys.forEach(function(k){ acc[k]={hit:0,n:0}; leadSum[k]=0; leadN[k]=0; });
  var dh5={hit:0,n:0}, dh10={hit:0,n:0}, rev={hit:0,n:0}, tgt={hit:0,n:0};
  var mtx={}; // aligned-count -> {dir:{hit,n}, tgt:{hit,n}}
  snaps.forEach(function(s){
    var o10=s.out10, o5=s.out5, sig=s.sig; if(!sig) return;
    // overall direction hit (does the read's lean match net move?)
    var lean=_dirOf(sig);
    if(o5 && lean!==0){ dh5.n++; if((o5.net>0&&lean>0)||(o5.net<0&&lean<0)) dh5.hit++; }
    if(o10 && lean!==0){ out.ready++; dh10.n++; if((o10.net>0&&lean>0)||(o10.net<0&&lean<0)) dh10.hit++; }
    // reversal catch: did a reversal flag in sig line up with revUp/revDn in outcome?
    if(o10){ var flagged=(sig.srb&&sig.srb.cross)?1:0; var actual=(o10.revUp||o10.revDn)?1:0;
             if(actual){ rev.n++; if(flagged) rev.hit++; } }
    // target: King magnet — did price reach the King within 10 bars?
    if(o10){ tgt.n++; if(o10.hitKing) tgt.hit++; }
    // per-signal accuracy vs 10-bar net
    if(o10){
      keys.forEach(function(k){
        var d=0;
        if(k==='king'&&sig.king) d=(sig.king.drift>0?1:(sig.king.drift<0?-1:0));
        else if(k==='trend'&&sig.trend) d=(sig.trend.state==='up'?1:(sig.trend.state==='down'?-1:0));
        else if(k==='srb'&&sig.srb) d=(sig.srb.dom==='support'?1:(sig.srb.dom==='resistance'?-1:0));
        else if(k==='conf'&&sig.conf) d=(sig.conf.dir||0);
        if(d!==0){ acc[k].n++; if((o10.net>0&&d>0)||(o10.net<0&&d<0)) acc[k].hit++; }
      });
    }
    // confluence-outcome matrix by aligned count
    if(o10 && sig.conf && typeof sig.conf.aligned==='number'){
      var a=sig.conf.aligned; if(!mtx[a]) mtx[a]={dir:{hit:0,n:0},tgt:{hit:0,n:0}};
      var cd=sig.conf.dir||0;
      if(cd!==0){ mtx[a].dir.n++; if((o10.net>0&&cd>0)||(o10.net<0&&cd<0)) mtx[a].dir.hit++; }
      mtx[a].tgt.n++; if(o10.hitKing) mtx[a].tgt.hit++;
    }
  });
  out.dirHit5=_pct(dh5.hit,dh5.n); out.dirHit10=_pct(dh10.hit,dh10.n);
  out.revCatch=_pct(rev.hit,rev.n); out.targetHit=_pct(tgt.hit,tgt.n); out.targetN=tgt.n; out.targetHitRaw=tgt.hit;
  keys.forEach(function(k){ out.perSig[k]=_pct(acc[k].hit,acc[k].n); });
  out.matrix=mtx;
  // node lifecycle: aggregate each strike's role + net trajectory across the day
  var byK={};
  snaps.forEach(function(s){ (s.nodes||[]).forEach(function(n){
    if(n.k==null) return; if(!byK[n.k]) byK[n.k]={k:n.k, roles:{}, firstNet:null, lastNet:null, lastSt:null, side:n.side};
    var e=byK[n.k]; if(n.role) e.roles[n.role]=(e.roles[n.role]||0)+1;
    var net=(typeof n.net==='number')?n.net:null;
    if(net!=null){ if(e.firstNet==null)e.firstNet=net; e.lastNet=net; }
    if(n.st) e.lastSt=n.st;
  }); });
  out.nodes=Object.keys(byK).map(function(k){ return byK[k]; })
    .sort(function(a,b){ return b.k-a.k; });
  return out;
}

function _accBar(pct,label,note){
  var col = pct==null?PAL.sub : (pct>=70?PAL.longAccent:(pct>=55?PAL.amber:PAL.shortAccent));
  var w = pct==null?0:pct;
  return '<div style="display:flex;align-items:center;gap:8px;padding:4px 0;border-bottom:1px solid #161b26">'+
    '<div style="flex:0 0 92px;font-size:11px;font-weight:700">'+label+'</div>'+
    '<div style="flex:1;height:8px;background:#0b0e14;border-radius:6px;overflow:hidden"><div style="width:'+w+'%;height:100%;background:'+col+'"></div></div>'+
    '<div style="flex:0 0 34px;text-align:right;font-size:11px;font-weight:800;color:'+col+'">'+(pct==null?'\u2013':pct+'%')+'</div>'+
    '<div style="flex:0 0 74px;font-size:9px;color:'+PAL.sub+';text-align:right">'+(note||'')+'</div>'+
  '</div>';
}
function _step(n,title){
  return '<div style="display:flex;align-items:center;gap:8px;margin-bottom:6px">'+
    '<div style="flex:0 0 22px;height:22px;border-radius:50%;background:'+PAL.gold+';color:#0b0e14;font-weight:800;font-size:12px;text-align:center;line-height:22px">'+n+'</div>'+
    '<div style="font-size:12px;font-weight:800">'+title+'</div>'+
    '<div style="flex:1;height:1px;background:'+PAL.line+'"></div>'+
  '</div>';
}
function _await(txt){ return '<div style="margin:0 0 14px 14px;font-size:10px;color:'+PAL.sub+';background:#0d1420;border-left:2px solid '+PAL.blue+';padding:6px 8px;border-radius:0 6px 6px 0">'+txt+'</div>'; }

// (v10.21) an edge row: label + hit% vs baseline + swing% + MFE/MAE payoff + n,
// with a coherence tooltip. Honest: null hit => '\u2013 (insufficient)'.
function A_edgeRow(label, edge, baseline, tip){
  var dh=edge.dirHit, sw=edge.swingHit, n=edge.n;
  var lift=(dh!=null&&baseline!=null)?(dh-baseline):null;
  var col = dh==null?PAL.sub:(dh>=60?PAL.longAccent:(dh>=50?PAL.amber:PAL.shortAccent));
  var liftTxt = lift==null?'':(' <span style="color:'+(lift>0?PAL.longAccent:PAL.sub)+'">('+(lift>0?'+':'')+lift+' vs base)</span>');
  var payoff = (edge.avgMFE!=null&&edge.avgMAE!=null)? (' \u00b7 MFE '+edge.avgMFE+' / MAE '+edge.avgMAE) : '';
  return '<div style="display:flex;align-items:center;gap:8px;padding:4px 0;border-bottom:1px solid #161b26"'+A_tip(tip)+'>'+
    '<span style="flex:0 0 128px;font-size:10px;color:#c8d3df">'+label+'</span>'+
    '<span style="flex:1;font-size:10px;color:'+col+';font-weight:700">'+(dh==null?'\u2013 (insufficient)':dh+'%')+liftTxt+
      '<span style="color:'+PAL.sub+';font-weight:400"> \u00b7 swing '+(sw==null?'\u2013':sw+'%')+payoff+' \u00b7 n'+n+'</span></span></div>';
}

// (v10.21) the coherent TOP of the Analysis tab: regime chip + King behavior +
// accumulation/dissipation/combined edges. Reads the causal chain in order and
// every item carries a role-in-the-story tooltip. Rendered ABOVE the legacy
// 7-step review so the tab tells ONE argument top-to-bottom.
function A_renderTop(sym){
  var day=A_day(); var reg=A_regime(day,sym); var kb=reg.kb||A_kingBehavior(day,sym);
  var h='';
  if(!kb || kb.pts<1) return h; // nothing to say yet; legacy block shows the await note
  // --- REGIME CHIP (the headline: what KIND of day is this) ---
  var rc = /Trend/.test(reg.label)?(/\u2193/.test(reg.label)?PAL.shortAccent:PAL.longAccent)
         : reg.label==='Whipsaw'?PAL.amber : reg.label==='Rainbow Road'?PAL.sub : PAL.blue;
  h+='<div style="margin:0 0 12px 0;background:#12161f;border:1px solid '+PAL.line+';border-left:3px solid '+rc+';border-radius:8px;padding:8px 10px"'+
     A_tip('REGIME sets the frame for everything below. Trend=trade pullbacks; Whipsaw=fade the range edges, avoid the middle; Rainbow Road=stand aside; the metrics below are read THROUGH this.')+'>'+
     '<div style="display:flex;justify-content:space-between;align-items:center">'+
       '<span style="font-size:13px;font-weight:800;color:'+rc+'">'+reg.label+'</span>'+
       '<span style="font-size:9px;color:'+PAL.sub+';text-transform:uppercase;letter-spacing:.5px">'+reg.conf+' confidence</span></div>'+
     '<div style="font-size:10px;color:#c8d3df;line-height:1.5;margin-top:3px">'+reg.why+'</div></div>';
  // --- KING BEHAVIOR (the anchor: where the magnet lived + did price obey) ---
  h+=_step(1,'\uD83D\uDC51 King behavior \u2014 the anchor');
  var pullTxt = kb.pullDir==='down'?'pulling DOWN (mostly below price)':kb.pullDir==='up'?'pulling UP (mostly above price)':'balanced';
  var pinTxt;
  if(kb.pinDist==null) pinTxt='\u2013';
  else if(kb.pinned) pinTxt='<span style="color:'+PAL.longAccent+'">PINNED</span> ('+kb.pinDist+'pt, '+kb.pinTiming+')';
  else pinTxt='<span style="color:'+PAL.amber+'">not pinned</span> ('+kb.pinDist+'pt off, '+(kb.pinTiming||'')+')';
  h+='<div style="margin:0 0 14px 14px;background:#12161f;border:1px solid '+PAL.line+';border-radius:10px;padding:8px 10px;font-size:10px;color:#c8d3df;line-height:1.7">'+
    '<div'+A_tip('The King is the settlement magnet \u2014 the direction price is \u201Csupposed\u201D to follow. Its net drift + which side of price it sat is the day\u2019s directional pull.')+'>Path: <b>'+(kb.core?kb.core.join('\u2192'):kb.levels.join('\u2192'))+'</b> \u00b7 net drift <b>'+kb.netDrift+'</b> \u00b7 '+pullTxt+'</div>'+
    '<div'+A_tip('Rolls = King relocations. Mostly one-way rolls that LEAD price = a trend (stair-step). Balanced flip-flop rolls = a range (Whipsaw).')+'>Rolls: <b>'+kb.rolls+'</b> ('+kb.rollUp+'\u2191/'+kb.rollDn+'\u2193, avg '+(kb.avgRollSize==null?'\u2013':kb.avgRollSize)+' strikes)</div>'+
    '<div'+A_tip('Reach = did price actually tag a held King level (the pull WORKING). Doc rule: early reach \u2192 dealers drive price off; late reach \u2192 settlement pin.')+'>Reach: <b>'+(kb.reachRate==null?'\u2013':kb.reachRate+'%')+'</b> ('+kb.reachHit+'/'+kb.reachN+' levels)'+(kb.avgTimeToReach!=null?(' \u00b7 ~'+kb.avgTimeToReach+' bars to tag'):'')+' \u00b7 gap converged '+(kb.convergeRate==null?'\u2013':kb.convergeRate+'%')+'</div>'+
    '<div'+A_tip('The payoff of the pull: did price CLOSE on the King (a settlement pin) or get driven off? Late pin confirms the magnet; a far close = magnet ignored/decoy.')+'>Close vs King: '+pinTxt+' \u00b7 King '+kb.closeK+' / px '+(kb.closePx!=null?(+kb.closePx).toFixed(2):'\u2013')+'</div>'+
  '</div>';
  // --- EDGE FAMILIES (regime-aware; on range days directional edges are noise) ---
  var acc=A_accumEdge(day,sym,'accum'), fade=A_accumEdge(day,sym,'fade'), ce=reg.ce||A_combinedEdge(day,sym);
  var isRange = (reg.label==='Whipsaw'||reg.label==='Rainbow Road');
  h+=_step(2,'\uD83D\uDCC8 Did accumulation mark price?');
  if(isRange) h+=_await('This was a '+reg.label+' \u2014 a range/no-edge day. Directional accumulation edges below are LOW-signal here (the right read was fade-the-edges, not follow-through). Shown for completeness.');
  h+='<div style="margin:0 0 14px 14px;background:#12161f;border:1px solid '+PAL.line+';border-radius:10px;padding:8px 10px">'+
    A_edgeRow('Support building \u2192 up', acc.support, acc.baseline, 'Building support BELOW price should pull price UP toward it. This is the King\u2019s magnet logic at the node level.')+
    A_edgeRow('Resistance building \u2192 dn', acc.resistance, acc.baseline, 'Building resistance ABOVE (a forming ceiling) should cap and press price DOWN.')+
    '<div style="font-size:9px;color:'+PAL.sub+';margin-top:5px">baseline up-rate '+(acc.baseline==null?'\u2013':acc.baseline+'%')+' \u00b7 lift over baseline is the real edge, not raw %</div></div>';
  h+=_step(3,'\uD83D\uDCC9 Did dissipation move price?');
  h+='<div style="margin:0 0 14px 14px;background:#12161f;border:1px solid '+PAL.line+';border-radius:10px;padding:8px 10px">'+
    A_edgeRow('Support fading \u2192 dn', fade.support, fade.baseline, 'A floor BLEEDING OUT below price removes support \u2014 price falls THROUGH the vacated level. Doc: rolling floors = bearish evidence.')+
    A_edgeRow('Resistance fading \u2192 up', fade.resistance, fade.baseline, 'A ceiling DISSOLVING above frees price to run UP. Doc: rolling ceilings/unwind = the move accelerating.')+
    '<div style="font-size:9px;color:'+PAL.sub+';margin-top:5px">dissipation is directional \u2014 not just \u201Cabsence of support\u201D</div></div>';
  h+=_step(4,'\uD83E\uDDE9 When both sides move (confluence)');
  h+='<div style="margin:0 0 16px 14px;background:#12161f;border:1px solid '+PAL.line+';border-radius:10px;padding:8px 10px;font-size:10px;color:#c8d3df;line-height:1.7">'+
    '<div'+A_tip('TRAPDOOR = resistance building overhead WHILE support fades below \u2014 the doc\u2019s coherent bearish stack. Should predict DOWN.')+'>Trapdoor (res build + sup fade \u2192 dn): <b>'+(ce.trapdoor.hit==null?'\u2013':ce.trapdoor.hit+'%')+'</b> <span style="color:'+PAL.sub+'">n'+ce.trapdoor.n+'</span></div>'+
    '<div'+A_tip('LIFTOFF = support building below WHILE resistance fades above \u2014 the bullish mirror. Should predict UP.')+'>Liftoff (sup build + res fade \u2192 up): <b>'+(ce.liftoff.hit==null?'\u2013':ce.liftoff.hit+'%')+'</b> <span style="color:'+PAL.sub+'">n'+ce.liftoff.n+'</span></div>'+
    '<div'+A_tip('BOTH sides building = compression \u2192 range/chop, NOT direction. High range-rate here means \u201Cdon\u2019t trade direction.\u201D')+'>Compression (both build \u2192 range): <b>'+(ce.compression.rangeRate==null?'\u2013':ce.compression.rangeRate+'%')+'</b> <span style="color:'+PAL.sub+'">n'+ce.compression.n+'</span></div>'+
    '<div'+A_tip('The whole thesis: does stacking reads (dual-confirmation) beat a single signal? If dual > single, confluence pays.')+'>Dual-confirmation dir hit: <b>'+(ce.dualVsSingle.dualHit==null?'\u2013':ce.dualVsSingle.dualHit+'%')+'</b> <span style="color:'+PAL.sub+'">n'+ce.dualVsSingle.dualN+'</span> \u00b7 net-flow polarity '+(ce.netFlow.dirHit==null?'\u2013':ce.netFlow.dirHit+'%')+'</div></div>';
  return h;
}

function analysisBlock(){
  var sym=ANALYSIS_SYM;
  var st=analysisStats(sym);
  var R=ANALYSIS_REVIEW; // optional LLM narrative
  var h='';
  // ---- header + day grade ----
  var grade = (R&&R.grade)?R.grade:(st.dirHit10==null?'\u2013':(st.dirHit10>=70?'A-':(st.dirHit10>=60?'B':(st.dirHit10>=50?'C':'D'))));
  var gCol = (st.dirHit10!=null&&st.dirHit10>=60)?PAL.longAccent:(st.dirHit10!=null&&st.dirHit10>=50?PAL.amber:PAL.shortAccent);
  var loaded=(typeof LOADED_DAY!=='undefined'&&LOADED_DAY);
  var srcLabel = loaded? ('\uD83D\uDCC2 loaded '+(LOADED_DAY.date||'file')) : (R?'\u25f7 LLM review loaded':'\u25f7 live from tape data');
  h+='<div style="display:flex;justify-content:space-between;align-items:flex-start;margin-bottom:10px">'+
      '<div><div style="font-size:14px;font-weight:800">End-of-Day Review</div>'+
      '<div style="font-size:10px;color:'+PAL.sub+'">'+sym+' \u00b7 '+st.date+' \u00b7 '+st.bars+' bars \u00b7 '+st.ready+' scored</div></div>'+
      '<div style="text-align:right">'+
        '<div style="font-size:9px;color:'+(loaded?PAL.gold:(R?PAL.blue:PAL.sub))+'">'+srcLabel+'</div>'+
        '<div style="margin-top:4px">'+
          '<span onclick="try{var j=window.prompt(\'Paste a saved gex_DATE.json to analyze:\');if(j)window.__gptsDebug.loadDay(j);}catch(e){}" title="Load a past day\u2019s export (or paste the recorder JSON) to render its analysis here." style="cursor:pointer;font-size:9px;color:'+PAL.blue+';border:1px solid '+PAL.line+';border-radius:10px;padding:2px 7px">\uD83D\uDCC2 Load day</span>'+
          (loaded?(' <span onclick="window.__gptsDebug.clearLoaded()" title="Clear the loaded day and return to the live session." style="cursor:pointer;font-size:9px;color:'+PAL.amber+';border:1px solid '+PAL.line+';border-radius:10px;padding:2px 7px">\u2715 clear</span>'):'')+
        '</div></div>'+
    '</div>';
  h+='<div style="display:flex;gap:8px;margin-bottom:16px;align-items:stretch">'+
      '<div style="flex:0 0 52px;background:#12161f;border:1px solid '+PAL.line+';border-radius:8px;padding:6px 4px;text-align:center;display:flex;flex-direction:column;justify-content:center"'+A_tip('Legacy per-bar signal-direction grade (30m forward). The REGIME chip above is the primary read; this grades signal accuracy, not the day type.')+'>'+
        '<div style="font-size:8px;color:'+PAL.sub+';letter-spacing:.5px">GRADE</div>'+
        '<div style="font-size:18px;font-weight:800;color:'+gCol+';line-height:1.1">'+grade+'</div></div>'+
      '<div style="flex:1;display:grid;grid-template-columns:1fr 1fr 1fr;gap:6px">'+
        _kpi('Direction hit', st.dirHit10==null?'\u2013':st.dirHit10+'%', '30m fwd', st.dirHit10)+
        _kpi('Reversal catch', st.revCatch==null?'\u2013':st.revCatch+'%', 'flag vs actual', st.revCatch)+
        _kpi('King target', st.targetN?(st.targetHitRaw+'/'+st.targetN):'\u2013', 'reached King', st.targetHit)+
      '</div>'+
    '</div>';

  // ---- IN-TAB SAVE BANNER (v10.17): the tab IS the trigger. If today's data
  // hasn't been exported this session, surface a one-tap "Save & prep review".
  // Once saved, show a confirmation. No surprise auto-download on tab open. ----
  if(st.bars){
    if(SAVED_TODAY===st.date){
      h+='<div style="margin:0 0 12px 0;background:#0e1a13;border:1px solid #1c3a28;border-radius:8px;padding:7px 10px;font-size:10px;color:'+PAL.longAccent+'">\u2713 Saved <b>gex_'+st.date+'.json</b> \u2014 drop it into AI Drive <span style="color:'+PAL.sub+'">daily-data/</span> to run the review (or paste it to the assistant).</div>';
    } else {
      h+='<div style="margin:0 0 12px 0;background:#1a160d;border:1px solid #3a301c;border-radius:8px;padding:7px 10px;display:flex;justify-content:space-between;align-items:center;gap:8px">'+
        '<span style="font-size:10px;color:'+PAL.amber+'">Today\u2019s data isn\u2019t saved yet.</span>'+
        '<span onclick="window.__gptsDebug&&window.__gptsDebug.saveDayToFile&&window.__gptsDebug.saveDayToFile()" title="Export today\u2019s labeled data as gex_'+st.date+'.json, then drop it into AI Drive daily-data/ for the scheduled review." style="cursor:pointer;white-space:nowrap;font-size:10px;font-weight:800;color:#0b0e14;background:'+PAL.amber+';border-radius:14px;padding:3px 10px">\uD83D\uDCE5 Save &amp; prep review</span>'+
      '</div>';
    }
  }

  // ---- (v10.21) COHERENT TOP: regime chip + King behavior + edge families ----
  // Reads the causal chain in order, every item tooltip'd with its role in the
  // King\u2192price\u2192S/R\u2192nodes story. Uses A_day() so a LOADED past day renders here
  // even when the live session (st.bars) is empty.
  var topHtml=A_renderTop(sym);
  h+=topHtml;

  if(!st.bars && !topHtml){
    h+=_await('No captured bars yet today. The recorder logs one snapshot per closed 3-min bar and labels outcomes after 5\u201310 bars. Come back after some live tape, or load a past day with __gptsDebug.loadDay(json).');
    return h;
  }
  if(!st.bars){ return h; } // loaded/live top rendered; legacy scorecard needs live stats

  // ===== LEGACY SIGNAL SCORECARD (per-bar signal accuracy + LLM narrative) =====
  h+='<div style="margin:14px 0 8px 0;font-size:11px;font-weight:800;color:'+PAL.sub+';text-transform:uppercase;letter-spacing:.5px">Signal scorecard &amp; review</div>';
  // ---- STEP 1: relationship timeline (King+price+S/R band) ----
  h+=_step(1,'How the day moved together');
  h+='<div style="font-size:10px;color:'+PAL.sub+';margin:0 0 6px 14px">King \u00b7 price \u00b7 S/R dominance band</div>';
  h+='<div style="margin:0 0 14px 14px;background:#12161f;border:1px solid '+PAL.line+';border-radius:10px;padding:8px"><div style="overflow-x:auto;padding-bottom:4px">'+timelineSvg(sym,st)+'</div></div>';

  // ---- STEP 2: King vs price convergence ----
  h+=_step(2,'Was the King pulling price?');
  h+='<div style="margin:0 0 14px 14px;background:#12161f;border:1px solid '+PAL.line+';border-radius:10px;padding:8px"><div style="overflow-x:auto">'+convergenceSvg(sym,st)+'</div></div>';

  // ---- STEP 3: node lifecycle (scrollable) ----
  h+=_step(3,'What the nodes did (multi-strike)');
  h+='<div style="margin:0 0 14px 14px;background:#12161f;border:1px solid '+PAL.line+';border-radius:10px;padding:8px;max-height:150px;overflow-y:auto">';
  h+='<div style="font-size:9px;color:'+PAL.sub+';margin-bottom:4px">strike \u00b7 role \u00b7 net drift (scroll \u2195)</div>';
  if(!st.nodes.length){ h+='<div style="font-size:10px;color:'+PAL.sub+'">no node history yet</div>'; }
  st.nodes.slice(0,14).forEach(function(n){
    var role=Object.keys(n.roles).sort(function(a,b){return n.roles[b]-n.roles[a];})[0]||'\u2013';
    var drift=(n.firstNet!=null&&n.lastNet!=null)?(n.lastNet-n.firstNet):null;
    var kc = (n.side==='above')?PAL.shortAccent:(n.side==='below')?PAL.longAccent:PAL.gold;
    var dtxt = drift==null?'':(drift>0?'built +'+drift:(drift<0?'faded '+drift:'flat'));
    var dcol = drift==null?PAL.sub:(drift>0?PAL.longAccent:(drift<0?PAL.shortAccent:PAL.sub));
    h+='<div style="display:flex;align-items:center;gap:6px;padding:3px 0;border-bottom:1px solid #161b26">'+
       '<span style="flex:0 0 40px;color:'+kc+';font-weight:700;font-size:11px">'+fmtNum(n.k)+'</span>'+
       '<span style="flex:0 0 62px;font-size:9px;color:'+PAL.sub+'">'+role+'</span>'+
       '<span style="flex:1;font-size:10px;color:'+dcol+'">'+dtxt+(n.lastSt?(' \u00b7 '+n.lastSt):'')+'</span></div>';
  });
  h+='</div>';

  // ---- STEP 4: confluence-outcome matrix ----
  h+=_step(4,'Did agreement pay off?');
  h+='<div style="margin:0 0 14px 14px;background:#12161f;border:1px solid '+PAL.line+';border-radius:10px;padding:8px">';
  h+='<div style="display:grid;grid-template-columns:64px 1fr 1fr;gap:4px;font-size:10px">'+
     '<div style="color:'+PAL.sub+';font-size:9px">agree</div><div style="color:'+PAL.sub+';font-size:9px;text-align:center">dir correct</div><div style="color:'+PAL.sub+';font-size:9px;text-align:center">reached King</div>';
  [4,3,2,1].forEach(function(a){ var m=st.matrix[a]; if(!m) return;
    var dp=_pct(m.dir.hit,m.dir.n), tp=_pct(m.tgt.hit,m.tgt.n);
    var dc=dp==null?PAL.sub:(dp>=70?PAL.longAccent:(dp>=55?PAL.amber:PAL.shortAccent));
    h+='<div style="font-weight:700">'+a+'/4</div>'+
       '<div style="text-align:center;background:#0b0e14;border-radius:4px;padding:3px;color:'+dc+';font-weight:700">'+(dp==null?'\u2013':dp+'%')+' <span style="color:'+PAL.sub+';font-weight:400">('+m.dir.n+')</span></div>'+
       '<div style="text-align:center;background:#0b0e14;border-radius:4px;padding:3px;color:'+PAL.sub+'">'+(tp==null?'\u2013':tp+'%')+'</div>';
  });
  h+='</div><div style="font-size:9px;color:'+PAL.sub+';margin-top:5px">Higher agreement should predict better \u2014 watch 4/4 vs 2/4.</div></div>';

  // ---- STEP 5: scorecard + (lead/lag when review present) ----
  h+=_step(5,'Which signal to trust');
  h+='<div style="margin:0 0 14px 14px;background:#12161f;border:1px solid '+PAL.line+';border-radius:10px;padding:8px 10px">';
  h+=_accBar(st.perSig.king,'\uD83D\uDC51 King', (R&&R.lead&&R.lead.king)||'');
  h+=_accBar(st.perSig.trend,'\uD83D\uDCC8 Trend', (R&&R.lead&&R.lead.trend)||'');
  h+=_accBar(st.perSig.srb,'\u2694\uFE0F S/R', (R&&R.lead&&R.lead.srb)||'');
  h+=_accBar(st.perSig.conf,'\uD83C\uDFAF Confluence', (R&&R.lead&&R.lead.conf)||'');
  h+='</div>';

  // ---- STEP 6: worked / missed / why (LLM) ----
  h+=_step(6,'What worked, what missed, why');
  if(R&&(R.worked||R.missed)){
    h+='<div style="margin:0 0 8px 14px;display:flex;gap:8px">'+
       '<div style="flex:1;background:#0e1a13;border:1px solid #1c3a28;border-radius:10px;padding:8px 10px"><div style="font-size:10px;font-weight:800;color:'+PAL.longAccent+';margin-bottom:4px">\u2713 WORKED</div><div style="font-size:10px;color:#c8d3df;line-height:1.5">'+(R.worked||'\u2013')+'</div></div>'+
       '<div style="flex:1;background:#1c1113;border:1px solid #3a1c22;border-radius:10px;padding:8px 10px"><div style="font-size:10px;font-weight:800;color:'+PAL.shortAccent+';margin-bottom:4px">\u2715 MISSED</div><div style="font-size:10px;color:#c8d3df;line-height:1.5">'+(R.missed||'\u2013')+'</div></div></div>';
    if(R.why) h+='<div style="margin:0 0 14px 14px;background:#12161f;border:1px solid '+PAL.line+';border-radius:10px;padding:8px 10px"><div style="font-size:10px;font-weight:800;color:'+PAL.blue+';margin-bottom:3px">\uD83D\uDD0D WHY</div><div style="font-size:10px;color:#c8d3df;line-height:1.5">'+R.why+'</div></div>';
  } else {
    h+=_await('Awaiting the end-of-day LLM review for the narrative (what worked / missed and the root-cause why). The numbers above are computed live from the labeled tape; the story is filled once the scheduled review runs on this day\u2019s export.');
  }

  // ---- STEP 7: discoveries + recommendations (LLM) ----
  h+=_step(7,'Discoveries & what to change');
  if(R&&(R.discovered||( R.recs&&R.recs.length))){
    if(R.discovered) h+='<div style="margin:0 0 8px 14px;background:#160f1f;border:1px solid #2e2140;border-radius:10px;padding:8px 10px"><div style="font-size:10px;font-weight:800;color:#b58ce0;margin-bottom:3px">\uD83D\uDCA1 DISCOVERED</div><div style="font-size:10px;color:#c8d3df;line-height:1.5">'+R.discovered+'</div></div>';
    (R.recs||[]).forEach(function(rc,i){ var col=[PAL.shortAccent,PAL.amber,PAL.blue][i]||PAL.sub;
      h+='<div style="margin:0 0 6px 14px;display:flex;gap:8px;align-items:flex-start;background:#12161f;border:1px solid '+PAL.line+';border-radius:8px;padding:7px 9px"><div style="flex:0 0 18px;height:18px;border-radius:50%;background:'+col+';color:#0b0e14;font-size:10px;font-weight:800;text-align:center;line-height:18px">'+(i+1)+'</div><div style="font-size:10px">'+rc+'</div></div>';
    });
  } else {
    h+=_await('Awaiting the LLM review for pattern discoveries and ranked recommendations. Trigger it with the \u201cSave &amp; prep review\u201d banner above (drops the file for the scheduled review), or paste the recorder JSON to the assistant.');
  }
  return h;
}

// small KPI card
function _kpi(label,val,sub,pct){
  var col = pct==null?PAL.ink:(pct>=70?PAL.longAccent:(pct>=55?PAL.amber:PAL.shortAccent));
  return '<div style="background:#12161f;border:1px solid '+PAL.line+';border-radius:8px;padding:6px 8px"><div style="font-size:9px;color:'+PAL.sub+'">'+label+'</div><div style="font-size:15px;font-weight:800;color:'+col+'">'+val+'</div><div style="font-size:8px;color:'+PAL.sub+'">'+sub+'</div></div>';
}
// timeline SVG from real snapshots (King gold, price light, S/R band)
// panel inner width (px) for responsive SVG sizing; falls back to 280 if unknown.
function _bodyW(){ try{ var w=elBody?elBody.clientWidth:0; return (w&&w>60)?(w-24):280; }catch(e){ return 280; } }
function timelineSvg(sym,st){
  var day=A_day(); var snaps=(day.snaps&&day.snaps[sym])||[];
  if(snaps.length<2) return '<svg width="280" height="60" viewBox="0 0 280 60" style="display:block;width:100%"><text x="6" y="32" fill="'+PAL.sub+'" font-size="11">not enough bars yet \u2014 need \u22652</text></svg>';
  // Responsive width: fit the panel, but give each bar >=6px; scroll only if wider.
  var fitW=_bodyW(); var W=Math.max(fitW, snaps.length*6); var H=132, padT=10, padB=32, padL=4, padR=4;
  var ks=[],ps=[]; snaps.forEach(function(s){ if(typeof s.king==='number')ks.push(s.king); if(typeof s.px==='number')ps.push(s.px); });
  var all=ks.concat(ps); var lo=Math.min.apply(null,all), hi=Math.max.apply(null,all);
  // pad the range so a nearly-flat King/price don't collapse onto one line
  var span=hi-lo; if(span<0.5){ var mid=(hi+lo)/2; lo=mid-0.5; hi=mid+0.5; span=1; } else { lo-=span*0.12; hi+=span*0.12; }
  function X(i){ return padL + (i/(snaps.length-1))*(W-padL-padR); }
  function Y(v){ return padT + (1-(v-lo)/(hi-lo))*(H-padT-padB); }
  var bw=(W-padL-padR)/snaps.length;
  var svg='<svg width="'+W+'" height="'+H+'" viewBox="0 0 '+W+' '+H+'" style="display:block;'+(W<=fitW?'width:100%':'')+'">';
  // S/R dominance band per bar — VISIBLE tint (green support / red resistance)
  snaps.forEach(function(s,i){ if(!s.sig||!s.sig.srb) return;
    var c=(s.sig.srb.dom==='support')?'#2ec27e':(s.sig.srb.dom==='resistance')?'#f0616d':null; if(!c) return;
    svg+='<rect x="'+(X(i)-bw/2).toFixed(1)+'" y="'+padT+'" width="'+bw.toFixed(1)+'" height="'+(H-padT-padB)+'" fill="'+c+'" opacity="0.13"/>'; });
  // price line first (under King), light + slightly thicker so both read
  var pp=''; snaps.forEach(function(s,i){ if(typeof s.px!=='number')return; pp+=(pp?' L ':'M ')+X(i).toFixed(1)+' '+Y(s.px).toFixed(1); });
  svg+='<path d="'+pp+'" fill="none" stroke="#c8d3df" stroke-width="1.6" opacity="0.9"/>';
  // King staircase on top, gold
  var kp=''; snaps.forEach(function(s,i){ if(typeof s.king!=='number')return; kp+=(kp?' L ':'M ')+X(i).toFixed(1)+' '+Y(s.king).toFixed(1); });
  svg+='<path d="'+kp+'" fill="none" stroke="'+PAL.gold+'" stroke-width="2.2"/>';
  // crossover markers
  snaps.forEach(function(s,i){ if(s.sig&&s.sig.srb&&s.sig.srb.cross){ var c=s.sig.srb.cross==='bears'?PAL.shortAccent:PAL.longAccent;
    svg+='<circle cx="'+X(i).toFixed(1)+'" cy="'+Y(s.px||s.king).toFixed(1)+'" r="3.5" fill="'+c+'" stroke="#0b0e14" stroke-width="0.8"/>'; } });
  // compact single-row legend at the bottom
  var ly=H-9;
  svg+='<line x1="'+padL+'" y1="'+ly+'" x2="'+(padL+14)+'" y2="'+ly+'" stroke="'+PAL.gold+'" stroke-width="2.2"/><text x="'+(padL+18)+'" y="'+(ly+3)+'" fill="'+PAL.sub+'" font-size="9">King</text>'+
       '<line x1="'+(padL+50)+'" y1="'+ly+'" x2="'+(padL+64)+'" y2="'+ly+'" stroke="#c8d3df" stroke-width="1.6"/><text x="'+(padL+68)+'" y="'+(ly+3)+'" fill="'+PAL.sub+'" font-size="9">price</text>'+
       '<rect x="'+(padL+102)+'" y="'+(ly-4)+'" width="8" height="8" fill="'+PAL.longAccent+'" opacity="0.5"/><text x="'+(padL+113)+'" y="'+(ly+3)+'" fill="'+PAL.sub+'" font-size="9">sup</text>'+
       '<rect x="'+(padL+138)+'" y="'+(ly-4)+'" width="8" height="8" fill="'+PAL.shortAccent+'" opacity="0.5"/><text x="'+(padL+149)+'" y="'+(ly+3)+'" fill="'+PAL.sub+'" font-size="9">res</text>';
  svg+='</svg>'; return svg;
}
// King-minus-price gap over the day
function convergenceSvg(sym,st){
  var day=A_day(); var snaps=(day.snaps&&day.snaps[sym])||[];
  var gaps=[]; snaps.forEach(function(s){ if(typeof s.king==='number'&&typeof s.px==='number') gaps.push({i:gaps.length,g:s.king-s.px}); });
  if(gaps.length<2) return '<svg width="280" height="50" viewBox="0 0 280 50" style="display:block;width:100%"><text x="6" y="28" fill="'+PAL.sub+'" font-size="11">not enough bars yet</text></svg>';
  var fitW=_bodyW(); var W=Math.max(fitW, gaps.length*6), H=72, mid=36, padL=4, padR=4;
  var mx=1; gaps.forEach(function(p){ mx=Math.max(mx,Math.abs(p.g)); });
  function X(i){ return padL+(i/(gaps.length-1))*(W-padL-padR); }
  function Y(g){ return mid - (g/mx)*(mid-10); }
  var svg='<svg width="'+W+'" height="'+H+'" viewBox="0 0 '+W+' '+H+'" style="display:block;'+(W<=fitW?'width:100%':'')+'">';
  svg+='<line x1="0" y1="'+mid+'" x2="'+W+'" y2="'+mid+'" stroke="'+PAL.line+'" stroke-dasharray="3 3"/>';
  svg+='<text x="4" y="11" fill="'+PAL.sub+'" font-size="8">King above price (+)</text>';
  svg+='<text x="4" y="'+(H-4)+'" fill="'+PAL.sub+'" font-size="8">King below price (\u2212)</text>';
  // shade area between gap line and zero (green when converging toward 0 handled visually by proximity)
  var p=''; gaps.forEach(function(pt){ p+=(p?' L ':'M ')+X(pt.i).toFixed(1)+' '+Y(pt.g).toFixed(1); });
  svg+='<path d="'+p+'" fill="none" stroke="#b58ce0" stroke-width="2"/>';
  svg+='</svg>'; return svg;
}
// tab bar shown above every view
function analysisTabBar(){
  function tab(label,active,act){
    return '<div onclick="window.__gptsDebug&&window.__gptsDebug.showAnalysis&&window.__gptsDebug.showAnalysis('+act+')" style="cursor:pointer;padding:5px 12px;font-size:11px;font-weight:'+(active?'800':'600')+';color:'+(active?PAL.ink:PAL.sub)+';'+(active?'border-bottom:2px solid '+PAL.gold+';background:#0b0e14;border-radius:6px 6px 0 0':'')+'">'+label+'</div>';
  }
  return '<div style="display:flex;gap:2px;background:#12161f;border-bottom:1px solid '+PAL.line+';padding:4px 4px 0;margin:-2px -2px 6px">'+
    tab('Dashboard', !ANALYSIS_VIEW, 'false')+
    tab('\uD83D\uDCCA Analysis', ANALYSIS_VIEW, 'true')+
  '</div>';
}

function render(){
  if(!elBody) return;
  RENDER_SEQ++;   // (v10.14) new tick: srBattle memoizes per render so its
                  // crossover state isn't corrupted by being called twice.
  var html='';
  html+=analysisTabBar();
  if(ANALYSIS_VIEW){
    // (v10.20) wrap the Analysis content in its own vertical scroll container so
    // all 7 steps are reachable even when the panel height is fixed/short. Height
    // is viewport-relative with a floor so it always scrolls rather than clipping.
    html+='<div class="gpts-analysis-scroll" style="max-height:calc(100vh - 180px);min-height:240px;overflow-y:auto;overflow-x:hidden;padding-right:3px">'+
      analysisBlock()+
    '</div>';
    elBody.innerHTML=html;
    return;
  }
  // Section order (v10.23): King (TOP) → BO → S/R Imbalance. The READ section was
  // REMOVED (Issue F) — it conflated offset vs drift and duplicated the King Path /
  // S/R Imbalance / Trend, so the King header is now the top of the panel.
  // (readBlock() + structuralReadHtml() are left defined but no longer rendered.)
  html+=kingBlock();            // King tracker (3-magnet header + ①②③ + sparkline + verdict)
  html+=sep();
  html+=gatekeeperBlock();       // (v10.25 Step 4) gatekeeper area + ④
  html+=sep();
  // (v10.27) Standalone BO / SPY Signals section REMOVED. The breakout-pullback
  // lifecycle is now shown as a per-node tag (BO / BO\u00b7FT\u00b7\u2026) on the Node Map row it
  // belongs to (see nodeMapBlock -> setupTagForNode). The state machine still runs
  // (runMachine/newSetup/STATE.setups) \u2014 only the grid rendering is gone. Saves space.
  html+=accumBlock();           // (v10.27 Step 5) Node Map (carries flow + per-node BO tag)
  html+='<div style="border-top:1px solid '+PAL.line+';margin:6px 0 3px 0"></div>';
  html+=feedStatusHtml();
  elBody.innerHTML=html;
  // #1 (v10.6) Setup grade badge (the "D\u00b7S" tag) removed from the header per
  // request \u2014 keep the element hidden so nothing else that references it breaks.
  var gEl=document.getElementById('gpts-grade');
  if(gEl){ gEl.style.display='none'; }
  // #7 freshness dot + stale dimming of live-dependent readouts.
  var frEl=document.getElementById('gpts-fresh');
  var fr=feedFreshness('SPY');
  if(frEl){
    frEl.textContent='\u25CF '+fr.label;
    frEl.style.color=fr.col;
    frEl.title=('Feed '+(fr.state==='live'?'live':fr.state)+' \u2014 last update '+fr.label+' ago. Amber \u2248 2\u00d7 the normal cadence, red \u2248 4\u00d7. When red, the live readouts (badges, rapid icons) are dimmed because they may be frozen.').replace(/"/g,'');
  }
  // When the feed is stale (red), dim the whole body so frozen numbers do not
  // read as live. Amber leaves it fully lit but flags the dot.
  elBody.style.opacity = (fr.state==='red') ? '0.55' : '1';
  // #10 fire any armed alerts for state transitions this render.
  runAlerts('SPY', fr);
  var csx=elBody.querySelectorAll('.gpts-clr-sym');
  for(var j=0;j<csx.length;j++){
    (function(el){ el.addEventListener('click', function(){ clearSignalsSym(el.getAttribute('data-sym')); }); })(csx[j]);
  }
  // (v10.25) render() overwrites elBody.innerHTML, destroying the step popover —
  // re-create it and re-wire the info icons each render.
  ensureStepPop();
  wireStepIcons();
  // King path defaults to the latest (rightmost) chip in view.
  // (v10.8) King path is now an SVG sparkline, not a horizontal chip scroller —
  // the old scroll-to-latest snippet is no longer needed.
}

// Build a minimal walls[] structure directly from the rendered Skylit tape,
// for when the network gex/levels feed is absent/stale. The tape gives %King
// and the King strike; abs/net exposure isn't on the tape, so those are null.
// pos (support vs resistance) is inferred from the node's side relative to the
// current price. This lets the panel populate off the visible tape alone.
function wallsFromTape(sym, px){
  var tp=tapeMap(sym);
  if(!tp || !tp.pct) return null;
  var walls=[], king=(typeof tp.king==='number')?tp.king:null;
  for(var key in tp.pct){
    var pv=tp.pct[key];
    if(typeof pv!=='number') continue;
    var k=parseFloat(key);
    var apct=Math.abs(pv);
    if(apct<MIN_STRENGTH && !(king!=null && Math.abs(k-king)<0.001)) continue;
    var isKing=(king!=null && Math.abs(k-king)<0.001);
    walls.push({
      k:k,
      pct:isKing?100:apct,
      abs:null,
      pos:(px!=null) ? (k<=px) : (pv>=0),   // below price = support/floor
      derived:false,
      net:null,
      fromTape:true
    });
  }
  walls.sort(function(a,b){ return a.k-b.k; });
  return { price:px, king:king, walls:walls };
}
function refreshSym(sym){
  try{
    var S=STATE[sym];
    var f=LASTFEED[sym];
    var haveFeed = !!(f && f.j && (Date.now()-f.ts) <= FEED_STALE_MS);
    // Pull fiber candles first so we have a live price even with no feed.
    var raw=readFiberCandles(sym);
    var conv=null;
    if(raw && raw.length){
      // (v10.23 Issue C) build the CONTINUOUS close series BEFORE applyCandles so
      // it can compute the prior-session count; SMA/trend read this, not today-only.
      S.contCloses=convertFiberCandlesCont(raw);
      conv=convertFiberCandles(raw);
      applyCandles(sym, conv);
      if(conv.length){ S.price=conv[conv.length-1].c; }
    }
    if(haveFeed){
      // ---- Primary path: network feed ----
      var j=f.j;
      var ex=extractWalls(j);
      if(ex.price!=null) S.price=ex.price;
      if(ex.king!=null) S.king=ex.king;
      S.walls=ex.walls;
      captureGuards(sym, ex.walls, ex.king);
      stashSlice(sym, j);
      sampleTapeHistory(sym);
      LAST_OK[sym]=Date.now();
      runMachine(sym);
      return;
    }
    // ---- Fallback path: no/stale feed -> read the visible Skylit tape ----
    var tw=wallsFromTape(sym, S.price);
    if(tw && tw.walls && tw.walls.length>=3){
      if(tw.king!=null) S.king=tw.king;
      S.walls=tw.walls;
      captureGuards(sym, tw.walls, tw.king);
      sampleTapeHistory(sym);
      LAST_OK[sym]=Date.now();
      runMachine(sym);
    }
  }catch(e){}
}

function tick(){
  if(!pastReset()){ render(); return; }
  TODAY=ctTodayStr();
  FUT.ES = (STATE.SPY.price!=null) ? mul(STATE.SPY.price, ES_RATIO) : null;
  FUT.NQ = lastCloseOf('NQ1');
  refreshSym('SPY');
  recordSession();
  // DATA layer: once-per-closed-bar node snapshots (throttled internally).
  recordNodeSnapshot('SPY');
  recordNodeSnapshot('QQQ');
  render();
}

function boot(){
  installFeedObserver();
  loadCfg();
  loadKingDay();      // King persistence: rehydrate today's journey
  MIN_STRENGTH = CFG.nodeThresh;
  TODAY=ctTodayStr();
  buildPanel();
  injectSliderCss();
  restoreState();
  render();
  tick();
  setInterval(tick, POLL_MS);
}

if(document.readyState==='complete' || document.readyState==='interactive'){
  setTimeout(boot, 800);
} else {
  window.addEventListener('DOMContentLoaded', function(){ setTimeout(boot, 800); });
}

console.log('[GPTS] v9.1 part5 loaded');
})();
