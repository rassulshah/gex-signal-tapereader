// ==UserScript==
// @name         GEX · InsiderFinance levels
// @namespace    gpts
// @version      1.3
// @description  Fetches the option chain InsiderFinance embeds in its page, computes CR/PS/Mag/MaxPain for 0DTE and through-Friday, and hands the result to the Tapereader via localStorage. Deliberately a SEPARATE script so the Tapereader can keep @grant none.
// @match        https://app.skylit.ai/atlas*
// @grant        GM_xmlhttpRequest
// @connect      insiderfinance.io
// @run-at       document-idle
// ==/UserScript==
(function(){
'use strict';

// ============================================================================================
// WHY THIS IS A SEPARATE SCRIPT
//
// The Tapereader runs @grant none, which puts it in PAGE context — and that is load-bearing: its feed
// hooks patch window.fetch / XMLHttpRequest to capture Skylit's gex/levels payload. Adding ANY @grant
// moves it into Tampermonkey's sandbox, where those hooks would patch a wrapper instead of the page and
// the tape would go dark.
//
// But reading insiderfinance.io from app.skylit.ai needs a cross-origin request, and their server sends no
// Access-Control-Allow-Origin — verified live in the console: fetch(...) -> "BLOCKED Failed to fetch".
// GM_xmlhttpRequest is privileged past CORS, and it requires a grant.
//
// So: two scripts on the same page. This one takes the grant and has no tape to break. The Tapereader
// stays exactly as it is and reads the result out of localStorage, which both share because they run on
// the same origin. If this script dies, the tape does not notice.
//
// WHAT IT READS
// Their page embeds the FULL option chain in __NEXT_DATA__ — 13,210 contracts for SPY, each with
// {strike, expireYear, expireMonth, expireDay, cp, gamma, delta, openInterest, impliedVol, bid, ask}.
// So we do not scrape their rendered header at all; we recompute from source. Their CSS, layout and
// wording can change freely. It only breaks if they stop embedding the chain.
//
// VERIFIED 2026-08-20 against their own published figures, spot 761.14 — every value reproduced exactly:
//   All expirations : Call 17.8B · Put -32.2B · Net -14.4B · ratio 0.55 · CR 800 · PS 760 · Mag 760
//   through Friday  : ratio 0.40 · CR 775 · PS 760 · Mag 760
//   0DTE            : ratio 0.02 · PS 760 · Mag 760 · CR suppressed (call side 0.05B -> they print N/A)
// ============================================================================================

var LS_KEY   = 'gpts_if_chain_v1';
var POLL_MS  = 5*60*1000;        // their walls move slowly; 5 minutes is plenty and stays polite
// (v1.3) SPX, NOT SPY, FOR THE ES LADDER. A gamma level is a STRIKE where open interest concentrates,
// not a price you can rescale: SPX strikes sit on a 5-point grid and SPY on a 1-point grid, and the two
// are separate chains with separate positioning. Dividing an SPX wall by ~10 invents a SPY level that
// does not exist in SPY's book. ES is a future ON SPX, so SPX is the book that actually governs it —
// and converting SPX to ES is a ~1.003 basis rather than a ~10.05 multiplier, so a rounding difference
// stays a rounding difference instead of becoming five ES points.
var SYMS     = ['SPX','QQQ'];
var SIDE_MIN = 0.05;             // a side under 5% of the book names no wall (they print N/A at ~2%)
var FAILCAP  = 4;
var fails    = {};

function log(){ try{ if(window.__GEXIF_DEBUG) console.log.apply(console,['[GEXIF]'].concat([].slice.call(arguments))); }catch(e){} }

// ---- extract the embedded chain -----------------------------------------------------------
// Anchored on the __NEXT_DATA__ script tag by id, not by position or class.
function extractChain(html){
  try{
    var m = html.match(/<script[^>]+id="__NEXT_DATA__"[^>]*>([\s\S]*?)<\/script>/);
    if(!m) return { err:'no __NEXT_DATA__ block — they may have changed how the page ships its data' };
    var J = JSON.parse(m[1]);
    var d = J && J.props && J.props.pageProps && J.props.pageProps.initialData;
    if(!d) return { err:'__NEXT_DATA__ present but initialData missing' };
    if(!d.options || !d.options.length) return { err:'initialData carries no options array' };
    if(typeof d.spot!=='number' || !(d.spot>0)) return { err:'no spot price in the payload' };
    // (v1.2) THEIR PUBLISHED NUMBERS, TAKEN AS-IS. Their page prints Zero Gamma, Call Wall and Put
    // Wall in its header; computing our own versions of numbers they already publish is how a value
    // ends up on our face with their name on it and a different figure underneath.
    // Key names are tried directly, then found by pattern, so a rename on their side self-heals.
    function pick(re){
      for(var k in d){ if(re.test(k) && typeof d[k]==='number' && isFinite(d[k])) return d[k]; }
      return null;
    }
    var pub={
      zeroGamma:(typeof d.zeroGamma==='number')?d.zeroGamma:pick(/zero.?gamma|gamma.?flip|flip.?point/i),
      callWall :(typeof d.callWall ==='number')?d.callWall :pick(/call.?wall/i),
      putWall  :(typeof d.putWall  ==='number')?d.putWall  :pick(/put.?wall/i)
    };
    return { spot:d.spot, options:d.options, t:d.timestamp||null, stale:!!d.isStale,
             ticker:d.ticker||null, pub:pub };
  }catch(e){ return { err:'parse failed: '+e.message }; }
}

// ---- the windows --------------------------------------------------------------------------
// 0DTE  = the nearest expiration on or after today.
// toFri = every expiration from today through the Friday of the CURRENT week (the user's definition —
//         deliberately NOT InsiderFinance's "Next week", which is a rolling 7 days).
function ymdOf(o){ return o.expireYear*10000 + o.expireMonth*100 + o.expireDay; }
function ymdNum(dt){ return dt.getUTCFullYear()*10000 + (dt.getUTCMonth()+1)*100 + dt.getUTCDate(); }
function windows(payloadTs){
  // Anchor on the payload's own timestamp so we follow THEIR clock, not the browser's.
  var now = payloadTs ? new Date(payloadTs) : new Date();
  // shift to US Eastern-ish so a late-UTC timestamp does not roll the trading date forward
  var et = new Date(now.getTime() - 4*3600*1000);
  var today = ymdNum(et);
  var dow = et.getUTCDay();                 // 0 Sun .. 6 Sat
  var toFriday = (dow===0) ? 5 : (dow===6 ? 6 : (5-dow));
  if(toFriday<0) toFriday=0;
  // (v1.1) THE FRIDAY COLLAPSE, same bug the Tapereader carried. On a Friday "through Friday of
  // this week" IS today, so the through-Friday window becomes a second copy of the 0DTE window and
  // the two level sets print identical numbers. Roll to NEXT Friday so the second window is
  // actually a second window. Disclosed via `rolled` so the panel never shows it silently.
  var rolled=false;
  if(dow===5){ toFriday=7; rolled=true; }
  var fri = new Date(et.getTime() + toFriday*86400000);
  return { today:today, friday:ymdNum(fri), rolled:rolled };
}

// ---- the maths ----------------------------------------------------------------------------
// GEX = gamma * OI * 100 * spot^2 * 0.01, puts negative. Verified: reproduces their published
// Call/Put/Net GEX and all three walls exactly.
function levelsFor(opts, spot, keep){
  var sel=[]; for(var i=0;i<opts.length;i++){ if(keep(opts[i])) sel.push(opts[i]); }
  if(!sel.length) return null;
  var byK={}, sc=0, sp=0, cOI=0, pOI=0;
  for(var q=0;q<sel.length;q++){
    var o=sel[q];
    if(typeof o.gamma!=='number' || typeof o.openInterest!=='number') continue;
    var g = o.gamma*o.openInterest*100*spot*spot*0.01*(o.cp==='C'?1:-1);
    if(o.cp==='C'){ sc+=g; cOI+=o.openInterest; } else { sp+=g; pOI+=o.openInterest; }
    var b=byK[o.strike]; if(!b) b=byK[o.strike]={k:o.strike, net:0};
    b.net+=g;
  }
  var rows=[]; for(var k in byK) rows.push(byK[k]);
  if(!rows.length) return null;
  rows.sort(function(a,b){ return a.k-b.k; });
  var cr=null,crN=null, ps=null,psN=null, mag=null,magA=-1;
  for(var r=0;r<rows.length;r++){
    var R=rows[r], a=Math.abs(R.net);
    if(a>magA){ magA=a; mag=R.k; }
    if(R.k>spot && (crN===null || R.net>crN)){ crN=R.net; cr=R.k; }   // most POSITIVE net above spot
    if(R.k<spot && (psN===null || R.net<psN)){ psN=R.net; ps=R.k; }   // most NEGATIVE net below spot
  }
  // A side holding almost none of the book names no wall — their page prints N/A in exactly this case
  // (0DTE call GEX was 0.05B against 2.42B of put, and they showed N/A).
  var tot=Math.abs(sc)+Math.abs(sp);
  var callShare = tot>0 ? Math.abs(sc)/tot : 0;
  var putShare  = tot>0 ? Math.abs(sp)/tot : 0;
  var crSup=null, psSup=null;
  if(cr!=null && callShare<SIDE_MIN){ crSup={k:cr, share:+(callShare*100).toFixed(2)}; cr=null; }
  if(ps!=null && putShare <SIDE_MIN){ psSup={k:ps, share:+(putShare*100).toFixed(2)}; ps=null; }
  // MAX PAIN — the strike minimising total payout to option holders at expiry. Needs open interest,
  // which is why this was impossible from the Skylit feed and is trivial here.
  var mp=null, mpv=null;
  for(var s1=0;s1<rows.length;s1++){
    var K=rows[s1].k, pay=0;
    for(var s2=0;s2<sel.length;s2++){
      var oo=sel[s2]; if(typeof oo.openInterest!=='number') continue;
      pay += oo.openInterest * (oo.cp==='C' ? Math.max(0,K-oo.strike) : Math.max(0,oo.strike-K));
    }
    if(mpv===null||pay<mpv){ mpv=pay; mp=K; }
  }
  return { cr:cr, ps:ps, mag:mag, maxPain:mp,
           crSuppressed:crSup, psSuppressed:psSup,
           callGEX:sc, putGEX:sp, netGEX:sc+sp,
           ratio:(sp!==0 ? +(sc/Math.abs(sp)).toFixed(3) : null),
           callShare:+(callShare*100).toFixed(2),
           pcOI:(cOI>0 ? +(pOI/cOI).toFixed(2) : null),
           contracts:sel.length, strikes:rows.length,
           kMin:rows[0].k, kMax:rows[rows.length-1].k };
}

function computeAll(ch){
  var W=windows(ch.t);
  var exps={}; for(var i=0;i<ch.options.length;i++){ exps[ymdOf(ch.options[i])]=1; }
  var all=Object.keys(exps).map(Number).sort(function(a,b){return a-b;});
  var front=null; for(var f=0;f<all.length;f++){ if(all[f]>=W.today){ front=all[f]; break; } }
  if(front==null) front=all[0];
  var inFri=all.filter(function(e){ return e>=W.today && e<=W.friday; });
  if(!inFri.length) inFri=[front];
  return {
    spot:ch.spot, ticker:ch.ticker, payloadT:ch.t, stale:ch.stale,
    asOf:Date.now(), today:W.today, friday:W.friday, rolled:!!W.rolled, pub:(ch.pub||null),
    dte0:{ exps:[front], lv:levelsFor(ch.options, ch.spot, function(o){ return ymdOf(o)===front; }) },
    toFri:{ exps:inFri, lv:levelsFor(ch.options, ch.spot, function(o){ return ymdOf(o)>=W.today && ymdOf(o)<=W.friday; }) },
    all:{ exps:[all[0],all[all.length-1]], nExps:all.length, lv:levelsFor(ch.options, ch.spot, function(){ return true; }) }
  };
}

// ---- fetch + store -------------------------------------------------------------------------
function store(sym, obj){
  try{
    var cur={}; try{ cur=JSON.parse(localStorage.getItem(LS_KEY)||'{}')||{}; }catch(e){}
    cur[sym]=obj; cur._v=1; cur._at=Date.now();
    localStorage.setItem(LS_KEY, JSON.stringify(cur));
    log('stored', sym, obj && obj.toFri && obj.toFri.lv && obj.toFri.lv.cr);
  }catch(e){ log('store failed', e.message); }
}
function storeErr(sym, msg){
  try{
    var cur={}; try{ cur=JSON.parse(localStorage.getItem(LS_KEY)||'{}')||{}; }catch(e){}
    cur[sym]={ err:msg, asOf:Date.now() }; cur._v=1; cur._at=Date.now();
    localStorage.setItem(LS_KEY, JSON.stringify(cur));
  }catch(e){}
}

function pull(sym){
  if((fails[sym]||0)>=FAILCAP) return;
  try{
    GM_xmlhttpRequest({
      method:'GET',
      url:'https://www.insiderfinance.io/gamma-exposure/'+sym,
      timeout:25000,
      onload:function(res){
        try{
          if(!res || res.status!==200){ fails[sym]=(fails[sym]||0)+1; storeErr(sym,'HTTP '+(res&&res.status)); return; }
          var ch=extractChain(res.responseText||'');
          if(ch.err){ fails[sym]=(fails[sym]||0)+1; storeErr(sym, ch.err); return; }
          fails[sym]=0;
          store(sym, computeAll(ch));
        }catch(e){ fails[sym]=(fails[sym]||0)+1; storeErr(sym,'compute: '+e.message); }
      },
      onerror:function(){ fails[sym]=(fails[sym]||0)+1; storeErr(sym,'network error'); },
      ontimeout:function(){ fails[sym]=(fails[sym]||0)+1; storeErr(sym,'timeout'); }
    });
  }catch(e){ fails[sym]=(fails[sym]||0)+1; storeErr(sym,'GM_xmlhttpRequest unavailable: '+e.message); }
}

function tick(){ try{ if(document.visibilityState!=='visible') return; for(var i=0;i<SYMS.length;i++) pull(SYMS[i]); }catch(e){} }

setTimeout(tick, 4000);
setInterval(tick, POLL_MS);

// debug surface, on the page so the Tapereader's console can reach it
try{
  window.__gexif = {
    pull:function(s){ fails[s||'SPY']=0; pull(s||'SPY'); return 'pulling '+(s||'SPY'); },
    read:function(){ try{ return JSON.parse(localStorage.getItem(LS_KEY)||'null'); }catch(e){ return null; } },
    clear:function(){ try{ localStorage.removeItem(LS_KEY); }catch(e){} return 'cleared'; },
    fails:function(){ return fails; },
    _extract:extractChain, _levelsFor:levelsFor, _windows:windows
  };
}catch(e){}
})();
