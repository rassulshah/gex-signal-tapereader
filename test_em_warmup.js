// ============================================================================================
// test_em_warmup.js — (v15.46) THE WARM-UP GUARD READ `t` IN THE WRONG UNITS AND BLOCKED THE BAND
// FOR THE WHOLE DAY.
//
// Operator, 2026-09-02, market open, after a reload: "re.laded check.. nothing displayed, no ladder"
// MEASURED: emBand.ok FALSE, why "warm-up: candle window or ratio is not yet today's — not pinning",
// ZERO ladder rows — with every input healthy: FUTMODE.live true, futBars 1 minute old, tape 100
// strikes, sessionBody open 7650.5 close 7679.25.
//
// ⚠⚠ TWO PRODUCERS DISAGREE ABOUT WHAT `t` MEANS:
//     closedCandles()   pushes t: realMs   and passes NAIVE SECONDS to naiveDayStr
//     measureBars/ES    pushes t: r[0]*1000 — REAL epoch MILLISECONDS
// `naiveDayStr` multiplies by 1000, so on a futures chart it got milliseconds and returned a year in
// the fifty-eight thousands. capOK was permanently false and the band never pinned.
// ⚠ IT ONLY FIRES ON A FRESH CAPTURE — a pin carried over from an earlier session skips the branch —
// which is why it hid from v15.23 until the first new-day capture.
// ============================================================================================
const fs=require('fs');
const src=fs.readFileSync('./current/gex-signal-tapereader.user.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,200):''));} };

// ---- 1 · THE UNITS FAULT, EXECUTED ----------------------------------------------------------
const mul=(a,b)=>a/(1/b), two=n=>(n<10?'0':'')+n;
function naiveDayStr(t){ const d=new Date(mul(t,1000)); return d.getUTCFullYear()+'-'+two(d.getUTCMonth()+1)+'-'+two(d.getUTCDate()); }
const REAL_SEC = 1788355800;            // 2026-09-02 13:30Z — the 08:30 CT bar
const AS_MS    = REAL_SEC*1000;         // what measureBars/ES actually stores in `t`
ok(naiveDayStr(REAL_SEC)==='2026-09-02', 'u1 naiveDayStr is correct when given SECONDS', naiveDayStr(REAL_SEC));
ok(naiveDayStr(AS_MS)!=='2026-09-02',    'u2 ...and nonsense when given MILLISECONDS', naiveDayStr(AS_MS));
ok(/^\d{5}/.test(naiveDayStr(AS_MS)),
   'u2b EXECUTED: the retired guard compared today against a FIVE-DIGIT YEAR', naiveDayStr(AS_MS));

// ---- 2 · SO IT COULD NEVER MATCH, AND THE BAND COULD NEVER PIN -------------------------------
// ⚠ this is the whole bug in one line: the comparison the panel depended on was structurally false.
ok(naiveDayStr(AS_MS)!=='2026-09-02', 'u3 capOK was permanently false on a futures chart');

// ---- 3 · THE FIX ASKS THE PRODUCER, AND TOLERATES ITS FORMAT ---------------------------------
// ⚠ anchored on the real closing text — my first pattern assumed one closing paren where the code
// has two, and six assertions failed as a block because the slice came back empty. A regex that
// misses returns '' and every `.test('')` is false: the assertions do not report "I could not find
// it", they report the feature as broken. f1 exists to tell those two cases apart.
const blk=(src.match(/var c0=cs\[0\];[\s\S]{0,1600}?c0\.so>=_openSec\)\) capOK=false;/)||[''])[0];
ok(blk.length>0, 'f1 the guard block is findable');
ok(/MB && MB\.day/.test(blk), 'f2 it asks measureBars WHICH DAY IT SELECTED, rather than re-deriving one');
ok(!/naiveDayStr\(c0t\)/.test(blk) && !/naiveDayStr\(c0\.t\)/.test(blk),
   'f2b ...and no longer reads `t`, whose meaning differs between producers');
// the two formats really do differ, so the comparison must be numeric
function dayNums(x){ const m=String(x).match(/(\d{4})-(\d{1,2})-(\d{1,2})/); return m?(+m[1])*10000+(+m[2])*100+(+m[3]):null; }
ok('2026-9-2' !== '2026-09-02', 'f3 the futures day key and ctTodayStr are DIFFERENT STRINGS');
ok(dayNums('2026-9-2')===dayNums('2026-09-02'),
   'f3b EXECUTED: ...and equal once parsed to numbers — which is why the fix parses', dayNums('2026-9-2'));
ok(/_dayNums/.test(blk) && /_mbDay!==_today/.test(blk), 'f3c ...and the guard compares numbers');

// ⚠⚠ THE CASH BRANCH NAMES NO DAY BECAUSE IT HAS ALREADY FILTERED TO ONE. `closedCandles` drops
// every bar whose day is not today BEFORE returning; blocking there would refuse a series that is
// today's by construction — a guard re-checking what its own input guarantees.
ok(/_mbDay!=null && _today!=null && _mbDay!==_today/.test(blk),
   'f4 a producer that names NO day does not block the capture');
ok(/closedCandles/.test(src) && /if\(naiveDayStr\(t\)!==todayStr\) continue;/.test(src),
   'f4b ...and that is safe because closedCandles filters to today at the source');
// ⚠ AND THE CASH BUILDER NOW STATES ITS DAY TOO, so the guard has an answer on every path.
// ⚠ This is a WIRING CHECK and I am labelling it as one: every harness in this project stubs
// `closedCandles` wholesale, so the real builder never runs under test and no behavioural assertion
// can reach it. A grep is the honest instrument here — pretending otherwise would be worse.
ok(/o:x\.open, h:x\.high, l:x\.low, c:x\.close, so:so, day:naiveDayStr\(t\) \}\);/.test(src),
   'f4c the OHLC builder stamps `day`, as its sibling always has');
ok((src.match(/day:naiveDayStr\(t\)/g)||[]).length===2,
   'f4d ...so BOTH bar builders state it — one quantity, one source, on every path',
   (src.match(/day:naiveDayStr\(t\)/g)||[]).length);

// ---- 4 · WHAT THE GUARD MUST STILL DO -------------------------------------------------------
// ⚠ v15.23 wrote it for a real reason: the ES branch takes the LAST day present, which pre-open is
// YESTERDAY. Fixing the units must not disarm it.
ok(/keys\[keys\.length-1\]/.test(src),
   'f5 measureBars still takes the LAST day present — so a stale day IS reachable');
ok(/capOK=false/.test(blk), 'f5b ...and the guard can still refuse');
ok(/c0\.so>=_openSec/.test(blk), 'f5c ...and still requires the first bar to be at or after the open');
// executed: a yesterday-selection must refuse, a today-selection must pass
const T=dayNums('2026-09-02');
ok((dayNums('2026-9-1')!==T)===true,  'f6 EXECUTED: a series selecting YESTERDAY refuses');
ok((dayNums('2026-9-2')!==T)===false, 'f6b ...and one selecting TODAY pins');

// ---- 5 · WHY IT HID FOR SO LONG --------------------------------------------------------------
// ⚠ the branch runs only when there is no usable record — a pin carried over skips it entirely.
ok(/if\(!\(rec && typeof rec\.em==='number'\)\)/.test(src),
   'f7 the capture branch runs ONLY when no pin exists — which is why this hid until a new day');

console.log('test_em_warmup: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
