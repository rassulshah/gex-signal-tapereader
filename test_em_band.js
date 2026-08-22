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
  const consts=src.match(/var EMOPEN_KEY=[^\n]*\n\s*var EM_FRESH_MIN=[^;]*;/);
  ok(!!consts, 'EMOPEN_KEY and EM_FRESH_MIN are declared together at module scope');
  eval(consts[0]);
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
  const emb=src.slice(src.indexOf("key:'emband'"), src.indexOf("key:'emband'")+2600);
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

console.log((fail? 'FAIL ':'')+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
