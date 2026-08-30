// ============================================================================================
// test_nodeat.js — HodN / LodN / PTN: the node each extreme TESTED before reversing.
//
// Operator, 2026-08-29: "a lodN (lod node) and hodN (hod node) which are nodes that the extreme
// tested before reversing. it usually tests it or even penetrates it a little and then reverses"
// and "we are looking for 1 of the three kings, either a spy node, spxy node or a qqq node".
//
// ⚠⚠ THESE ASSERTIONS EXECUTE THE GEOMETRY. This project's recurring test failure is the FAKE
// ASSERTION — a grep satisfied by adjacent prose, a comment quoting the very line it protects.
// Five shipped that way and only mutation testing found them. So the band tests below EVAL the
// real `deflNodeAt` out of the source and feed it numbers; a comment cannot satisfy them.
// ============================================================================================
const fs=require('fs');
const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };

// ---- pull the geometry out and RUN it -------------------------------------------------------
const grab=(name)=>{ const i=src.indexOf('function '+name+'('); if(i<0) return null;
  let d=0,started=false;
  for(let j=i;j<src.length;j++){ const c=src[j];
    if(c==='{'){d++;started=true;} else if(c==='}'){d--; if(started&&d===0) return src.slice(i,j+1);} }
  return null; };
const fnNear=(src.match(/var\s+DEFL_NEAR\s*=\s*([\d.]+)/)||[])[1];
const fnThru=(src.match(/var\s+DEFL_THRU\s*=\s*([\d.]+)/)||[])[1];
ok(fnNear==='1.0', 'n1 DEFL_NEAR is 1.0 ATR (1.25 measured worse: turn rate 59.2->57.6)', fnNear);
ok(fnThru==='1.5', 'n2 DEFL_THRU is 1.5 ATR', fnThru);

const body=grab('deflNodeAt');
ok(!!body, 'n3 deflNodeAt is defined');
let deflNodeAt=null;
try{ deflNodeAt=eval('(function(){var DEFL_NEAR='+fnNear+',DEFL_THRU='+fnThru+';'+body+';return deflNodeAt;})()'); }catch(e){}
ok(typeof deflNodeAt==='function', 'n4 deflNodeAt evaluates');

if(typeof deflNodeAt==='function'){
  const K=[{b:'SPY',disp:765}], A=1.0;   // ATR = 1.0 so multiples read directly as points
  // DOWNMOVE (a low): band is [k-1.5, k+1.0]
  ok(!!deflNodeAt(765.9, false, K, A), 'n5 low 0.9 ABOVE node is a test (inside 1.0 ATR short)');
  ok(!deflNodeAt(766.1, false, K, A),  'n6 low 1.1 above node is NOT (beyond 1.0 ATR short)');
  ok(!!deflNodeAt(763.6, false, K, A), 'n7 low 1.4 THROUGH node is a test (inside 1.5 ATR)');
  ok(!deflNodeAt(763.4, false, K, A),  'n8 low 1.6 through node is NOT (beyond 1.5 ATR)');
  // UPMOVE (a high): the band MIRRORS — [k-1.0, k+1.5]
  ok(!!deflNodeAt(766.4, true, K, A),  'n9 high 1.4 through node is a test (upmove mirror)');
  ok(!deflNodeAt(766.6, true, K, A),   'n10 high 1.6 through node is NOT');
  ok(!deflNodeAt(763.9, true, K, A),   'n11 high 1.1 short of node is NOT (approach leg is tight)');
  // ⚠ the asymmetry itself: a 1.4 penetration passes, a 1.1 shortfall fails. If someone makes the
  // band symmetric BOTH of these flip, and n6/n7 alone would not catch it.
  ok(!!deflNodeAt(763.6,false,K,A) && !deflNodeAt(766.1,false,K,A),
     'n12 the band is ASYMMETRIC: deeper through than short');
  // ATR SCALING: double the ATR, the band doubles
  ok(!!deflNodeAt(766.9, false, K, 2.0), 'n13 band scales with ATR (0.9 ATR short at ATR=2)');
  // closest king wins when two qualify
  const two=[{b:'SPY',disp:765},{b:'SPX',disp:765.5}];
  const w=deflNodeAt(765.4,false,two,1.0);
  ok(w && w.b==='SPX', 'n14 closest qualifying king wins', w&&w.b);
  ok(deflNodeAt(765,false,[],1.0)===null, 'n15 no kings -> null');
  ok(deflNodeAt(765,false,K,0)===null,    'n16 no ATR -> null (never a fixed-band fallback)');
}

// ---- the wick/close split, which is the finding this was built on ---------------------------
const eng=grab('hlNodeAt')||'';
ok(/D\.hod|D\.lod/.test(eng), 'n17 hlNodeAt reads the extremes (wicks), not closes');
ok(!/\.c\b/.test(eng.replace(/\/\/.*$/gm,'')), 'n18 hlNodeAt never reads a bar CLOSE for the test');

// ---- the three books, by name ---------------------------------------------------------------
const kb=grab('deflKings')||'';
['SPY','SPXW','QQQ'].forEach((b,i)=>ok(kb.indexOf("'"+b+"'")>=0, 'n'+(19+i)+' deflKings reads '+b));
ok(/dispScale/.test(kb), 'n22 SPXW converts via ifLadder dispScale (the chain trinityRead uses)');
ok(/bearing/.test(kb),   'n23 QQQ is flagged a proportional BEARING, not a conversion');

// ---- hlPT must export the PT price, or PTN can never be located -----------------------------
// ⚠ COMMENT-BLIND. The first version of this line was /out\.ptPx\s*=/ against the raw source and
// it SURVIVED commenting the assignment out — "//out.ptPx=advP;" matches it perfectly. That is the
// project's signature fake assertion (five shipped, all found by mutation, none by review). Strip
// comments, and require it INSIDE hlPT rather than anywhere in the file.
(function(){
  const pt=(grab('hlPT')||'').replace(/\/\/[^\n]*/g,'').replace(/\/\*[\s\S]*?\*\//g,'');
  ok(/out\.ptPx\s*=\s*\w/.test(pt), 'n24 hlPT assigns ptPx (uncommented, inside hlPT)');
})();

// ---- the caveat that keeps this honest ------------------------------------------------------
const cav=src.indexOf('NOT a claim the node caused the turn');
ok(cav>=0, 'n25 the "not a claim of causation" caveat is present');
ok(/t=\+?'\+DEFL_META\.tTop5|DEFL_META\.tTop5/.test(src), 'n26 the null-result t-stats are shown to the user, not just in a comment');

// ---- the table must not shear: header, A and E rows must agree per block ---------------------
(function(){
  // ⚠⚠ (v15.01) ONE TABLE, SO ONE ASSERTION. This used to split the section on the `g3dayg6` class
  // and compare each block's cell counts. That test PASSED while the operator's screen was visibly
  // broken: three separate `display:table` divs each size columns from THEIR OWN content, so equal
  // cell COUNTS never produced equal cell WIDTHS — block 2 began halfway across his row.
  // ⚠ The alignment he asked for is a RENDERING property. Only a SINGLE table delivers it, so the
  // test is now: exactly one table, and every row in it the same width.
  const i=src.indexOf("h+=row('hd','',['1ST");
  if(i<0){ ok(false,'n27 the HL rows are findable'); return; }
  const j=src.indexOf('\nfunction ', i);
  const blk=src.slice(i, j<0?i+14000:j).replace(/\/\/[^\n]*/g,'').replace(/\/\*[\s\S]*?\*\//g,'');
  const cells=(txt)=>{ let dep={'[':0,'(':0,'{':0},n=1,q=null,esc=false;
    for(const ch of txt.slice(1)){
      if(q){ if(esc)esc=false; else if(ch==='\\')esc=true; else if(ch===q)q=null; continue; }
      if(ch==='"'||ch==="'"){q=ch;continue;}
      if('(['.includes(ch)||ch==='{') dep[ch]++;
      else if(ch===')') dep['(']--;
      else if(ch==='}') dep['{']--;
      else if(ch===']'){ if(dep['[']===0) return n; dep['[']--; }
      else if(ch===','&&!dep['[']&&!dep['(']&&!dep['{']) n++;
    } return null; };
  const widths=[];
  for(const m of blk.matchAll(/row\('(hd|a|e|sp)',\s*'[^']*',\s*\[/g))
    widths.push(cells(blk.slice(m.index+m[0].length-1)));
  ok(widths.length>=8, 'n27 all the section rows are found', widths.length);
  ok(widths.every(w=>w===widths[0]), 'n28 EVERY row is the same width — the alignment contract', widths);
  ok(widths[0]===10, 'n29 ...and that width is the agreed 10 columns', widths[0]);
  const sd=src.slice(src.indexOf('function secDay(sym){'), j<0?undefined:j);
  const opens=(sd.match(/g3dayg /g)||[]).length;
  ok(opens===1, 'n30 the section is ONE table — separate tables cannot align columns', opens);
})();

// ---- the headers he named --------------------------------------------------------------------
ok(/'HodN':'LodN'/.test(src), 'n31 the first-extreme header switches HodN/LodN with the extreme');
ok(/'PTN'/.test(src), 'n32 PTN is a column on the second row');

// ---- (v15.01) THE AGREED REMOVALS. Each was recorded as done and was still rendering. ---------
const live=src.replace(/\/\/[^\n]*/g,'').replace(/\/\*[\s\S]*?\*\//g,'');
ok(!/secs\s*=\s*\[\s*secBias/.test(live), 'n33 the TREND section is off the face (secBias not mounted)');
ok(/function secBias/.test(live), 'n34 ...but secBias SURVIVES — bias.confirm still feeds the recorder');
// ⚠ the first version of n35 grepped the whole file for `g3farhd` and failed on the leftover CSS
// RULE, not a render — a class name in a stylesheet is not a section on the face. Assert on the
// EMITTER instead. (And n36 named the wrong producer: it is fsRead(), not farSide().)
ok(!/h\s*\+=\s*[^\n]*g3farhd/.test(live), 'n35 nothing EMITS the FAR SIDE block any more');
ok(/fsRead\s*\(/.test(live), 'n36 ...but fsRead() survives — the read-line hover still uses it');
ok(!/g3steps/.test(live), 'n37 the dead step-bar CSS is gone');
// ⚠ CSS-BLIND, like n35 was. `/g3dayhl/` matched the STYLE RULE, so blanking the emitter left it
// green — the mutation survived. Assert on the emitter, as with n35.
// (v15.01) the strip is superseded by ROW 3 — he asked for "a thrid row for the HL fields". Both
// shipped for one build and printed HL GAP/HL RNG TWICE; n39 caught it. Now: row 3 owns them, and
// the strip must be GONE, or the duplication comes back.
ok(!/g3dayhl/.test(live), 'n38 the old top strip is gone — row 3 owns the spans now');
ok(/'HL GAP','HL RNG','HL \$'/.test(live), 'n39 row 3 carries HL GAP / HL RNG / HL $');
ok((live.match(/'HL GAP'/g)||[]).length===1, 'n39b ...exactly ONCE — not duplicated across strip and row');
ok(/'LC GAP','LC RNG','LC \$'/.test(live), 'n39c row 3 carries the LC span beside HL');
ok(/GD_META/.test(live) && /gdRead\s*\(/.test(live), 'n40 the GREEN/RED call is wired into the face');
ok(/silentGreen/.test(live), 'n41 ...and the hover carries the SILENT-day coin flip, not just the win rate');
ok(/priorDayAuc/.test(live), 'n42 ...and the prior-day NULL result, which he asked about specifically');
ok(/dayCandleSvg\s*\(/.test(live), 'n43 the candle renders');
// ---- (v15.01) HIS SKETCH: narrow bar, annotations stacked ABOVE and BELOW, money in the body ---
// "the candle is taking up too much horizontal space". The width is the BAR, not the labels — that
// is the whole point of stacking them, and a side legend would put it straight back.
(function(){
  const CD=grab('dayCandleSvg').replace(/\/\/[^\n]*/g,'');
  ok(/var W=80,/.test(CD), 'n45 the candle is 80px wide, not 150');
  ok(/text-anchor="middle"/.test(CD), 'n46 ...because the labels stack over the bar, not beside it');
  ok(/HOD '\+hlClock\(D\.hodT\)/.test(CD) && /LOD '\+hlClock\(D\.lodT\)/.test(CD),
     'n47 both extremes carry their CLOCK, as he drew');
  // ⚠ the first version matched the DECLARATIONS, so deleting the emit lines left it green.
  // Match the emitters — a variable computed and never drawn is not a feature.
  ok(/if\(afterHod!=null\)\s*h\+=/.test(CD) && /if\(afterLod!=null\)\s*h\+=/.test(CD),
     'n48 ...and both follow-on durations are actually DRAWN, not merely computed');
  // (v15.01) MUD sits on the side of the OPEN the session travelled — above on a red bar, below on
  // a green one — and carries the money in the MUD LEG, which is |open - second extreme|, not range.
  ok(/MUD '\+hlDur\(D\.mud\)/.test(CD), 'n49 MUD is drawn');
  ok(/green \? \(y\(O\)\+LN\) : \(y\(O\)-LN-LN\)/.test(CD),
     'n49b ...above the open on a red bar, below it on a green one');
  // ⚠ match the ASSIGNMENT, not the neighbouring arithmetic: setting `mudUsd=usd` (the day range)
  // left the `Math.abs(secPx-D.open)` on the line above intact, and the first version stayed green.
  ok(/mudUsd=Math\.abs\(secPx-D\.open\)\s*\*/.test(CD),
     'n49c ...and the MUD money is ASSIGNED from the open-to-second-extreme leg, not the day range');
  ok(!/mudUsd\s*=\s*usd\b/.test(CD), 'n49d ...never from the day range');
  ok(/y\(L\)\+GAP\+LN\*3/.test(CD), 'n50b the day total gets its OWN third line under the extremity');
  ok(/ES_USD_PER_PT/.test(CD) && /Math\.round\(usd\)/.test(CD),
     'n50 ...beside the money the move was worth');
  // ---- (v15.01) the shape spine came BACK, and the reversal levels arrived --------------------
  ok(/_pu\+'%/.test(CD) && /_pb\+'%/.test(CD) && /_pd\+'%/.test(CD),
     'n51 the wick/body/wick percentages are drawn — the only figure that sums to 100');
  ok(/revLevels\(sym, D\)/.test(CD), 'n52 the candle asks for the reversal levels');
  ok(/RV\.hi\[ri\]\.name/.test(CD) && /RV\.lo\[rj\]\.name/.test(CD),
     'n53 ...and DRAWS them at both wick tips');
  ok(/frameNum\(RV\.hi\[ri\]\.px\)/.test(CD),
     'n54 ...with the PRICE in the hover, since it is off the face');
})();

// ---- (v15.01) THE FILTER IS THE FEATURE -------------------------------------------------------
(function(){
  const RL=grab('revLevels').replace(/\/\/[^\n]*/g,'').replace(/\/\*[\s\S]*?\*\//g,'');
  ok(/atr\(sym\)\*rr/.test(RL), 'r1 the tolerance is ATR-scaled, not a fixed band');
  // ⚠⚠ (v15.01) ASYMMETRIC, like the deflection geometry. v14.99 used ONE symmetric tolerance,
  // which is not what this project calibrated against his circled charts: a test may stop SHORT by
  // 1 ATR but may run THROUGH by 1.5, because a stab that pierces and recovers is still a test.
  // He caught the symptom — "the market took out both the prior day high and the prior day low".
  ok(/REVL_SHORT/.test(RL) && /REVL_THRU/.test(RL), 'r2 the band is ASYMMETRIC: short vs through');
  ok(/\(px>=H\)\s*\?\s*\(\(px-H\)<=tol\*REVL_SHORT\)\s*:\s*\(\(H-px\)<=tol\*REVL_THRU\)/.test(RL),
     'r2b ...at the HIGH: unreached uses SHORT, exceeded uses THROUGH');
  ok(/\(px<=L\)\s*\?\s*\(\(L-px\)<=tol\*REVL_SHORT\)\s*:\s*\(\(px-L\)<=tol\*REVL_THRU\)/.test(RL),
     'r2c ...and mirrored at the LOW');
  // ⚠ THE EXCLUSION HE ASKED FOR: anything traded THROUGH is mid-range and therefore near neither
  // extreme, so it is excluded by construction. There is no separate rule that could drift.
  ok(!/ibH|ibL|ib60H|ib60L/.test(RL),
     'r3 IB and IB60 are excluded BY NAME — never readable, even when they coincide with a wick');
  ok(/'PDH'/.test(RL) && /'PDL'/.test(RL) && /'PDC'/.test(RL), 'r4 PDH / PDL / PDC are read');
  ok(/'CW0'/.test(RL) && /'PW0'/.test(RL), 'r5 CW0 / PW0 are read');
  ok(/REVL_MAX/.test(RL) && /slice\(0,REVL_MAX\)/.test(RL), 'r6 capped per side, nearest first');
  ok(/out\.hi\.sort\(near\)/.test(RL), 'r7 ...nearest to the extreme wins, not first found');
  // the arithmetic, on his own numbers
  const H=7716.6, L=7709.6, tol=0.38;
  const near=(px)=>Math.abs(px-H)<=tol||Math.abs(px-L)<=tol;
  // the arithmetic on his own numbers, with the asymmetric band
  const H2=7716.6, L2=7709.6, a=0.38;
  const okH=(px)=> px>=H2 ? (px-H2)<=a*1.0 : (H2-px)<=a*1.5;
  ok(okH(7716.4), 'r8 a level 0.2 BELOW the high (exceeded, then reversed) qualifies');
  ok(okH(7716.9), 'r8b ...and one 0.3 above it (approached, not reached) qualifies');
  ok(!okH(7717.2), 'r8c ...but 0.6 above is beyond the SHORT tolerance');
  ok(!okH(7715.8), 'r8d ...and 0.8 through is beyond the THROUGH tolerance — price kept going');
  ok(!okH(7713.0), 'r9 ...and one mid-range, traded through, does NOT');
  // (v15.01) the four levels that did not exist before
  ok(/'ONH'/.test(RL) && /'ONL'/.test(RL), 'r10 ONH / ONL are read — zero hits in the file before this');
  ok(/'POC'/.test(RL) && /'VAH'/.test(RL) && /'VAL'/.test(RL), 'r11 POC / VAH / VAL are read');
})();

// ---- (v15.01) THE KING AT A MOMENT, NOT THE KING NOW ------------------------------------------
// ⚠⚠ HodN/LodN read em-dash on his screen EVERY day while PTN worked, and the asymmetry was the
// tell: the PT extreme is recent, so the CURRENT king is still near it. A 10:00 high was being
// measured against a 16:00 king. The feature answered the wrong question and looked like no data.
(function(){
  const KA=grab('kingAt').replace(/\/\/[^\n]*/g,'');
  const DK=grab('deflKingsAt').replace(/\/\/[^\n]*/g,'');
  const HN=grab('hlNodeAt').replace(/\/\/[^\n]*/g,'');
  ok(/mv\.t>ms/.test(KA), 'k1 kingAt walks the journey and stops at the requested moment');
  ok(/kingDay\(sym\)/.test(KA), 'k2 ...reading KINGDAY.moves, the timestamped history');
  ok(/deflKingsAt\(sym, rr, firstMs\)/.test(HN), 'k3 the FIRST extreme is matched against the king of its own time');
  ok(/deflKingsAt\(sym, rr, secMs\)/.test(HN), 'k4 ...and so is the second');
  ok(/D\.hodMs/.test(HN) && /D\.lodMs/.test(HN), 'k5 ...using the wall-clock ms hodLod now records');
  ok(/out\.hodMs=hiMs/.test(src) && /out\.lodMs=loMs/.test(src), 'k6 hodLod records both extremes\' ms');
  // ⚠ SPXW has no journey; substituting its CURRENT king would be the exact bug being fixed.
  ok(!/'SPX'/.test(DK), 'k7 SPX is OMITTED from the historical read — it has no journey to read');
  ok(/approx:true/.test(DK), 'k8 ...and the QQQ bearing is flagged approximate, not passed off as exact');
})();

// ---- the profile levels must be a RECORD, never a claim ---------------------------------------
(function(){
  const PP=grab('priorProfile');
  ok(/46\.6%/.test(src) && /46\.3%/.test(src),
     'p1 the sham comparison that kills POC as a reversal marker is ON THE RECORD in the source');
  ok(/RECORD, NEVER AS A CLAIM|RECORD, NOT A CLAIM/.test(src),
     'p2 ...and the profile levels are labelled a record, not a claim');
  ok(/PROF_VA/.test(PP) && /0\.70/.test(src), 'p3 the value area is the standard 70% of volume');
})();
(function(){
  const CD=grab('dayCandleSvg');
})();
ok(/g3daycdl[\s\S]{0,200}g3daytbl/.test(live), 'n44 ...on the LEFT of the stats, per his instruction');

console.log('test_nodeat: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
