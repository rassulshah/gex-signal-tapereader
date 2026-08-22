// ==UserScript==
// @name         GEX · InsiderFinance levels
// @namespace    gpts
// @version      1.11
// @description  Fetches the option chain InsiderFinance embeds in its page, computes CR/PS/Mag/MaxPain for 0DTE and through-Friday, and hands the result to the Tapereader via localStorage. Deliberately a SEPARATE script so the Tapereader can keep @grant none.
// @match        https://app.skylit.ai/atlas*
// @grant        GM_xmlhttpRequest
// @connect      insiderfinance.io
// @run-at       document-idle
// (v1.11) WITHOUT THESE TWO LINES TAMPERMONKEY WILL NEVER OFFER AN UPDATE FOR THIS SCRIPT — EVER.
// The tapereader has carried them for releases; the companion never did, so it sat silently at an
// old version while the tapereader moved on. That is how v1.9's truncated Zero Gamma survived a
// reload: the repo was fixed, the browser was not, and nothing on either side said so.
// @updateURL    https://raw.githubusercontent.com/rassulshah/gex-signal-tapereader/main/current/gex-if-levels.user.js
// @downloadURL  https://raw.githubusercontent.com/rassulshah/gex-signal-tapereader/main/current/gex-if-levels.user.js
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
// ---- (v1.9) THEIR RENDERED HEADER -------------------------------------------------------------
// `pick()` walks initialData and every published metric came back NULL, and the conclusion drawn from
// that was "their page computes client-side, so we must compute too". That conclusion was WRONG, and it
// is failure pattern #4 for the third time: the payload is not the page. Their numbers are in the
// server-rendered MARKUP, outside __NEXT_DATA__ — verified 2026-08-22 by fetching the raw HTML with no
// JavaScript running at all:
//     Zero Gamma 7646.90 · Call Wall 7900 · Put Wall 7500 · ATM IV 6.2 · Put/Call 1.36 · Term Slope 1.3
// So we read the LABELS the page prints, not class names or DOM shape — class names are exactly what
// churns. Every value stays null-safe: a label that moves yields null and the computed path still runs.
function hdrText(html){
  try{
    return String(html).replace(/<script[\s\S]*?<\/script>/gi,' ').replace(/<style[\s\S]*?<\/style>/gi,' ')
                       .replace(/<[^>]+>/g,' ').replace(/&nbsp;/g,' ').replace(/&amp;/g,'&').replace(/\s+/g,' ');
  }catch(e){ return ''; }
}
function hdrNum(txt, label){
  try{
    var i=txt.indexOf(label); if(i<0) return null;
    var win=txt.slice(i+label.length, i+label.length+80);
    // ⚠ ONE ALTERNATION, NOT TWO. The first version tried a comma-grouped branch FIRST
    // (`[0-9]{1,3}(?:,[0-9]{3})*`) and fell back to a plain branch. On `7646.90` — which is how their
    // page ACTUALLY renders it, with no comma — the first branch matched `764`, succeeded, and the
    // plain branch never ran. Their Zero Gamma reached our face as 764 against a spot of 7674, under
    // THEIR name, sourced IF-pub. The unit test passed because its fixture used `$7,646.90` WITH a
    // comma: the fixture did not match the page. Commas are now optional INSIDE one number, so there
    // is no branch to pick wrong.
    var m=win.match(/(-?)\s?\$?\s?([0-9][0-9,]*(?:\.[0-9]+)?)/);
    if(!m) return null;
    var v=parseFloat(m[2].replace(/,/g,'')); if(!isFinite(v)) return null;
    return (m[1]==='-') ? -v : v;
  }catch(e){ return null; }
}
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
    // (v1.7) THIS USED TO SCAN TOP-LEVEL FIELDS ONLY. Every published metric came back null and the
    // conclusion drawn was "they compute client-side, so we must compute too" — which is exactly the
    // thing this project keeps getting wrong. A nested object would have looked identical to absent.
    // Walk the tree before concluding anything is missing.
    function pick(re, maxDepth){
      var found=null;
      (function walk(o, depth){
        if(found!==null || !o || typeof o!=='object' || depth>(maxDepth||4)) return;
        for(var k in o){
          if(k==='options') continue;                       // the contract array, not a metric
          var v=o[k];
          if(re.test(k) && typeof v==='number' && isFinite(v)){ found=v; return; }
          if(v && typeof v==='object' && !Array.isArray(v)) walk(v, depth+1);
        }
      })(d, 0);
      return found;
    }
    // what the payload actually contains, so "absent" is a finding rather than an assumption
    function shapeOf(o, depth, path, out){
      out=out||[]; if(!o||typeof o!=='object'||depth>3||out.length>60) return out;
      for(var k in o){
        if(k==='options'){ out.push(path+'options[]'); continue; }
        var v=o[k], t=Object.prototype.toString.call(v);
        if(t==='[object Object]'){ out.push(path+k+'{}'); shapeOf(v, depth+1, path+k+'.', out); }
        else if(t==='[object Array]') out.push(path+k+'['+v.length+']');
        else out.push(path+k+'='+String(v).slice(0,18));
      }
      return out;
    }
    // (v1.9) THREE SOURCES, IN ORDER: the payload field, a walk of the payload tree, then THEIR RENDERED
    // HEADER. The header is what finally makes these non-null — see hdrText/hdrNum above. `pubSrc` records
    // which source won for each, so a value on our face can always be traced to where it came from.
    var HT=hdrText(html), pubSrc={};
    // A PRICE LEVEL MUST LOOK LIKE ONE. The truncation bug above produced 764 beside a spot of 7674 and
    // nothing objected — a parser can fail in ways nobody predicted, so the value is checked against the
    // one fact we always have. Levels only; ratios, IV and slopes are not prices and are not gated.
    function levelSane(v){
      if(typeof v!=='number' || !isFinite(v)) return false;
      if(typeof d.spot!=='number' || !(d.spot>0)) return true;   // nothing to compare against
      return v > d.spot*0.5 && v < d.spot*2;
    }
    function three(key, direct, re, label, isLevel){
      function take(v, src){
        if(typeof v!=='number' || !isFinite(v)) return false;
        if(isLevel && !levelSane(v)){ pubSrc[key]='REJECTED:'+src+'='+v; return false; }
        pubSrc[key]=src; return true;
      }
      if(take(direct,'payload')) return direct;
      var v=pick(re);      if(take(v,'tree'))   return v;
      v=hdrNum(HT, label); if(take(v,'header')) return v;
      if(!pubSrc[key]) pubSrc[key]=null;
      return null;
    }
    var pub={
      zeroGamma:three('zeroGamma', d.zeroGamma, /zero.?gamma|gamma.?flip|flip.?point/i, 'Zero Gamma', true),
      callWall :three('callWall',  d.callWall,  /call.?wall/i, 'Call Wall', true),
      putWall  :three('putWall',   d.putWall,   /put.?wall/i,  'Put Wall', true),
      // (v1.4) Their page prints these; whether they are in the PAYLOAD is the open question that
      // decides whether SKEW is free or has to be computed. Zero Gamma was rendered and absent, so
      // "it is on the page" proves nothing.
      skew     :three('skew',      null, /(^|[^a-z])skew$|delta.?skew|d25.?skew/i, '25' + String.fromCharCode(916) + ' Skew'),
      skewSlope:three('skewSlope', null, /skew.?slope/i, 'Skew Slope'),
      termSlope:three('termSlope', null, /term.?slope/i, 'Term Slope'),
      atmIV    :three('atmIV',     null, /atm.?iv|iv.?atm/i, 'ATM IV'),
      pcRatio  :three('pcRatio',   null, /put.?call.?ratio|pc.?ratio/i, 'Put/Call'),
      maxPain  :three('maxPain',   null, /max.?pain/i, 'Max Pain', true)
    };
    // ⚠ THEIR HEADER WALLS ARE ALL-EXPIRY (CW 7900 / PW 7500 on 2026-08-22) while our ladder's CR0/PS0
    // are 0DTE (7700/7665, verified reproducing their 0DTE view exactly). BOTH are "their values" and
    // they answer DIFFERENT questions. Never substitute one for the other — that is failure pattern #1
    // wearing a "use their published numbers" badge.
    pub.wallsAreAllExpiry = true;
    // (v1.4) ONE-LINE SHAPE ANSWER: which fields does a single option actually carry? This decides
    // whether DEX (needs delta) and a computed SKEW (needs per-contract IV) are buildable at all.
    var optKeys=null;
    try{ if(d.options && d.options.length) optKeys=Object.keys(d.options[0]).join(','); }catch(eK){}
    var shape=null;
    try{ shape=shapeOf(d,0,'',[]).slice(0,60).join(' | '); }catch(eSh){}
    return { spot:d.spot, options:d.options, t:d.timestamp||null, stale:!!d.isStale,
             ticker:d.ticker||null, pub:pub, pubSrc:pubSrc, optKeys:optKeys, shape:shape };
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


// ---- (v1.5) DEX AND A REAL 25-DELTA SKEW ----
// Their payload carries per-contract `delta` and `impliedVol` (confirmed 2026-08-22 via optKeys:
// strike,expireYear,expireMonth,expireDay,cp,gamma,delta,openInterest,impliedVol,bid,ask). Their own
// published metrics come back NULL because their page computes them client-side, so we compute.
//
// DEX = sum(delta * OI * 100 * spot). Puts carry negative delta already, so no sign flip is applied.
// Read it as a hedging map, not an arrow: dealers short delta must BUY as price rises.
//
// SKEW = 25-delta put IV minus 25-delta call IV, which is the metric their page prints as "25Δ Skew".
// Nearest-delta match on each side rather than an interpolation, so one thin quote cannot invent a
// number. Positive = puts richer = downside bid.
function dexSkewFor(opts, spot, keep){
  var sel=[]; for(var i=0;i<opts.length;i++){ if(keep(opts[i])) sel.push(opts[i]); }
  if(!sel.length) return null;
  var net=0, byK={}, n=0;
  var bestC=null, bestP=null, atmC=null, atmP=null;
  for(var q=0;q<sel.length;q++){
    var o=sel[q];
    if(typeof o.delta!=='number' || typeof o.openInterest!=='number') continue;
    var d=o.delta*o.openInterest*100*spot;
    net+=d; n++;
    var b=byK[o.strike]; if(!b) b=byK[o.strike]={k:o.strike, dex:0};
    b.dex+=d;
    if(typeof o.impliedVol==='number' && o.impliedVol>0){
      if(o.cp==='C'){
        var ec=Math.abs(o.delta-0.25);
        if(!bestC || ec<bestC.e) bestC={e:ec, iv:o.impliedVol, k:o.strike, d:o.delta};
        var ac=Math.abs(o.delta-0.50);
        if(!atmC || ac<atmC.e) atmC={e:ac, iv:o.impliedVol, k:o.strike};
      } else {
        var ep=Math.abs(o.delta+0.25);
        if(!bestP || ep<bestP.e) bestP={e:ep, iv:o.impliedVol, k:o.strike, d:o.delta};
        var ap=Math.abs(o.delta+0.50);
        if(!atmP || ap<atmP.e) atmP={e:ap, iv:o.impliedVol, k:o.strike};
      }
    }
  }
  if(!n) return null;
  var rows=[]; for(var k2 in byK) rows.push(byK[k2]);
  rows.sort(function(a,b2){ return a.k-b2.k; });
  // (v1.8) BOTH profiles from THEIR chain. The panel was scoring IF's levels against SKYLIT's gamma —
  // two different books — and Skylit's gamma peaks at spot while IF's walls sit well away from it, so
  // every level scored as gamma-thin. The left column is labelled "IF structure"; it has to BE IF.
  // Skylit gamma keeps its own job on the flow side.
  var prof=[], gprof=[], gByK={};
  for(var q2=0;q2<sel.length;q2++){
    var o2=sel[q2];
    if(typeof o2.gamma!=='number' || typeof o2.openInterest!=='number') continue;
    var gg = o2.gamma*o2.openInterest*100*spot*spot*0.01*(o2.cp==='C'?1:-1);
    gByK[o2.strike]=(gByK[o2.strike]||0)+gg;
  }
  for(var z=0;z<rows.length;z++){
    var pk=rows[z].k;
    if(Math.abs(pk-spot) > spot*0.05) continue;
    prof.push([pk, +(rows[z].dex/1e6).toFixed(1)]);
    if(gByK[pk]!=null) gprof.push([pk, +(gByK[pk]/1e6).toFixed(1)]);
  }
  // only trust a 25-delta reading when the match is actually near 25 delta
  var skew=null, skewOk=(bestC && bestP && bestC.e<=0.08 && bestP.e<=0.08);
  if(skewOk) skew=+(((bestP.iv-bestC.iv))*100).toFixed(2);
  var atmIV=null;
  if(atmC && atmP) atmIV=+(((atmC.iv+atmP.iv)/2)*100).toFixed(2);
  else if(atmC) atmIV=+(atmC.iv*100).toFixed(2);
  return { netDex:net, dexProf:prof, gexProf:gprof, strikes:rows.length,
           skew25:skew, skewPutIV:skewOk?+(bestP.iv*100).toFixed(2):null,
           skewCallIV:skewOk?+(bestC.iv*100).toFixed(2):null,
           skewPutK:skewOk?bestP.k:null, skewCallK:skewOk?bestC.k:null,
           atmIV:atmIV };
}

// ---- (v1.6) BLACK-SCHOLES DERIVED READS ----
// Their payload gives strike, cp, delta, gamma, impliedVol, openInterest, bid and ask per contract.
// Everything below is computed from those; nothing is taken on faith from a number they render, because
// every published metric on their page comes back null in the payload — they compute client-side.
// Rates and dividends are taken as zero: over the days-to-expiry horizons this panel trades, the carry
// term moves these numbers less than the bid/ask spread does, and pretending to know r would be false
// precision rather than accuracy.
function npdf(x){ return Math.exp(-0.5*x*x)/Math.sqrt(2*Math.PI); }
function yearsTo(o, todayNum){
  try{
    var e=o.expireYear*10000+o.expireMonth*100+o.expireDay;
    var y1=Math.floor(todayNum/10000), m1=Math.floor(todayNum/100)%100, d1=todayNum%100;
    var y2=Math.floor(e/10000), m2=Math.floor(e/100)%100, d2=e%100;
    var days=(Date.UTC(y2,m2-1,d2)-Date.UTC(y1,m1-1,d1))/86400000;
    if(days<0) return null;
    // an expiring contract still has hours of life; treat 0DTE as a few hours rather than zero, which
    // would divide by nothing and blow every greek to Infinity
    return Math.max(days,0.25)/365;
  }catch(e){ return null; }
}
function d1Of(S,K,sig,T){
  if(!(S>0&&K>0&&sig>0&&T>0)) return null;
  return (Math.log(S/K)+(sig*sig/2)*T)/(sig*Math.sqrt(T));
}

// EXPECTED MOVE — the ATM straddle, which is the market's own estimate of the move by expiry.
// Mid prices, and only when both legs actually quote: a one-sided straddle is not a straddle.
function expectedMove(opts, spot, keep){
  try{
    var bestC=null, bestP=null;
    for(var i=0;i<opts.length;i++){
      var o=opts[i]; if(!keep(o)) continue;
      if(typeof o.bid!=='number' || typeof o.ask!=='number' || o.ask<=0) continue;
      var d=Math.abs(o.strike-spot);
      if(o.cp==='C'){ if(!bestC||d<bestC.d) bestC={d:d,o:o}; }
      else { if(!bestP||d<bestP.d) bestP={d:d,o:o}; }
    }
    if(!bestC||!bestP) return null;
    if(Math.abs(bestC.o.strike-bestP.o.strike)>0.001) return null;   // must be the SAME strike
    if(Math.abs(bestC.o.strike-spot) > spot*0.01) return null;       // and genuinely at the money
    var cm=(bestC.o.bid+bestC.o.ask)/2, pm=(bestP.o.bid+bestP.o.ask)/2;
    if(!(cm>0)||!(pm>0)) return null;
    var em=cm+pm;
    return { em:+em.toFixed(2), pct:+((em/spot)*100).toFixed(2), k:bestC.o.strike,
             call:+cm.toFixed(2), put:+pm.toFixed(2) };
  }catch(e){ return null; }
}

// GAMMA FLIP — a FALLBACK ONLY. If their payload carries a zero-gamma level we take theirs; this exists
// for when it does not, and whatever uses it must label it as derived. Total dealer gamma is
// re-evaluated at candidate SPOTS, because gamma is spot-dependent: the level where the book crosses
// from net long to net short is NOT the strike where a running total happens to change sign, which is
// the cheap approximation this replaces. Scans +-4% and interpolates the crossing nearest spot.
function gammaFlip(opts, spot, keep, todayNum){
  try{
    var sel=[];
    for(var i=0;i<opts.length;i++){
      var o=opts[i]; if(!keep(o)) continue;
      if(typeof o.impliedVol!=='number' || typeof o.openInterest!=='number' || !(o.impliedVol>0)) continue;
      var T=yearsTo(o, todayNum); if(!T) continue;
      sel.push({K:o.strike, sig:o.impliedVol, T:T, oi:o.openInterest, call:(o.cp==='C')});
    }
    if(sel.length<20) return null;
    function netAt(S){
      var g=0;
      for(var j=0;j<sel.length;j++){
        var c=sel[j];
        var d1=d1Of(S,c.K,c.sig,c.T); if(d1==null) continue;
        var gam=npdf(d1)/(S*c.sig*Math.sqrt(c.T));
        g += gam*c.oi*100*S*S*0.01*(c.call?1:-1);
      }
      return g;
    }
    var lo=spot*0.96, hi=spot*1.04, steps=48, prevS=null, prevV=null, best=null;
    for(var k=0;k<=steps;k++){
      var S=lo+((hi-lo)*k/steps), v=netAt(S);
      if(prevV!=null && ((prevV<0&&v>=0)||(prevV>0&&v<=0))){
        var span=v-prevV;
        var x=(span!==0)?(prevS+((0-prevV)/span)*(S-prevS)):S;
        if(best===null || Math.abs(x-spot)<Math.abs(best-spot)) best=+x.toFixed(2);
      }
      prevS=S; prevV=v;
    }
    if(best==null) return null;
    return { flip:best, netAtSpot:netAt(spot) };
  }catch(e){ return null; }
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
    asOf:Date.now(), today:W.today, friday:W.friday, rolled:!!W.rolled, pub:(ch.pub||null), pubSrc:(ch.pubSrc||null), optKeys:(ch.optKeys||null), shape:(ch.shape||null),
    dte0:{ exps:[front], lv:levelsFor(ch.options, ch.spot, function(o){ return ymdOf(o)===front; }),
           ds:dexSkewFor(ch.options, ch.spot, function(o){ return ymdOf(o)===front; }),
           em:expectedMove(ch.options, ch.spot, function(o){ return ymdOf(o)===front; }),
           gf:gammaFlip(ch.options, ch.spot, function(o){ return ymdOf(o)===front; }, W.today) },
    toFri:{ exps:inFri, lv:levelsFor(ch.options, ch.spot, function(o){ return ymdOf(o)>=W.today && ymdOf(o)<=W.friday; }),
            ds:dexSkewFor(ch.options, ch.spot, function(o){ return ymdOf(o)>=W.today && ymdOf(o)<=W.friday; }),
            em:expectedMove(ch.options, ch.spot, function(o){ return ymdOf(o)>=W.today && ymdOf(o)<=W.friday; }),
            gf:gammaFlip(ch.options, ch.spot, function(o){ return ymdOf(o)>=W.today && ymdOf(o)<=W.friday; }, W.today) },
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
