// ============================================================================================
// test_lastbook.js — (v14.55) THE CLOSE-OF-SESSION BOOK
//
// Operator, 2026-08-27, an hour after the close: "we are suppose to have a rule to show the last
// day so i can continue working."
//
// ⚠ THE EXISTING RULE WAS NOT BROKEN, AND THESE TESTS EXIST PARTLY TO STOP SOMEONE "FIXING" IT.
// Measured on the live panel at 17:11 CT:
//     session   showing 2026-08-27 · replay:false · rth:false     <- pickSessionDay was CORRECT
//     velocity strikes by expiry:  2026-08-28 -> 256 · 2026-09-16 -> 70 · 2026-08-27 -> 0
//     strikes with a non-zero 15m delta: 0 of 326
// `pickSessionDay` answers "which day's PRICE BARS do I draw" and it answered right — today had a
// session. It was never built to answer "which expiry's NODES do I draw", and Skylit drops the
// expired chain at the close. The panel was FLAT, not blank.
//
// ⚠⚠ THE RISK THIS FEATURE CARRIES IS THE RECORDER. Serving latched numbers through velAt() and
// tapeMap() reaches every consumer, the recorder included. A latched book written into data/*.json
// as though it were live would poison every base rate the learning layer computes, permanently and
// undetectably — DECISIONS D-10, exactly. Most of what follows guards that one thing.
// ============================================================================================
const fs = require('fs');
const src = fs.readFileSync('./v10.js', 'utf8');
let pass = 0, fail = 0;
const ok = (c, m, g) => { if (c) { pass++; console.log('PASS ' + m); }
                          else { fail++; console.log('FAIL ' + m + (g !== undefined ? ' -> ' + JSON.stringify(g) : '')); } };
function ex(n){ const re=new RegExp('function\\s+'+n+'\\s*\\(','g'); const m=re.exec(src);
  if(!m) return ''; let i=src.indexOf('{',m.index),d=0,e=-1;
  for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){d--; if(d===0){e=k;break;}} }
  return src.slice(m.index,e+1); }

// ---- 1 · THE RECORDER IS BLIND, AND IT IS BLIND IN ONE PLACE ---------------------------------
const RB = ex('recorderBlind');
ok(/inReplay\(\)\s*\|\|\s*showingStaleBook\(\)/.test(RB),
   'r1 recorderBlind is replay OR stale-book — one guard covering both modes');

// ⚠ THE CENTRAL INVARIANT. Nine write paths used to test inReplay() directly. If even one is left
// testing it, that path records a latched book as live and the corpus is silently poisoned.
{
  const guards = (src.match(/if\(typeof recorderBlind==='function' && recorderBlind\(\)\)/g) || []).length;
  ok(guards >= 9, 'r2 every recorder write path is guarded by recorderBlind (9 of them)', guards);

  // strip the declaration, the latch's own guard, the DISPLAY guard and the debug hook; nothing
  // else may still be reaching for inReplay() on its own.
  const strays = src.split('\n')
    .map((l, i) => [i + 1, l])
    .filter(([, l]) => /inReplay\(\)/.test(l))
    .filter(([, l]) => !/function inReplay|recorderBlind|SESSION_DAY&&SESSION_DAY\.fallback/.test(l))
    .filter(([, l]) => !/^\s*\/\//.test(l))            // comments are not call sites
    .filter(([, l]) => !/inReplay\(\) \|\| showingStaleBook\(\)/.test(l)) // recorderBlind's own body
    // ⚠ NOT a recorder guard: hodLod asks WHICH CLOCK to measure against, and in both a replay and a
    // frozen book the wall clock is not the session clock. Same predicate, different question — it
    // reads, it never writes. Using recorderBlind() there would name it wrongly.
    .filter(([, l]) => !/inReplay\(\)\|\|showingStaleBook\(\)\)\?lastT/.test(l))
    .filter(([, l]) => !/return '';/.test(l))            // gammaProfileHtml — a DISPLAY guard, correct
    .filter(([, l]) => !/out\.replay=/.test(l))          // __gptsDebug.session
    .filter(([, l]) => !/if\(inReplay\(\)\) return;/.test(l)); // lastBookSave's own guard
  ok(strays.length === 0,
     'r3 no write path still tests inReplay() directly instead of recorderBlind()', strays);
}

// ⚠⚠ r4 IS REVERSED, DELIBERATELY (v14.81). It guarded gammaProfileHtml's DISPLAY check — that it
// tested inReplay() and not recorderBlind(), so the profile still drew over a frozen book. The
// operator cut the gamma profile on 2026-08-28 ("just remove it"), so the guard it protected no
// longer exists. The assertion now pins the REMOVAL instead of deleting quietly: if the profile ever
// returns, this goes red and whoever restores it must re-make the recorderBlind decision on purpose
// rather than inherit it. The r3 stray-scan filter above is left in place and is simply inert.
ok(!/function gammaProfileHtml/.test(src),
   'r4 the gammaProfileHtml display guard is gone WITH the profile — restore it and re-decide');

// ---- 2 · FOUR CONDITIONS, ALL REQUIRED -------------------------------------------------------
const SSB = ex('showingStaleBook');
ok(/CFG\.lastBook===false/.test(SSB),        's1 the setting can switch it off');
ok(/P\.rth/.test(SSB) && /return false/.test(SSB), 's2 it NEVER serves during a live session');
ok(/B\.day!==sessionDayStr\(\)/.test(SSB),   's3 the latch must belong to the session being shown');
ok(/fe!==B\.exp/.test(SSB),                  's4 ...and the live front expiry must actually have ROLLED');
ok(!/return true;\s*\}catch/.test(SSB),      's5 it never defaults to serving on an error path');
// ⚠ EVERY GATE FAILS CLOSED. The only `return true` in the whole function is the final comparison
// that has actually proved the book rolled; every other exit is `return false`. A mutation that
// turned the "no live book to compare" exit into `return true` would serve the latch on a freshly
// loaded tab with an empty VEL — stale numbers during a live session, which is the one thing this
// feature must never become. Counting the exits is what catches that.
{
  const trues  = (SSB.match(/return true/g)  || []).length;
  const falses = (SSB.match(/return false/g) || []).length;
  ok(trues === 0, 's5b there is no bare `return true` — the only true is the rolled comparison itself', trues);
  ok(falses >= 5, 's5c every other exit fails CLOSED, back to the live book', falses);
  ok(/return fe!==B\.exp;/.test(SSB),
     's5d ...and the single serving decision IS the rolled comparison, not a shortcut');
}

// ---- 3 · THE LATCH IS ONLY WRITTEN FROM A HEALTHY, LIVE BOOK ---------------------------------
const LBS = ex('lastBookSave');
ok(/if\(inReplay\(\)\) return;/.test(LBS),   'l1 a replay is never latched');
ok(/if\(!P \|\| !P\.rth\) return;/.test(LBS),'l2 nothing is latched outside RTH — only a live session');
ok(/n>=SK_MIN_STRIKES/.test(LBS),
   'l3 a DEGRADED book is never latched — it would be served for hours as though it were the close');
// ⚠ the latch must read the RAW reader, never the front door, or it re-latches its own output.
// (v15.09) the book is now a PARAMETER — SPXW and QQQ are both latched — but the invariant is
// unchanged and is what this asserts: the RAW reader, never the front door.
ok(/tapeMapLive\(bk\)/.test(LBS) && !/tapeMap\(bk\)/.test(LBS),
   'l4 the latch reads tapeMapLive, so it can never feed itself and never ages out');

// ---- (v15.09) ONE LATCH PER GOVERNING BOOK ----------------------------------------------------
// ⚠⚠ It stored ONE book (SPXW) under one key while the serve gate never asked which chart was
// drawn. Right for SPY, whose ladder is governed by SPX; WRONG for QQQ, governed by its own book —
// which was never latched at all. He switched to QQQ after the close and the ladder was empty.
ok(/\['SPXW','QQQ'\]\.forEach/.test(LBS), 'l10 BOTH governing books are latched');
ok(/if\(!store\.SPXW && !store\.QQQ\) return;/.test(LBS),
   'l11 ...and a latch is never overwritten with nothing');
const LBL2=(()=>{const i=src.indexOf('function lastBookLoad(');const j=src.indexOf('\nfunction ',i+5);return src.slice(i,j);})();
ok(/if\(LASTBOOK\.pct\) return/.test(LBL2),
   'l12 a legacy single-book payload still reads — an old latch is not silently lost');
ok(/function lastBookGov/.test(src) && /sym==='QQQ'\) \? 'QQQ' : 'SPXW'/.test(src),
   'l13 the governing book is named per symbol');
const SSB2=(()=>{const i=src.indexOf('function showingStaleBook(');const j=src.indexOf('\nfunction ',i+5);return src.slice(i,j);})();
ok(/lastBookLoad\(lastBookGov\(_sy\)\)/.test(SSB2),
   'l14 the serve gate asks for the book that governs the CHART BEING DRAWN');
ok(/\(sym==='SPXW'\|\|sym==='QQQ'\) && showingStaleBook\(\)/.test(src),
   'l15 tapeMap serves the stale book for BOTH symbols, not SPXW alone');

// ---- (v15.09) AN ABSENCE THE CODE CAN EXPLAIN MUST NOT BE SHOWN AS A BLANK -------------------
// ⚠⚠ He was handed an empty ladder after the close with nothing saying why and read it as a broken
// build. The cause was specific: on QQQ the latch has no QQQ book, because only SPXW was ever saved.
const SBW=(()=>{const i=src.indexOf('function staleBookWhy(');const j=src.indexOf('\nfunction ',i+5);return src.slice(i,j);})();
ok(!!SBW, 'l16 staleBookWhy exists');
ok(/no '\+gov\+' book was saved/.test(SBW), 'l17 ...and names the BOOK that is missing, not just "no data"');
ok(!/return B|return any/.test(SBW),
   'l18 ...and returns a REASON, never a substitute book — serving SPXW on QQQ is the bug v15.09 removed');
ok(/no last-session book for/.test(src), 'l19 the reason is rendered on the face, not only computed');

// ---- 4 · THE FRONT DOOR / RAW READER SPLIT ---------------------------------------------------
ok(/function tapeMapLive\(sym\)/.test(src), 't1 the original reader survives as tapeMapLive');
const TM = ex('tapeMap');
ok(/showingStaleBook\(\)/.test(TM) && /src:'lastbook'/.test(TM),
   't2 tapeMap serves the latch when the book has rolled, tagged as lastbook');
ok(/return tapeMapLive\(sym\)/.test(TM), 't3 ...and falls through to the live reader otherwise');
ok(/sym==='SPXW'/.test(TM), 't4 only the SPXW book is substituted — SPY/QQQ are untouched');

const VA = ex('velAt');
ok(/showingStaleBook\(\)/.test(VA), 'v1 velAt serves the latched velocity objects in stale mode');
ok(/lastBook:true/.test(VA), 'v2 ...and marks them, so a consumer can tell if it needs to');
// ⚠ stale:false is deliberate and the comment must say why, or a later reader will "fix" it and
// blank every column this mode exists to show.
ok(/stale:false` IS DELIBERATE|`stale:false` is DELIBERATE|stale:false. is DELIBERATE/i.test(src) ||
   /stale:false. IS DELIBERATE/.test(src),
   'v3 the deliberate stale:false is explained where it is written');

// ---- 5 · A MODE YOU CANNOT SEE IS A MODE THAT LIES -------------------------------------------
ok(/book \\u2014 frozen |book — frozen /.test(src), 'b1 the footer badge names the frozen clock time');
ok(/LB\.day\|\|'last session'/.test(src),           'b2 ...and the session it belongs to');
ok(/warn\+=\(warn\?' ':''\)\+'<span title="THE MARKET IS CLOSED/.test(src),
   'b3 the badge appends to `warn`, the variable its own scope builds (the first draft used `out`)');
ok(/recorder is blind to it by construction/.test(src),
   'b4 the hover states that nothing is recorded while it shows');

// ---- 6 · THE DEBUG HOOK EXPLAINS A NON-ENGAGEMENT --------------------------------------------
// A mode that silently fails to engage is as bad as one that silently does.
ok(/__gptsDebug\.lastBook\s*=/.test(src), 'd1 there is a debug hook');
['settingOn','rthNow','haveLatch','latchDay','liveFrontExp','rolled','recorderBlind','why']
  .forEach(f => ok(new RegExp('\\b' + f + ':').test(src), 'd2 ...reporting ' + f));

// ---- 7 · THE DAY RULE IS UNTOUCHED -----------------------------------------------------------
// ⚠ Someone reading "show the last day" could easily go and loosen pickSessionDay instead. Its two
// hard guards are what stop yesterday's PRICE being drawn as today's.
const PSD = ex('pickSessionDay');
ok(/if\(todayHas\) return out;/.test(PSD), 'p1 pickSessionDay still returns today when today has bars');
ok(/if\(P && P\.rth\) return out;/.test(PSD), 'p2 ...and still never substitutes during RTH');

console.log('test_lastbook: ' + pass + ' passed, ' + fail + ' failed');
process.exit(fail ? 1 : 0);
