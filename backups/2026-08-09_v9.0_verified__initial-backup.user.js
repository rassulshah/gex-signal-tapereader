// ==UserScript==
// @name         Gex Signal Tapereader
// @namespace    gpts
// @version      9.0
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
var CFG_KEY   = 'gpts_cfg_v7';
var STATS_KEY = 'gpts_stats_v7';

var TREND_WINDOW = 20;
var TREND_DOM = 15;
var TREND_RUNMIN = 3;
var TREND_BANDX = 0.25;
var TREND_SLOPEX = 0.15;
var ACC_WINDOW = 6;
var ACC_SAMPLES = 4;
var ACC_SAMPLE_STEP = 2;
var ACC_FLOOR_PCT = 20;
var ACC_NET = 8;
var ACC_ROWS = 4;
var GK_MAX_DIST = 5;
var STATS_DAYS = 5;
var READ_NEARX = 0.25;
var READ_APPRX = 0.75;

var CFG = {
  ftReq: true, boPb: true, dir: 'both', nodeThresh: 20, voidBackN: 2,
  trendOn: true, trendMA: { SPY:50, QQQ:50 },
  smaShort: { SPY:9, QQQ:9 }, smaLong: { SPY:21, QQQ:21 },
  hideGO: true, cfgOpen: true, showSPY: true, showQQQ: true
};

function loadCfg(){
  try{
    var raw=localStorage.getItem(CFG_KEY);
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
    }
    MIN_STRENGTH = CFG.nodeThresh;
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
function onFeed(sym, feed, j){
  if(sym!=='SPY' && sym!=='QQQ') return;
  if(!j || !j.levels || !j.levels.length) return;
  if(feed==='vanna'){ LASTVEX[sym] = { j:j, ts:Date.now() }; return; }
  if(feed==='gamma' || feed==='combined'){ LASTFEED[sym] = { j:j, feed:feed, ts:Date.now() }; }
}

console.log('[GPTS] v9.0 part1 loaded');

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
  var king = 0; native.forEach(function(n){ if(n.v>king) king=n.v; });
  if(king<=0) return {price:price, king:null, walls:[]};
  var byK = {};
  native.forEach(function(n){
    var pct = Math.round(mul(100, n.v/king));
    if(pct>=MIN_STRENGTH){ byK[n.k.toFixed(2)] = {k:n.k, pct:pct, abs:n.v, pos:(n.d>0), derived:false, net:n.net}; }
  });
  var haveDerived = j.derived && j.derived.length;
  if(haveDerived){
    (j.derived||[]).forEach(function(dd){
      if(!dd.levels || !dd.levels.length) return;
      var dlast = dd.levels[dd.levels.length-1];
      (dlast.l||[]).forEach(function(n){
        var snapped = Math.round(mul(n.k,2))/2;
        var pct = Math.round(mul(100, n.v/king));
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
    synthDerived(native, king, byK);
  }
  var walls = [];
  for(var kk in byK){ walls.push(byK[kk]); }
  walls.sort(function(a,b){ return a.k-b.k; });
  return {price:price, king:king, walls:walls};
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
    var pct = Math.round(mul(100, vAvg/king));
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

console.log('[GPTS] v9.0 part2 loaded');

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
function smaVal(sym, period){
  var c=closedCandles(sym);
  if(c.length<period) return null;
  var sum=0;
  for(var i=c.length-period;i<c.length;i++){ sum+=c[i].c; }
  return sum/period;
}
function trendOkFor(sym, dir){
  if(!CFG.trendOn) return true;
  var mp=parseInt(CFG.trendMA[sym],10);
  if(isNaN(mp)||mp<1){ mp=50; }
  var ma=smaVal(sym, mp);
  if(ma==null) return true;
  var px=STATE[sym].price;
  if(px==null) return true;
  if(dir==='long') return px>ma;
  return px<ma;
}

function trendVerdict(sym){
  var mp=parseInt(CFG.trendMA[sym],10);
  if(isNaN(mp)||mp<1) mp=50;
  var c=closedCandles(sym);
  if(c.length<mp+1) return { state:'flat', up:0, win:0, ma:null, slope:0 };
  var win=Math.min(TREND_WINDOW, c.length-mp);
  if(win<1) return { state:'flat', up:0, win:0, ma:null, slope:0 };
  var band=mul(atr(sym), TREND_BANDX);
  var up=0, dn=0, lastMa=null, firstMa=null;
  for(var i=c.length-win;i<c.length;i++){
    var sum=0;
    for(var j=i-mp+1;j<=i;j++){ sum+=c[j].c; }
    var ma=sum/mp;
    if(firstMa==null) firstMa=ma;
    lastMa=ma;
    var px=c[i].c;
    if(px > ma+band) up++;
    else if(px < ma-band) dn++;
  }
  var slope = (lastMa!=null && firstMa!=null) ? (lastMa-firstMa) : 0;
  var slopeBand = mul(atr(sym), TREND_SLOPEX);
  var slopeUp = slope > slopeBand;
  var slopeDn = slope < -slopeBand;
  var state='flat';
  if(up>=TREND_DOM && up>dn && slopeUp) state='up';
  else if(dn>=TREND_DOM && dn>up && slopeDn) state='dn';
  var dom = state==='dn' ? dn : up;
  return { state:state, up:dom, win:win, ma:lastMa, slope:slope };
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
    if(!arr.length || arr[arr.length-1].t!==last.t){
      arr.push({t:last.t, s:last.s, l:(last.l||[]).map(function(n){return {k:n.k,v:n.v,d:n.d,net:(typeof n.net==='number'?n.net:null)};})});
      if(arr.length>500) arr.shift();
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
    var kg=kingOf(s); if(kg<=0) return;
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
  var latest=snaps[snaps.length-1];
  var lk=kingOf(latest); if(lk<=0) return [];
  var byK={};
  snaps.forEach(function(s){
    var kg=kingOf(s); if(kg<=0) return;
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
  var walls=STATE[sym].walls||[];
  for(var i=0;i<walls.length;i++){
    if(Math.abs(walls[i].k - k) < 0.001) return walls[i].pct;
  }
  return null;
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
      if(!liveL && doneL && trendOkFor(sym,'long')){ newSetup(sym,S,k,'long',last); }
    }
    if(last.c<k && prev.c>=k){
      var liveS=liveSetup(S,k,'short');
      var lastAttemptS=null;
      for(var key2 in S.setups){ var s2=S.setups[key2]; if(s2.strike===k&&s2.dir==='short'){ if(!lastAttemptS||s2.attempt>lastAttemptS.attempt) lastAttemptS=s2; } }
      var doneS = (!lastAttemptS || lastAttemptS.voided || lastAttemptS.stage==='GO');
      if(!liveS && doneS && trendOkFor(sym,'short')){ newSetup(sym,S,k,'short',last); }
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
  s.targets=beyond.slice(0,3).map(function(w){return w.k;});
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
window.__gptsDebug.STATE = STATE;
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

console.log('[GPTS] v9.0 part3 loaded');

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
  css(hdr,{padding:'7px 11px', background:'#0f131b', borderBottom:'1px solid '+PAL.line,
    borderRadius:'10px 10px 0 0', cursor:'move', display:'flex',
    justifyContent:'space-between', alignItems:'center'});
  var ttl=document.createElement('span');
  ttl.textContent='Gex Signal Tapereader';
  css(ttl,{color:PAL.ink, fontSize:'13px', fontWeight:'800', letterSpacing:'0.4px',
    borderLeft:'3px solid '+PAL.blue, paddingLeft:'7px'});
  hdr.appendChild(ttl);
  var right=document.createElement('span');
  css(right,{display:'flex', alignItems:'center', gap:'6px'});
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
  clr.textContent='Clear All';
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
  css(elBody,{padding:'9px 10px'});
  PANEL.appendChild(elBody);

  var grip=document.createElement('div');
  grip.id='gpts-grip';
  css(grip,{position:'absolute', right:'0', bottom:'0', width:'16px', height:'16px',
    cursor:'nwse-resize', background:'linear-gradient(135deg, transparent 50%, #3a4150 50%, #3a4150 60%, transparent 60%, transparent 70%, #3a4150 70%, #3a4150 80%, transparent 80%)'});
  PANEL.appendChild(grip);

  document.body.appendChild(PANEL);
  restorePos();
  restoreSize();
  makeDraggable(hdr);
  makeResizable(grip);
}

function toggleCfg(){
  if(!elCfg) return;
  if(elCfg.style.display==='none'){ renderCfg(); elCfg.style.display='block'; }
  else { elCfg.style.display='none'; }
}

function makeDraggable(hdr){
  var dragging=false, sx=0, sy=0, ox=0, oy=0;
  hdr.addEventListener('mousedown', function(e){
    dragging=true; sx=e.clientX; sy=e.clientY;
    var r=PANEL.getBoundingClientRect(); ox=r.left; oy=r.top;
    PANEL.style.right=''; PANEL.style.left=ox+'px'; PANEL.style.top=oy+'px';
    e.preventDefault();
  });
  document.addEventListener('mousemove', function(e){
    if(!dragging) return;
    var nx=ox+(e.clientX-sx), ny=oy+(e.clientY-sy);
    PANEL.style.left=nx+'px'; PANEL.style.top=ny+'px';
  });
  document.addEventListener('mouseup', function(){
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

function sectionHdr(text){
  return '<div style="background:'+PAL.card+';color:'+PAL.ink+';font-size:11px;font-weight:800;'+
    'letter-spacing:0.5px;padding:4px 9px;margin:5px 0 3px 0;border-left:3px solid '+PAL.blue+';'+
    'border-radius:6px">'+text+'</div>';
}
function symSignalsHdr(sym){
  return '<div style="background:'+PAL.card+';color:'+PAL.ink+';font-size:11px;font-weight:800;'+
    'letter-spacing:0.5px;padding:4px 9px;margin:5px 0 3px 0;border-left:3px solid '+PAL.blue+';'+
    'border-radius:6px;display:flex;justify-content:space-between;align-items:center">'+
    '<span>'+sym+' Signals</span>'+
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
  return html;
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
    '#gpts-panel table.gpts-grid{ border-collapse:collapse; }'+
    '#gpts-panel table.gpts-grid td{ padding:2px 10px; font-size:11px; white-space:nowrap; text-align:right; cursor:default; color:'+PAL.ink+'; }'+
    '#gpts-panel table.gpts-grid td.g-lbl{ position:sticky; left:0; z-index:2; text-align:left; color:'+PAL.sub+'; font-weight:700; border-right:2px solid '+PAL.line+'; }'+
    '#gpts-panel table.gpts-grid td.g-sep{ border-left:1px solid '+PAL.line+'; }'+
    '#gpts-panel table.gpts-grid td.g-time{ color:'+PAL.time+'; }'+
    '#gpts-panel table.gpts-grid td.g-na{ color:'+PAL.amber+'; }'+
    '#gpts-panel table.gpts-grid td.g-void{ color:'+PAL.amber+'; }';
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
}

console.log('[GPTS] v9.0 part4 loaded');

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
  var html=typeHdr('Pullback Signals');
  var showLong = CFG.dir!=='shorts';
  var showShort = CFG.dir!=='longs';
  if(showLong) html+=gridFor('long', rows);
  if(showShort) html+=gridFor('short', rows);
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

function trendBadgeHtml(){
  var v=trendVerdict('SPY');
  var label, col;
  if(v.state==='up'){ label='Up'; col=PAL.longAccent; }
  else if(v.state==='dn'){ label='Dn'; col=PAL.shortAccent; }
  else { label='Sideways'; col=PAL.gold; }
  var counter = v.win ? (v.up+'/'+v.win) : '';
  var slopeTxt = (v.slope>0?'+':'')+(Math.round(mul(v.slope,100))/100);
  var tip='Intraday trend: closes vs SMA'+CFG.trendMA.SPY+' over the last '+(v.win||0)+' closed 3m bars, plus SMA slope agreement. Counter = closes on the dominant side ('+counter+'). Slope '+slopeTxt+'. Matches Skylit SMA 50.';
  return '<span title="'+tip+'" style="color:'+col+';font-weight:700;font-size:10px;padding:1px 8px;border:1px solid '+col+';border-radius:20px;background:rgba(255,255,255,0.02)">'+label+' '+counter+'</span>';
}

function targetLadder(){
  var S=STATE.SPY;
  var px=S.price;
  var v=trendVerdict('SPY');
  if(px==null || !S.walls.length || v.state==='flat') return { t1:null, t2:null, t1star:false, dir:null };
  var longSide = v.state==='up';
  var dir = longSide ? 'long' : 'short';
  var beyond=S.walls.filter(function(w){ return longSide ? w.k>px : w.k<px; });
  beyond.sort(function(a,b){ return longSide ? a.k-b.k : b.k-a.k; });
  var kingNode=null;
  beyond.forEach(function(w){ if(!kingNode || w.pct>kingNode.pct) kingNode=w; });
  if(!kingNode){ S.walls.forEach(function(w){ if(!kingNode || w.pct>kingNode.pct) kingNode=w; }); }
  var t2 = kingNode ? kingNode.k : null;
  var t1=null, t1star=false;
  if(t2!=null){
    var between=beyond.filter(function(w){ return longSide ? (w.k<t2) : (w.k>t2); });
    var gk=null;
    between.forEach(function(w){ if(Math.abs(w.k-px)<=GK_MAX_DIST){ if(!gk || w.pct>gk.pct) gk=w; } });
    if(gk) t1=gk.k;
    var acc=accumData('SPY');
    var accPick=null;
    acc.forEach(function(r){
      var onSide = longSide ? (r.k>px && r.k<=t2) : (r.k<px && r.k>=t2);
      if(onSide && r.delta>=ACC_NET){ if(!accPick || r.delta>accPick.delta) accPick=r; }
    });
    if(accPick){ t1=accPick.k; t1star=true; }
  }
  return { t1:t1, t2:t2, t1star:t1star, dir:dir };
}

function targetLine(){
  var S=STATE.SPY;
  var px=S.price;
  var lad=targetLadder();
  var pxTxt=(px!=null)?fmtNum(px):'&mdash;';
  var accent = lad.dir==='long' ? PAL.longAccent : (lad.dir==='short' ? PAL.shortAccent : PAL.ink);
  var ladTxt;
  if(lad.t2==null){ ladTxt='<span style="color:'+PAL.sub+'">&mdash;</span>'; }
  else {
    var star = lad.t1star ? '<span style="color:'+PAL.gold+'">&#9733;</span>' : '';
    var t1part = (lad.t1!=null)
      ? '<span style="color:'+accent+'">T1 '+fmtNum(lad.t1)+'</span>'+star+'<span style="color:'+PAL.sub+'"> &rsaquo; </span>'
      : '';
    ladTxt = t1part+'<span style="color:'+accent+';font-weight:800">T2 '+fmtNum(lad.t2)+'</span>';
  }
  var tip='Target ladder. T1 = gatekeeper: strongest intermediate node between price and the King on the trend side (a star means an accumulation-override node is climbing fast). T2 = King: strongest node on the trend side of price. Sideways trend shows a dash. Descriptive only — not a recommendation.';
  return '<div style="display:flex;justify-content:space-between;align-items:center;padding:5px 9px;background:'+PAL.card+';border:1px solid '+PAL.line+';border-radius:8px;margin:2px 0">'+
    '<span style="display:flex;align-items:center;gap:7px">'+
      '<span style="color:'+PAL.ink+';font-size:13px;font-weight:800">SPY '+pxTxt+'</span>'+
      trendBadgeHtml()+
    '</span>'+
    '<span title="'+tip.replace(/"/g,'')+'" style="font-size:12px;font-weight:700">'+ladTxt+'</span>'+
    '</div>';
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
  var html = sectionHdr('Structural Read');

  try{
    if(RESHUFFLE.SPY===true){
      html += structuralWarn(
        'R5 STRUCTURAL WARNING',
        'Levels recently reshuffled — map continuity is reduced until structure settles.'
      );
    }
  }catch(e){}

  try{
    var anc = resolveAnchor('SPY');
    var nq1 = nodeQuality('SPY');

    if(anc && anc.ok && anc.active && anc.active.k!=null){
      var role = (anc.price!=null && anc.price>=anc.active.k) ? 'support' : 'resistance';
      var dirTxt = anc.dir ? anc.dir.toUpperCase() : '—';
      var srcTxt = anc.source ? anc.source : '—';
      var nodeTxt = (nq1 && nq1.ok && nq1.cls) ? nq1.cls : 'ungraded';
      var oppTxt = (anc.hasOpposing && anc.opposing && anc.opposing.k!=null)
        ? 'grade ' + nodeTxt + ' · source ' + srcTxt + ' · opposing ' + fmtNum(anc.opposing.k)
        : 'grade ' + nodeTxt + ' · source ' + srcTxt + ' · no opposing wall yet';

      html += structuralBox(
        'R1 ACTIVE NODE',
        dirTxt+' '+fmtNum(anc.active.k)+' '+role,
        oppTxt
      );
    } else {
      var why1 = (anc && anc.reason) ? anc.reason : 'unavailable';
      html += structuralBox('R1 ACTIVE NODE', 'Unavailable — ' + why1 + '.', '');
    }
  }catch(e1){
    html += structuralBox('R1 ACTIVE NODE', 'Unavailable — engine error.', '');
  }

  try{
    var nq = nodeQuality('SPY');
    if(nq && nq.ok){
      var gateTxt = nq.gate ? (' · gate ' + nq.gate) : '';
      var trail2 = (nq.trail && nq.trail.length) ? nq.trail.join(' · ') : 'no extra factors';
      html += structuralBox(
        'R2 NODE QUALITY',
        'Class: ' + (nq.cls||'ungraded') + gateTxt + '.',
        trail2
      );
    } else {
      var why2 = (nq && nq.reason) ? nq.reason : 'unavailable';
      html += structuralBox('R2 NODE QUALITY', 'Unavailable — ' + why2 + '.', '');
    }
  }catch(e2){
    html += structuralBox('R2 NODE QUALITY', 'Unavailable — engine error.', '');
  }

  try{
    var pq = pathQuality('SPY');
    if(pq && pq.ok){
      var opp3 = (pq.opposing && pq.opposing.k!=null) ? ('opposing ' + fmtNum(pq.opposing.k)) : 'no opposing wall yet';
      var trail3 = (pq.trail && pq.trail.length) ? pq.trail.join(' · ') : 'no extra factors';
      html += structuralBox(
        'R3 PATH QUALITY',
        'Class: ' + (pq.cls||'ungraded') + ' · ' + opp3 + '.',
        trail3
      );
    } else {
      var why3 = (pq && pq.reason) ? pq.reason : 'unavailable';
      html += structuralBox('R3 PATH QUALITY', 'Unavailable — ' + why3 + '.', '');
    }
  }catch(e3){
    html += structuralBox('R3 PATH QUALITY', 'Unavailable — engine error.', '');
  }

  try{
    var sh = setupHealth('SPY');
    if(sh && sh.ok){
      var stage4 = sh.liveStage ? (' · live stage ' + sh.liveStage) : '';
      var trail4 = (sh.trail && sh.trail.length) ? sh.trail.join(' · ') : 'no extra factors';
      html += structuralBox(
        'R4 SETUP HEALTH',
        'Class: ' + (sh.cls||'ungraded') + stage4 + '.',
        trail4
      );
    } else {
      var why4 = (sh && sh.reason) ? sh.reason : 'unavailable';
      html += structuralBox('R4 SETUP HEALTH', 'Unavailable — ' + why4 + '.', '');
    }
  }catch(e4){
    html += structuralBox('R4 SETUP HEALTH', 'Unavailable — engine error.', '');
  }

  try{
    var rd = reader('SPY');
    var txt = (rd && rd.text) ? rd.text : 'Limited structure to read right now.';
    var badge = (rd && rd.ok) ? 'LIVE' : 'FALLBACK';
    html += '<div style="padding:5px 9px;background:'+PAL.card+';border:1px solid '+PAL.line+';border-radius:8px;margin:2px 0">'+
      '<div style="display:flex;align-items:center;gap:6px;margin-bottom:2px">'+
        '<span style="color:'+PAL.blue+';font-weight:800;font-size:10px;letter-spacing:0.5px">R6 STRUCTURAL READ</span>'+
        '<span style="color:'+PAL.sub+';font-size:8px;font-weight:700;letter-spacing:0.5px;border:1px solid '+PAL.line+';border-radius:10px;padding:0 5px">'+badge+'</span>'+
      '</div>'+
      '<div style="color:'+PAL.ink+';font-size:11px;line-height:1.35">'+txt+'</div>'+
    '</div>';
  }catch(e5){
    html += '<div style="padding:5px 9px;background:'+PAL.card+';border:1px solid '+PAL.line+';border-radius:8px;margin:2px 0">'+
      '<div style="display:flex;align-items:center;gap:6px;margin-bottom:2px">'+
        '<span style="color:'+PAL.blue+';font-weight:800;font-size:10px;letter-spacing:0.5px">R6 STRUCTURAL READ</span>'+
        '<span style="color:'+PAL.sub+';font-size:8px;font-weight:700;letter-spacing:0.5px;border:1px solid '+PAL.line+';border-radius:10px;padding:0 5px">FALLBACK</span>'+
      '</div>'+
      '<div style="color:'+PAL.sub+';font-size:11px;line-height:1.35">Structural read unavailable.</div>'+
    '</div>';
  }

  return html;
}

function accTrajHtml(seq){ return seq.map(function(p){ return p+'%'; }).join(' '); }
function accumBlock(){
  var html=sectionHdr('Accumulation');
  var rows=accumData('SPY');
  if(!rows.length){
    html+='<div style="color:'+PAL.sub+';padding:2px 6px;font-size:11px">No nodes under accumulation yet.</div>';
    return html;
  }
  html+='<div style="max-height:140px;overflow-y:auto">';
  rows.forEach(function(r){
    var hero=r.hero;
    var star=hero?'<span style="color:'+PAL.gold+';font-weight:700;margin-right:3px">★</span>':'';
    var kcol=hero?PAL.gold:PAL.ink;
    var deltaCol=r.delta>=0?PAL.longAccent:PAL.shortAccent;
    var sign=r.delta>=0?'+':'';
    var tip=('Node '+fmtNum(r.k)+' accumulation trajectory: '+accTrajHtml(r.seq)+'. Net change '+sign+r.delta+' pts of King-relative strength'+(hero?'. Being directly accumulated (hero).':'.')).replace(/"/g,'');
    html+='<div title="'+tip+'" style="display:flex;justify-content:space-between;align-items:center;padding:2px 6px;cursor:default">'+
      '<span style="color:'+kcol+';font-weight:700;font-size:11px">'+star+fmtNum(r.k)+'</span>'+
      '<span style="color:'+PAL.sub+';font-size:10px;font-family:monospace">'+accTrajHtml(r.seq)+'</span>'+
      '<span style="color:'+deltaCol+';font-size:10px;font-weight:700;min-width:34px;text-align:right">'+sign+r.delta+'</span>'+
      '</div>';
  });
  html+='</div>';
  return html;
}

function feedStatusHtml(){
  var now=Date.now();
  var f=LASTFEED.SPY;
  var txt, col;
  if(!f){ txt='SPY:idle'; col=PAL.sub; }
  else {
    var age=now-f.ts;
    if(age>FEED_STALE_MS){ txt='SPY:stale'; col=PAL.amber; }
    else { txt='SPY:'+f.feed; col=PAL.longAccent; }
  }
  return '<div style="display:flex;justify-content:space-between;align-items:center;color:'+PAL.sub+';font-size:9px;letter-spacing:0.3px">'+
    '<span style="color:'+col+'">'+txt+'</span>'+
    '<span>feed v9.0</span>'+
    '</div>';
}

function render(){
  if(!elBody) return;
  var html='';
  html+=structuralReadHtml();
  html+=targetLine();
  html+=sep();
  html+=symSignalsHdr('SPY');
  html+=signalGrid();
  html+=sep();
  html+=accumBlock();
  html+='<div style="border-top:1px solid '+PAL.line+';margin:6px 0 3px 0"></div>';
  html+=feedStatusHtml();
  elBody.innerHTML=html;
  var csx=elBody.querySelectorAll('.gpts-clr-sym');
  for(var j=0;j<csx.length;j++){
    (function(el){ el.addEventListener('click', function(){ clearSignalsSym(el.getAttribute('data-sym')); }); })(csx[j]);
  }
}

function refreshSym(sym){
  try{
    var f=LASTFEED[sym];
    if(!f || !f.j){ return; }
    if((Date.now()-f.ts) > FEED_STALE_MS){ return; }
    var j=f.j;
    var ex=extractWalls(j);
    var S=STATE[sym];
    if(ex.price!=null) S.price=ex.price;
    if(ex.king!=null) S.king=ex.king;
    S.walls=ex.walls;
    captureGuards(sym, ex.walls, ex.king);
    stashSlice(sym, j);
    LAST_OK[sym]=Date.now();
    var raw=readFiberCandles(sym);
    if(raw && raw.length){
      var conv=convertFiberCandles(raw);
      applyCandles(sym, conv);
      if(conv.length){ S.price=conv[conv.length-1].c; }
    }
    runMachine(sym);
  }catch(e){}
}

function tick(){
  if(!pastReset()){ render(); return; }
  TODAY=ctTodayStr();
  FUT.ES = (STATE.SPY.price!=null) ? mul(STATE.SPY.price, ES_RATIO) : null;
  FUT.NQ = lastCloseOf('NQ1');
  refreshSym('SPY');
  recordSession();
  render();
}

function boot(){
  installFeedObserver();
  loadCfg();
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

console.log('[GPTS] v9.0 part5 loaded');
})();
