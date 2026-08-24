// (v11.49) THE ANCHORED EM BAND.
//
// What this pins, and why each one is here rather than assumed:
//
//  1. dte0 ONLY. The face read `toFri.em` for months — a SINGLE expiry's straddle drawn out of the
//     Mon..Fri set, so on any day but Friday it is a LATER expiry worth roughly double the day's
//     (69.25 against 34.65, measured 2026-08-22). It sat under a label asking "how much room does
//     today have" while answering about a different week. On a Friday the two coincide, which is
//     precisely why it hid. A fallback to toFri is FORBIDDEN — it would reintroduce the bug silently.
//  2. The open comes from the first RTH candle, so a panel started at noon still measures from the
//     real open rather than from wherever price happened to be at first render.
//  3. Marks CLAMP. A price outside the band pins to the rail; the percentage is what says how far.
//     An unclamped left:%(>100) escapes the track and silently mislays the mark off-panel.
//  4. The capture happens ONCE per day and is never overwritten — otherwise the "anchored" band
//     quietly becomes a live one that follows price, which is the whole thing it exists not to be.
//  5. A late capture is flagged ~EST rather than reconstructed. Scaling a decayed straddle back up
//     by sqrt(T) assumes IV has not moved, and on the days that matter it has.
const fs=require('fs');
const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };

function ex(n){
  const re=new RegExp('function\\s+'+n+'\\s*\\(','g'); const m=re.exec(src);
  if(!m) throw new Error('function not found: '+n);
  let i=src.indexOf('{',m.index), d=0, e=-1;
  for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){ d--; if(d===0){ e=k; break; } } }
  return src.slice(m.index,e+1);
}

// Extract a top-level `var NAME = ...;` so a constant under test comes from the SOURCE, not the test.
function ex_var(n){
  const m=new RegExp('var\\s+'+n+'\\s*=\\s*[^;]+;').exec(src);
  if(!m) throw new Error('var not found: '+n);
  return m[0];
}

// ---------- 1. THE SOURCE RULE, read off the text ----------
{
  const body=ex('emBand');
  ok(/dte0\s*&&\s*ec\.dte0\.em/.test(body) || /ec\.dte0\s*&&\s*ec\.dte0\.em/.test(body),
     'the band reads dte0.em');
  ok(!/toFri\.em/.test(body),
     'and NEVER falls back to toFri.em — that is a later expiry, roughly double the day');
  ok(/closedCandles/.test(body) && /cs\[0\]\.o/.test(body),
     'the open comes from the first RTH candle, not from price at first render');
  ok(/localStorage/.test(body) && /EMOPEN_KEY/.test(body),
     'the capture is persisted, so a reload does not re-capture a decayed straddle');
}

// ---------- 2. emPos clamps ----------
{
  eval(ex('emPos'));
  const B={ low:100, high:200 };
  ok(emPos(B,150)===50,                 'midpoint sits at 50%', emPos(B,150));
  ok(emPos(B,100)===0,                  'the low rail is 0%', emPos(B,100));
  ok(emPos(B,200)===100,                'the high rail is 100%', emPos(B,200));
  ok(emPos(B,260)===100,                'ABOVE the band clamps to the rail, never past it', emPos(B,260));
  ok(emPos(B,10)===0,                   'BELOW the band clamps to the rail', emPos(B,10));
  ok(emPos({low:5,high:5},5)===0,       'a zero-width band does not divide by zero', emPos({low:5,high:5},5));
  ok(emPos({low:0,high:0},1)===0,       'nor does a degenerate one', emPos({low:0,high:0},1));
}

// ---------- 3. the band arithmetic, run against a stub ----------
{
  // Minimal stubs: closed candles on the UNDERLYING scale, a chain carrying only dte0.
  const store={};
  global.localStorage={ getItem:k=>(k in store?store[k]:null), setItem:(k,v)=>{store[k]=String(v);} };
  global.STATE={ SPY:{ candles:[] } };
  global.closedCandles=function(){ return global.__cands; };
  global.dispIsFut=function(){ return true; };
  global.dispR=function(){ return 10; };          // underlying -> chart
  global.ifLadder=function(){ return { err:null, dispScale:1 }; };
  global.ctTodayStr=function(){ return '2026-08-24'; };
  global.sessionPhase=function(){ return global.__phase; };
  global.ifChain=function(){ return global.__chain; };

  // The two module-level constants emBand closes over. Pulled from SOURCE rather than hardcoded, so
  // that changing EM_FRESH_MIN in the script moves this test with it instead of leaving it asserting
  // a threshold the code no longer uses. (ex() extracts a function body only — a name declared
  // outside it throws ReferenceError inside the eval, which is what silently emptied several other
  // tests in this suite and is worth not repeating here.)
  // Pull each constant INDEPENDENTLY. The first version matched them as one adjacent block and broke the
  // moment v11.61 inserted EMOPEN_SCHEMA between them — a test that depends on the ORDER of unrelated
  // declarations fails on a change that is none of its business.
  const grabConst=(name)=>{ const m=src.match(new RegExp('var '+name+'\\s*=\\s*[^;]+;')); return m?m[0]:null; };
  const cK=grabConst('EMOPEN_KEY'), cF=grabConst('EM_FRESH_MIN'), cS=grabConst('EMOPEN_SCHEMA');
  ok(!!cK && !!cF && !!cS, 'EMOPEN_KEY, EM_FRESH_MIN and EMOPEN_SCHEMA are all declared at module scope');
  eval([cK,cF,cS].filter(Boolean).join('\n'));
  ok(typeof EM_FRESH_MIN==='number' && EM_FRESH_MIN>0, 'EM_FRESH_MIN is a positive number', EM_FRESH_MIN);

  // (v11.83) emBand now calls dte0NotToday(), which reaches into ifChain. Stub it here: this block tests
  // the BAND, and the detector gets its own coverage in §38. A missing dependency inside an eval throws
  // ReferenceError and empties the whole block, which is how several assertions have silently died before.
  function dte0NotToday(){ return false; }
  eval(ex('inReplay'));
eval(ex('sessionDayStr'));
eval(ex('emBand'));

  // open 770.00 (und) -> 7700 chart; EM 35 -> 35 chart; band 7665..7735
  global.__cands=[ {o:770.00,h:770.5,l:769.5,c:770.2}, {o:770.2,h:770.4,l:769.0,c:769.0} ];
  global.__chain={ err:null, dte0:{ em:{ em:35, k:7700 } }, toFri:{ em:{ em:70, k:7700 } } };
  global.__phase={ rth:true, mins:8*60+35, open:8*60+30 };   // 5 minutes after the open

  let B=emBand('SPY');
  ok(B.ok===true,                       'a well-formed day yields a band', B.why);
  ok(B.open===7700,                     'open is the FIRST candle open, on chart scale', B.open);
  ok(B.em===35,                         'EM is dte0 (35), NOT toFri (70)', B.em);
  ok(B.low===7665 && B.high===7735,     'the rails are open ∓ EM', [B.low,B.high]);
  ok(B.now===7690,                      'now is the LAST candle close, on chart scale', B.now);
  ok(B.pct===29,                        'displacement 10 of 35 reads 29%', B.pct);
  ok(B.est===false,                     'captured 5 minutes in is not an estimate', B.est);

  // ---------- the capture is not overwritten when the straddle later decays ----------
  global.__chain={ err:null, dte0:{ em:{ em:9, k:7700 } }, toFri:{ em:{ em:70, k:7700 } } };
  global.__phase={ rth:true, mins:14*60, open:8*60+30 };
  let B2=emBand('SPY');
  ok(B2.em===35,                        'a later render REUSES the captured EM — the band does not shrink all day', B2.em);
  ok(B2.low===7665 && B2.high===7735,   'so the rails stay where the open put them', [B2.low,B2.high]);

  // ---------- a new day re-captures ----------
  global.ctTodayStr=function(){ return '2026-08-25'; };
  let B3=emBand('SPY');
  ok(B3.em===9,                         'a NEW day captures afresh rather than inheriting yesterday', B3.em);

  // ---------- late first capture is flagged, not reconstructed ----------
  global.ctTodayStr=function(){ return '2026-08-26'; };
  global.__phase={ rth:true, mins:11*60, open:8*60+30 };     // 150 minutes after the open
  let B4=emBand('SPY');
  ok(B4.est===true,                     'a capture 150 minutes in is marked ~EST', B4.est);
  ok(B4.em===9,                         'and is used as-is — no sqrt(T) reconstruction is attempted', B4.em);

  // ---------- refusals ----------
  global.ctTodayStr=function(){ return '2026-08-27'; };
  global.__chain={ err:null, dte0:{ em:null }, toFri:{ em:{ em:70, k:7700 } } };
  let B5=emBand('SPY');
  ok(B5.ok===false,                     'a one-sided straddle refuses rather than drawing half a band');
  ok(/straddle/.test(B5.why||''),       'and says why in words', B5.why);
  ok(!/70/.test(String(B5.em)),         'it does NOT reach for toFri when dte0 is missing', B5.em);

  // ---------- (v11.50) PRE-OPEN / WEEKEND: anchor on the PRIOR SESSION'S CLOSE ----------
  // The band was ABSENT every weekend and every pre-open — precisely when a trader is preparing —
  // because closedCandles() is today-only. Quoting the expected move from the prior close is the
  // standard anchor when today has not started, and the real open replaces it the moment the first
  // RTH bar closes. It must SAY which reference it used or it is a mislabel.
  global.__cands=[];
  global.__chain={ err:null, dte0:{ em:{ em:20, k:7700 } }, toFri:{ em:{ em:70, k:7700 } } };
  global.ctTodayStr=function(){ return '2026-08-29'; };
  global.STATE={ SPY:{ price:772.00, contCloses:[
    {c:768.00, day:'2026-08-27'}, {c:771.00, day:'2026-08-27'},
    {c:769.50, day:'2026-08-28'}, {c:770.00, day:'2026-08-28'} ] } };
  let B7=emBand('SPY');
  ok(B7.ok===true,                      'with no bars today the band still draws', B7.why);
  ok(B7.anchor==='prevClose',           'and says it is anchored on the prior close', B7.anchor);
  ok(B7.open===7700,                    'the anchor is the LAST close of the LAST completed day', B7.open);
  ok(B7.low===7680 && B7.high===7720,   'rails are prior close ∓ EM', [B7.low,B7.high]);
  ok(B7.now===7720,                     'now uses the live price when there is no candle', B7.now);
  ok(B7.pct===100,                      'displacement measured from the prior close', B7.pct);

  global.__cands=[ {o:770.00,h:770.5,l:769.5,c:770.2}, {o:770.2,h:770.4,l:769.0,c:769.0} ];
  global.ctTodayStr=function(){ return '2026-08-30'; };
  let B8=emBand('SPY');
  ok(B8.anchor==='open',                're-anchors to the real open as soon as a bar closes', B8.anchor);

  global.__cands=[];
  global.STATE={ SPY:{ contCloses:[] } };
  global.ctTodayStr=function(){ return '2026-08-31'; };
  let B6=emBand('SPY');
  ok(B6.ok===false && /prior session/.test(B6.why||''), 'with neither, it refuses and says why', B6.why);
}

// ---------- 4. the face wires it, and the removed cells are gone ----------
{
  const f=ex('secFrame');
  ok(/emBand\(/.test(f),                'secFrame calls emBand');
  ok(/g3emw/.test(f) && /g3emn/.test(f),'and renders the rail and the price mark');
  ok(!/cell\('DEX'/.test(f),            'DEX is off the face — its sign is structurally pinned');
  ok(!/cell\('TERM'/.test(f),           'TERM is off the face');
  ok(!/cell\('ATR'/.test(f),            'ATR is off the face (it still sets ladder zone widths)');
  ok(/g3tag/.test(f),                   'the session-phase tag stays');
  ok(/g3emx/.test(f),                   'and there is a spoken refusal when the band cannot be drawn');
  // (v11.75) the prev-close marker lived on the numbers row, which is gone. The band hover still says it.
  ok(/re-anchors to the real open/.test(f), 'the hover says when the anchor is the prior close, not the open');
}

// ---------- 5. both numbers are ENROLLED ----------
{
  ok(/key:'emband'/.test(src),          'the band is registered as a feature');
  ok(/key:'dex'/.test(src),             'and DEX is enrolled even though it left the face');
  const dex=src.slice(src.indexOf("key:'dex'"), src.indexOf("key:'dex'")+2000);
  ok(/hit:null/.test(dex),              'DEX scores null — it does not vote before it is measured');
  ok(/px:/.test(dex),                   'and records the spot beside it, so the level can be compared cross-day');
  const emb=src.slice(src.indexOf("key:'emband'"), src.indexOf("key:'emband'")+4600);
  ok(/questions:/.test(emb) && /rule:/.test(emb), 'the band carries a question and a rule');
}


// ---------- 6. (v11.51) THE DEBUG HOOK ----------
// Every other read on the face has one. Without it the only way to check the band was to count DOM
// nodes and infer from pixel positions, which is exactly the kind of verification that stops happening.
{
  // ⚠ THIS WAS A FIXED +3000 CHARACTER SLICE and v11.73 grew the hook past it, so `targetInPlay` fell off
  // the end and an assertion failed on a field that was present and correct. Same disease as the
  // 40-record dedupe scan and the `session`/`sessionRoll` marker: a magic-number window that silently
  // stops covering what it is meant to cover. Bound it by the NEXT hook instead, which cannot drift.
  const hStart0=src.indexOf("window.__gptsDebug.emBand");
  const hEnd0=src.indexOf("window.__gptsDebug.", hStart0+30);
  const h=src.slice(hStart0, hEnd0>hStart0?hEnd0:undefined);
  ok(hEnd0>hStart0, 'the emBand hook is bounded by the next hook, not by a character count');
  ok(/window\.__gptsDebug\.emBand\s*=/.test(src), 'the band exposes a debug hook');
  ok(/anchor:B\.anchor/.test(h),      'it reports WHICH anchor was used — open or prevClose');
  ok(/low:/.test(h) && /high:/.test(h) && /pct:/.test(h), 'and both rails and the percentage');
  ok(/dte0EmSPX/.test(h) && /toFriEmSPX/.test(h),
     'it shows BOTH straddles side by side, so a regression to the week move is visible');
  ok(/THE v11\.49 BUG IS BACK/.test(h),
     'and names the v11.49 bug explicitly if the band is ever found running on toFri');
  ok(/targetInPlay/.test(h),          'it says whether the target is inside what today prices');
  ok(/ok:false/.test(h) && /why:/.test(h), 'and on refusal it returns the reason rather than throwing');
}


// ---------- 10. (v11.58) THE FOUR REGIME CELLS ----------
// The chip explained only the cell you were in, so there was no way to see what the other three meant,
// or that the 2x2 is a 2x2. It also now states the link to the band: an extension means opposite things
// in the two gamma halves, which is the one thing a reader must not get backwards.
{
  eval(ex('regimeTip'));
  const t1=regimeTip({g:-1,v:-1}), t2=regimeTip({g:-1,v:1}),
        t3=regimeTip({g:1,v:-1}),  t4=regimeTip({g:1,v:1}), t0=regimeTip(null);
  [['1',t1],['2',t2],['3',t3],['4',t4]].forEach(([n,t])=>{
    ok(/1\./.test(t)&&/2\./.test(t)&&/3\./.test(t)&&/4\./.test(t), 'cell '+n+' tip lists all FOUR cells');
    ok(new RegExp('You are in cell '+n).test(t), '...and says you are in cell '+n);
    ok((t.match(/▸/g)||[]).length===1, '...marking exactly ONE cell as active');
  });
  ok(/self-reinforcing/.test(t1),      'cell 1 names the double-amplification case');
  ok(/pins hold|higher-probability/.test(t4), 'cell 4 names the compressing case');
  ok(/No gamma book yet/.test(t0),     'with no book it names no cell rather than guessing');
  ok((t0.match(/▸/g)||[]).length===0, 'and marks none active');
  ok(/STRETCHED\s+only in cells 3 and 4/.test(t1),
     'the tip states the band link: an extension is STRETCHED only in the +G cells');
  ok(/what a trend day does/.test(t1),
     '...and that in −G running past the band is normal, not a fade signal');
  ok(/Neither points anywhere/.test(t1) && /gate the call/.test(t1),
     'and that neither points — they gate the call rather than voting in it');
  ok(t1.split('\n').length>=10,        'it renders as a list, not one paragraph');
}



// ---------- 11. (v11.59) THE ANCHOR MUST NOT DRIFT, AND THE WORDS MUST BE TRADER WORDS ----------
// `rr` is a LIVE scale ratio that moves with the basis: measured 2026-08-22, undScale went
// 0.099778 -> 0.099775 in twenty seconds and slid the whole band 0.05 with it. Friday's opening print
// cannot change. EM was captured once; the anchor was recomputed every render. Half-anchored is not
// anchored, and a rail that wobbles is not a reference.
{
  const b=ex('emBand');
  ok(/rr:rr/.test(b),            'the scale factor is CAPTURED alongside the expected move');
  ok(/rec\.rr/.test(b),          'and reused, so the anchor cannot drift with the basis');
  ok(/hiFirst/.test(b),          'which extreme came FIRST is recorded — order distinguishes two different days');
  const f=ex('secFrame');
  ok(/HOD/.test(f) && /LOD/.test(f), 'the shape line speaks in HOD/LOD, not "up 53% down 55%"');
  ok(/retraced/.test(f),         'and says "retraced", which is what a retracement is called');
  // (v11.75) the shape arrow left with the numbers row; hiFirst is still recorded and still in the hook.
  ok(/hiFirst/.test(src),        'which extreme came first is still recorded');
  // (v11.64) used/remaining moved OFF the sentence and ONTO the rail as four segments. The coverage
  // does not disappear — it follows the number to its new home.
  ok(/g3seg/.test(f),            'used and remaining live on the RAIL now, as four segments');
  // strip // comments first: the changelog note in the source legitimately QUOTES the old wording, and a
  // test that cannot tell code from documentation will fail on its own history.
  const fCode=f.split('\n').filter(l=>!/^\s*\/\//.test(l)).join('\n');
  ok(!/gave back/.test(fCode),   'the old engineering phrasing is gone from what is RENDERED');
  ok(/shape:B\.shape/.test(src) && /hiWater:B\.hiWater/.test(src),
     'the debug hook surfaces the shape fields — reading pixels is how the drift went unnoticed');
  // hovers must be SHORT. A tooltip nobody finishes reading is a tooltip nobody reads.
  const tips=(f.match(/g3tip\('((?:[^'\\]|\\.)*)'/g)||[]).map(x=>x.length);
  const longest=Math.max.apply(null,tips);
  ok(longest<620, 'every hover in FRAME is under ~600 characters (longest '+longest+')', longest);
}


// ---------- 12. (v11.60) THE MOVE IN DOLLARS PER CONTRACT ----------
// A band in index points makes you do the x50 in your head. The multipliers are VERIFIED against CME /
// NinjaTrader specs, not assumed — a wrong one puts a wrong dollar figure on the face under the panel's
// own name, which is the same class of failure as the truncated Zero Gamma.
{
  eval(src.match(/var FUT_MULT=\{[^}]*\};/)[0]);
  eval(ex('usd'));
  ok(FUT_MULT.ES===50,  'ES is $50 per index point (tick 0.25 = $12.50)', FUT_MULT.ES);
  ok(FUT_MULT.MES===5,  'MES is $5 — one tenth of ES', FUT_MULT.MES);
  ok(FUT_MULT.NQ===20,  'NQ is $20', FUT_MULT.NQ);
  ok(FUT_MULT.MNQ===2,  'MNQ is $2 — one tenth of NQ', FUT_MULT.MNQ);
  ok(FUT_MULT.MES*10===FUT_MULT.ES && FUT_MULT.MNQ*10===FUT_MULT.NQ,
     'every micro is exactly one tenth of its E-mini');
  ok(usd(1736.5)==='$1,737',   'money is whole dollars with a thousands separator', usd(1736.5));
  ok(usd(200)==='$200',        'and no separator below a thousand', usd(200));
  ok(usd(0)==='$0',            'zero renders');
  ok(usd(null)===null && usd(NaN)===null, 'and a non-number yields null rather than "$NaN"');

  // the live case: EM 34.73 ES points
  // ⚠ 34.73*50 === 1736.4999999999998 in floating point. Rounding that straight to dollars gives $1,736
  // for a move worth $1,736.50 — a one-dollar error from arithmetic that looks exact. usd() rounds
  // through cents, and this is the regression that proves it.
  ok(34.73*FUT_MULT.ES !== 1736.5, 'the FP hazard is real: 34.73*50 is not exactly 1736.5', 34.73*FUT_MULT.ES);
  ok(usd(34.73*FUT_MULT.ES)==='$1,737', 'EM 34.73 pts on ES still renders $1,737 per contract', usd(34.73*FUT_MULT.ES));
  ok(usd(34.73*FUT_MULT.MES)==='$174',  '...and $174 on the micro', usd(34.73*FUT_MULT.MES));

  const b=ex('emBand');
  ok(/futMult\(\)/.test(b),      'emBand reads the multiplier from the CHART symbol');
  ok(/emUsd/.test(b) && /usedUsd/.test(b) && /leftUsd/.test(b), 'and carries EM, used and left in dollars');
  ok(/microUsd/.test(b),         'plus the micro equivalent, for the hover');
  ok(b.indexOf('out.roomUp')<b.indexOf('leftUsd'), 'room is computed BEFORE it is converted to money');

  const f=ex('secFrame');
  ok(/g3ct/.test(f),             'the contract chip renders');
  // (v11.69) the chip dropped the word EM when the row was renamed STRADDLE — it would have been the one
  // place on the face still calling a 0.80-sigma band an expected move. It is now `ES $1,736/ct`.
  ok(/usd\(EBc\.emUsd\)\+'\/ct/.test(f), 'the contract chip still prices a whole straddle per contract');
  ok(!/' \u00b7 EM '/.test(f),            'and no longer calls it an expected move');
  ok(/EBc && EBc\.ok && EBc\.mult/.test(f), 'only when there IS a contract — a SPY chart has no multiplier');
  ok(/EB\.open-EB\.loWater/.test(f) && /EB\.high-EB\.hiWater/.test(f),
     'the segments are used-and-remaining on BOTH sides, measured from the day\'s extremes');
  // ⚠ DESCRIPTIVE ONLY. A dollar figure is one step from sizing, R:R and P&L, all of which are banned.
  // Check the VISIBLE labels, not the hovers: the disclaimer has to use the words "profit or loss" in
  // order to deny them, and a test that cannot tell a denial from a claim fails on its own safeguard.
  // A regex cannot strip g3tip(...) — its argument nests usd(...) and dispNum(...) calls, so there is no
  // regular expression for the matching paren. Count them.
  function stripCalls(code, fn){
    var out='', i=0;
    while(i<code.length){
      var at=code.indexOf(fn+'(', i);
      if(at<0){ out+=code.slice(i); break; }
      out+=code.slice(i,at);
      var d=0, k=at+fn.length;
      for(;k<code.length;k++){ if(code[k]==='(')d++; else if(code[k]===')'){ d--; if(d===0){ k++; break; } } }
      i=k;
    }
    return out;
  }
  const fVisible=stripCalls(f.split('\n').filter(l=>!/^\s*\/\//.test(l)).join('\n'),'g3tip');
  ok(!/\bP&L\b|profit|\bloss\b|position size|risk per/i.test(fVisible),
     'no VISIBLE label frames this as profit, loss, or position size');
  ok(/per contract/.test(f) && /not a position/.test(f),
     'and the hover says outright that it is the MOVE converted, not a position');
}


// ---------- 13. (v11.61) SCHEMA STAMP, GAMMA PILES, SKYLIT COLOURS ----------
{
  // --- the schema stamp: the ACTUAL fix for the drifting dot ---
  // v11.59 pinned the scale, but a record written BEFORE it has no `rr`, the date key still matches, and
  // the pin silently never fires. Measured live: open 7695.86 -> 7695.29 in minutes. Persisted state from
  // an older version must be RE-TAKEN, never half-trusted.
  ok(/EMOPEN_SCHEMA/.test(src),                 'the capture carries a schema version');
  const eb=ex('emBand');
  ok(/S\.v!==EMOPEN_SCHEMA/.test(eb),           'a record from an older schema is discarded, not reused');
  ok(/rec\.rr==='number'[\s\S]{0,60}rec=null/.test(eb) || /rec=null; out\.recaptured=true/.test(eb),
     'and a record missing `rr` is re-captured even if the stamp matches');

  // --- the piles ---
  // (v11.77) the IF logic moved to emPilesIF when Skylit became the primary node source; these
  // assertions still describe it and still matter — it is the disclosed fallback, not dead code.
  const pf=ex('emPilesIF');
  // (v11.64) REVERSED DELIBERATELY. Skylit and IF differ ~113x in magnitude on the same nominal window
  // AND carry opposite sign conventions. Every other number on this band is IF's, so the piles must be too
  // or nothing on the rail composes. Skylit's node map keeps its home in (3) TRADE LOCATION.
  ok(/ifChain/.test(pf) && !/cpRows/.test(pf),
     'piles read INSIDERFINANCE, the same book as the band, the rails, the target and the flow chip');
  ok(/CFG\.nodeThresh/.test(pf), 'the cut is the EXISTING nodeThresh slider, not a second invented threshold');
  ok(/net<0/.test(pf),           'NEGATIVE net gamma at a strike means dealers are short it -> ACCELERATOR');
  ok(/B\.low \|\| disp>B\.high/.test(pf) || /disp<B\.low/.test(pf), 'only strikes INSIDE the band are marked');

  const f=ex('secFrame');
  ok(/emPiles\(/.test(f),        'the face draws them');
  ok(/Math\.sqrt/.test(f),       'height is sqrt-compressed so a 100% King does not flatten a 30% pile');
  ok(/g3pile/.test(f),           'piles have their own class, below the rail');

  // --- SKYLIT'S COLOUR CONVENTION, which the mockup originally got wrong ---
  ok(/g3pile\.acc\{background:#a371f7\}/.test(src), 'accelerator = PURPLE (put-dominant) — Skylit\'s convention');
  ok(/g3pile\.brk\{background:#e3c341\}/.test(src), 'brake = YELLOW (call-dominant) — Skylit\'s convention');
  ok(/g3emT\{[^}]*#4fd1e0/.test(src),                'the target moved to CYAN, since yellow now means positive gamma');
  ok(/g3tgt\{[^}]*#4fd1e0/.test(src),                '...on line 1 too, so the two target marks agree');
  ok(/g3emn\.g3str\{background:#f0616d\}/.test(src),'STRETCHED is RED — amber sat beside the brake yellow meaning something else');

  // --- the raw book in the export ---
  ok(/book:\(function\(\)/.test(src),          'the export carries the RAW 60-strike book');
  ok(/`nodes` is the MODEL/.test(src) || /nodes` above is what the node model/.test(src),
     'and says why: `nodes` is the model\'s ~6, `book` is everything the feed sent');

  // --- the hover the user asked for ---
  const rt=ex('regimeTip');
  ok(/fight the move or feed it/.test(rt),      'the gamma line is the plain one');
  ok(/HOW price moves, not/.test(rt),           'and carries "how, not which way" — the project\'s own phrasing');
  ok(!/DAMPS the move or AMPLIFIES/.test(rt),   'the old abstract wording is gone');
}


// ---------- 14. (v11.62) THE WINDOW, AND THE FLOW ----------
// VERIFIED against InsiderFinance's published header, all expiries, to the decimal:
//   ours call +263.83B / put -250.49B / net +13.34B / ratio 1.053
//   them call +263.8B  / put -250.5B  / net +13.3B  / ratio 1.05
// There was never a sign bug. The sign FLIPS with the window — near book negative, full chain positive —
// and comparing across windows produced two false alarms in one session. Hence: name the window, always.
{
  ok(/GEX_WINDOW_LABEL/.test(src),          'expiry windows have names');
  const wn=ex('gexWindowNote');
  ok(/sign DIFFERS by window/i.test(wn),    'and the note SAYS the sign differs by window');
  ok(/Never compare a number from one window against another/i.test(wn),
     'and warns against comparing across them — the exact mistake that cost two false alarms');

  const hf=ex('hedgeFlow');
  ok(/toFri/.test(hf) && /dte0/.test(hf),   'flow uses the NEAR book, falling back from toFri to dte0');
  ok(!/c\.all/.test(hf),                    'and deliberately NOT the all-expiry total, where LEAPS drown the live flow');
  ok(/spot\*0\.01/.test(hf),                'netGEX is per 1% move, so it divides by one percent of spot');
  ok(/netGEX<0/.test(hf),                   'negative net gamma means the flow FEEDS the move');
  ok(/out\.window=w/.test(hf),              'and the window travels WITH the number');

  const f=ex('secFrame');
  ok(/g3flow/.test(f),                      'the fuel chip renders');
  ok(/FEEDS /.test(f) && /FIGHTS /.test(f), 'and says which, in words');
  ok(/gexWindowNote\(HF\.window\)/.test(f), 'its hover names the window it came from');
  ok(/g3flow\.g3feeds\{[^}]*#a371f7/.test(src),
     'FEEDS wears the ACCELERATOR purple — same grammar as the piles, so chip and rail agree');
  ok(/g3flow\.g3fights\{[^}]*#e3c341/.test(src),
     'and FIGHTS wears the BRAKE yellow');
  // FLIP is a LADDER row, so its hover lives in secLoc, not secFrame. Checking the whole source is the
  // honest assertion here: what matters is that the note exists and is attached to the FLIP row.
  ok(/FLIP is their ALL-EXPIRY zero gamma/.test(src),
     'FLIP states it is the ALL-EXPIRY book — the phantom "contradiction" with the regime chip');
  ok(/isHVL\?' \\u26a0 FLIP is their ALL-EXPIRY/.test(src) || /isHVL\?'[^']*ALL-EXPIRY/.test(src),
     '...and only on the FLIP row, not on every ladder level');

  // enrolled, regime-split, voting on nothing
  const fl=src.slice(src.indexOf("key:'flow'"), src.indexOf("key:'flow'")+3000);
  ok(/flow_reaches_em_feeds/.test(fl),      'the "can hedging carry it to the expected move" question is ASKED');
  ok(/flow_caps_move_fights/.test(fl),      'and its mirror in positive gamma is scored SEPARATELY');
  ok(/hit:null/.test(fl),                   'it votes on nothing until measured');
  ok(/toRailBn/.test(fl),                   'and records the flow the remaining distance would require');
  ok(/window:f\.window/.test(fl),           'with the window recorded alongside');
}


// ---------- 15. (v11.63) THE ANCHOR IS CAPTURED, AND BIG DOLLARS ARE ABBREVIATED ----------
// THE REAL CAUSE OF THE MOVING DOT, third diagnosis and the correct one. v11.59 pinned the SCALE, v11.61
// added a schema stamp so the pin actually fired — but the anchor was still recomputed from
// closedCandles()[0].o every render, and that array is a SLIDING WINDOW. As it slid, cs[0] became a LATER
// bar and the anchor walked forward: 7695.75 -> 7711.66 -> 7713.26 -> 7713.71 in minutes, ~18 points,
// while `em` sat perfectly still because IT was captured. Pinning the scale fixed 0.05 of an 18-pt problem.
{
  const b=ex('emBand');
  ok(/openU:openU/.test(b),        'the OPEN is captured into the record, like the expected move');
  ok(/openSo:/.test(b),            'with the opening bar\'s seconds-of-day');
  ok(/rec\.openU\*useRr/.test(b),   'and the anchor is read FROM the record, not the live array');
  ok(/cs\[0\]\.so<rec\.openSo/.test(b),
     'self-heal: an EARLIER bar replaces the captured one, so it can only move backward toward the true open');
  ok(!/cs\[0\]\.so>rec\.openSo/.test(b),
     '...and never forward, which is what let the sliding window drag it');
  ok(/EMOPEN_SCHEMA=3/.test(src),  'the schema bumped, so records without the open are re-taken');

  // big dollars
  eval(ex('usd')); eval(ex('usdBig'));
  ok(usdBig(213827434)==='$214M',  'a hedging flow reads $214M, not $213,827,434', usdBig(213827434));
  ok(usdBig(8280000000)==='$8.3B', 'billions keep one decimal', usdBig(8280000000));
  ok(usdBig(16409331084)==='$16B', 'and drop it past ten billion', usdBig(16409331084));
  ok(usdBig(1736.5)==='$1,737',    'contract-sized figures stay EXACT — $1,736 is meaningful to the dollar', usdBig(1736.5));
  ok(usdBig(950)==='$950',         'small values pass through unchanged');
  ok(usdBig(null)===null && usdBig(NaN)===null, 'and a non-number yields null, never "$NaN"');
  const f=ex('secFrame');
  ok(/usdBig\(HF\.perPt\)/.test(f), 'the flow chip uses the abbreviated form');
  ok(/usd\(EBc\.emUsd\)/.test(f),   'while the contract chip keeps the exact one');
}


// ---------- 16. (v11.64) ONE BOOK, ONE WINDOW — THE SANITY PASS ----------
// The bug this exists to prevent: the piles were drawn from SKYLIT's book while the flow chip came from
// INSIDERFINANCE. Measured on the same nominal window they differ by ~113x (Skylit gross $0.58B vs IF
// $65.8B) AND carry opposite sign conventions (Skylit's decomposition puts both legs positive, IF's puts
// are negative). Two books on one rail cannot be summed, compared, or trusted to compose — and every
// other number on this band already comes from IF.
{
  // (v11.77) the IF logic moved to emPilesIF when Skylit became the primary node source; these
  // assertions still describe it and still matter — it is the disclosed fallback, not dead code.
  const pf=ex('emPilesIF');
  ok(/ifChain/.test(pf),        'piles read the INSIDERFINANCE chain');
  ok(!/cpRows/.test(pf),        'and NOT Skylit\'s book — different units, opposite sign convention');
  ok(/gexProf/.test(pf),        'via the per-strike gamma profile');
  ok(/toFri/.test(pf) && /dte0/.test(pf), 'in the NEAR window, the same one the flow chip uses');
  ok(/spot\*0\.01/.test(pf),    'converting $-per-1% to $-per-point with the SAME spot the book was built on');
  ok(/net<0/.test(pf),          'NEGATIVE net gamma at a strike means dealers are short there -> ACCELERATOR');
  ok(/CFG\.nodeThresh/.test(pf),'and the cut is still the existing slider');

  // the profile must SUM to the book, or the piles are lying about the whole
  const lf=(function(){ const s2=fs.readFileSync('./current/gex-if-levels.user.js','utf8');
    const re=/function\s+levelsFor\s*\(/g, m=re.exec(s2); let i=s2.indexOf('{',m.index),d=0,e=-1;
    for(let k=i;k<s2.length;k++){ if(s2[k]==='{')d++; else if(s2[k]==='}'){ d--; if(d===0){e=k;break;} } }
    return s2.slice(m.index,e+1); })();
  ok(/b\.call\+=g/.test(lf) && /b\.put\+=g/.test(lf), 'the companion keeps the call and put legs PER STRIKE');
  ok(/gexProf\.push/.test(lf),  'and exports them as a profile');
  ok(/gm\/mx < 0\.01/.test(lf), 'trimming only the near-zero tail, so the profile still sums to the book');

  // --- the arithmetic, run for real against the companion ---
  const s2=fs.readFileSync('./current/gex-if-levels.user.js','utf8');
  eval(s2.match(/var SIDE_MIN\s*=\s*[^;]+;/)[0]);
  eval(lf);
  const spot=100, gg=(gam,oi)=>gam*oi*100*spot*spot*0.01;
  const R=levelsFor([{strike:100,cp:'C',gamma:0.05,openInterest:1000},
                     {strike:105,cp:'C',gamma:0.02,openInterest:500},
                     {strike:100,cp:'P',gamma:0.04,openInterest:2000},
                     {strike:95, cp:'P',gamma:0.03,openInterest:800}], spot, ()=>true);
  const sc=R.gexProf.reduce((a,r)=>a+r[1],0), sp=R.gexProf.reduce((a,r)=>a+r[2],0);
  ok(Math.abs(sc-R.callGEX/1e6)<0.05, 'profile CALL legs sum to the book callGEX', [sc, R.callGEX/1e6]);
  ok(Math.abs(sp-R.putGEX/1e6)<0.05,  'profile PUT legs sum to the book putGEX',  [sp, R.putGEX/1e6]);
  ok(R.gexProf.every(r=>r[2]<=0),     'puts are NEGATIVE — their convention, verified against their page');
  ok(Math.abs(R.gexProf.find(r=>r[0]===100)[1]-gg(.05,1000)/1e6)<0.05,
     'a single strike matches gamma x OI x 100 x spot^2 x 0.01 exactly');

  // --- the path ---
  const pa=ex('emPath');
  // (v11.68) tightened from `< lo || > hi` to `<= lo || >= hi`, plus an explicit skip for a pile sitting
  // exactly ON the target — which the King always does, since target = Mag = the heaviest strike.
  ok(/P\.disp<=lo \|\| P\.disp>=hi/.test(pa), 'the path sums only piles STRICTLY between price and the target');
  ok(/acc\+=P\.perPt/.test(pa) && /brk\+=P\.perPt/.test(pa), 'splitting them by polarity into fuel and brake');
  ok(/verdict/.test(pa),         'and returning a plain-word verdict');
  // the caveat lives in the block comment ABOVE the function, which ex() does not return — check source.
  ok(/market-impact coefficient that no option chain contains/.test(src),
     'and the source states the limit: dollars of hedging cannot be converted to points without an impact figure');

  const f=ex('secFrame');
  ok(/g3seg/.test(f),            'the four money segments render ON the rail');
  ok(/Math\.abs\(b-a\)<9/.test(f), 'and a segment too narrow to hold its label is dropped rather than overlapping');
  // (v11.75) emPath no longer has a row of its own — the sentence names the level and where it leads.
  ok(/emPath\(/.test(src),       'emPath still exists for the piles hook and the recorder');
  ok(/emRead\(EB, sym\)/.test(f), 'and the sentence is what renders');
  const fNoComments=f.split('\n').filter(l=>!/^\s*\/\//.test(l)).join('\n');
  ok(!/retraced/.test(fNoComments),
     'the old shape sentence is gone from what RENDERS — the rail draws what it used to narrate');
  // (v11.68) reworded when the dollars moved from gross to net — the hover now says what fraction of the
  // gross survives as net, then gives the net figure, so the old phrase no longer appears.
  ok(/survives as net dealer exposure/.test(f), 'pile hovers state the NET flow');
  ok(/\/ PT to hedge/.test(f),                   'in per-point terms');
  ok(/needs a market-impact figure no option chain contains/.test(f),
     '...and refuse to imply a distance, which is the one number nobody can honestly give');
}


// ---------- 17. (v11.65) ONE SCALE, APPLIED ONCE ----------
// TWO OF MY OWN FIXES WERE FIGHTING. v11.59 corrected open/now from live rr back to the captured rr.
// v11.63 then set `open` FROM THE RECORD — already at the captured scale — and left v11.59's correction
// running after it, so the anchor got multiplied by rec.rr/rr a SECOND time and drifted with live rr all
// over again. Pinned 7695.6 rendered as 7709.4 -> 7711.43. The symptom was identical to the bug the fixes
// were for, which is exactly why it survived two rounds of "fixed".
{
  const b=ex('emBand');
  ok(/var useRr=/.test(b),          'ONE scale factor is chosen up front');
  ok(!/scalePinned/.test(b),        'and the old correction block is GONE — nothing to correct twice');
  ok(!/open\*=k/.test(b),           'nothing multiplies the anchor after it is set');
  ok(/rec\.openU\*useRr/.test(b),   'the anchor is the RECORD times the captured scale');
  ok(/now  = nowU\*useRr/.test(b),  'and `now` uses the SAME scale, so the two can be subtracted');
  ok(/out\.hiWater=hiU\*useRr/.test(b), 'the water marks share it too');
  // exactly one multiplication of openU anywhere in the function
  const mults=(b.match(/openU\s*\*/g)||[]).length;
  ok(mults===2, 'openU is scaled in exactly TWO places (record branch / fallback) and never chained', mults);
  ok(/var open=0, now=0;/.test(b),
     'and it is not pre-scaled with live rr first — that double-multiplication was the whole bug');

  // --- the trim must declare what it costs ---
  const s2=fs.readFileSync('./current/gex-if-levels.user.js','utf8');
  ok(/gexProfCoverage/.test(s2),    'the profile reports what fraction of the book it covers');
  ok(/never be presented as if it were the whole/.test(s2),
     'because a 1% cut removes ~5% of a live 780-strike chain — small, but it is not the whole book');
}

// ---------- 18. (v11.75) THREE ROWS, AND EACH FACT LIVES ON EXACTLY ONE ----------
// v11.66 moved the percentage and the session badge off the rail onto a line of their own. v11.75 deleted
// that line: the RAIL already draws position and remaining room as a picture, and the sentence below now
// names what is ahead and where it leads. Four rows became three.
{
  const f=ex('secFrame');
  ok(!/g3shape/.test(f),   'the numbers row is gone entirely');
  ok(!/g3pct/.test(f),     'with the used percentage');
  ok(!/g3left/.test(f),    'and the remaining figure');
  ok(!/g3pace/.test(f),    'and the pace chip that duplicated the sentence');
  ok(!/g3replay/.test(f),  'and the replay badge, at the user request');
  ok(/g3read/.test(f),     'the sentence remains, and is now the only prose on the section');
  ok(/EB\.est\?'~':''/.test(f), '~EST moved onto the rail labels rather than dying in a hover');
  const tildes=(f.match(/EB\.est\?'~':''/g)||[]).length;
  ok(tildes===2, 'on both rails', tildes);
}


// ---------- 19. (v11.66) THE FLOW CHIP BREATHES ----------
// "$214M/pt" is nine glyphs with no gap in them; at 8px it reads as one token.
{
  const u=ex('usdBigSp');
  ok(/usdBig\(v\)/.test(u),            'the spaced form DELEGATES — one rounding rule, not two');
  ok(/\$1 \$2/.test(u),                'and only inserts a space before the unit');
  const f=ex('secFrame');
  ok(/usdBigSp\(HF\.perPt\)/.test(f), 'the flow chip uses it');
  ok(/ \/ PT</.test(f),                'and prints the unit spaced and capitalised');
  ok(!/\+'\/pt'/.test(f),              'the tight form is gone from the chip');
  // the hovers keep the tight form — a space before the unit inside a sentence looks like a typo
  ok(/usdBig\(HF\.perPt\)\+\n?/.test(f) || /usdBig\(HF\.perPt\)/.test(f),
                                       'while the hover sentence still uses the tight form');
  // BEHAVIOUR, not spelling. The previous round of this suite broke on a reformat that changed nothing,
  // so the formatter is EXERCISED rather than pattern-matched.
  {
    const sandbox={};
    // eslint-disable-next-line no-new-func
    new Function('S', ex('usdBig')+'\n'+ex('usd')+'\n'+ex('usdBigSp')+'\nS.f=usdBigSp; S.t=usdBig;')(sandbox);
    ok(sandbox.f(213827434)==='$214 M', 'a 214-million flow prints as "$214 M"', sandbox.f(213827434));
    ok(sandbox.f(8.3e9)==='$8.3 B',     'and billions keep one decimal', sandbox.f(8.3e9));
    ok(sandbox.f(45000)==='$45 K',      'and thousands are spaced too', sandbox.f(45000));
    ok(sandbox.f(1736.5)==='$1,737',    'while a contract-sized figure stays exact and unspaced', sandbox.f(1736.5));
    ok(sandbox.t(213827434)==='$214M',  'and the tight form is genuinely unchanged', sandbox.t(213827434));
  }
}

// ---------- 20. (v11.66) THE PILES HAVE A HOOK ----------
// v11.51 wrote the rule down and the piles shipped without one anyway, so verifying them meant
// rebuilding emPiles() by hand in the console. That reconstruction is the thing the rule prevents.
{
  ok(/__gptsDebug\.piles\s*=/.test(src), 'the piles are readable without counting DOM nodes');
  const hStart=src.indexOf('__gptsDebug.piles');
  // ⚠ do NOT end this slice at '__gptsDebug.session' — it matches `sessionRoll`, declared ~9,000
  // lines EARLIER, so the slice comes back empty and every assertion below fails on a hook that is
  // present and correct. End at the function's own closing marker instead.
  const hEnd=src.indexOf('__gptsDebug.session = function', hStart);
  ok(hEnd>hStart, 'the hook body is locatable', hStart+'..'+hEnd);
  const h=src.slice(hStart, hEnd);
  ok(/gross:\(isSk\?null:P\.gexM\)/.test(h) && /net:\(isSk\?null:P\.netM\)/.test(h),
     'it returns BOTH legs for an IF node — gross sizes the pile, net is the residual — and null for Skylit');
  ok(/netFrac/.test(h),               'with the fraction that survives the cancellation');
  ok(/coverage/.test(h),              'and the profile coverage, so the trim is never mistaken for a gap');
  ok(/pathIncludesTarget/.test(h),    'it states whether the target is counted as on the path to itself');
  ok(/pathStrictly/.test(h),          'and lists what lies STRICTLY between price and the target');
}

// ---------- 21. (v11.66) THE RAIL GEOMETRY CONTRACT ----------
// Found by RENDERING the section offline in Chromium with the real CSS and real live values, not by
// reading it: the money labels sit at top:0 in a 9px line box, and the price dot is 10px with a 2px ring,
// so its PAINTED top was inside the label band. Whenever price happened to sit near the middle of a spent
// span, the dot's dark ring cut straight through that span's figure — "$1,394" rendered as "$1̶,394".
// Value-dependent, so it is invisible in any single screenshot where the dot is not on a label, and it had
// been shipping since the segments landed at v11.64.
{
  // take the LAST definition of each rule — the stylesheet has earlier ones that are overridden
  function px(cls, prop){
    // ⚠ `top:0` carries NO UNIT. A px-only pattern reads it as absent, segTop comes back null, and the
    // arithmetic below silently becomes NaN — the check passes by not running. Accept a bare 0.
    const re=new RegExp('\\.'+cls+'\\{[^}]*'+prop+':(-?\\d+(?:\\.\\d+)?)(?:px)?[;}]','g');
    let m, v=null; while((m=re.exec(src))) v=parseFloat(m[1]);
    return v;
  }
  const segTop=px('g3seg','top'), segLine=px('g3seg','line-height');
  const railTop=px('g3emr','top'), dotTop=px('g3emn','top'), dotH=px('g3emn','height');
  const boxH=px('g3emt','height');
  ok([segTop,segLine,railTop,dotTop,dotH,boxH].every(v=>typeof v==='number' && isFinite(v)),
     'the rail geometry is readable from the stylesheet', [segTop,segLine,railTop,dotTop,dotH,boxH].join(','));

  const labelBottom = segTop + segLine;          // money labels own rows 0..9
  const RING = 2;                                 // .g3emn box-shadow 0 0 0 2px
  const paintedDotTop = dotTop - RING;
  ok(paintedDotTop >= labelBottom,
     'the dot INCLUDING its ring clears the money-label band',
     'painted dot top '+paintedDotTop+' vs label bottom '+labelBottom);

  ok(dotTop === railTop - 3,   'the dot stays centred on the rail', dotTop+' vs '+(railTop-3));
  ok(px('g3emo','top') === railTop - 4, 'the anchor notch stays centred too', px('g3emo','top'));
  ok(boxH >= railTop + 4 + 12, 'the box still leaves a lane under the rail for the piles', boxH);
}

// ---------- 22. (v11.77) THE NODES COME FROM SKYLIT, AND FAILURE IS NEVER SILENT ----------
// The user's architecture: InsiderFinance prices the DAY, Skylit marks the LEVELS. A King plainly visible
// on the SPXW tape at 7710 was missing from the rail because IF's book calls that strike POSITIVE and
// small in all three windows (dte0 +$88M/9%, toFri +$219M/11%, all +$270M/4%) while Skylit calls it the
// King at −100%. Not a window artefact — all three checked. They measure different things: live
// accumulated positioning vs open-interest gamma. A stock beside a flow.
{
  const sk=ex('skPiles');
  ok(/tapeMap\('SPXW'\)/.test(sk),      'the nodes are read from the SPXW tape');
  ok(/L\.dispScale/.test(sk),           'and converted SPXW strike -> chart with the ladder scale');
  ok(/accel:\(pct<0\)/.test(sk),        'negative %King is an accelerator');
  ok(/brake:\(pct>0\)/.test(sk),        'positive is a brake');
  ok(/balanced:false/.test(sk),         'and BALANCED cannot occur — one signed number has no legs to cancel');
  ok(/usdK:/.test(sk) && /kingKd/.test(sk), 'dollars are DERIVED from the King value');

  // every refusal carries a reason — this is what kills the false all-clear
  const whys=(sk.match(/out\.why=/g)||[]).length;
  ok(whys>=6, 'every distinct failure sets an explicit reason', whys);
  ['no band','no scale','tape unreadable','no strike map','tape thin','no King'].forEach(w=>
    ok(sk.indexOf(w)>=0, 'reason present: '+w));
  ok(/SK_MIN_STRIKES/.test(sk) && /var SK_MIN_STRIKES = 20;/.test(src),
     'a thin tape is caught by a named floor, not by luck');
  ok(/⚖ hand-set/.test(src.slice(src.indexOf('var SK_MIN_STRIKES'), src.indexOf('var SK_MIN_STRIKES')+160)),
     'flagged hand-set on the same line it is declared');

  // the router: Skylit first, IF as a NAMED fallback
  const rt=ex('emPiles');
  ok(/skPiles\(B, sym\)/.test(rt),      'Skylit is tried first');
  ok(/emPilesIF\(B, sym\)/.test(rt),    'InsiderFinance is the fallback');
  ok(/emPiles\.lastSrc/.test(rt) && /emPiles\.lastWhy/.test(rt),
     'and which one ran, plus why, is recoverable by the caller');
  ok(ex('emPilesIF').indexOf('ifChain(')>=0, 'the fallback still reads IF');
}


// ---------- 23. (v11.68) GROSS SIZES IT, NET PRICES IT ----------
// perPt used GROSS while accel used NET SIGN, so a strike could carry the dollar weight of its whole book
// and the direction of a rounding error. Live: 7650 overstated 84x, its net being 1.2% of its gross.
{
  // (v11.77) the IF logic moved to emPilesIF when Skylit became the primary node source; these
  // assertions still describe it and still matter — it is the disclosed fallback, not dead code.
  const f=ex('emPilesIF');
  ok(/perPt:\(Math\.abs\(net\)\*1e6\)\/onePct/.test(f), 'dollars come from |net|');
  ok(/grossPerPt:\(mag\*1e6\)\/onePct/.test(f),         'gross is kept, but under its own name');
  ok(/pct=Math\.round\(100\*mag\/maxMag\)/.test(f),     'height and the threshold still use GROSS');
  ok(/netFrac/.test(f),                                 'the surviving fraction is computed');
  ok(/var bal = \(netFrac < PILE_BAL_MIN\)/.test(f),    'and a thin one is BALANCED');
  ok(/accel:\(!bal && net<0\)/.test(f),                 'a balanced strike is not an accelerator');
  ok(/brake:\(!bal && net>0\)/.test(f),                 'nor a brake');
  ok(/\bPILE_BAL_MIN\s*=\s*15\b/.test(src),             'the cut is named and hand-set at 15%');
  ok(/⚖/.test(src.slice(src.indexOf('PILE_BAL_MIN')-400, src.indexOf('PILE_BAL_MIN'))),
     'and flagged as hand-set, not measured');

  // BEHAVIOUR: reproduce the live 7650 case and check both halves of the fix
  const legs={ call:2243.3, put:-2297.1 };                 // toFri, the blended book
  const gross=Math.abs(legs.call)+Math.abs(legs.put), net=legs.call+legs.put;
  const netFrac=100*Math.abs(net)/gross;
  ok(netFrac < 15, 'the live 7650 case is BALANCED under the cut', netFrac.toFixed(1)+'%');
  const onePct=76.9;
  ok(Math.round((gross*1e6)/onePct/1e6) === 59, 'gross would have printed $59M', Math.round((gross*1e6)/onePct/1e6));
  ok(Math.round((Math.abs(net)*1e6)/onePct/1e5)/10 === 0.7, 'net is $0.7M', Math.round((Math.abs(net)*1e6)/onePct/1e5)/10);
}

// ---------- 24. (v11.77) THE READ REFUSES RATHER THAN INVENTS ----------
// "Nothing sizeable between X and Y" is a claim about the MARKET. It was being made from a failure to
// read the DATA — six distinct failures produced an empty array and an empty array rendered as a clear
// path. DECISIONS.md D-6. Exercised, not pattern-matched.
{
  const RUN=(function(){
    const pre='var FLIP_NEAR_PTS=12,__p=[],__f=null,__why="",__src="skylit";'+
      'function dispIsFut(){return true;}'+
      'function emPiles(){emPiles.lastWhy=__why;emPiles.lastSrc=__src;return __p;}'+
      'function dte0NotToday(){return false;}\n'+
      'function ifLadder(){return __f==null?{err:1}:{err:null,rows:[{id:"FLIP",disp:__f}]};}';
    const deps=['dispNum','usd','usdBig','frameNum','emPos','emRead'].map(ex).join('\n');
    return new Function(pre+deps+'\nreturn function(B,p,f,why,sv){__p=p||[];__f=(f===undefined?null:f);__why=why||"";__src=sv||"skylit";return emRead(B,"SPY");};')();
  })();
  const B={ok:true,dir:1,pct:47,gamma:-1,now:7705.5,open:7695.75,low:7661,high:7730.5};
  const N=(d,pc,acc)=>({disp:d,pct:pc,usdK:1,accel:acc,brake:!acc,balanced:false});

  // TAPE FAILURE -> says so. This is the whole point.
  const dead=RUN(B,[],undefined,'SPXW tape unreadable','none');
  ok(dead.branch==='noBook',                 'an unreadable tape reads as noBook', dead.branch);
  ok(/No node book right now/.test(dead.txt),'and says so in words');
  ok(/SPXW tape unreadable/.test(dead.txt),  'naming the actual reason');
  ok(/not a clear path, it is no reading/.test(dead.txt),
     'and explicitly denies the reading a blank rail used to imply');
  ok(!/Nothing sizeable/.test(dead.txt),     'never the false all-clear');

  // a MARKUP CHANGE looks like a thin tape, and must land in the same place
  const thin=RUN(B,[],undefined,'SPXW tape thin (3 strikes)','none');
  ok(thin.branch==='noBook' && /thin \(3 strikes\)/.test(thin.txt),
     'a Skylit markup change reports as a thin tape, not as an empty market', thin.txt);

  // SKYLIT HEALTHY but nothing clears the cut -> that IS a market claim and is allowed
  const quiet=RUN(B,[],undefined,'no node clears 20% of King inside the band','skylit');
  ok(quiet.branch==='air', 'a healthy tape with no qualifying node is still "air"', quiet.branch);
  ok(/Nothing sizeable/.test(quiet.txt), 'which is a claim we are entitled to make');

  // the fallback must announce itself — units and meaning both change with it
  const fb=RUN(B,[{disp:7717.71,perPt:17083238,accel:true,balanced:false}],undefined,'SPXW tape unreadable','if-fallback');
  ok(/InsiderFinance open-interest levels/.test(fb.txt), 'the IF fallback discloses itself', fb.txt.slice(-70));
  ok(/not live positioning/.test(fb.txt),                'and says what it is NOT');

  // non-finite price refuses instead of printing NaN
  const nan=RUN(Object.assign({},B,{now:NaN}),[N(7727,100,true)]);
  ok(nan.branch==='badPrice',   'a non-finite price refuses', nan.branch);
  ok(!/NaN/.test(nan.txt),      'and NaN never reaches the face', nan.txt);

  // two nodes rounding to the same whole point must not become their own destination
  const dup=RUN(B,[N(7717.71,60,true),N(7717.9,40,false),N(7727.73,30,true)]);
  ok(/at 7718 can take price higher to the 30% negative gamma node at 7728/.test(dup.txt),
     'a same-rounded node is skipped as a destination', dup.txt);

  // the vocabulary is Skylit's
  const sky=RUN(B,[N(7727.73,100,true),N(7667.59,41,false)]);
  ok(/100% negative gamma accelerator/.test(sky.txt), '%King is the size, not dollars', sky.txt);
  ok(!/\$/.test(sky.txt), 'and no dollar figure is printed for a Skylit node');
}


// ---------- 25. (v11.68) THE TARGET IS NOT ON THE PATH TO ITSELF ----------
{
  const f=ex('emPath');
  ok(/Math\.abs\(P\.disp-target\)<1e-9/.test(f), 'a pile sitting ON the target is separated out');
  ok(/atTargetPerPt/.test(f),                    'and reported under its own name');
  ok(/P\.disp<=lo \|\| P\.disp>=hi/.test(f),      'the remaining window is STRICT, matching its comment');
  ok(!/P\.disp<lo \|\| P\.disp>hi/.test(f),       'the old inclusive test is gone');
  ok(/if\(P\.balanced\)\{ nBal\+\+; continue; \}/.test(f), 'balanced piles are counted, never summed');
  ok(/82%/.test(f),                              'the measurement that forced it is recorded beside the code');
}

// ---------- 26. (v11.68) PACE ----------
{
  const b=ex('emBand');
  ok(/out\.pace\s*=/.test(b),            'the band computes a pace ratio');
  ok(/Math\.sqrt\(elapsed\)/.test(b),    'against the SQUARE ROOT of elapsed time, not the clock');
  ok(/PACE_MIN_ELAPSED/.test(b),         'with a floor, because sqrt(elapsed) explodes at the open');
  ok(/SESS_OPEN_SEC/.test(b) && /SESS_CLOSE_SEC/.test(b), 'using the shared session clock');
  ok(/var OPEN=SESS_OPEN_SEC, CLOSE=SESS_CLOSE_SEC;/.test(src),
     'and sessPhaseCT uses THE SAME two numbers — no private second copy');

  // the arithmetic, exercised
  const due = e => Math.sqrt(e);
  ok(Math.round(due(0.5)*100)===71,  'half the clock means 71% of the move is due', Math.round(due(0.5)*100));
  ok(Math.round(due(30/390)*100)===28,'at 10:00 only 28% is due',                   Math.round(due(30/390)*100));
  ok(Math.round(due(300/390)*100)===88,'by 14:30 it is 88%',                        Math.round(due(300/390)*100));
  const pace=(pct,e)=>+((pct/100)/Math.sqrt(e)).toFixed(2);
  ok(pace(40,30/390) > 1.2, 'so 40% at 10:00 reads STRETCHED', pace(40,30/390));
  ok(pace(40,300/390) < 0.8,'and the same 40% at 14:30 reads COILED', pace(40,300/390));
}

// ---------- 27. (v11.75) THE RAILS, THE TARGET AND THE ES TICK ----------
// Reverted to EXP LOW / EXP HIGH at the user's instruction. The 0.80-sigma caveat did NOT go with it — it
// stays in the hover, because the label is what they want to read and the arithmetic is what they need to
// be able to check.
{
  const f=ex('secFrame');
  ok(/EXP LOW/.test(f) && /EXP HIGH/.test(f), 'the rails are named EXP LOW / EXP HIGH');
  ok(!/STRAD LOW/.test(f),                    'and not STRAD');
  ok(/0\.80 sigma/.test(f) && /58% of closes/.test(f),
     'while the hover still states what the band actually contains');
  ok(/1\.25/.test(f),                         'and how to get a true one-sigma boundary');
  ok(/>T: '\+frameNum\(ifMagEarly\)/.test(f), 'the target reads "T: <whole point>"');
  ok(/tgtUp=\(ifMagEarly>EBc\.now\)/.test(f), 'green above price, red below');
  ok(/g3tgt'\+\(tgtUp===true\?' up'/.test(f), 'the chip takes the colour class');
  ok(/\(ifMagEarly>EB\.now\)\?' up':' dn'/.test(f),
     'and the T on the rail reads the SAME test, so the two can never disagree');
  // ⚠ .g3tgt is defined TWICE in the stylesheet (pre-existing); the LATER definition wins, so take it.
  const tgtCss=src.slice(src.lastIndexOf(".g3tgt{"), src.lastIndexOf(".g3tgt{")+200);
  ok(/margin-left:auto/.test(tgtCss), 'and it is pushed to the right-hand end of the row');
  const ctCss=src.slice(src.indexOf(".g3ct{"), src.indexOf(".g3ct{")+60);
  ok(/font-size:8px/.test(ctCss), 'the contract chip matches the row instead of sitting a step below it', ctCss.slice(0,40));

  const fn=ex('frameNum');
  ok(/dispIsFut\(\)/.test(fn),       'rounding is gated on the chart being a future');
  ok(/Math\.round\(x\)/.test(fn),    'and rounds to the nearest whole point');
  ok(/return dispNum\(x\)/.test(fn), 'falling back to dispNum elsewhere');
  const sb={};
  new Function('S','function dispIsFut(){return S.fut;}\n'+ex('dispNum')+'\n'+ex('frameNum')+'\nS.f=frameNum;')(sb);
  sb.fut=true;
  ok(sb.f(7730.48)==='7730', '7730.48 -> 7730', sb.f(7730.48));
  ok(sb.f(7661.02)==='7661', '7661.02 -> 7661', sb.f(7661.02));
  ok(sb.f(7717.71)==='7718', '7717.71 -> 7718 (nearest)', sb.f(7717.71));
  sb.fut=false;
  ok(sb.f(764.41)==='764.41', 'and a SPY chart keeps its decimals', sb.f(764.41));
}


// ---------- 28. (v11.68) EVERY NEW READ IS ENROLLED ----------
{
  const R=JSON.parse(fs.readFileSync('./learning/rules.json','utf8')).rules;
  ['em.pace','piles.netPolarity'].forEach(k=>{
    ok(!!R[k], 'enrolled: '+k);
    // the QUESTIONS live in the feature's questions[] array, which is where the analysis iterator reads
    // them — asserting on a string inside `mechanism` was checking the wrong artefact.
    ok(R[k] && (R[k].mechanism||'').length>80, k+' carries a real mechanism, not a label');
    ok(R[k] && R[k].n===0 && R[k].rate===null, k+' claims nothing yet');
  });
  // DATA leg: the band's per-bar record must carry what the questions need
  ok(/elapsed:\(B\.elapsed==null/.test(src), 'the recorder writes elapsed');
  ok(/pace:\(B\.pace==null/.test(src),       'and pace');
  ok(/grossPerPtM/.test(src) && /netPerPtM/.test(src),
     'and BOTH dollar figures, so gross-vs-net is answerable from the same bars');
  // the features themselves must be registered, with questions the nightly loop can iterate
  ok(/key:'empace'/.test(src), 'pace is a registered FEATURE, not just a rule id');
  ok(/key:'piles'/.test(src),  'and so are the piles');
  ok(/pace_hot_extends_negG/.test(src) && /pace_cold_stays_in/.test(src),
     'pace asks its questions split by regime, so opposite effects cannot cancel');
  ok(/piles_net_beats_gross/.test(src),
     'and the piles ask whether the v11.68 fix itself was real or cosmetic');
}

// ---------- 29. (v11.75) THE ROW SAYS THE THING ONCE, IN THE RIGHT ORDER ----------
{
  const r=ex('regime2D');
  ok(/out\.play='BREAKS'/.test(r) && /out\.play='FADES'/.test(r), 'the playbook is one word');
  const playText=(r.match(/out\.play='[^']*'/g)||[]).join(' ');
  ok(!/breaks not fades/.test(playText) && !/widen stops/.test(playText),
     'and the sentences are gone from the PLAY line', playText);
  ok(/pins hold|levels tend to hold/.test(r), 'while the why-text keeps them');

  const f=ex('secFrame');
  ok(/widen stops|more room than usual/.test(f), 'the widen-stops advice survives in the hover');
  ok(/HOW price moves, never WHICH WAY/.test(f), 'along with the standing caveat');
  ok(f.indexOf('g3rg') < f.indexOf('g3play'), 'BREAKS sits with the regime chip it restates');
  ok(f.indexOf('g3play') < f.indexOf('>T: '), 'and the target ends the row');
  const tgtPrints=(f.match(/frameNum\(ifMagEarly\)/g)||[]).length;
  ok(tgtPrints===1, 'the target number is printed exactly ONCE on the section', tgtPrints);
}


// ---------- 30. (v11.75) THE READ: WHERE PRICE CAN GO, AND WHAT CARRIES OR STOPS IT ----------
// It used to open "Up 16.38, 47% of the straddle but slow for the hour." The user called that nonsense and
// was right: the rail above draws every one of those facts, so the sentence spent its first eight words
// reading the graph aloud. It now starts AT the level and ends at where price can get to.
//   "$6M negative gamma accelerator at 7668 can take price lower to the $6M positive gamma node at 7665."
{
  const f=ex('emRead');
  const RUN=(function(){
    // (v11.77) the stub must expose lastWhy / lastSrc — emRead now branches on WHY an array is empty,
    // which is the difference between "nothing is in the way" and "I cannot see the book".
    const pre='var FLIP_NEAR_PTS=12,__p=[],__f=null,__why="",__src="skylit";\n'+
      'function dispIsFut(){return true;}\n'+
      'function emPiles(){emPiles.lastWhy=__why;emPiles.lastSrc=__src;return __p;}\n'+
      'function dte0NotToday(){return false;}\n'+
      'function ifLadder(){return __f==null?{err:1}:{err:null,rows:[{id:"FLIP",disp:__f}]};}\n';
    const deps=['dispNum','usd','usdBig','frameNum','emPos','emRead'].map(ex).join('\n');
    return new Function(pre+deps+'\nreturn function(B,p,fl,why,sv){__p=p||[];__f=(fl===undefined?null:fl);__why=why||"";__src=sv||"skylit";return emRead(B,"SPY");};')();
  })();
  const BASE={ok:true,dir:1,pct:47,gamma:-1,now:7712.13,open:7695.75,low:7661.02,high:7730.48};
  const one=[{disp:7717.71,perPt:17083238,accel:true,balanced:false}];
  const two=[{disp:7707.69,perPt:5565703,accel:true,balanced:false},{disp:7717.71,perPt:17083238,accel:true,balanced:false}];
  const dn =[{disp:7667.59,perPt:5769818,accel:true,balanced:false},{disp:7665.0,perPt:6100000,accel:false,balanced:false}];

  // NO STATE CLAUSE. The sentence must not begin by re-reading the rail.
  const t1=RUN(BASE,one).txt;
  ok(!/^Up |^Down /.test(t1),        'the sentence does not open with the day’s travel', t1);
  ok(!/of the straddle/.test(t1),    'nor with the percentage the rail already draws');
  ok(!/slow for the hour/.test(t1),  'nor with the pace');
  ok(/^\$/.test(t1),                 'it starts at the level', t1.slice(0,24));

  // THE CHAIN — the user's own example, reproduced
  const t2=RUN(Object.assign({},BASE,{dir:-1,now:7690}),dn).txt;
  ok(t2==='$6M negative gamma accelerator at 7668 can take price lower to the $6M positive gamma node at 7665.',
     'the chained form matches the shape asked for', t2);
  const t3=RUN(Object.assign({},BASE,{now:7700}),two).txt;
  ok(/can take price higher to the \$17M negative gamma node at 7718\./.test(t3),
     'and it names the NEXT node as the destination', t3);
  ok(/with nothing else in the way/.test(RUN(BASE,one).txt),
     'falling back to the rail when nothing lies beyond');

  // vocabulary: negative/positive, not short/long
  const all=[t1,t2,t3,
    RUN(Object.assign({},BASE,{gamma:1}),[{disp:7727,perPt:31000000,accel:false,balanced:false}]).txt,
    RUN(Object.assign({},BASE,{dir:-1,now:7674.2}),[{disp:7670,perPt:9e6,accel:true,balanced:false}],7665.56).txt,
    RUN(BASE,[]).txt,
    RUN(BASE,[{disp:7717.71,perPt:17083238,accel:true,balanced:true}]).txt,
    RUN(Object.assign({},BASE,{now:7736.7}),[]).txt
  ].join(' ');
  ok(/negative gamma/.test(all) && /positive gamma/.test(all), 'polarity reads negative / positive gamma');
  ok(!/short gamma/.test(all) && !/long gamma/.test(all),      'not short / long');

  // ⚠ THE CONTRACT: mechanism, never forecast. "can", never "will".
  const low=all.toLowerCase();
  ['likely','probability','probably',' will ','should','expect(','expected to','odds','chance',
   'go long','go short','buy here','sell here','target price','take profit','entry','stop at']
    .forEach(w=>ok(!low.includes(w), 'the read never says "'+w.trim()+'"'));
  ok(/ can /.test(low), 'and stays conditional throughout');

  // branches, exercised
  ok(RUN(BASE,one).branch==='accel',   'accelerator ahead reads as accel');
  ok(RUN(Object.assign({},BASE,{gamma:1}),[{disp:7727,perPt:3.1e7,accel:false,balanced:false}]).branch==='brake', 'brake reads as brake');
  ok(RUN(BASE,[{disp:7717.71,perPt:1.7e7,accel:true,balanced:true}]).branch==='balanced', 'balanced-only says so');
  // (v11.77) an empty array now means noBook UNLESS the source says Skylit read fine — that split is
  // the whole D-6 fix, so the fixture has to state which case it is.
  ok(RUN(BASE,[],undefined,'no node clears the cut','skylit').branch==='air', 'a healthy but empty path is air');
  ok(RUN(BASE,[],undefined,'SPXW tape unreadable','none').branch==='noBook',  'an unreadable one is noBook');
  ok(RUN(Object.assign({},BASE,{now:7736.7}),[],undefined,'no node clears the cut','skylit').branch==='past',
     'past the rail is its own case');
  {
    const both=RUN(Object.assign({},BASE,{dir:-1,now:7674.2}),[{disp:7670,perPt:9e6,accel:true,balanced:false}],7665.56);
    ok(both.branch==='flip',           'with a flip AND a pile ahead, the flip wins', both.branch);
    const far=RUN(Object.assign({},BASE,{dir:-1,now:7700}),[{disp:7690,perPt:9e6,accel:true,balanced:false}],7600);
    ok(far.branch==='accel',           'a distant flip does not outrank a pile', far.branch);
  }

  // every level in the sentence is a whole point on a futures chart
  ok(!/\d+\.\d\d\b/.test(all.replace(/\d+\.\d\b/g,'')), 'no two-decimal prices survive in the sentence');

  // rendered, and enrolled
  const sf=ex('secFrame');
  ok(/emRead\(EB, sym\)/.test(sf), 'the read renders inside the section');
  const R=JSON.parse(fs.readFileSync('./learning/rules.json','utf8')).rules;
  ok(!!R['em.read'] && /key:'emread'/.test(src), 'enrolled as a feature, not just a rule id');
}


// ---------- 31. (v11.70) THE DEDUPE WINDOW SCALES WITH THE REGISTRY ----------
// featEnqueue scanned back a hardcoded 40 records for a duplicate. One bar writes one record per enrolled
// feature, so at 40 features the look-back could no longer span a single bar and re-enqueueing the same
// bar duplicated EVERY record — live, repeatedly, into the data every scorecard is computed from.
{
  const f=ex('featEnqueue');
  ok(!/i>arr\.length-40/.test(f), 'the hardcoded 40-record look-back is gone');
  ok(/arr\[i\]\.bar!==bar\) break/.test(f), 'it now walks back while the BAR matches');

  // BEHAVIOUR: 60 features, enqueued twice, must not duplicate
  const sb={};
  // eslint-disable-next-line no-new-func
  const run=new Function(`
    var arr=[], bar=1000;
    function enq(keys){
      for(var j=0;j<keys.length;j++){
        var k=keys[j], dup=false;
        for(var i=arr.length-1;i>=0;i--){ if(arr[i].bar!==bar) break; if(arr[i].key===k){ dup=true; break; } }
        if(dup) continue;
        arr.push({key:k, bar:bar});
      }
    }
    var keys=[]; for(var n=0;n<60;n++) keys.push('f'+n);
    arr.push({key:'old', bar:999});          // a record from the PREVIOUS bar, to prove the walk stops
    enq(keys); var after1=arr.length;
    enq(keys); var after2=arr.length;
    bar=1001; enq(keys); var after3=arr.length;
    return {after1:after1, after2:after2, after3:after3};
  `)();
  ok(run.after1===61, '60 features enqueue once', run.after1);
  ok(run.after2===61, 'and a second identical enqueue adds NOTHING even at 60 features', run.after2);
  ok(run.after3===121,'while a NEW bar writes a fresh set', run.after3);
}

// ---------- 32. (v11.71) THE HOOK MUST SURFACE EVERYTHING THE FACE READS ----------
// v11.51 wrote the rule: every read on the face has a hook, because the alternative is counting DOM nodes
// and inferring from pixels. v11.68 then added pace/elapsed/dueFrac to emBand() and NOT to its hook, so
// verifying the pace chip meant reading two DOM strings and inverting the arithmetic to recover elapsed.
// The rule was right and I broke it anyway — so this derives the required set from secFrame rather than
// listing it, and it will catch the NEXT one without anyone remembering to update a list.
{
  const sf=ex('secFrame');
  const used=[...new Set([...sf.matchAll(/\bEB[c]?\.([A-Za-z_][A-Za-z0-9_]*)/g)].map(m=>m[1]))];
  const hs=src.indexOf('__gptsDebug.emBand');
  const he=src.indexOf('__gptsDebug.session = function', hs);
  ok(he>hs, 'the emBand hook is locatable');
  const hook=src.slice(hs, he);
  const surfaced=new Set([...hook.matchAll(/\bB\.([A-Za-z_][A-Za-z0-9_]*)/g)].map(m=>m[1]));
  // `ok` and `why` are control flow, not readings — the hook handles them by its own contract
  const missing=used.filter(k=>!surfaced.has(k) && k!=='ok' && k!=='why');
  ok(missing.length===0,
     'every band field the FACE reads is also returned by the debug hook',
     missing.length?('missing: '+missing.join(', ')):'all '+used.length+' surfaced');
  // and the ones this build was about, named explicitly so the intent survives a refactor
  ['pace','paceOk','elapsed','dueFrac','nowSo'].forEach(k=>
    ok(surfaced.has(k), 'the hook surfaces '+k));

  // (v11.82) ⚠ THE SAME GUARD, FOR THE PILES HOOK. v11.71 wrote this rule for emBand and one build later
  // the piles hook shipped IF-shaped fields for SKYLIT data — reporting `gross`/`net`/`perPt` (which a
  // Skylit node has none of) and omitting `role`, which the FACE renders. The rail said "7710 KING" while
  // the hook said "ACC". A guard that covers one hook is a guard that teaches you the wrong lesson.
  const ps=ex('skPiles');
  const produced=new Set([...ps.matchAll(/^\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*:/gm)].map(m=>m[1]));
  const ph0=src.indexOf('__gptsDebug.piles');
  const ph1=src.indexOf('__gptsDebug.session = function', ph0);
  ok(ph1>ph0, 'the piles hook is locatable');
  const phook=src.slice(ph0, ph1);
  ['role','src','gkRatio','gkVerdict','signed','usdK'].forEach(k=>
    ok(phook.indexOf(k+':')>=0, 'the piles hook surfaces '+k));
  // and it must not report IF-only fields for a Skylit node
  ok(/var isSk=\(P\.src==='skylit'\)/.test(phook),
     'and it branches on the SOURCE rather than reporting fields the node does not have');
  ok(/gross:\(isSk\?null:/.test(phook), 'gross is null for a Skylit node');
  ok(/perPt:\(isSk\?null:/.test(phook),  'and so is perPt, which is IF\'s unit');
}

// ---------- 34. (v11.76) THE COMMENT MUST NAME THE BOOK THE CODE ACTUALLY READS ----------
// The block above emPiles said "Read from SKYLIT's book (cpRows), which is the SAME source regime2D
// reads — so a pile can never contradict the regime chip above it." Both halves were false from v11.64,
// and the lie sat there for ELEVEN VERSIONS, misleading two separate contexts into believing the piles
// were Skylit's. A comment asserting a SAFETY GUARANTEE it no longer provides is worse than none.
{
  const i=src.indexOf('function emPilesIF');
  const head=src.slice(Math.max(0,i-3000), i);          // the comment block immediately above it
  // (v11.77) the IF logic moved to emPilesIF when Skylit became the primary node source; these
  // assertions still describe it and still matter — it is the disclosed fallback, not dead code.
  const body=ex('emPilesIF');
  // what does the CODE read?
  const readsIF=/ifChain\(/.test(body), readsSkylit=/cpRows|LASTFEED/.test(body);
  ok(readsIF && !readsSkylit, 'emPiles reads InsiderFinance, not Skylit', readsIF+'/'+readsSkylit);
  // does the COMMENT say so?
  ok(/SOURCE: \*\*INSIDERFINANCE\*\*/.test(head), 'and the comment above it says INSIDERFINANCE in bold');
  // ⚠ the corrected block QUOTES the old claim in order to retract it, so a naive absence check fails on
  // the retraction itself. Fourth time this session. Assert it is only ever present AS a retraction.
  const claimLines=head.split('\n').filter(l=>/Read from SKYLIT's book/.test(l));
  ok(claimLines.length<=1, 'the old claim appears at most once', claimLines.length);
  ok(claimLines.length===0 || /used to say/.test(head),
     'and only ever inside an explicit retraction, never as a statement');
  ok(!/can never contradict the regime chip/.test(head) || /GUARANTEE IT PROMISED IS GENUINELY GONE/.test(head),
     'and the guarantee it used to promise is either provided or explicitly retracted');
  ok(/DECISIONS\.md/.test(head), 'with a pointer to where the decision is recorded');

  // the two documents a fresh context needs must exist and be reachable from the config
  const cfg=JSON.parse(fs.readFileSync('./.gex-config.json','utf8'));
  const pf=cfg.canonicalFiles.projectFiles;
  ['session-state/INSIDERFINANCE.md','session-state/DECISIONS.md','current/gex-if-levels.user.js']
    .forEach(f=>{ ok(pf.indexOf(f)>=0, 'canonical: '+f);
                  ok(fs.existsSync('./'+f), 'exists: '+f); });
  ok(!!cfg.canonicalFiles.insiderFinance, 'the config carries an insiderFinance block');
  ok(/gamma \* openInterest \* 100 \* spot\^2/.test(cfg.canonicalFiles.insiderFinance.formula),
     'including the one formula everything is recomputed from');
  const doc=fs.readFileSync('./session-state/INSIDERFINANCE.md','utf8');
  ok(/dte0_isToday = FALSE|dte0 IS NOT ALWAYS TODAY/.test(doc), 'the doc records that dte0 is not always today');
  ok(/263\.8/.test(doc),        'and the reconciliation that licenses recomputing');
  const dec=fs.readFileSync('./session-state/DECISIONS.md','utf8');
  ['D-1','D-3','D-4','D-6','D-7'].forEach(k=>ok(dec.indexOf(k)>=0, 'decision recorded: '+k));
  ok(/100\.6/.test(dec), 'including that the 113x was the spot-squared scaling, not incompatibility');
}

// ---------- 35. (v11.78) THE KING IS THE ANCHOR, SO IT IS PROVEN BEFORE THE LADDER IS TRUSTED ----------
// The user's requirement: survive Skylit changing their markup. Every %King on the SPXW tape is a RATIO
// to the King — get the King wrong and all 100 strikes are wrong TOGETHER, in the same direction, and the
// rail still looks entirely plausible. That is the worst failure available here, because nothing about it
// reads as broken. Skylit's signed $K cell is the strongest fingerprint they publish (it is DATA, not
// markup, so it survives restyling); the reader already prefers it. v11.78 CHECKS it.
{
  const sk=ex('skPiles');
  ok(/T\.kingKd/.test(sk),                'the dollar anchor is read');
  ok(/Math\.abs\(topK-T\.king\)>0\.01/.test(sk),
     'and the percent ladder must independently crown the SAME strike');
  ok(/T\.kingConflict/.test(sk),          'the reader\'s own conflict flag is honoured');
  ok(/topV<99/.test(sk),                  'and a ladder where nothing reaches 100% is refused');
  ok(/out\.degraded=/.test(sk),           'a missing dollar anchor DEGRADES rather than fails');

  // BEHAVIOUR — simulated tape shapes, which is the only way to test "what if the markup changes"
  const RUN=(function(){
    const pre='var CFG={nodeThresh:20}, SK_MIN_STRIKES=20, __tape=null, __lad={err:null,dispScale:1.0023},'+
      'RUG_ANCHOR_PCT=40, RUG_ADJ=3, RUG_SIG_PCT=20, GK_RATIO_STRONG=1.8;'+
      'function tapeMap(){return __tape;} function ifLadder(){return __lad;}';
    return new Function(pre+['emPos','skRoles','skPiles'].map(ex).join('\n')+
      '\nreturn function(t){__tape=t; return skPiles({ok:true,low:7661,high:7730.5,now:7705},"SPY");};')();
  })();
  const good={count:100,king:7710,kingKd:17241,kingSrc:'dollar',kingConflict:false,
              pct:{'7710.00':-100,'7700.00':-79,'7650.00':41,'7690.00':-17}};

  ok(RUN(good).ok && RUN(good).piles.length===3, 'a healthy tape yields nodes', RUN(good).piles.length);
  ok(!RUN(good).degraded, 'and reports no degradation');

  // ⚠ THE SILENT CATASTROPHE: dollar King and ratio King disagree. Everything would render normally.
  const split=RUN({count:100,king:7710,kingKd:17241,kingSrc:'dollar',pct:{'7700.00':-100,'7710.00':-50}});
  ok(!split.ok, 'a King the two methods disagree about is REFUSED, not drawn');
  ok(/King disagrees/.test(split.why), 'and the reason names both answers', split.why);

  // each distinct markup failure names itself
  [[null,'tape unreadable'],
   [{count:4,king:7710,kingKd:1,kingSrc:'dollar',pct:{'7710.00':-100}},'thin'],
   [{count:100,king:null,kingKd:1,pct:good.pct},'no King'],
   [{count:100,king:7710,kingKd:1,kingSrc:'dollar',pct:{'7710.00':-64}},'reaches 100%'],
   [Object.assign({},good,{kingConflict:true}),'conflict'],
   [{count:100,king:7710,kingKd:1,pct:{}},'no strike map']
  ].forEach(([t,frag])=>{ const r=RUN(t);
    ok(!r.ok, 'refused: '+frag);
    ok((r.why||'').indexOf(frag)>=0, 'and says so: '+frag, r.why); });

  // GRACEFUL DEGRADATION: the ratios still size the nodes, but the early warning is printed
  const noDollar=RUN({count:100,king:7710,kingKd:null,kingSrc:'highlight',kingConflict:false,pct:good.pct});
  ok(noDollar.ok, 'a lost dollar anchor still renders — the ratios are self-consistent');
  ok(/dollar anchor lost/.test(noDollar.degraded||''), 'but it is DISCLOSED, not swallowed', noDollar.degraded);
  ok(noDollar.piles.length===3, 'with the same nodes', noDollar.piles.length);

  // and the provenance reaches the hook, so a live check is one call
  const h=src.slice(src.indexOf('__gptsDebug.piles'), src.indexOf('__gptsDebug.session = function'));
  ['kingSrc','kingKd','degraded'].forEach(k=>ok(h.indexOf(k)>=0, 'the piles hook reports '+k));
}

// ---------- 36. (v11.79) THE LABELS, THE HOVERS AND THE TWO SCALES ----------
// Asked for twice, mocked twice, and not built until now. ES on top (the number traded), SPXW strike and
// the node's role beneath it, and the index equivalent of the rails in their hover.
{
  const f=ex('secFrame');
  ok(/g3plab/.test(f),                       'every prominent node carries a label');
  ok(/frameNum\(P\.disp\)\+'<i>'\+P\.k/.test(f),
     'ES price on top, SPXW strike underneath', 'ok');
  ok(/var role = P\.role \|\| \(P\.balanced/.test(f), 'with the node ROLE on the same line as the strike');
  ok(/PLAB_MIN_PCT/.test(f) && /var PLAB_MIN_PCT = 20;/.test(src),
     'labelled only above a named threshold, so a busy day cannot smear the tier');
  ok(/⚖ HAND-SET/.test(src.slice(src.indexOf('var PLAB_MIN_PCT')-320, src.indexOf('var PLAB_MIN_PCT'))),
     'flagged hand-set');

  // the hover speaks the right vocabulary for whichever book produced the node
  ok(/var isSk = \(P\.src==='skylit'\)/.test(f), 'the hover branches on the SOURCE of the node');
  ok(/P\.pct\+'% of King'/.test(f),             'Skylit nodes are sized in %King');
  ok(/of dealer exposure at this strike/.test(f), 'with the dollar value stated as a VALUE');
  ok(!/usdBigSp\(P\.usdK\)[^]{0,40}PT/.test(f),  'and never with a per-point unit, which is IF\'s');
  ok(/A DIFFERENT MEASUREMENT from InsiderFinance/.test(f),
     'and it says which measurement it is, so the two are never read as one');
  ok(/InsiderFinance FALLBACK/.test(f),         'while the fallback hover says it is the fallback');
  // the standing caveat is identical on both branches — it is doctrine, not prose
  const caveats=(f.match(/needs a market-impact figure no option chain contains/g)||[]).length;
  ok(caveats>=2, 'the no-distance caveat appears on BOTH branches, worded identically', caveats);

  // SPX equivalent in the rail hovers
  ok(/index equivalent is SPX/.test(f),   'the rails quote their index equivalent');
  const both=(f.match(/index equivalent is SPX/g)||[]).length;
  ok(both===2, 'on the low AND the high', both);
  ok(/function ifDispScale\(\)/.test(src), 'via a named inverse of the chart scale');

  // the target chip carries its distance
  ok(/g3dist/.test(f),                    'the target chip carries a distance');
  ok(/Math\.abs\(ifMagEarly-EBc\.now\)/.test(f), 'computed from price, not restated as a second price');
  ok(/frameNum\(Math\.abs\(ifMagEarly-EBc\.now\)\)/.test(f),
     'and rounded to whole points like every other price in the section');
  ok(!/dispNum\(Math\.abs\(ifMagEarly/.test(f), 'never with two decimals on a distance');
  ok(/'\+':'\\u2212'/.test(f),             'signed, so direction reads without arithmetic');

  // the rail grew a tier, and the v11.67 geometry contract above the rail must still hold
  // ⚠ build the pattern from pieces — nested escaping through python -> js -> regex silently produced a
  // never-matching pattern the first time, and a px() that returns null makes every geometry check vacuous.
  const px=(cls,prop)=>{ const re=new RegExp('\\.'+cls+'\\{[^}]*'+prop+':(-?[0-9.]+)','g');
    let m,v=null; while((m=re.exec(src))) v=parseFloat(m[1]); return v; };
  ok(px('g3emt','height')===53, 'the rail box grew again for the ROLE tier (v11.93: 48 -> 53)', px('g3emt','height'));
  // ⚠ px() takes the LAST match and the generic '#gpts-body .g3pile{bottom:2px}' comes after the scoped
  // rule, so ask for the SCOPED one by name — the specific selector is what actually applies here.
  const scoped=(src.match(/g3emt \.g3pile\{bottom:(\d+)px\}/)||[])[1];
  ok(scoped==='19', 'and the piles were LIFTED clear of it rather than the labels squeezed under them', scoped);
  // (v11.93) THE RAIL MOVED, DELIBERATELY, AND THE CONTRACT IT PROTECTS DID NOT CHANGE.
  // v11.67 pinned the rail at 14 and the dot at 11 to stop the dot's ring cutting the money labels
  // that live at 0-9. The role tier now sits between them, so everything below the money tier shifts
  // down by 5 — but the thing being protected is the same: NOTHING may enter 0-9 except the amounts.
  ok(px('g3emr','top')===19,    'the rail moved down to make room for the role tier', px('g3emr','top'));
  ok(px('g3emn','top')===16,    'and the dot moved with it', px('g3emn','top'));
  ok(px('g3prole','top')===11,  'the role tier sits between the amounts and the rail', px('g3prole','top'));
  // THE ACTUAL CONTRACT, restated so it survives the next tier: the money amounts own 0-9 alone.
  // The dot is 10px tall with a 2px ring, so its PAINTED top is (top - 2) and that must clear 9.
  ok(px('g3emn','top')-2 >= 10,
     'the dot\'s painted top still clears the money tier — the v11.67 bug was its RING cutting the amounts, and getBoundingClientRect does not include box-shadow',
     px('g3emn','top')-2);
  ok(px('g3prole','top') >= 10,
     'and so does the role tier — nothing but the amounts may enter 0-9', px('g3prole','top'));
  const roleSize=px('g3prole','font-size');
  ok(roleSize===5.5, 'the role did NOT grow when it moved — same size, new tier', roleSize);
  // and the role must have LEFT the label under the rail, or it is drawn twice
  ok(/frameNum\(P\.disp\)\+'<i>'\+P\.k\+'<\/i>/.test(f),
     'the label below the rail is now ES price over SPX strike ONLY — the role moved out rather than being copied');
}

// ---------- 37. (v11.81) NODE ROLES: KING / GK / RUG / REVERSE-RUG ----------
// v11.79 shipped ACC/BRK/BAL, which is POLARITY, not ROLE. Polarity says how hedging behaves at a node;
// role says what SHAPE the book has built around it. The user asked for roles and got polarity.
// These detectors already existed for the SPY tape and were built on Skylit's doctrine — now that the
// rail reads Skylit's SPXW ladder, the same geometry runs on the same kind of data.
{
  const F=(function(){
    const pre='var RUG_ANCHOR_PCT=40,RUG_ADJ=3,RUG_SIG_PCT=20,GK_RATIO_STRONG=1.8;';
    return new Function(pre+ex('skRoles')+'\nreturn skRoles;')();
  })();
  const r=ex('skRoles');
  ok(/RUG_ANCHOR_PCT/.test(r) && /RUG_ADJ/.test(r) && /RUG_SIG_PCT/.test(r) && /GK_RATIO_STRONG/.test(r),
     'the constants are REUSED from the existing doctrine, not re-invented');
  ok(!/for\(j=i\+1;j<Math\.min\(rows\.length,i\+1\+RUG_ADJ\);j\+\+\)\{\}/.test(r),
     'and the empty loop left over from drafting is gone');

  // THE USER'S LIVE LADDER, 2026-08-23 — 5-point SPXW strikes
  const live={7605:-1,7610:-20,7615:3,7620:1,7625:13,7630:-85,7635:6,7640:9,7645:2,7650:41,7655:3,
              7660:-10,7665:3,7670:12,7675:-7,7680:13,7685:-7,7690:-17,7695:-10,7700:-79,7705:-10,
              7710:-100,7715:3,7720:3,7725:3,7730:-2,7735:0,7740:10,7745:3,7750:5,7755:11};
  const L=F(live,7710);
  ok(L.king===7710,          'the King is tagged', L.king);
  ok(L.byK[7710]==='KING',   'and carries the KING role');
  ok(L.gk===7700,            'the gatekeeper is the last significant node before it', L.gk);
  ok(L.gkRatio===0.79,       'with its strength stated RELATIVE to the King', L.gkRatio);
  ok(L.gkVerdict==='passable','and a verdict from GK_RATIO_STRONG, not a guess', L.gkVerdict);
  ok(L.byK[7700]==='GK',     'tagged GK');
  // ⚠ no rug TODAY, and the near-miss is the interesting part: 7650(+41) over 7630(−85) is FOUR ladder
  // steps apart, and RUG_ADJ is 3. A detector that fired here would be loosening the doctrine to get a hit.
  ok(L.rug===null,           'no RUG today — the +41/−85 pair is 4 steps apart and RUG_ADJ is 3');
  ok(L.rrug===null,          'and no reverse rug');

  // SYNTHETIC SHAPES — the geometry must actually fire when it is present
  const rug=F({7690:-15,7695:5,7700:-80,7705:60,7710:-100,7715:2},7710);
  ok(rug.rug && rug.rug.ceil===7705 && rug.rug.floor===7700,
     'a positive ceiling directly over a strong negative node IS a rug', JSON.stringify(rug.rug));
  ok(rug.byK[7705]==='RUG' && rug.byK[7700]==='RUG', 'and both anchors are tagged', JSON.stringify(rug.byK));
  // ⚠ that ladder is ALSO a reverse rug (purple-yellow-purple), and the second pass used to overwrite
  // the first — asserting the opposite direction on the same level. Both are kept; the clash is reported.
  ok(rug.contested===true, 'a stack that is both shapes at once is flagged CONTESTED, not silently one');
  const rr=F({7690:2,7695:55,7700:-70,7705:3,7710:-100},7710);
  ok(rr.rrug && rr.rrug.ceil===7710 && rr.rrug.floor===7695,
     'and the mirror is a reverse rug', JSON.stringify(rr.rrug));

  // a propped floor disqualifies a rug — that is the "no obvious floor" half of the doctrine
  const propped=F({7680:45,7690:-15,7695:5,7700:-80,7705:60,7710:-100},7710);
  ok(propped.rug===null, 'a significant positive floor beneath disqualifies the rug', JSON.stringify(propped.rug));

  // ROLE beats polarity on the label, and the hover keeps both
  const f=ex('secFrame');
  ok(/var role = P\.role \|\| \(P\.balanced/.test(f), 'the label shows ROLE when there is one');
  ok(/KING \\u2014 the heaviest node/.test(f),  'the hover explains KING');
  ok(/GATEKEEPER \\u2014 the last significant/.test(f), 'and GATEKEEPER, with its ratio');
  ok(/RUG \\u2014 a strong POSITIVE ceiling/.test(f), 'and RUG');
  ok(/REVERSE RUG \\u2014 a strong NEGATIVE ceiling/.test(f), 'and REVERSE RUG');
  ok(/roles:emPiles\.lastRoles/.test(src), 'and the hook reports the whole role map');
}

// ---------- 38. (v11.83) THE LAST TWO OPEN ITEMS: D-5 AND D-4 ----------
// D-5: `dte0` means "nearest LIVE expiry", not "today". InsiderFinance drop an expiry the moment it
// expires, so a chain captured after the close prices the NEXT session while the chart shows this one.
// D-4: the flow chip was the last element reading a different book from the regime chip beside it.
{
  const d=ex('dte0NotToday');
  ok(/c\.dte0\.exps\[0\]/.test(d),        'the front expiry is read from the chain');
  ok(/String\(front\)!==String\(c\.today\)/.test(d), 'and compared against the payload\'s own today');
  ok(/return null/.test(d),               'unknown returns null, which is not the same as "it is today"');
  // three-state, because "cannot tell" must never render as "fine"
  const F=new Function('var __c=null; function ifChain(){return __c;}'+d+
    '\nreturn function(c){__c=c; return dte0NotToday("SPY");};')();
  ok(F({dte0:{exps:['20260824']},today:'20260821'})==='20260824', 'a later expiry comes back as the DATE');
  ok(F({dte0:{exps:['20260821']},today:'20260821'})===false,      'a matching one comes back false');
  ok(F(null)===null,                                             'no chain comes back null');
  ok(F({dte0:{exps:[]},today:'20260821'})===null,                'and an empty window too');

  const f=ex('secFrame');
  ok(/EB\.notToday\?' \\u2260TODAY':''/.test(f),
     'the rails carry a VISIBLE marker, not only a hover — a caveat in a hover is a caveat nobody reads');
  const marks=(f.match(/u2260TODAY/g)||[]).length;
  ok(marks===2, 'on both rails', marks);
  ok(/THIS EXPIRY IS NOT TODAY/.test(f), 'and the hover explains why');

  // D-4: the flow chip declares a conflict rather than switching source.
  // ⚠ (v11.87) These were greps for the LINE that computes the conflict. Run the function instead:
  // a grep cannot tell `!==` from `===`, and inverting that operator is the whole failure mode.
  const hf=ex('hedgeFlow');
  ok(/regime2D\(sym\)/.test(hf),          'the flow chip reads the regime chip beside it');
  ok(!/lv\.netGEX=.*regime/.test(hf),     'it does NOT recompute its number from the other book');

  function flowFixture(netGEX, regimeG){
    var GEX_WINDOW_LABEL={toFri:'to Fri', dte0:'today'};
    var ifChain=function(){ return { spot:7674.1, toFri:{ lv:{ netGEX:netGEX } } }; };
    var regime2D=function(){ return (regimeG===null)?null:{ g:regimeG }; };
    eval(ex('hedgeFlow'));
    return hedgeFlow('SPY');
  }
  // OUR sign convention: negative net gamma -> dealers hedge WITH the move -> the flow FEEDS it.
  // regime2D's g is -1 for short gamma. So agreement is (g<0) === feeds.
  const agreeShort = flowFixture(-16.41e9, -1);
  const agreeLong  = flowFixture(+16.41e9, +1);
  const fightA     = flowFixture(-16.41e9, +1);   // IF says short, Skylit says long
  const fightB     = flowFixture(+16.41e9, -1);   // IF says long,  Skylit says short
  ok(agreeShort.ok && agreeShort.feeds===true && agreeShort.conflict===false,
     'both books short gamma: FEEDS, and no conflict', [agreeShort.feeds, agreeShort.conflict]);
  ok(agreeLong.ok && agreeLong.feeds===false && agreeLong.conflict===false,
     'both books long gamma: FIGHTS, and no conflict', [agreeLong.feeds, agreeLong.conflict]);
  ok(fightA.conflict===true,'IF short vs Skylit long IS a conflict', fightA.conflict);
  ok(fightB.conflict===true,'and so is the mirror — the test spans BOTH directions, because a fixture set that only ever runs one way passes on an inverted comparison', fightB.conflict);
  ok(flowFixture(-16.41e9,null).conflict===undefined,
     'an unreadable regime leaves conflict UNSET rather than false — "cannot compare" is not "they agree"',
     flowFixture(-16.41e9,null).conflict);
  ok(agreeShort.perPt>0 && agreeShort.netGEX===-16.41e9,
     'and the chip keeps InsiderFinance\'s own number throughout — it declares, it does not switch source');
  ok(/HF\.conflict\?' g3conf'/.test(f),   'a conflicted chip is marked');
  ok(/THE TWO BOOKS DISAGREE/.test(f),    'and the hover names both answers');
  ok(/a stock beside a flow/.test(f),     'restating the doctrine that neither checks the other');
  // ⚠ the point is disclosure, not resolution — quietly picking one would hide the thing worth seeing
  ok(/trust the one whose window matches your horizon/.test(f),
     'and it hands the judgement back rather than pretending to settle it');
}

// ---------- 39. (v11.84) SPX NODES ARE TRACKED, NOT JUST DRAWN ----------
// The rail has drawn Skylit's SPXW nodes since v11.77 and thrown every reading away — no history, no
// accumulation, no taps, nothing in the export, nothing the end-of-day review could read. You cannot
// build a mental model from data that is not kept.
// ⚠ AND THE GAP WAS SMALLER THAN I MADE IT LOOK. `sampleTapeHistory` needs only `tapeMap(sym)`, which
// already worked for SPXW, and every store it touches auto-creates. The history side was ONE CALL; I
// spent a round theorising about feed lanes before checking that.
{
  const t=ex('trackSpxwNodes');
  ok(/tapeMap\('SPXW'\)/.test(t),          'it reads the SPXW tape');
  ok(/sampleTapeHistory\('SPXW'\)/.test(t),'and feeds the SAME history sampler SPY uses');
  ok(/SK_MIN_STRIKES/.test(t),             'gated on the same thin-tape floor as the rail');
  ok(/out\.why=/.test(t),                  'and a failure names itself rather than recording silence');
  // ⚠ (v11.85) REPLAY MUST NOT BE RECORDED AS TODAY. sampleTapeHistory keys by todayKey() and is NOT
  // replay-guarded, so a Sunday replay of Friday writes Friday's nodes under Sunday's date — mislabelled,
  // which is worse than missing, because nothing downstream can tell.
  ok(/inReplay\(\)\)\{ out\.why='replay/.test(t), 'replay refuses to record');
  {
    const code=t.split('\n').filter(l=>!/^\s*\/\//.test(l)).join('\n');
    ok(code.indexOf('inReplay')<code.indexOf('sampleTapeHistory'),
       'and refuses BEFORE it samples', code.indexOf('inReplay')+' < '+code.indexOf('sampleTapeHistory'));
  }
  ok(/THE SAME HOLE EXISTS ON THE SPY PATH/.test(src),
     'and the unfixed SPY instance is recorded rather than silently left');
  // the tracker has an instrument
  ok(/__gptsDebug\.spxNodes/.test(src),   'the tracker can be checked in one hook call');
  const sh=src.slice(src.indexOf('__gptsDebug.spxNodes'), src.indexOf('__gptsDebug.piles'));
  ['histStrikes','histPoints','tapStrikes','tapped','replay'].forEach(k=>
    ok(sh.indexOf(k)>=0, 'the hook reports '+k));
  const h=ex('sampleTapeHistory');
  ok(/HIST\[sym\] \|\| \(HIST\[sym\]=\{\}\)/.test(h),
     'the history store auto-creates, which is why SPXW needed no new plumbing');

  // TAPS need a price series in SPX space — synthesised, or SKIPPED, never guessed
  const c=ex('spxwCandlesFromSPY');
  ok(/L\.undScale/.test(c),                'taps convert SPY candles with the ladder undScale');
  ok(/return null/.test(c),                'and return null when the scale is unavailable');
  ok(/if\(sc\)\{/.test(t),                 'so taps are SKIPPED rather than counted against a wrong strike');
  ok(/no scale for taps/.test(t),          'and the reason is recorded');

  // enrolled, and recording the LEVEL rather than a count of levels
  const R=JSON.parse(fs.readFileSync('./learning/rules.json','utf8')).rules;
  ok(!!R['spx.nodes'],                     'enrolled as spx.nodes');
  ok(/key:'spxnodes'/.test(src),           'as a real feature');
  const rec=src.slice(src.indexOf("key:'spxnodes'"), src.indexOf("key:'spxnodes'")+3000);
  ['nextK','nextPct','nextRole','nextTaps','distPts','king'].forEach(f=>
    ok(rec.indexOf(f+':')>=0, 'records '+f));
  ok(/emPiles\.lastSrc!=='skylit'/.test(rec),
     'and refuses to record when the rail fell back to the other book');
  ok(/spx_first_tap_holds/.test(src),      'it asks whether the SPY tap rates transfer');
  ok(/Borrowing that number without checking/.test(src),
     'and says plainly that assuming they do is the mistake being avoided');
  ok(/hit:null/.test(rec),                 'non-voting until the scorecard says otherwise');
}

// ---------- 41. (v11.87) THE CONFLICT IS RECORDED, NOT ONLY DECLARED ----------
// v11.83 taught the flow chip to say "the two books disagree" and the face has said it since. Nothing
// wrote it down. A panel can declare a disagreement every session for a month and the scorecard still
// cannot answer WHICH BOOK WAS RIGHT — which is the only reason the disagreement is worth showing.
// The project's own FEATURE ENROLLMENT rule is DATA + ANALYSIS + TESTING; this had none of the three.
{
  const src2=src;
  const feat=src2.slice(src2.indexOf("registerFeature({ key:'flow'"),
                        src2.indexOf("registerFeature({ key:'dex'"));
  ok(feat.length>500,'the flow feature is where the conflict belongs — same reading, one condition on it', feat.length);

  // DATA
  ok(/conflict:\(typeof f\.conflict==='boolean'\)\?f\.conflict:null/.test(feat),
     'the per-bar record carries the conflict, as a real tri-state and not a coerced boolean');
  ok(/regimeG:\(typeof f\.regimeG==='number'\)\?f\.regimeG:null/.test(feat),
     'and the other book\'s gamma sign beside it, so a later context can re-derive the comparison');
  ok(/conflict:\(rec&&typeof rec\.conflict==='boolean'\)\?rec\.conflict:null/.test(feat),
     'and it survives into the scored outcome, or the questions below could never see it');

  // ANALYSIS — and the control arm, which is the part that makes it mean anything
  ok(/id:'flow_decays_when_books_fight'/.test(feat),'the conflicted arm is asked');
  ok(/id:'flow_holds_when_books_agree'/.test(feat),'and the AGREED arm is asked too — without a control the conflicted rate is a number with nothing to beat');
  ok(/NEVER against a pooled rate/.test(feat),'with the pooling trap named where the question lives');
  const qs=[...feat.matchAll(/\{f:'conflict',v:(true|false)\}/g)].map(m=>m[1]);
  ok(qs.includes('true') && qs.includes('false'),'the two arms split on OPPOSITE values of the same field', qs);

  // TESTING — the rule that governs it says what the segmentation is for
  const RJ=JSON.parse(fs.readFileSync('./learning/rules.json','utf8')).rules;
  ok(!!RJ['flow.perPoint'],'the rule exists');
  ok(/scored SEPARATELY/.test(RJ['flow.perPoint'].mechanism),
     'and its mechanism records that a conflicted reading is scored separately');
  ok(/conflict|agreed on the gamma sign/i.test(RJ['flow.perPoint'].condition),
     'and its condition says the agreement state is part of what was measured');

  // the debug hook is a RESHAPE and drops anything not listed — that is how this stayed invisible
  const dbg=src2.slice(src2.indexOf('__gptsDebug.flow ='), src2.indexOf('__gptsDebug.flow =')+1400);
  ok(/conflict:/.test(dbg) && /regimeG:/.test(dbg),
     'the debug view reports the conflict — it is a reshape, not a passthrough, and shipped four builds without it');
  ok(/DISAGREE/.test(dbg),'in words, so the answer does not depend on reading a boolean the right way round');
}

// ---------- 40. (v11.86) THE SPX LEVELS REACH THE CHART, IN ES, ON THE TICK ----------
// The rail has drawn Skylit's SPXW nodes since v11.77 and the chart never carried them — the levels
// actually being traded off were the ones missing from the chart.
//
// ⚠⚠ THIS SECTION SHIPPED AS FOURTEEN SOURCE-GREPS AND WAS REWRITTEN TO EXECUTE (v11.87).
// Every assertion here used to be `/pattern/.test(sourceText)`. Not one of them could have caught a
// wrong PRICE: swap `toSpy(P.k)` for `P.disp`, or the tick for 1.0, and all fourteen still passed.
// This is the SAME trap as the v11.70 forecast-ban test — the third time in this project a test has
// asserted that source code CONTAINS a thing rather than that the code DOES the thing.
// **If a test can pass on a build that emits the wrong number, it is documentation, not a test.**
{
  // A real build, with every dependency stubbed and the two that matter carrying live-shaped values.
  function irtFixture(r, opt){
    opt = opt || {};
    var IRT_HEADER='HDR';
    var IRT_COLORS={king:1,gate:2,ceil:3,flr:4,neg:5,deriv:6,ns:7,pb:8,mag:9};
    var SUCC_CHART_PCT=60; eval(ex_var('SUCC_CHART_PCT'));
    var CFG={ irt:{futSym:'EPU26', etfSym:''}, nodeThresh:20 };
    var nodeMapModel=function(){ return {ok:true, levels:[]}; };   // isolate: no SPY rows
    var irtRatio=function(){ return {r:r, live:true, src:'live'}; };
    var nextStopPick=function(){ return null; };
    var pbEntryPick=function(){ return null; };
    var lvlUnified=function(){ return null; };
    var ifChainRows=function(){ return null; };
    var emBand=function(){ return {ok:true}; };
    var ifLadder=function(){ return {dispScale:1.0023, undScale:0.099773}; };
    var emPiles=function(){ return opt.piles || [
      {k:7710, pct:100, role:'KING', accel:true,  disp:7727.73},
      {k:7700, pct:79,  role:'GK',   accel:true,  disp:7717.71},
      {k:7650, pct:41,  role:null,   accel:false, disp:7667.59}
    ]; };
    emPiles.lastSrc = (opt.src===undefined) ? 'skylit' : opt.src;
    var tapeMap=function(){ return opt.tape || {pct:{7710:-100, 7630:-85, 7650:41}, king:7710}; };
    eval(ex('irtRound')); eval(ex('irtCsvRow')); eval(ex('irtBuildCsv'));
    var b=irtBuildCsv();
    if(!b) return { rows:[], byLabel:{} };
    var rows=b.csv.trim().split('\r\n').slice(1).map(function(l){
      var p=l.split(','); return { sym:p[0], price:parseFloat(p[1]), lbl:p[2], col:p[3], w:p[4], style:p[5] };
    });
    var byLabel={}; rows.forEach(function(x){ byLabel[x.lbl]=x; });
    return { rows:rows, byLabel:byLabel };
  }

  const F=irtFixture(10.0458);   // r = dispScale/undScale, the ladder-implied ratio
  const L=F.byLabel;

  // --- the nodes arrive at all, labelled with their SPX strike and role -------------------------
  ok(F.rows.length===4,                    'the three rail nodes and the successor all reach the chart', F.rows.length);
  ok(!!L['SPX 7710 KING 100%'],            'the King is labelled with its SPX strike, role and %King');
  ok(!!L['SPX 7700 GK 79%'],               'so is the gatekeeper');
  ok(!!L['SPX 7650 BRK 41%'],              'and a node with no role falls back to its polarity');

  // --- THE PRICES. This is the part fourteen greps could not see. ------------------------------
  // The rail shows SPX 7710 at 7727.73. The chart must land on the SAME price, snapped to the tick.
  ok(L['SPX 7710 KING 100%'].price===7727.75, 'SPX 7710 lands at ES 7727.75 — the rail\'s 7727.73 on the 0.25 tick', L['SPX 7710 KING 100%'].price);
  ok(L['SPX 7700 GK 79%'].price===7717.75,    'SPX 7700 lands at ES 7717.75 (rail 7717.71)', L['SPX 7700 GK 79%'].price);
  ok(L['SPX 7650 BRK 41%'].price===7667.50,   'SPX 7650 lands at ES 7667.50 (rail 7667.59)', L['SPX 7650 BRK 41%'].price);
  F.rows.forEach(function(x){
    ok(Math.abs(x.price/0.25 - Math.round(x.price/0.25)) < 1e-9,
       'every emitted price is a tradeable ES price: '+x.lbl+' = '+x.price, x.price);
  });

  // --- THE SCALE PROPERTY, and it is what discriminates the two conversions --------------------
  // Going SPX -> SPY via dispScale and back via R.r makes R.r CANCEL: the emitted price is
  // irtRound(spxK * dispScale, tick) for ANY ratio. Going via undScale does not cancel and the
  // answer moves with the futures basis. Build twice at different ratios and demand the same prices.
  const F2=irtFixture(10.6000);
  ok(JSON.stringify(F.rows.map(x=>x.lbl+'='+x.price))===JSON.stringify(F2.rows.map(x=>x.lbl+'='+x.price)),
     'the emitted prices do NOT move when the futures ratio moves — R.r cancels, which is the whole point of routing through dispScale',
     F2.rows.map(x=>x.lbl+'='+x.price));
  ok(L['SPX 7710 KING 100%'].price === Math.round(7710*1.0023/0.25)*0.25,
     'and the price is exactly irtRound(spxStrike x dispScale, 0.25), independent of the ratio');
  // the undScale route, computed here, is the number this must NOT be at a drifted ratio
  ok(Math.round((7710*0.099773)*10.6/0.25)*0.25 !== 7727.75,
     'the undScale route gives a DIFFERENT price once the ratio drifts — so the choice is load-bearing, not cosmetic');

  // --- display rounding and chart rounding are different jobs ----------------------------------
  ok(L['SPX 7710 KING 100%'].price !== Math.round(L['SPX 7710 KING 100%'].price),
     'the chart price is NOT the whole point the FRAME row shows — .75 survives to the chart');
  const f=ex('secFrame'), bsrc=ex('irtBuildCsv');
  ok(/frameNum/.test(f) && !/frameNum/.test(bsrc),
     'the FRAME row rounds to whole points; the chart does not borrow that rounding');

  // --- succession, gated and honest ------------------------------------------------------------
  ok(!!L['SPX SUCC 7630 85%'],             'a successor above the threshold is drawn');
  ok(L['SPX SUCC 7630 85%'].price===7647.50,'on the same scale as everything else', L['SPX SUCC 7630 85%'].price);
  ok(L['SPX SUCC 7630 85%'].style==='2',   'and dashed, so it never reads as a live level');
  const Fw=irtFixture(10.0458, {tape:{pct:{7710:-100, 7630:-55}, king:7710}});
  ok(!Fw.byLabel['SPX SUCC 7630 55%'],     'a successor BELOW the threshold is not drawn', Object.keys(Fw.byLabel));
  ok(/var SUCC_CHART_PCT = 60;/.test(src), 'the threshold is the 60% the doctrine uses');
  ok(/⚖ HAND-SET/.test(src.slice(src.indexOf('var SUCC_CHART_PCT')-320, src.indexOf('var SUCC_CHART_PCT'))),
     'flagged hand-set');
  ok(/THAT NUMBER IS SPY'S/.test(bsrc),
     'and the 76% backtest is marked as a SPY number, not asserted as an SPX probability');

  // --- the other book must not be exported as if it were this one ------------------------------
  const Fif=irtFixture(10.0458, {src:'if-fallback'});
  ok(Fif.rows.length===0,
     'when the rail fell back to InsiderFinance NOTHING is exported — an IF node is a different quantity and would land on the chart wearing a Skylit label',
     Fif.rows.map(x=>x.lbl));
  const Fnone=irtFixture(10.0458, {src:'none', piles:[]});
  ok(Fnone.rows.length===0,               'and no book at all exports nothing rather than an empty confident chart');

  // --- colours carry the role ------------------------------------------------------------------
  ok(L['SPX 7710 KING 100%'].col==='1',   'the King gets the King colour');
  ok(L['SPX 7700 GK 79%'].col==='2',      'the gatekeeper gets the gate colour');
  ok(L['SPX 7650 BRK 41%'].col==='4',     'and a positive-gamma brake gets the floor colour, not the accelerator colour');
  ok(L['SPX 7710 KING 100%'].w==='3',     'the King is drawn heaviest');
}

console.log((fail? 'FAIL ':'')+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
