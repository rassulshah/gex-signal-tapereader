// (v10.51) DIRECTION HIERARCHY — the model, not the arithmetic.
//
//   1. The SMA-50 five-state IS the direction. Drift NEVER chooses it.
//   2. Drift only CONFIRMS (better grade) or DIVERGES (grade dies at C).
//   3. No confirmed trend => TENTATIVE: drift may lend a provisional lean, capped at C.
//   4. Broken states (up-broken / dn-broken) are NOT a trend: they are tentative, and
//      they vote 0 until dir.trend5 has measured whether they continue or reverse.
//   5. The v10.50 hard caps (mid-range, chop, SIDE≠A, session odds) still override all.
//
// Also pins the READ sentence VARIANT chosen by `relation`, and the recorder entries
// that make the hierarchy falsifiable later.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m+(g!==undefined?' -> '+g:''));} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+g:''));} };

// ---------------- mocks ----------------
// (v10.57) drift is in SHADOW mode live (DRIFT_LIVE=false). These pins exercise the hierarchy AS IT RUNS
// WHEN DRIFT IS PROMOTED, so the flag is forced on here; test_drift_shadow.js pins the live shadow behaviour.
global.DRIFT_LIVE=true;
global.STATE={SPY:{price:772.6, candles:[], lastClosedB:1}, QQQ:{price:null, candles:[]}};
var TV={state:'up', up:16, dn:1, win:20, slope:0.4};
var DRIFT={verdict:'AGREE-UP', dir:1, gvwap:773.9, vvwap:775.0, overlap:true};
var NET={bias:'resistance-heavy', dir:-1, ratio:3.0, decisive:true};   // deliberately AGAINST
var RG={tag:'trend', er:0.6};
var MODEL={ok:true, px:772.6, flr:{k:772}, ceil:{k:778}, levels:[], kingK:775};
var MINS=10*60+30;
global.trendVerdict=function(){ return TV; };
global.driftRead=function(){ return DRIFT; };
global.netPositioning=function(){ return NET; };
global.regimeTag=function(){ return RG; };
global.nodeMapModel=function(){ return MODEL; };
global.closedCandles=function(){ return []; };
global.ctMinutesSinceMidnight=function(){ return MINS; };
global.ctNow=function(){ return new Date(2026,7,17,11,0); };
global.ruleTier=function(){ return '⚖'; };
global.PAL={sub:'#8b98a9'};
global.fmtNum=function(x){ return (Math.round(x*100)/100).toString(); };

eval(['rangePosOf','gradeOfScore','gradeDisp','isOpexDay','sessionBucket','directionGrade',
      '_nmIsAcc','_nmIsDec','zoneRole','read3Beat','_trend5Vote'].map(ex).join('\n'));

function G(){ return directionGrade('SPY'); }
function setTrend(s,up,dn){ TV={state:s, up:(up==null?16:up), dn:(dn==null?1:dn), win:20, slope:0.4}; }
function setDrift(v,dir,ov){ DRIFT={verdict:v, dir:dir, gvwap:773.9, vvwap:775.0, overlap:!!ov}; }

// ================= 1. CONFIRMED TREND + AGREEING DRIFT => HIGHER GRADE ==============
setTrend('up'); setDrift('SPLIT',0,false);
var trendOnly=G();
setDrift('LEAN-UP',1,false);
var leanUp=G();
setDrift('AGREE-UP',1,true);
var agreeUp=G();
ok(trendOnly.grade==='B' && trendOnly.score===3, '1a confirmed trend alone = B (score 3)', trendOnly.score+'/'+trendOnly.grade);
ok(leanUp.score===trendOnly.score+1, '1b an agreeing LEAN-* adds exactly +1', leanUp.score);
ok(agreeUp.score===trendOnly.score+2, '1c an agreeing AGREE-* (bands overlap) adds +2', agreeUp.score);
ok(agreeUp.grade==='A' && leanUp.grade==='B', '1d agreement with overlap is what buys the A', leanUp.grade+' -> '+agreeUp.grade);
ok(agreeUp.score>leanUp.score && leanUp.score>trendOnly.score, '1e strictly monotone: AGREE > LEAN > nothing');
ok(agreeUp.relation==='confirmed' && leanUp.relation==='confirmed' && trendOnly.relation==='trend-only',
   '1f relations are named confirmed / confirmed / trend-only');

// down side is the mirror image
setTrend('dn',1,16); setDrift('AGREE-DN',-1,true);
var agreeDn=G();
ok(agreeDn.dir==='DN' && agreeDn.grade==='A' && agreeDn.relation==='confirmed',
   '1g the downtrend mirror behaves identically', agreeDn.dir+'/'+agreeDn.grade);

// ================= 2. CONFIRMED TREND + OPPOSING DRIFT => GRADE C ===================
setTrend('up'); setDrift('AGREE-DN',-1,true);
var divA=G();
setDrift('LEAN-DN',-1,false);
var divL=G();
ok(divA.relation==='divergence' && divL.relation==='divergence', '2a opposing drift = divergence', divA.relation);
ok(divA.grade==='C' && divL.grade==='C', '2b a diverging trend can NEVER grade above C', divA.grade+'/'+divL.grade);
ok(divA.score===1, '2c score = base 3 − 2 for the divergence', divA.score);
setTrend('dn',1,16); setDrift('AGREE-UP',1,true);
var divDn=G();
ok(divDn.grade==='C' && divDn.relation==='divergence', '2d mirrored on a downtrend', divDn.grade);

// ================= 3. DRIFT NEVER FLIPS A CONFIRMED TREND'S DIRECTION ===============
// This is the whole point of the hierarchy: under v10.50's weighted lean, a drift of
// -1 (weight 2) beat everything and OWNED the side. It may no longer.
setTrend('up');
['AGREE-DN','LEAN-DN'].forEach(function(v){
  setDrift(v,-1,v.indexOf('AGREE')===0);
  ok(G().dir==='UP', '3·'+v+' cannot flip a confirmed UPTREND to DN', G().dir);
});
setTrend('dn',1,16);
['AGREE-UP','LEAN-UP'].forEach(function(v){
  setDrift(v,1,v.indexOf('AGREE')===0);
  ok(G().dir==='DN', '3·'+v+' cannot flip a confirmed DOWNTREND to UP', G().dir);
});
// nor can structure, which no longer votes at all
setTrend('up'); setDrift('SPLIT',0,false);
NET={bias:'resistance-heavy', dir:-1, ratio:9, decisive:true};
ok(G().dir==='UP', '3z structure pointing the other way does not move the direction either', G().dir);
NET={bias:'support-heavy', dir:1, ratio:2, decisive:true};
var withStruct=G();
NET={bias:'resistance-heavy', dir:-1, ratio:2, decisive:true};
var vsStruct=G();
ok(withStruct.score===vsStruct.score, '3y ...and it does not move the SCORE (recorded, not voted)', withStruct.score+'=='+vsStruct.score);

// ================= 4. NO CONFIRMED TREND => TENTATIVE, dir FROM DRIFT, CAP C ========
['flat','up-broken','dn-broken','na'].forEach(function(st){
  setTrend(st,4,4); setDrift('AGREE-UP',1,true);
  var t=G();
  ok(t.relation==='tentative' && t.tentative===true, '4·'+st+' -> tentative', t.relation);
  ok(t.dir==='UP', '4·'+st+' -> direction comes from DRIFT', t.dir);
  ok(t.grade==='C', '4·'+st+' -> grade hard-capped at C even on an AGREE drift', t.grade);
  ok(t.score===1, '4·'+st+' -> score 1', t.score);
  setDrift('AGREE-DN',-1,true);
  ok(G().dir==='DN', '4·'+st+' -> a DOWN drift gives a DOWN provisional lean', G().dir);
  setDrift('SPLIT',0,false);
  ok(G().dir==='SIDE', '4·'+st+' -> no drift lean means SIDE, not a guess', G().dir);
  ok(G().trendState===st, '4·'+st+' -> the five-state is reported verbatim, never collapsed', G().trendState);
});
// broken states are DISTINCT from flat in the record even though both vote 0 here
ok(_trend5Vote('up-broken')===1 && _trend5Vote('dn-broken')===-1,
   '4y the RECORDER still assigns broken states an implied direction (so they can be measured)');
ok(_trend5Vote('flat')===0 && _trend5Vote('na')===0,
   '4z flat / na imply nothing and are skipped, never counted as misses');

// ================= 5. THE v10.50 HARD CAPS STILL APPLY ==============================
// 5.1 mid-range beats even a perfect confirmed trend
setTrend('up'); setDrift('AGREE-UP',1,true);
global.STATE.SPY.price=775.0; MODEL.px=775.0;            // dead centre of 772..778
var mid=G();
ok(mid.score===5 && mid.grade==='C', '5a mid-range hard-caps a score-5 A down to C', mid.score+'/'+mid.grade);
ok(mid.capped==='mid-range', '5b the cap is named', mid.capped);
ok(mid.noOdds===true, '5c ...and no odds may be quoted');
ok(mid.dir==='UP', '5d the DIRECTION is untouched by the grade cap', mid.dir);
// 5.2 chop caps the grade AND forces SIDE
global.STATE.SPY.price=772.6; MODEL.px=772.6;
RG={tag:'chop', er:0.18};
var ch=G();
ok(ch.grade==='C' && ch.dir==='SIDE' && ch.noOdds===true, '5e chop -> C + SIDE + no odds', ch.grade+'/'+ch.dir);
ok(/chop/.test(ch.capped||''), '5f the chop cap is named', ch.capped);
RG={tag:'trend', er:0.6};
// 5.3 SIDE can never be A
setTrend('flat',3,3); setDrift('NONE',0,false);
var side=G();
ok(side.dir==='SIDE' && side.grade!=='A', '5g a SIDE read is never an A', side.dir+'/'+side.grade);
// 5.4 session cap
setTrend('up'); setDrift('AGREE-UP',1,true);
MINS=14*60+45;
ok(G().noOdds===true, '5h power hour still suppresses the odds claim');
MINS=9*60+15;
ok(G().noOdds===true, '5i open drive too');
MINS=10*60+30;
ok(G().noOdds===false, '5j ...and the morning does not');

// ================= 6. THE READ SENTENCE VARIANT FOLLOWS `relation` =================
function L(o){ return Object.assign({state:'Steady',chg:0,taps:0,pos:true},o); }
var flr=L({k:772,isFlr:true,state:'Building'}), ceil=L({k:776,isCeil:true});
var king=L({k:773,isKing:true});
var vConf=read3Beat('UP', king, 773.4, flr, ceil, {dir:1,verdict:'AGREE-UP'}, 776, 'Ceil', 'confirmed');
ok(vConf.sentence==='At King 773. Support building, uptrend confirmed by GEX and VEX leaning up. Potential bounce to 776.',
   '6a confirmed variant', vConf.sentence);
var vDiv=read3Beat('UP', king, 773.4, flr, ceil, {dir:-1,verdict:'AGREE-DN'}, 772, '', 'divergence');
ok(vDiv.sentence==='At King 773. Uptrend, but GEX and VEX lean down — divergence, lower confidence. Watch 772.',
   '6b divergence variant', vDiv.sentence);
var vTent=read3Beat('UP', king, 773.4, flr, ceil, {dir:1,verdict:'AGREE-UP'}, null, '', 'tentative');
ok(vTent.sentence==='Near Flr 772–776. No trend; GEX and VEX lean up — tentative only.',
   '6c tentative variant (v10.54: the head is the REAL range zone, not a fixed Mid-range)', vTent.sentence);
var vOnly=read3Beat('UP', king, 773.4, flr, ceil, {dir:0,verdict:'SPLIT'}, 776, 'Ceil', 'trend-only');
ok(vOnly.sentence==='At King 773. Support building. Potential bounce to 776.',
   '6d trend-only keeps the 3-beat shape with the drift clause dropped', vOnly.sentence);
var vLegacy=read3Beat('UP', king, 773.4, flr, ceil, {dir:1,verdict:'AGREE-UP'}, 776, 'Ceil');
ok(vLegacy.sentence==='At King 773. Support building with GEX and VEX leaning up. Potential bounce to 776.',
   '6e omitting relation still emits the v10.50 voice (back-compatible)', vLegacy.sentence);
ok(vDiv.dir==='UP' && vDiv.verdict==='BULLISH',
   '6f the divergence sentence still reports the TREND direction, only with lower confidence');
ok(!/\b(buy|sell|long|short|stop|size|entry)\b/i.test(vConf.sentence+vDiv.sentence+vTent.sentence+vOnly.sentence),
   '6g every variant stays descriptive — no order words');

// ================= 7. RECORDING: the hierarchy must be falsifiable ==================
var FSRC=ex('registerCoreFeatures');
['dir.trend5','dir.drift','dir.relation','dir.struct','dir.kingRoll','netGamma','dir.trendFast'].forEach(function(k){
  ok(FSRC.indexOf("key:'"+k+"'")>=0, '7·'+k+' is enrolled as its own feature');
});
ok(/RECORDED, NOT VOTED|RECORDED not voted/.test(FSRC), '7a the non-voting candidates say so in source');
ok(/voting:false/.test(FSRC), '7b ...and carry it on the record itself');
// FCHIST samples but must NOT compute or vote rolling yet
ok(/gpts_flrceilhist_v1/.test(src), '7c FCHIST uses the new key gpts_flrceilhist_v1');
ok(/function fcHistSample/.test(src) && /fcHistSample\('SPY'\)/.test(src), '7d it is sampled in the snapshot cycle');
ok(/DAY-OVER-DAY|day-over-day/.test(src) && /2 consecutive/.test(src),
   '7e the comment states rolling is day-over-day (2 consecutive = signal, 3 = confirmation)');
ok(!/function flrRoll|function ceilRoll/.test(src), '7f no intraday rolling verdict is computed or voted yet');
// the analysis table's anti-artifact columns
var AFN=ex('dirFactorsHtml');
ok(/votes ↑\/↓/.test(AFN), '7g the factors table shows the vote-direction split on every row');
ok(/baseline/i.test(AFN), '7h ...and the period baseline beside it');
ok(/lift/i.test(AFN), '7i ...and the lift over that baseline');

console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
