// (v10.48) DUAL-CAPTURE + MODE-INDEPENDENT KING.
// The Skylit heatmap can display GEX (gamma), VEX (vanna) or GEX+VEX (combined),
// but only the DISPLAYED data_type is pushed on the network. These tests prove:
//   1. feedStructMap() builds the correct King strike + signed %King straight from
//      a raw gamma payload (extractWalls-derived), independent of the DOM tape.
//   2. onFeed() ignores `combined` for LASTFEED (no contamination), writes `gamma`
//      to LASTFEED and `vanna` to LASTVEX, and records the displayed mode in LASTDISP.
//   3. With LASTDISP='combined' (or 'vanna'), tapeMap() resolves structure from the
//      pure-gamma feed, NOT from a (differently-crowned) mocked DOM tape.
//   4. ensureFeeds() self-fetches only the STALE / missing mode(s).
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m+(g!==undefined?' -> '+g:''));} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+g:''));} };

// ---- global mocks the extracted functions reference (bare names) ----
global.mul=function(a,b){ return a/(1/b); };
global.MIN_STRENGTH=20;
global.FEED_STALE_MS=12000;
global.LASTFEED={ SPY:null, QQQ:null };
global.LASTVEX ={ SPY:null, QQQ:null };
global.LASTDISP={ SPY:null, QQQ:null };
global.LASTFEEDURL=null;
global.observeFeedCadence=function(){};              // no-op cadence tracker
global.TAPE_CACHE={ SPY:{t:0,data:null}, QQQ:{t:0,data:null} };
let SELF_CALLS=[];
global.selfFetch=function(sym,type){ SELF_CALLS.push(sym+type); };  // recorder mock
global.document={ visibilityState:'visible' };
// mocked DOM tape that crowns a DIFFERENT King (999) than the gamma feed (775):
global.readTapeFromDOM=function(sym){ return { pct:{'999.00':100,'998.00':50,'997.00':40,'996.00':30,'995.00':25,'994.00':22}, king:999, count:6, kingSrc:'dollar', kingConflict:false }; };

eval(src.match(/var FEED_REJECTS=[\s\S]*?;\n/)[0]);
// (v14.55) tapeMap became a thin front door over tapeMapLive: it serves the latched
// close-of-session book when the live front expiry has rolled away. This suite tests the LIVE
// reader, so both halves are loaded and the stale-book gate is stubbed OFF.
global.showingStaleBook=()=>false;
eval(['feedNewestT','extractWalls','synthDerived','feedStructMap','onFeed','ensureFeeds','tapeMapLive','tapeMap'].map(ex).join('\n'));

// ---- synthetic RAW GAMMA payload: King 775 (|v| largest), 770 is -gamma, 780 +gamma ----
function gammaPayload(){
  return { levels:[ { s:772.0, l:[
    {k:775, v: 1000, d: 1, net:0},   // King  -> 100%  (+gamma)
    {k:770, v: -600, d:-1, net:0},   // 60%   -> -60   (-gamma)
    {k:780, v:  400, d: 1, net:0},   //  40%  ->  40   (+gamma)
    {k:765, v:  100, d:-1, net:0}    //  10%  -> dropped (< MIN_STRENGTH 20)
  ] } ] };
}

// ========================= 1. feedStructMap =========================
global.LASTFEED.SPY={ j:gammaPayload(), feed:'gamma', ts:Date.now() };
var fm=feedStructMap('SPY');
ok(!!fm, '1a feedStructMap returns a map from a gamma payload');
ok(fm && fm.king===775, '1b King is the largest-|v| strike (775)', fm&&fm.king);
ok(fm && fm.kingSrc==='feed' && fm.fromFeed===true, '1c kingSrc=feed / fromFeed=true');
ok(fm && fm.kingConflict===false && fm.kingKd===null, '1d no conflict, kingKd null');
ok(fm && fm.kingTagged===775, '1e kingTagged mirrors King', fm&&fm.kingTagged);
ok(fm && fm.pct['775.00']===100, '1f King node is 100%', fm&&fm.pct['775.00']);
ok(fm && fm.pct['770.00']===-60, '1g -gamma node carries negative sign (-60)', fm&&fm.pct['770.00']);
ok(fm && fm.pct['780.00']===40, '1h +gamma node positive (40)', fm&&fm.pct['780.00']);
ok(fm && fm.pct['765.00']===undefined, '1i sub-threshold node dropped (765)');
ok(feedStructMap('QQQ')===null, '1j null when no feed for the symbol');

// ========================= 2. onFeed routing =========================
global.LASTFEED={ SPY:null, QQQ:null };
global.LASTVEX ={ SPY:null, QQQ:null };
global.LASTDISP={ SPY:null, QQQ:null };
// combined display must NOT write LASTFEED (the A.4 wrong-King contamination)
onFeed('SPY','combined',gammaPayload());
ok(global.LASTFEED.SPY===null, '2a combined leaves LASTFEED untouched');
ok(global.LASTDISP.SPY==='combined', '2b combined display recorded in LASTDISP', global.LASTDISP.SPY);
// gamma writes LASTFEED
onFeed('SPY','gamma',gammaPayload());
ok(global.LASTFEED.SPY && global.LASTFEED.SPY.feed==='gamma', '2c gamma writes LASTFEED');
ok(global.LASTDISP.SPY==='gamma', '2d gamma display recorded');
// vanna writes LASTVEX, not LASTFEED
var feedRef=global.LASTFEED.SPY;
onFeed('SPY','vanna',gammaPayload());
ok(!!global.LASTVEX.SPY, '2e vanna writes LASTVEX');
ok(global.LASTFEED.SPY===feedRef, '2f vanna does NOT overwrite LASTFEED');
ok(global.LASTDISP.SPY==='vanna', '2g vanna display recorded');
// viaSelf=true must NOT clobber LASTDISP (out-of-band gamma while user views vanna)
onFeed('SPY','gamma',gammaPayload(),true);
ok(global.LASTDISP.SPY==='vanna', '2h self-fetch (viaSelf) does not change LASTDISP', global.LASTDISP.SPY);

// ================ 3. tapeMap resolves King from gamma feed, not DOM ================
global.LASTFEED.SPY={ j:gammaPayload(), feed:'gamma', ts:Date.now() };
// display = combined -> structure MUST come from the gamma feed (King 775), not the
// mocked DOM tape (King 999).
global.LASTDISP.SPY='combined';
global.TAPE_CACHE.SPY={t:0,data:null};
var tmC=tapeMap('SPY');
ok(tmC && tmC.king===775, '3a combined display -> King from gamma feed (775) not DOM (999)', tmC&&tmC.king);
ok(tmC && tmC.fromFeed===true, '3b combined map is feed-derived');
// display = vanna -> same: feed structure wins
global.LASTDISP.SPY='vanna';
global.TAPE_CACHE.SPY={t:0,data:null};
var tmV=tapeMap('SPY');
ok(tmV && tmV.king===775, '3c vanna display -> King still 775 from gamma feed', tmV&&tmV.king);
// display = gamma -> the readable DOM tape is used (King 999 from mock)
global.LASTDISP.SPY='gamma';
global.TAPE_CACHE.SPY={t:0,data:null};
var tmG=tapeMap('SPY');
ok(tmG && tmG.king===999, '3d gamma display -> DOM tape used (King 999)', tmG&&tmG.king);
// unreadable DOM tape (count<5) falls back to the gamma feed
global.readTapeFromDOM=function(){ return null; };
global.LASTDISP.SPY='gamma';
global.TAPE_CACHE.SPY={t:0,data:null};
var tmF=tapeMap('SPY');
ok(tmF && tmF.king===775 && tmF.fromFeed===true, '3e unreadable tape -> falls back to gamma feed', tmF&&tmF.king);

// ================ 4. ensureFeeds self-fetches only stale/missing modes ================
global.LASTFEEDURL='https://app.skylit.ai/tv/api/gex/levels?symbol=SPY&data_type=gamma&v=1';
var nowT=Date.now();
global.LASTFEED={ SPY:{j:{},feed:'gamma',ts:nowT}, QQQ:null };   // SPY gamma FRESH, QQQ gamma MISSING
global.LASTVEX ={ SPY:{j:{},ts:nowT-20000},        QQQ:null };   // SPY vanna STALE, QQQ vanna MISSING
SELF_CALLS=[];
ensureFeeds();
ok(SELF_CALLS.indexOf('SPYgamma')===-1, '4a fresh SPY gamma is NOT self-fetched');
ok(SELF_CALLS.indexOf('SPYvanna')>=0, '4b stale SPY vanna IS self-fetched');
ok(SELF_CALLS.indexOf('QQQgamma')>=0, '4c missing QQQ gamma IS self-fetched');
ok(SELF_CALLS.indexOf('QQQvanna')>=0, '4d missing QQQ vanna IS self-fetched');
// guard: nothing happens when hidden or before any real feed URL is captured
SELF_CALLS=[]; global.document.visibilityState='hidden'; ensureFeeds();
ok(SELF_CALLS.length===0, '4e hidden tab -> no self-fetch');
SELF_CALLS=[]; global.document.visibilityState='visible'; global.LASTFEEDURL=null; ensureFeeds();
ok(SELF_CALLS.length===0, '4f no captured feed URL -> no self-fetch');

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail===0?0:1);
