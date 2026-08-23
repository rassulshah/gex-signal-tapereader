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
  const pf=ex('emPiles');
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
  const pf=ex('emPiles');
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
  ok(/emPath\(/.test(f),         'the path line renders');
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

// ---------- 18. (v11.66) THE RAIL GETS THE WHOLE ROW ----------
// The band row was rendering the low, the rail, the high, THEN a percentage, THEN a session chip — so the
// one element on the face that is a measurement was laid out into whatever width was left after two
// labels. Worse, the percentage is the SAME FACT the dot already draws, so the rail was being squeezed by
// its own restatement. Both moved to line 3.
{
  const f=ex('secFrame');
  const band=f.slice(f.indexOf('g3emk'), f.indexOf('var l3='));
  ok(band.length>0,                    'the band row and line 3 are separable in the source');
  ok(!/OF EM/.test(band),              'the band row no longer prints the percentage');
  ok(!/g3replay/.test(band) || /sessBadge\+=/.test(band),
                                       'and it does not append the session chip to the row either');
  ok(/var sessBadge=''/.test(f),       'the session chip is HELD, not emitted inline');
  const badgeWrites=(f.match(/sessBadge\+=/g)||[]).length;
  ok(badgeWrites===2,                  'both branches (replay chip / phase tag) write to it', badgeWrites);

  const l3=f.slice(f.indexOf('var l3='));
  ok(/OF STRAD/.test(l3),              'line 3 carries the percentage now, named for the straddle');
  ok(/g3pct/.test(l3),                 'as its own chip, sized to be read as a caption');
  ok(/EB\.stretched/.test(l3),         'and it keeps the stretched flag, which used to ride on `str`');
  ok(!/\bstr\b\s*\?/.test(l3),        'without reaching for `str`, which is scoped to the band block');
  ok(/l3\+='<span class="g3pth"/.test(l3), 'the path text is a SPAN on the same row, not its own div');
  ok(/if\(l3 \|\| sessBadge\)/.test(l3), 'line 3 renders when it has anything to say');
  ok(/g3sbg/.test(l3),                 'and the session chip is pushed to the far right');
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
  ok(/gross:P\.gexM/.test(h) && /net:P\.netM/.test(h),
     'and it returns BOTH legs — gross sizes the pile, net is the residual');
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

// ---------- 22. (v11.68) THE PILES READ THE BAND'S OWN WINDOW ----------
// They read toFri. On a FRIDAY the roll makes that today plus a whole extra week, and measured live on
// 2026-08-21 only 29.2% of the toFri gross gamma expired that day. Seven tenths of the obstacles drawn on
// today's band belonged to other days.
{
  const f=ex('emPiles');
  ok(/var w='dte0'/.test(f),          'the piles start from dte0, the same window as the band');
  ok(!/var w='toFri'/.test(f),        'and toFri is no longer the first choice');
  ok(/w='toFri'/.test(f),             'it survives only as a fallback when dte0 has no profile');
  // the fallback must come AFTER the primary, or it is not a fallback
  ok(f.indexOf("var w='dte0'") < f.indexOf("w='toFri'"),
     'and the fallback is genuinely second', f.indexOf("var w='dte0'")+' < '+f.indexOf("w='toFri'"));
  ok(/29\.2%/.test(f),                'the measurement that forced it is recorded beside the code');
  // the band and the piles must not be able to drift apart again
  const b=ex('emBand');
  ok(/dte0/.test(b) && /var w='dte0'/.test(f), 'band and piles now name the same window');
}

// ---------- 23. (v11.68) GROSS SIZES IT, NET PRICES IT ----------
// perPt used GROSS while accel used NET SIGN, so a strike could carry the dollar weight of its whole book
// and the direction of a rounding error. Live: 7650 overstated 84x, its net being 1.2% of its gross.
{
  const f=ex('emPiles');
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

// ---------- 24. (v11.68) ONE PERCENT IS IN CHART POINTS ----------
{
  const f=ex('emPiles');
  ok(/var onePct=c\.spot\*0\.01\*L\.dispScale/.test(f),
     'the per-point denominator converts to the scale the rail is drawn in');
  // and L must exist before it is used
  ok(f.indexOf('L=ifLadder') < f.indexOf('var onePct='), 'the ladder is fetched first', 'ok');
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

// ---------- 27. (v11.68) THE LABEL STOPS CLAIMING A SIGMA IT DOES NOT HAVE ----------
{
  const f=ex('secFrame');
  ok(/STRAD LOW/.test(f) && /STRAD HIGH/.test(f), 'the rails are named for the straddle');
  ok(!/EXP LOW/.test(f) && !/EXP HIGH/.test(f),   'and no longer for an "expected move"');
  ok(/OF STRAD/.test(f),                          'line 3 agrees with them');
  ok(/0\.80 sigma/.test(f),                       'the hover states what it actually is');
  ok(/58% of closes/.test(f),                     'and what that contains');
  ok(/1\.25/.test(f),                             'and how to get a real one-sigma boundary');
  // MP must not impersonate their published figure
  ok(/'MP\*'/.test(src),                          'our recomputed max pain is starred');
  ok(/NOT the max pain InsiderFinance publish/.test(src), 'and the hover says whose it is not');
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

// ---------- 29. (v11.69) THE ROW SAYS THE THING ONCE ----------
// "momentum — breaks not fades · widen stops" was three restatements of one fact on the busiest row of
// the panel, beside a chip that already said −G −V ⚠. And line 3 printed "path ↑ to 7717.71" directly
// under a chip reading "→ 7717.71".
{
  const r=ex('regime2D');
  ok(/out\.play='BREAKS'/.test(r) && /out\.play='FADES'/.test(r), 'the playbook is one word');
  // ⚠ check the PLAY lines only. `out.why` in the same function still carries the full sentences and
  // must — those are the hover text. A blanket search over the function fails on its own explanation.
  const playText=(r.match(/out\.play='[^']*'/g)||[]).join(' ');
  ok(!/breaks not fades/.test(playText) && !/widen stops/.test(playText) && !/pins hold/.test(playText),
     'and the sentences are gone from the PLAY line', playText);
  ok(/pins hold|levels tend to hold/.test(r), 'while the why-text keeps them');
  const plays=(r.match(/out\.play='[^']*'/g)||[]);
  ok(plays.length===4, 'all four regime cells still set one', plays.length);
  ok(plays.every(p=>p.replace(/out\.play='|'/g,'').split(/\s+/).length===1),
     'and every one of them is a single token', plays.join(' '));
  // the explanation must still EXIST — trimmed, not deleted
  const f=ex('secFrame');
  ok(/widen stops|more room than usual/.test(f), 'the widen-stops advice survives in the hover');
  ok(/HOW price moves, never WHICH WAY/.test(f), 'along with the standing caveat');

  // line 3 must not restate row 1's target
  ok(!/path '\+arrow\+' to/.test(f), 'line 3 no longer repeats the destination');
  ok(/txt=arrow\+/.test(f),          'it keeps only the direction');
  const tgtPrints=(f.match(/dispNum\(ifMagEarly\)/g)||[]).length;
  ok(tgtPrints===1, 'the target number is printed exactly ONCE on the whole section', tgtPrints);
}

console.log((fail? 'FAIL ':'')+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
