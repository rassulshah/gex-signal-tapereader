// (v10.54, audit 3) THE FRAME OUTCOME — the panel finally measures what it SHOWED.
//
// Every in-play card renders a tgt and an inval. Nothing scored them: the ONLY outcome
// recorded was "did price travel DIR_PTS in the stated direction within FEAT_FWD bars",
// which is not the claim the card makes. `frame` asks the card's own question — did price
// reach the RECORDED target before the RECORDED invalidation, inside the same window —
// using candle HIGH/LOW, because a target is touched intrabar, not at the close.
//
// This file pins: the scoring itself, the both-in-one-bar tie-break, the R:R that rides
// with it, the fields on the resolved record, and the fact that `drift` (the old 0.5-pt
// hit) is KEPT beside it rather than replaced.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m+(g!==undefined?' -> '+g:''));} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+g:''));} };

global.FRAME_HALF=0.25; global.FRAME_FALLBACK=0.5;
global.RR_FLOOR=3; global.RR_MIN=2;
eval(['frameOutcome','frameRR','rrText'].map(ex).join('\n'));

// a bar helper: {h,l}
function B(h,l){ return { h:h, l:l, c:(h+l)/2 }; }

// ================= 1. TARGET BEFORE INVALIDATION =============================
// frame: entry 773, tgt 776 (up), inval 772.
var up={ k:773, tgt:776, inval:772 };
var hitTgt=[ B(773.4,772.9), B(774.8,773.2), B(776.1,775.0), B(776.5,775.4) ];
var f=frameOutcome(up, hitTgt, 0, hitTgt.length);
ok(f.first==='tgt', '1a price printed the target first -> first="tgt"', f.first);
ok(f.tgtHit===true && f.invalHit===false, '1b tgtHit true, invalHit false', f.tgtHit+'/'+f.invalHit);
ok(f.rr===3, '1c the R:R the trader was offered rides with it: |776-773|/|773-772| = 3', f.rr);

// ================= 2. INVALIDATION FIRST =====================================
var hitInval=[ B(773.4,772.9), B(773.2,771.8), B(776.2,775.0) ];
var f2=frameOutcome(up, hitInval, 0, hitInval.length);
ok(f2.first==='inval', '2a the invalidation printed first, even though the target printed later', f2.first);
ok(f2.invalHit===true && f2.tgtHit===true, '2b BOTH are recorded — the card was wrong AND the level was later reached', f2.tgtHit+'/'+f2.invalHit);

// ================= 3. NEITHER, INSIDE THE WINDOW =============================
var neither=[ B(773.4,772.9), B(774.0,773.1), B(774.4,773.6) ];
var f3=frameOutcome(up, neither, 0, neither.length);
ok(f3.first===null, '3a neither level printed -> first=null, NOT a miss and NOT a hit', f3.first);
ok(f3.tgtHit===false && f3.invalHit===false, '3b ...and both flags are honestly false');

// ================= 4. THE SAME-BAR TIE IS SCORED PESSIMISTICALLY =============
// One bar trades through BOTH. The tick order inside a 3-minute bar is unknown, so the
// only honest reading is the one that does not flatter the tool.
var both=[ B(776.5,771.5) ];
var f4=frameOutcome(up, both, 0, 1);
ok(f4.tgtHit===true && f4.invalHit===true, '4a a bar through both records both');
ok(f4.first==='inval', '4b ...and first is the INVALIDATION: the pessimistic reading, because tick order is unknown', f4.first);

// ================= 5. A DOWNWARD FRAME IS THE MIRROR IMAGE ===================
var dn={ k:776, tgt:772, inval:777 };
var dnTgt=[ B(776.2,775.4), B(775.6,773.0), B(773.4,771.9) ];
var f5=frameOutcome(dn, dnTgt, 0, dnTgt.length);
ok(f5.first==='tgt', '5a a DOWN frame reaches its target by trading LOW enough', f5.first);
ok(f5.rr===4, '5b ...and its R:R is |772-776|/|776-777| = 4', f5.rr);
var dnInval=[ B(777.4,776.2), B(773.0,771.5) ];
ok(frameOutcome(dn, dnInval, 0, 2).first==='inval', '5c ...and it invalidates by trading HIGH enough');

// ================= 6. CANDLE HIGH/LOW, NOT THE CLOSE ========================
// The target is touched intrabar and given back. Close-only scoring would call this a
// miss, which is not what the card claimed.
var wick=[ B(776.4, 773.0) ];   // close is (776.4+773)/2 = 774.7, well short of 776
ok(frameOutcome(up, wick, 0, 1).first==='tgt', '6a a wick THROUGH the target counts — the level was reached');
ok(/b\.h>=tgt/.test(ex('frameOutcome')) && /b\.l<=tgt/.test(ex('frameOutcome')),
   '6b the source reads candle high/low, never the close');

// ================= 7. IT DEGRADES HONESTLY ==================================
ok(frameOutcome({k:773, tgt:null, inval:772}, hitTgt, 0, 4)===null, '7a no target recorded -> no frame outcome (null), never a guess');
ok(frameOutcome({k:773, tgt:776, inval:null}, hitTgt, 0, 4)===null, '7b no invalidation -> null');
ok(frameOutcome(null, hitTgt, 0, 4)===null, '7c no record -> null');
ok(frameOutcome({k:773, tgt:776, inval:776}, hitTgt, 0, 4)===null, '7d tgt === inval is not a frame -> null');
var noHL=[ {c:776.5}, {c:777} ];
ok(frameOutcome(up, noHL, 0, 2).first===null, '7e bars with no h/l are SKIPPED, not treated as touches');

// ================= 8. R:R IS PURE GEOMETRY ==================================
ok(frameRR({k:773, tgt:776, inval:772})===3,   '8a rr = |tgt-k| / |k-inval|', frameRR({k:773,tgt:776,inval:772}));
ok(frameRR({k:773, tgt:773.7, inval:772})===0.7, '8b a thin frame reports 0.7, it is not rounded up', frameRR({k:773,tgt:773.7,inval:772}));
ok(frameRR({k:773, tgt:776, inval:773})===null, '8c zero risk = no R:R, never Infinity');
ok(rrText(2.4)==='R:R 2.4:1', '8d rendered as "R:R 2.4:1"', rrText(2.4));

// ================= 9. WIRED INTO THE RESOLVER ================================
var RES=ex('resolveFeatureOutcomes');
ok(/frame:frameOutcome\(r\.rec, cs, startIdx, endIdx\)/.test(RES), '9a the resolver computes the frame over the SAME window as the drift hit');
ok(/r\.drift=r\.hit;/.test(RES), '9b ...and KEEPS the 0.5-pt drift hit beside it, rather than replacing it');
ok(/r\.frame=fwd\.frame;/.test(RES), '9c ...storing it on the record');
ok(/fwd=\{[\s\S]*frame:frameOutcome/.test(RES), '9d ...and passing it to f.outcome, so a feature can score ITSELF on the frame');
// the records that draw a frame must RECORD it, or none of this is scoreable
var FR=ex('_frameRecOf');
ok(/tradeFrame\(sym, L, dirNum\|\|nodeHoldDir\(L, px\)\)/.test(FR), '9e the recorded frame is the SAME tradeFrame the card renders');
ok(/out\.rr=frameRR\(fr\)/.test(FR), '9f ...with the R:R the trader was actually offered');
var FEAT=ex('registerCoreFeatures');
['dir','node','decision','act'].forEach(function(k){
  var i=FEAT.indexOf("key:'"+k+"'");
  ok(i>=0 && /tgt:|_frameRecOf/.test(FEAT.slice(i, i+1400)), '9·'+k+' records the frame it drew');
});
// the decision cell scores itself on the frame for TAKE cells
ok(/fwd\.frame\.first==='tgt'/.test(FEAT), '9g a take-labelled decision cell is scored on tgt-before-inval');
ok(/fwd\.mfe<DIR_PTS && fwd\.mae>-DIR_PTS/.test(FEAT), '9h ...while a skip cell is scored on "no move >= DIR_PTS EITHER WAY"');
// and the Analysis tab surfaces it
ok(/frame '\+Math\.round\(100\*fb\.tgt\/fb\.n\)/.test(ex('featureScorecardsHtml')), '9i the scorecard shows the frame rate beside the drift rate');
ok(/r\.frame && \(r\.frame\.first==='tgt' \|\| r\.frame\.first==='inval'\)/.test(ex('featStats')), '9j featStats tallies it per feature key');

// ================= 10. LATE-SESSION RECORDS ARE NOT SILENTLY DROPPED =========
// (audit 13) Everything recorded in the last 30 minutes never got a full forward window,
// stayed pending forever and was discarded with the day — which is exactly why the
// power-hour bucket could never accumulate a single observation, on any feature, ever.
// After the close a short window is scored on the bars that DID exist and marked
// partial:true with the window actually used, so it is aggregable and can never be
// mistaken for a full-window result.
var RES2=ex('resolveFeatureOutcomes');
ok(/var avail=n-startIdx;/.test(RES2), '10a the resolver measures how many bars are actually available');
ok(/late=\(ctNowSecOfDay\(\)>=15\*3600\)/.test(RES2), '10b ...and only relaxes the window AFTER the close');
ok(/if\(!\(late && avail>=3\)\) return;/.test(RES2), '10c ...never below 3 bars: a 1-bar "outcome" is noise, not a short window');
ok(/partial=true;/.test(RES2), '10d ...marking the record partial');
ok(/r\.partial=partial;/.test(RES2) && /r\.fwdUsed=endIdx-startIdx;/.test(RES2),
   '10e ...with the window ACTUALLY used stored beside it, so it is never read as a full one');
ok(/partial:partial/.test(RES2), '10f ...and passed to f.outcome, so a feature can refuse a partial if it wants to');
// mid-session it still refuses: a window that has not closed has not closed
ok(RES2.indexOf('// window not closed yet')>=0, '10g mid-session the record simply stays pending');
// the analysis layer counts them and says so rather than hiding them
ok(/if\(r\.partial\)/.test(ex('featStats')), '10h featStats tallies partial records separately');
ok(/b\.partial\+\+; out\.partial\+\+;/.test(ex('featStats')), '10i ...per feature and in total');
ok(/late-session records are partial/.test(ex('featureScorecardsHtml')),
   '10j ...and the scorecard hover says how many, so a thin late-day sample is visible');

console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
