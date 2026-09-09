// test_v1586.js — (v15.86) THE LEARN CORPUS GROWS BY TEN CIRCLES: E005 (ES) and E006 (NQ), 2026-09-08 — his training
// ("here are 6 deflections identified by the circles. each should be counted as 1 deflection … here are 4 examples from
// the NQ"), the LIQUIDITY LEVEL factor (his ask: "you must look at both and track both"), the rules L10 · L11 · L12, the
// open item Q12 (top 3 vs top 5, due ~2026-09-15) and the locked item AHI · ALO · LHI · LLO.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function lit(n){ const m=src.match(new RegExp('^var '+n+'=(.*);$','m')); return JSON.parse(m[1]); }

ok(/@version\s+15\.(8[6-9]|9\d)/.test(src) && /var GPTS_VERSION='15\.(8[6-9]|9\d)';/.test(src), '0a v15.86 or later in both spots');

const file=JSON.parse(fs.readFileSync('learning/deflections/examples.json','utf8')); const seed=lit('LEARN_SEED');
ok(JSON.stringify(seed)===JSON.stringify(file), '1a LEARN_SEED equals examples.json');
const E5=file.examples.find(e=>e.id==='E005'), E6=file.examples.find(e=>e.id==='E006');
ok(E5 && E5.date==='2026-09-08' && E5.legs.filter(l=>!l.uncircled).length===6 && E5.legs.length===10 && E5.blind===null && /each should be counted as 1 deflection/.test(E5.his), '1b E005: six circled legs (+ four uncircled first-hour turns counted on his ruling of 2026-09-09, v15.88), taught (not scored), his words verbatim');
ok(E6 && E6.date==='2026-09-08' && E6.legs.filter(l=>!l.uncircled).length===4 && E6.legs.length===5 && E6.blind===null && /NQ1/.test(E6.chart), '1c E006: four circled NQ legs (+ the uncircled LOD at the PDL sweep, v15.88), taught');
ok(fs.existsSync('learning/deflections/img/E005.png') && fs.existsSync('learning/deflections/img/E006.png'), '1d both images on disk');
ok(E5.legs.filter(l=>l.kind==='LOD').length===1 && E5.legs.filter(l=>l.kind==='PULLBACK').length===9, '1e E005: nine pullback turns (five circled, four uncircled) and the LOD');
ok(E5.legs.every(l=>/SPXW|SPY/.test(l.node) && /\$\d+M|%/.test(l.data)), '1f every ES leg names the node and carries the record\'s numbers');
ok(E5.legs.some(l=>/ONL \/ LLO 7687\.50/.test(l.node) || /ONL and LLO/.test(l.data)) && E6.legs.some(l=>/PDC 29569/.test(l.node)) && E6.legs.some(l=>/POC 29512/.test(l.node)), '1g the liquidity levels are read at the taps (ONL/LLO at the ES King; the PDC and the POC at the NQ Kings)');
const C5=E5.legs.filter(l=>!l.uncircled);
ok(C5[5].factors.indexOf('new')>=0 && C5[5].factors.indexOf('stack')>=0 && C5[3].factors.indexOf('liq')>=0, '1h the LOD leg is new + stack; the flush leg carries liq (the circled legs, in order)');
ok(/HIS RULING 2026-09-09/.test(E5.open) && /tag other hours/.test(E5.open) && /defl/.test(E5.open) && E5.legs.filter(l=>l.uncircled).every(l=>/^H[12]$/.test(l.hour) && /UNCIRCLED/.test(l.data)), '1i (v15.88) E005.open carries his ruling on the uncircled turns; the m-legs are tagged H1 / H2 and say UNCIRCLED; the panel ledger\'s miss is still recorded');

ok(file.factors.some(f=>f.id==='liq' && /AHI/.test(f.what) && /LLO/.test(f.what) && /17:00–02:00 CT/.test(f.what)), '2a the LIQUIDITY LEVEL factor with the Asia / London names and hours');
const R=id=>file.rules.find(r=>r.id===id);
ok(R('L10') && R('L10').status==='PROPOSED' && R('L10').n===4 && /−γ KING IS NOT THE FLOOR/.test(R('L10').rule), '2b L10 proposed on four legs');
ok(R('L11') && R('L11').status==='PROPOSED' && R('L11').n===1 && /DECAYS BY THE TAP/.test(R('L11').rule), '2c L11 proposed, n=1, honest');
ok(R('L12') && R('L12').status==='PROPOSED' && R('L12').n===4 && /SWEPT BEFORE THE TURN/.test(R('L12').rule), '2d L12 proposed on four legs');
ok(R('L1').n===24 && R('L1').from.indexOf('E005')>=0 && R('L1').from.indexOf('E006')>=0 && R('L1').status==='CONFIRMED', '2e L1 grew to n=24 (six agree, three weak) and stays CONFIRMED', [R('L1').n, R('L1').agree, R('L1').weak]);
ok(R('L5').n===10 && R('L5').agree===9 && R('L9').n===12 && R('L7').n===31 && R('L7').weak===15 && /CLASS, NOT A RULE/.test(R('L7').rule), '2f L5 / L9 counts moved with the ten circles; L7 (v15.88) is the hour class over the 31 legs', [R('L5').n, R('L9').n, R('L7').n, R('L7').weak]);
ok(file.rules.length===12 && file.examples.length===6, '2g twelve rules, six examples');
const md=fs.readFileSync('learning/deflections/LEARNING.md','utf8');
ok(/### E005 —/.test(md) && /### E006 —/.test(md) && /\*\*L10 · /.test(md) && /\*\*L12 · /.test(md) && /LIQUIDITY LEVEL/.test(md), '2h LEARNING.md carries the two examples, the three rules and the factor');

const oq=fs.readFileSync('session-state/OPEN-QUESTIONS.md','utf8');
ok(/### Q12 · \*\*Top 3 or top 5/.test(oq) && /2026-09-15/.test(oq) && /keep it as an open item/.test(oq), '3a Q12: top 3 vs top 5, due ~2026-09-15, his words');
const li=fs.readFileSync('session-state/LOCKED-ITEMS.md','utf8');
ok(/AHI · ALO · LHI · LLO/.test(li) && /NOT BUILT/.test(li) && /Asia 17:00–02:00 CT/.test(li) && /London 02:00–08:30 CT/.test(li), '3b the Asia / London levels are a locked item with the proposed boundaries');
const tr=fs.readFileSync('design/TAP-RECORD.md','utf8');
ok(/AHI · ALO · LHI · LLO/.test(tr) && /4b · What his first ten circles taught/.test(tr) && /XG2/.test(tr), '3c the tap-record design carries the four levels, the lessons of the ten circles, the v15.85 labels');
const cl=fs.readFileSync('changelog/CHANGELOG.md','utf8'); ok(/^## v15\.86 /m.test(cl) && /E005/.test(cl) && /E006/.test(cl), '3d CHANGELOG v15.86');
const dc=fs.readFileSync('session-state/DECISIONS.md','utf8'); ok(/each should be counted as 1 deflection/.test(dc) && /ALO, AHI, LLO, LHI/.test(dc), '3e DECISIONS carries his training words');

console.log('test_v1586: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
