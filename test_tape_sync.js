// (v10.38) TAPE <-> APP RECONCILIATION — the standing guarantee.
//
// The 2026-08-14 incident was possible because ONE parser was the sole authority
// on the King. This layer derives the King three independent ways and refuses to
// render structural data without consensus.
//
// THE HEADLINE TEST is "would this have caught the real incident" below: on that
// day the tape $K tag said 780 and the raw feed said 780, while the broken
// parser's max-%King said 775. Majority = 780. The reconciler produces the
// CORRECT King even with the parser still broken, and flags the disagreement.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
function ok(c,m){ if(c){pass++; console.log('PASS '+m);} else {fail++; console.log('FAIL '+m);} }
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);
  let i=src.indexOf('{',m.index),d=0,e=-1;
  for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}
  return src.slice(m.index,e+1);}

var RECON_MIN_AGREE=2;
eval(ex('reconcileVotes'));

// ---------------------------------------------------------------- SYNC GUARDS
ok(/var RECON_MIN_AGREE\s*=\s*2/.test(src),      'GUARD: consensus threshold is 2');
ok(/function reconcileVotes/.test(src),          'GUARD: reconcileVotes exists');
ok(/function kingFromFeed/.test(src),            'GUARD: PATH 2 (feed) exists');
ok(/function kingFromTapeTag/.test(src),         'GUARD: PATH 1 (tape $K tag) exists');
ok(/function kingFromTapeMax/.test(src),         'GUARD: PATH 3 (tape max %King) exists');
ok(/function tapeSync/.test(src),                'GUARD: tapeSync gate exists');
// (v15.53) removed: outOfSyncBlock (the suppression panel) archived (D-zero-callers); syncBannerHtml is the live surface
// (v11.40) v10.47 DELIBERATELY replaced the blocking gate with a one-line banner, user-directed:
// "the app must stay visible (weekends / parse hiccups) so it can be inspected". This assertion
// pinned the pre-v10.47 behaviour and has been failing ever since, in the bucket everyone ignores.
ok(/if\(syncBannerShow\(__sync\)\)\{ html\+=syncBannerHtml\(__sync\); \}/.test(src),
   'GUARD: render() surfaces the sync banner on failure (non-blocking by design since v10.47)');
ok(/html\+=syncBannerHtml\(__sync\)/.test(src) && /Out of sync/.test(src),  'GUARD (v10.47): one-line sync banner is rendered on failure and the app still renders');
ok(/tapeGate:\s*true/.test(src),                 'GUARD: CFG.tapeGate defaults ON');
ok(/syncReport/.test(src),                       'GUARD: operator diagnostic hook exposed');

// ------------------------------------------ THE REAL INCIDENT (2026-08-14 SPY)
// Tape $K tag: 780. Raw feed: 780. Broken parser max-%King: 775.
var incident = reconcileVotes({ tag:780, feed:780, tapemax:775 });
ok(incident.ok === true,              'INCIDENT: consensus reached despite the broken parser');
ok(incident.king === 780,             'INCIDENT: King resolves to 780 — the CORRECT strike');
ok(incident.king !== 775,             'INCIDENT: 775 is NOT crowned (the shipped bug)');
ok(incident.reason === 'majority',    'INCIDENT: reported as a majority, not unanimous');
ok(incident.disagree.length === 1,    'INCIDENT: the dissenting path is recorded');
ok(incident.disagree[0] === 'tapemax:775', 'INCIDENT: dissenter named exactly');
ok(incident.agree.length === 2,       'INCIDENT: two paths agreed');

// ------------------------------------------------------------------- HEALTHY
var u = reconcileVotes({ tag:780, feed:780, tapemax:780 });
ok(u.ok === true && u.king === 780,   'unanimous 3/3 -> King 780');
ok(u.confidence === 'high',           'unanimous -> high confidence');
ok(u.reason === 'unanimous',          'unanimous reason');
ok(u.disagree.length === 0,           'unanimous -> no dissent');

// --------------------------------------------------------- SUPPRESSION CASES
var none = reconcileVotes({ tag:780, feed:775, tapemax:770 });
ok(none.ok === false,                 'all three disagree -> NOT ok');
ok(none.king === null,                'all three disagree -> no King is asserted');
ok(none.reason === 'no-consensus',    'all three disagree -> no-consensus');
ok(none.confidence === 'none',        'all three disagree -> no confidence');

var tie = reconcileVotes({ tag:780, feed:775, tapemax:null });
ok(tie.ok === false,                  'two sources that disagree -> NOT ok (1-1 is not consensus)');
ok(tie.king === null,                 'two-way tie asserts no King');

var single = reconcileVotes({ tag:null, feed:null, tapemax:775 });
ok(single.ok === false,               'single source -> NOT ok (cannot self-corroborate)');
ok(single.confidence === 'low',       'single source -> low confidence');
ok(single.reason === 'single-source', 'single source reason');
ok(single.king === 775,               'single source still reports its value for diagnosis');

var empty = reconcileVotes({ tag:null, feed:null, tapemax:null });
ok(empty.ok === false,                'no sources -> NOT ok');
ok(empty.reason === 'no-source',      'no sources reason');
ok(empty.king === null,               'no sources -> no King');

// two agreeing sources IS consensus even with the third missing
var two = reconcileVotes({ tag:780, feed:780, tapemax:null });
ok(two.ok === true && two.king === 780, 'two agreeing sources -> consensus');
ok(two.sources === 2,                   'source count reported');

// --------------------------------------------- INDEPENDENCE / FAILURE MODES
// Each path fails differently — no single fault can take a majority.
var parseBroken = reconcileVotes({ tag:780, feed:780, tapemax:1 });    // parser wrong
ok(parseBroken.king === 780,   'FAILURE MODE: parser defect outvoted by tag + feed');
var feedStale   = reconcileVotes({ tag:780, feed:9999, tapemax:780 }); // feed wrong
ok(feedStale.king === 780,     'FAILURE MODE: stale/bad feed outvoted by tape paths');
var domChanged  = reconcileVotes({ tag:null, feed:780, tapemax:null }); // DOM broke
ok(domChanged.ok === false,    'FAILURE MODE: DOM change leaves one source -> suppressed, not guessed');

// -------------------------------------------------- RECURRENCE / ESCALATION
ok(/RECON_FAIL_ESCAL\s*=\s*3/.test(src),   'RECURRENCE: escalation threshold is 3 consecutive');
ok(/r\.recurring\s*=\s*\(st\.streak>=RECON_FAIL_ESCAL\)/.test(src),
                                            'RECURRENCE: recurring flag derives from the streak');
ok(/st\.streak=0;/.test(src),              'RECURRENCE: streak resets on a healthy read');
ok(/st\.log\.push/.test(src),              'RECURRENCE: failures are logged for diagnosis');
ok(/RECON_LOG_MAX/.test(src),              'RECURRENCE: the log is bounded');
ok(/RECURRING \('\+r\.streak\+' consecutive\)/.test(src),
                                            'RECURRENCE: surfaced to the operator in the panel');

// ----------------------------------------- PARSER INVARIANTS FEED THE GATE
ok(/if\(tm && tm\.kingConflict\)\{ r\.ok=false;/.test(src),
   'A flagged parse invariant forces the gate closed regardless of vote consensus');

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
