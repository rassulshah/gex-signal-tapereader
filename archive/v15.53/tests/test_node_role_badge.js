// Test v10.26-prep: role/setup badge REPLACES the removed predictive verdict pill.
// Factual role only (King/Gatekeeper/Rug/Floor/Ceiling); priority order; no Bounce/Break prediction.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
function ok(c,m){ if(c){pass++; console.log('PASS '+m);} else {fail++; console.log('FAIL '+m);} }
var PAL={longAccent:'#2ec27e', shortAccent:'#f0616d', gold:'#e3c341', amber:'#f2b45a', sub:'#8b98a9'};
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
// (v11.40) nodeRolePill reads the flr/ceil thresholds; without them the file died partway and every
// assertion after that point was reported as a failure it had never actually run.
global.FLRCEIL_MIN_PCT=15; global.FLRCEIL_EDGE_PCT=40; global.FLRCEIL_FAR=6;
eval(ex('nodeRoleBadge'));

// (v11.40) This used to key off `padding:0 6px">`, so a CSS tweak broke five assertions that
// had nothing to do with CSS — and they then sat in the 'known stale' bucket where they
// camouflaged a REAL regression in another file. Match the last text node instead.
function label(html){ if(html==='') return '(none)';
  var m=html.match(/>([^<>]+)<\/span>\s*$/) || html.match(/>([^<>]+)</);
  return m ? m[1].trim() : '?'; }

// each role
ok(label(nodeRoleBadge({isKing:true}))==='King', 'King badge');
ok(nodeRoleBadge({isKing:true}).indexOf(PAL.gold)>=0, 'King colored gold');
ok(label(nodeRoleBadge({isGatekeeper:true}))==='Gate', 'Gatekeeper badge -> Gate (v10.36)');
ok(label(nodeRoleBadge({isRugCeil:true, rugType:'Rug'}))==='RugC', 'Rug-ceiling badge (abbrev RugC)');
ok(label(nodeRoleBadge({isRugFloor:true, rugType:'Rug'}))==='RugF', 'Rug-floor badge (abbrev RugF)');
ok(label(nodeRoleBadge({isRugCeil:true, rugType:'Reverse-Rug'}))==='RRugF', 'Reverse-Rug flips ceil label (abbrev RRugF)');
// (v11.40) Two stale contracts in one line each. The badge reads `isFlr`/`isCeil`, not `role`, and
// v10.47b prefixed the range-edge labels with a glyph. Assert the CURRENT shape.
ok(/Flr$/.test(label(nodeRoleBadge({isFlr:true}))), 'Floor badge ends in Flr (glyph-prefixed since v10.47b)');
ok(/Ceil$/.test(label(nodeRoleBadge({isCeil:true}))), 'Ceiling badge ends in Ceil');

// PRIORITY ORDER: King > Gatekeeper > Rug > Floor/Ceiling
ok(label(nodeRoleBadge({isKing:true, isGatekeeper:true, role:'Ceiling'}))==='King', 'King wins over Gatekeeper+Ceiling');
ok(label(nodeRoleBadge({isGatekeeper:true, isRugCeil:true, rugType:'Rug', role:'Ceiling'}))==='Gate', 'Gate wins over Rug+Ceiling');
ok(label(nodeRoleBadge({isRugFloor:true, rugType:'Rug', role:'Floor'}))==='RugF', 'Rug wins over plain Floor');

// new roles: Double-Stack + Pika Cloud (cluster) + Barney, and their priority order
// (v10.31) Cluster badge renamed Clst -> 'Pika' to match Skylit Academy vocabulary
// (SOURCE OF TRUTH: a +gamma cluster IS a Pika Cloud). Dense -gamma = 'Barn' (Barney).
ok(label(nodeRoleBadge({isStack:true, role:'Floor'}))==='DStk', 'Double-Stack badge (abbrev DStk)');
ok(label(nodeRoleBadge({isCluster:true, role:'Ceiling'}))==='Pika', 'Pika Cloud badge (abbrev Pika, was Clst)');
ok(label(nodeRoleBadge({isBarney:true, role:'Ceiling'}))==='Barn', 'Barney badge (abbrev Barn, dense -gamma)');
ok(label(nodeRoleBadge({isStack:true, isCluster:true, role:'Floor'}))==='DStk', 'Double-Stack wins over Pika');
ok(label(nodeRoleBadge({isRugFloor:true, rugType:'Rug', isStack:true}))==='RugF', 'Rug wins over Double-Stack');
ok(label(nodeRoleBadge({isCluster:true, role:'Floor'}))==='Pika', 'Pika wins over plain Floor');
ok(label(nodeRoleBadge({isCluster:true, isBarney:true, role:'Floor'}))==='Pika', 'Pika (checked) wins over Barney in badge order');

// secondary role -> tooltip (rug-target on a King)
var kingWithTgt=nodeRoleBadge({isKing:true, isRugTarget:true});
ok(kingWithTgt.indexOf('rug-target')>=0, 'secondary rug-target noted in King tooltip');

// NO predictive vocabulary anywhere in the badge
var allRoles=[{isKing:true},{isGatekeeper:true},{isRugCeil:true,rugType:'Rug'},{role:'Floor'},{role:'Ceiling'}].map(nodeRoleBadge).join(' ');
ok(!/Bounce|Break-through|Pullback/.test(allRoles), 'NO predictive Bounce/Break/Pullback text in any role badge');

// unknown node -> no badge (empty)
// (v11.40) A node with no role is not badge-less — it falls through to 'Mag', a minor magnet under
// FLRCEIL_MIN_PCT of King mass. The old '' expectation predates that fallthrough.
ok(label(nodeRoleBadge({}))==='Mag', 'a node with no role reads as a minor magnet, not a blank');

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);

// (v10.35) IDENTITY PILL — merges icon+word, kills duplication
eval(ex('nodeRolePill'));
var m={rug:{ok:false,shown:false}};
function strip(h){ return h.replace(/<[^>]+>/g,'').trim(); }
ok(strip(nodeRolePill({isKing:true,role:'King'})).indexOf('King')>=0 && strip(nodeRolePill({isKing:true,role:'King'})).indexOf('\uD83D\uDC51')>=0, 'King pill has crown + word');
ok(strip(nodeRolePill({isGatekeeper:true,role:'Gatekeeper'})).indexOf('\uD83D\uDEAA')>=0, 'Gatekeeper pill has door icon');
ok(strip(nodeRolePill({role:'Floor'}))==='Flr', 'Floor pill = word only (no icon)');
