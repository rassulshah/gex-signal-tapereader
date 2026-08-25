// v13.5 RAIL MOTION — circles, rationed animation, roll lane.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function noc(s){ return String(s).replace(/\/\*[\s\S]*?\*\//g,'').replace(/^\s*\/\/.*$/gm,''); }
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);if(!m)return '';
  let i=src.indexOf('{',m.index),d=0,e=-1;
  for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}
  return src.slice(m.index,e+1);}
let pass=0,fail=0;
function ok(c,m,x){ if(c){pass++;} else {fail++; console.log('FAIL '+m+(x!==undefined?'  got: '+x:''));} }

// ---------- circles ----------
ok(/\.g3pile\{position:absolute;bottom:2px;transform:translateX\(-50%\);border-radius:50%\}/.test(src),
   'nodes are discs, not bars');
ok(/var dia=Math\.max\(6, Math\.min\(18, Math\.round\(Math\.sqrt\(P\.pct\/100\)\*15\)\)\)/.test(src),
   'diameter scales by sqrt so a King does not flatten a 30% node into invisibility');

// ---------- motion is RATIONED, and decided once ----------
{
  ok(/var G3_MOTION_MAX=3;/.test(src), 'at most three nodes animate at once');
  // ⚠ the cap must bind on the RECEIVING branch too — asserting the token exists anywhere passes
  // even when the recv branch is uncapped, which a mutation proved.
  ok(/if\(recvSet\[m\.k\]\)\{ if\(used<G3_MOTION_MAX\)/.test(src),
     'and the cap binds on the receiving branch, not just growing');
  ok(/if\(m\.d15>0 && used<G3_MOTION_MAX\)/.test(src), 'and on the growing branch');
  ok(/mv\.sort\(function\(a,b\)\{ return Math\.abs\(b\.d15\)-Math\.abs\(a\.d15\); \}\)/.test(src),
     'the most decisive 15m moves earn the attention');
  // ⚠ receiving beats growing on a node doing both
  const order=src.indexOf("motion[m.k]='recv'"), grow=src.indexOf("motion[m.k]='grow'");
  ok(order>-1 && grow>-1 && order<grow, 'RECEIVING is assigned before GROWING — a node doing both shows arrival');
  // ⚠ dissipating never animates
  ok(/motion\[m\.k\]='fade'/.test(src) && /g3fade\{opacity:\.34/.test(src), 'dissipating is dimmed');
  ok(!/g3fade::after\{[^}]*animation/.test(src), 'and NEVER animated — it is the node you are leaving');
}
// ---------- direction is the trick ----------
ok(/g3grow::after\{border-color:#2ec27e;animation:g3sonar/.test(src), 'growing pushes rings OUTWARD');
ok(/g3recv::after\{border-color:#7cc7ff;animation:g3inward/.test(src), 'receiving pulls rings INWARD');
ok(/@keyframes g3sonar\{0%\{transform:scale\(1\)/.test(src) && /@keyframes g3inward\{0%\{transform:scale\(2\.6\)/.test(src),
   'and the two keyframes are genuine opposites, so they cannot be confused');

// ---------- ONE roll computation, two consumers ----------
{
  ok(/var RAILROLLS=\[\];/.test(src), 'rolls are computed once for the rail');
  ok(/rollsNow = RAILROLLS;/.test(src), 'and the circles reuse that array');
  const rl=ex('railRollLane');
  ok(/rolls\.slice\(0, 3\)/.test(rl), 'the lane draws the same capped set');
  // ⚠ INVERTED: a second rollScan inside the render would drift within a single frame
  const railBlock=noc(src).slice(noc(src).indexOf('var RAILROLLS'), noc(src).indexOf('var RAILROLLS')+9000);
  ok((railBlock.match(/rollScan\(/g)||[]).length<=1,
     'and rollScan is NOT called twice in one render (INVERTED — an arrow pointing at a still node)');
}
// ---------- the lane owns its own band ----------
{
  const rl=ex('railRollLane');
  ok(/if\(!rolls \|\| !rolls\.length\) return '';/.test(rl), 'no rolls means no lane and no wasted height');
  ok(/vector-effect="non-scaling-stroke"/.test(rl), 'strokes survive the stretched viewBox');
  ok(/border-top:4px solid/.test(rl), 'and the arrowhead is an HTML triangle, so it never distorts');
  ok(/g3rl\{position:relative;height:22px/.test(src), 'the lane sits in a band of its own');
  ok(/FINDINGS F6/.test(rl), 'the hover states what a roll is and is NOT evidence of');
}
// ---------- every viewer gets an off switch ----------
// ⚠ a reduced-motion block ALREADY existed for .g3pulse, so asserting the at-rule exists passes on
// someone else's rule. Assert OUR selectors are the ones disabled.
ok(/@media \(prefers-reduced-motion: reduce\)\{#gpts-body \.g3pile::after,#gpts-body \.g3rl \.fl\{animation:none !important\}\}/.test(src),
   'the OS preference disables THESE animations, not merely some animation somewhere');
ok(/motion: true,/.test(src), 'and there is a real setting');
ok(/classList\.toggle\('g3nomo', CFG\.motion===false\)/.test(src), 'applied on every render, no reload needed');
console.log('\n'+pass+' pass / '+fail+' fail');
