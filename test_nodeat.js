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
  const i=src.indexOf("h+=row('hd','',['1ST'");
  if(i<0){ ok(false,'n27 the HL blocks are findable'); return; }
  // ⚠ STRIP COMMENTS FIRST. This counter reported block 2 as 11 cells when it has 10 — it was
  // counting commas inside a `//` comment sitting between two cells ("...= W.END, and TOOK + BOP
  // = WICK to the minute"). Ninth comment-blind assertion this session, and the first one inside a
  // test HELPER rather than an assertion. A helper that reads source must strip comments too.
  const blk=src.slice(i,i+13000).replace(/\/\/[^\n]*/g,'').replace(/\/\*[\s\S]*?\*\//g,'');
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
  const re=/row\('(hd|a|e)',\s*'[^']*',\s*\[/g; let m, counts=[];
  while((m=re.exec(blk))) counts.push([m.group||m[1], cells(blk.slice(m.index+m[0].length-1)), m.index]);
  // ⚠ split on the DIV that opens block 2, not on the string "2ND" — block 2's header row starts
  // BEFORE the '2ND' literal it contains, so a string split puts it in block 1 and the test fails
  // on itself. (It did, first run.)
  const bound=blk.indexOf('g3dayg6');
  const b1=counts.filter(c=>c[2]<bound).map(c=>c[1]);
  const b2=counts.filter(c=>c[2]>bound).map(c=>c[1]);
  ok(b1.length>=3 && b1.every(x=>x===b1[0]), 'n27 block 1 rows all have the same cell count', b1);
  ok(b2.length>=3 && b2.every(x=>x===b2[0]), 'n28 block 2 rows all have the same cell count', b2);
  // ⚠⚠ THE ALIGNMENT CONTRACT (v14.94). The blocks must be the SAME WIDTH or his column pairing
  // silently breaks — PTWick% under WICK%, PTMUD under MUD, from his very first sketch. This is
  // exactly what shipped broken in v14.94: HL GAP / HL RNG / LC·RNG were still living in block 2
  // and pushed the whole PT family three columns left. A per-block width check would NOT have
  // caught it; only comparing the two does.
  ok(b1[0]===b2[0], 'n29 both blocks are the SAME width — the alignment contract', [b1[0],b2[0]]);
  ok(b2.every(x=>x===b2[0]), 'n29b row 3 joins the same contract', b2);
  ok(b1[0]===10, 'n30 the agreed scheme is 10 columns', b1[0]);
})();

// ---- the headers he named --------------------------------------------------------------------
ok(/'HodN':'LodN'/.test(src), 'n31 the first-extreme header switches HodN/LodN with the extreme');
ok(/'PTN'/.test(src), 'n32 PTN is a column on the second row');

// ---- (v14.94) THE AGREED REMOVALS. Each was recorded as done and was still rendering. ---------
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
// (v14.94) the strip is superseded by ROW 3 — he asked for "a thrid row for the HL fields". Both
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
ok(/g3daycdl[\s\S]{0,200}g3daytbl/.test(live), 'n44 ...on the LEFT of the stats, per his instruction');

console.log('test_nodeat: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
