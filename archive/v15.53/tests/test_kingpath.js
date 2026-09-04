// Issue A regression: King Path price line ALWAYS present + labeled, King strike
// labeled on the dot, canvas +33% taller (H=112).
const fs = require('fs');
const src = fs.readFileSync('./v10.js','utf8');

function mul(a,b){ return a/(1/b); }
global.mul = mul;
global.PAL = { longAccent:'#2ec27e', shortAccent:'#f0616d', gold:'#e3c341', blue:'#4a90d9', sub:'#8b98a9', card:'#12161f', line:'#1e2530', ink:'#e6edf3' };
global.fmtNum = function(n){ return (Math.round(n*10)/10).toString(); };
global.STATE = { SPY:{ price:771.88 } };

function extract(name){
  const re = new RegExp('function\\s+'+name+'\\s*\\(', 'g');
  const m = re.exec(src); if(!m) throw new Error('not found: '+name);
  let i = src.indexOf('{', m.index), depth=0, end=-1;
  for(let k=i;k<src.length;k++){ if(src[k]==='{')depth++; else if(src[k]==='}'){depth--; if(depth===0){end=k;break;}} }
  return src.slice(m.index, end+1);
}
eval(extract('kingPathSigMoves'));
eval(extract('kingSparkline'));

const now = 1000*60*60*10;   // arbitrary
const sess = { start: now-3*3600000, end: now+2*3600000 };
function mkMv(strikes){ // oldest-first roll path
  return strikes.map((k,i)=>({ k, dir:(i>0?(k>strikes[i-1]?1:(k<strikes[i-1]?-1:0)):0), t: now-(strikes.length-1-i)*15*60000 }));
}

let pass=0, fail=0;
function ok(name, cond, got){ if(cond){pass++; console.log('PASS '+name+(got!==undefined?' -> '+got:''));} else {fail++; console.log('FAIL '+name+(got!==undefined?' -> got '+got:''));} }
function countLines(svg){ return (svg.match(/<line /g)||[]).length; }
function hasPriceLine(svg){ return /stroke-dasharray="4 3"/.test(svg); }   // (v10.27) price line dash restyled 2 3 -> 4 3 for contrast vs the gold King staircase

// --- Scenario 1: today's real case. King path 775->774->773, price 771.88 BELOW cluster (the reported bug) ---
let r = kingSparkline(mkMv([775,774,773]), 773, 771.88, now, sess, PAL.sub);
ok('canvas is +33% taller (H=112)', /height="112"/.test(r.svg), (r.svg.match(/height="(\d+)"/)||[])[1]);
ok('price line PRESENT even though price is below the King cluster (the bug)', hasPriceLine(r.svg), hasPriceLine(r.svg));
ok('price label shows value 771.9', r.svg.indexOf('771.9')!==-1, true);
ok('price label shows signed offset (\u22121.1 below King 773)', /\u22121\.1|\(-1\.1\)|\u22121/.test(r.svg), true);
ok('King strike labeled on the dot (\uD83D\uDC51773)', r.svg.indexOf('773')!==-1 && r.svg.indexOf('\uD83D\uDC51')!==-1, true);

// --- Scenario 2: price FAR below range -> line clamped but still present (+caret) ---
r = kingSparkline(mkMv([775,775]), 775, 770.0, now, sess, PAL.sub);
ok('price line present when price far below range (clamped)', hasPriceLine(r.svg), true);

// --- Scenario 3: single flat strike, price at it ---
r = kingSparkline(mkMv([772]), 772, 772.0, now, sess, PAL.sub);
ok('price line present on single flat-strike path', hasPriceLine(r.svg), true);

// --- Scenario 4: px null -> uses last-known STATE price, dimmed, still present ---
r = kingSparkline(mkMv([774,773]), 773, null, now, sess, PAL.sub);
ok('price line present when px is null (last-known fallback)', hasPriceLine(r.svg), true);
ok('stale price line is dimmed (opacity 0.35)', /opacity="0.35"/.test(r.svg), true);   // (v10.27) stale op 0.28 -> 0.35

console.log(fail===0 ? '\nALL PASS' : '\n'+fail+' FAILED');
process.exit(fail===0?0:1);
