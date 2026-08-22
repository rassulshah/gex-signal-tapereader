// (v11.14) LEVELS COMPUTED NATIVELY ON THE SPXW LANE, IN SPX POINTS.
// Why this exists: comparing our levels to an SPX gamma page always ran through a ratio, so any disagreement
// could be blamed on the conversion. Skylit's `derived` SPXW lane carries SPX-scale strikes with absolute
// dollar values, so the levels can be computed in SPX points directly and the conversion leaves the argument.
//
// Ground truth for the expectations below — their SPX page on 2026-08-20, spot 7642.21:
//     view        Call Wall  Put Wall  Zero Gamma  ratio
//     0DTE          7760       7640      7695.17   0.16
//     next week     7775       7640      7672.98   0.44
//     all exps      7900       7640      7660.22   0.82
// The put wall is 7640 in ALL THREE. The call wall moves 140 points across them and is defined on CALL gamma,
// which this feed does not decompose — so ours is reported as CR(net) and must never be labelled Call Resistance.
//
// ===================================================================================================
// (v11.53) RETIRED — THE CODE THIS TESTS NO LONGER EXISTS.
//
// `spxwLane()` and `spxLevels()` are ABSENT from the source. This file has been throwing
// `TypeError: Cannot read properties of null (reading 'index')` from its own ex() helper — the regex
// finds no match — and has been red for an unknown number of releases while reading like a real failure.
//
// It is retired rather than deleted so the ground-truth table above survives: their SPX page on
// 2026-08-20 at spot 7642.21, and the reasoning that a call wall defined on CALL gamma must never be
// labelled Call Resistance when the feed does not decompose call/put.
//
// WHERE THE COVERAGE WENT. SPX levels no longer come from Skylit's SPXW lane at all — they come from
// the InsiderFinance chain via `ifLadder`, a different source with a different shape, so these 25
// assertions could not be retargeted. That path is covered by:
//     test_if_ladder.js        45 assertions   the SPX ladder, CR0/PS0/FLIP/Mag
//     test_levels_unified.js  122 assertions   the unified level set
//     test_spxw_confluence.js  26 assertions   what remains of the SPXW lane
//
// ⚠ IF SPXW-NATIVE LEVELS EVER COME BACK, restore this file from git history rather than rewriting it.
// Do NOT quietly delete tests to turn a suite green — 23 stale failures once camouflaged two live bugs
// for months. This is a retirement WITH a reason, which is the opposite of that.
// ===================================================================================================
const fs=require('fs');
const src=fs.readFileSync('./v10.js','utf8');
const gone=['spxwLane','spxLevels'].filter(n=>!new RegExp('function\\s+'+n+'\\s*\\(').test(src));
if(gone.length===0){
  console.log('FAIL spxwLane/spxLevels are BACK in source — un-retire this file from git history');
  process.exit(1);
}
console.log('SKIPPED test_spx_levels: '+gone.join(', ')+' no longer exist in source (retired v11.53).');
console.log('  coverage moved to test_if_ladder.js / test_levels_unified.js / test_spxw_confluence.js');
console.log('0 passed, 0 failed (retired)');
process.exit(0);
