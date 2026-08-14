// (v10.30) Cluster + Double-Stack detectors — HARDENED.
// Cluster = >=CLUSTER_MIN_N significant (>=CLUSTER_SIG_PCT) nodes within CLUSTER_BAND -> pins/chops.
// Double-Stack = EXACTLY a comparably-strong adjacent PAIR (within STACK_GAP), NOT in a cluster,
//   weaker >= STACK_BALANCE*stronger. A run of 3+ is a cluster, never a double-stack.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
function ok(c,m){ if(c){pass++; console.log('PASS '+m);} else {fail++; console.log('FAIL '+m);} }

var STATE={SPY:{price:772, walls:[]}};
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}

// threshold consts (mirror the shipped values; sync-guarded below)
var CLUSTER_SIG_PCT=40, CLUSTER_BAND=3.0, CLUSTER_MIN_N=3, STACK_GAP=1.0, STACK_BALANCE=0.5, STACK_POS_ONLY=true;
ok(src.indexOf('var CLUSTER_SIG_PCT = 40')>=0, 'shipped CLUSTER_SIG_PCT=40 matches test (sync-guard)');
ok(src.indexOf('var CLUSTER_BAND    = 3.0')>=0, 'shipped CLUSTER_BAND=3.0 matches test');
ok(src.indexOf('var CLUSTER_MIN_N   = 3')>=0, 'shipped CLUSTER_MIN_N=3 matches test');
ok(src.indexOf('var STACK_GAP       = 1.0')>=0, 'shipped STACK_GAP=1.0 matches test');
ok(src.indexOf('var STACK_BALANCE   = 0.5')>=0, 'shipped STACK_BALANCE=0.5 matches test');
ok(src.indexOf('var STACK_POS_ONLY  = true')>=0, 'shipped STACK_POS_ONLY=true matches test (v10.31)');

eval(['clusterDetect','doubleStackDetect','barneyDetect'].map(ex).join('\n'));
// (v10.31) W() now stamps polarity. Default pos:true (Pika/+gamma) preserves the
// intent of the pre-polarity fixtures. Pass a 3rd element (0/false) for -gamma.
function W(list){ return list.map(function(a){return {k:a[0], pct:a[1], pos:(a.length>2 ? !!a[2] : true)};}); }

// ---- CLUSTER: 3 nodes within 3 strikes (all >=40%) ----
STATE.SPY.walls=W([[770,45],[771,50],[772,55],[780,60]]);   // 770-772 = 3 within band; 780 alone
var c1=clusterDetect('SPY');
ok(c1.ok && c1.regions.length===1, 'one cluster region found ('+c1.regions.length+')');
ok(c1.regions[0].n===3 && c1.regions[0].lo===770 && c1.regions[0].hi===772, 'cluster spans 770-772 with 3 members');
ok(c1.memberK['772.00']===true && c1.memberK['780.00']!==true, 'members flagged; lone 780 NOT a cluster member');

// raised floor: a 30% node no longer counts (was significant at old 25%)
STATE.SPY.walls=W([[770,45],[771,30],[772,55]]);   // 771 (30) now below 40 floor
ok(!clusterDetect('SPY').ok, '30% node below the raised 40% floor -> only 2 significant -> no cluster');

// ---- DOUBLE-STACK: exactly a comparably-strong adjacent PAIR ----
STATE.SPY.walls=W([[769,55],[770,45],[780,60]]);   // 769 & 770 adjacent, both strong, balanced
var cl=clusterDetect('SPY');
var s1=doubleStackDetect('SPY', cl);
ok(s1.ok && s1.stacks.length===1, 'one double-stack found (a clean pair)');
ok(s1.stacks[0].n===2 && s1.stacks[0].lo===769 && s1.stacks[0].hi===770, 'stack = 769 & 770 (exactly 2)');
ok(s1.memberK['769.00']===true && s1.memberK['780.00']!==true, 'stack members flagged; lone 780 excluded');

// BALANCE rule: a dominant node next to a marginal one is NOT a double-stack
STATE.SPY.walls=W([[769,100],[770,42]]);   // 42 < 0.5*100 -> unbalanced
ok(!doubleStackDetect('SPY', clusterDetect('SPY')).ok, 'dominant+marginal pair -> NOT a double-stack (balance rule)');

// EXACTLY 2: a run of 3 adjacent strong nodes is a CLUSTER, not a double-stack (the over-fire fix)
STATE.SPY.walls=W([[768,45],[769,50],[770,55]]);   // 3 adjacent within band 3
var cl3=clusterDetect('SPY');
var s3=doubleStackDetect('SPY', cl3);
ok(cl3.ok && cl3.regions.length===1 && cl3.regions[0].n===3, '3 adjacent strong -> ONE cluster');
ok(!s3.ok, '3-run is a cluster, NOT a double-stack (no over-fire) -> stacks='+s3.stacks.length);

// mutual exclusivity: cluster members never also become stack members
STATE.SPY.walls=W([[768,45],[769,50],[770,55],[773,60],[774,58]]);  // 768-770 cluster; 773-774 clean pair
var clX=clusterDetect('SPY'); var sX=doubleStackDetect('SPY', clX);
ok(clX.regions.length===1 && clX.regions[0].n===3, 'cluster 768-770 (3)');
ok(sX.ok && sX.stacks.length===1 && sX.stacks[0].lo===773 && sX.stacks[0].hi===774, 'separate clean pair 773-774 = the only double-stack');
ok(!sX.memberK['769.00'], 'a cluster member (769) is NOT also a double-stack member (mutual exclusivity)');

// nodes too far apart -> no stack
STATE.SPY.walls=W([[769,55],[772,45]]);   // 3 strikes apart > STACK_GAP
ok(!doubleStackDetect('SPY', clusterDetect('SPY')).ok, 'nodes 3 strikes apart -> no double-stack');

// nodes too far apart -> no stack
STATE.SPY.walls=W([[769,55],[772,45]]);   // 3 strikes apart > STACK_GAP (re-assert after polarity)
ok(!doubleStackDetect('SPY', clusterDetect('SPY')).ok, 'nodes 3 strikes apart -> no double-stack (re-check)');

// ==================== (v10.31) POLARITY GATE ====================
// SOURCE OF TRUTH (Skylit Academy): Pika Cloud/Cluster = POSITIVE gamma only.
// A dense NEGATIVE region is a Barney, NOT a cluster.

// a dense PURPLE (-gamma) region must NOT register as a Cluster/Pika
STATE.SPY.walls=W([[770,45,false],[771,50,false],[772,55,false]]);  // 3 adjacent, all -gamma
ok(!clusterDetect('SPY').ok, '-gamma dense region -> NOT a cluster (Pika = +gamma only)');
ok(barneyDetect('SPY').ok && barneyDetect('SPY').regions[0].n===3, 'same -gamma region -> detected as a Barney (dense -gamma)');

// the SAME geometry but +gamma IS a cluster, and is NOT a Barney
STATE.SPY.walls=W([[770,45,true],[771,50,true],[772,55,true]]);
ok(clusterDetect('SPY').ok && clusterDetect('SPY').regions[0].n===3, '+gamma dense region -> IS a cluster (Pika)');
ok(!barneyDetect('SPY').ok, '+gamma region -> NOT a Barney');

// MIXED polarity in a band: only the +gamma members count toward a cluster
STATE.SPY.walls=W([[770,45,true],[771,50,false],[772,55,true]]);  // middle is purple
ok(!clusterDetect('SPY').ok, 'mixed band with only 2 +gamma members -> not enough for a cluster');

// DOUBLE-STACK polarity gate: a -gamma adjacent pair is NOT a bounce shelf
STATE.SPY.walls=W([[769,55,false],[770,50,false],[780,60,true]]);
ok(!doubleStackDetect('SPY', clusterDetect('SPY')).ok, '-gamma adjacent pair -> NOT a double-stack (STACK_POS_ONLY)');
// a +gamma adjacent pair still IS a double-stack
STATE.SPY.walls=W([[769,55,true],[770,50,true],[780,60,true]]);
ok(doubleStackDetect('SPY', clusterDetect('SPY')).ok, '+gamma adjacent pair -> IS a double-stack');

// empty / degenerate
STATE.SPY.walls=[];
ok(!clusterDetect('SPY').ok && !doubleStackDetect('SPY', {memberK:{}}).ok && !barneyDetect('SPY').ok, 'no walls -> none detect');

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
