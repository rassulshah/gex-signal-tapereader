// v13.4 NODE EVENT LEDGER — the row that makes roll analysis possible.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function noc(s){ return String(s).replace(/\/\*[\s\S]*?\*\//g,'').replace(/^\s*\/\/.*$/gm,''); }
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);if(!m)return '';
  let i=src.indexOf('{',m.index),d=0,e=-1;
  for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}
  return src.slice(m.index,e+1);}
let pass=0,fail=0;
function ok(c,m,x){ if(c){pass++;} else {fail++; console.log('FAIL '+m+(x!==undefined?'  got: '+x:''));} }

// ---------- the classifier must MATCH the study tool, or live labels and analysis diverge ----------
{
  const c=ex('nevClassify');
  ok(/kind:'DEFLECT'/.test(c) && /kind:'PIN'/.test(c) && /kind:'BREAK'/.test(c), 'three outcomes, not a binary');
  ok(/inBand\/r\.length >= PINF/.test(c), 'PIN is decided by TIME SPENT in the band');
  const iPin=c.indexOf("kind:'PIN'"), iBreak=c.indexOf("kind:'BREAK'");
  ok(iPin>-1 && iBreak>-1 && iPin<iBreak, 'PIN is tested BEFORE break — a magnet crosses by small amounts');
  ok(/overshoot:true/.test(c), 'and a Beach Ball is a DEFLECT with a flag, never a BREAK');
  ok(/Only BREAK is failure/.test(src), 'the reason is written down where it will be read');
}
// ---------- rolls must NOT be gated on size (FINDINGS F6) ----------
{
  const sc=ex('nevScan');
  ok(/toIsBiggest/.test(sc), 'destination size is RECORDED');
  ok(!/if\(!best\.toIsBiggest\) continue|isBiggest.*continue/.test(noc(sc)), 'but never used to filter');
  ok(/NEV_MAXD/.test(sc) && /NEV_RATIO/.test(sc), 'proximity and share are the gates');
  // ⚠ any `continue` guarded by pct is a size filter, whatever shape it is written in
  ok(!/pct[^;\n]*\)\s*continue/.test(noc(sc)), 'no %King floor excludes small nodes');
  ok(/would have deleted real pullback-support/.test(src) || /deleting this case/.test(src) || /thrown those away/.test(src),
     'and the measured reason for not gating is recorded');
}
// ---------- the why-vector must capture what an explanation needs ----------
{
  const w=ex('nevWhy');
  ['d5','d15','d60','d1d'].forEach(f=>ok(new RegExp('w\\.'+f+'=').test(w), 'why carries '+f));
  ok(/w\.pull=/.test(w), 'magnetism is computed');
  ok(/NOT SKYLIT'S/.test(w), '⚠ and is labelled as OURS — they publish no equation');
  ok(/w\.rg=/.test(w) && /w\.trend=/.test(w) && /w\.emPct=/.test(w) && /w\.hhmm=/.test(w),
     'regime, trend, EM position and time of day are captured');
  ok(/nodeTapCount\('SPXW'/.test(w), 'taps are read from the SPXW book the nodes came from');
  ok(!/ledger\(sym, node\.k\)/.test(noc(w)), 'and NOT from an invented accessor that would silently return nothing');
  ok(/w\.stage=lc\.stage/.test(w), 'lifecycle stage rides along (Fresh/Tested/Delivered/Decaying)');
}
// ---------- outcomes get back-filled at three horizons ----------
{
  const b=ex('nevBackfill');
  ok(/\[\[5,'o5'\],\[10,'o10'\],\[20,'o20'\]\]/.test(b), 'three horizons, because holding is time-bounded');
  ok(/nevClassify\(lvl, side/.test(b), 'using the same classifier as the live labels');
  ok(/NOTOUCH/.test(b), 'and "price never came" is recorded as its own answer, not left null');
}
// ---------- capture must never be poisoned or block the older recorder ----------
{
  const sc=ex('nevScan');
  ok(/inReplay\(\)\) return null/.test(sc), 'a replayed session is never recorded as today');
  ok(/day\._lastBar===bar/.test(sc), 'one scan per closed bar, not per render');
  ok(/try\{ nevScan\(activeSym\(\)\); \}catch\(eNS\)\{\}/.test(src) &&
     /try\{ nevBackfill\(activeSym\(\)\); \}catch\(eNB\)\{\}/.test(src),
     'both are wrapped so an event fault cannot stop the bar snapshot');
  ok(/nodeEvents:\(function\(\)\{ try\{ return nevDay\(\)\.ev/.test(src), 'events ride the day export');
  ok(/nodeEvents:'one row per NODE EVENT/.test(src), 'with a legend, so the nightly need not guess');
}
console.log('\n'+pass+' pass / '+fail+' fail');

// (v14.6) exit code added: this file could print FAIL and still exit 0 — the silent-red pattern.
process.exit((typeof fail!=="undefined"&&fail)?1:0);
