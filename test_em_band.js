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
  ok(/FROM PREV CLOSE/.test(f),         'and the face SAYS when the anchor is the prior close, not the open');
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
  const h=src.slice(src.indexOf("window.__gptsDebug.emBand"), src.indexOf("window.__gptsDebug.emBand")+3000);
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
  ok(/\u2192/.test(f),            'the arrow carries which extreme came first');
  ok(/pts to EXP /.test(f),      'and room left is in points to a named rail');
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
  ok(/EM '\+usd\(EBc\.emUsd\)\+'\/ct/.test(f), 'and says what a WHOLE expected move is worth per contract');
  ok(/EBc && EBc\.ok && EBc\.mult/.test(f), 'only when there IS a contract — a SPY chart has no multiplier');
  ok(/used · '\+usd\(EB\.leftUsd\)\+' left/.test(f), 'the shape line reports used and left in dollars');
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
  const pf=ex('emPiles');
  ok(/cpRows/.test(pf),          'piles read SKYLIT\'s book — the same source regime2D uses, so they cannot contradict the regime chip');
  ok(/CFG\.nodeThresh/.test(pf), 'the cut is the EXISTING nodeThresh slider, not a second invented threshold');
  ok(/put>cal/.test(pf),         'put-dominant means negative gamma means ACCELERATOR');
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

console.log((fail? 'FAIL ':'')+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
