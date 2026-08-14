// (v10.41) TWO-COLUMN DASHBOARD — layout guards.
// LEFT = King (badge + analyzer + path). RIGHT = Deflections + Node Map.
// Responsive: flex-wrap, 1 1 300px per column -> narrow panel stacks (old
// behavior), wide panel goes side-by-side. One-time width migration to 690px.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
function ok(c,m){ if(c){pass++; console.log('PASS '+m);} else {fail++; console.log('FAIL '+m);} }

ok(/id="gpts-2col"/.test(src),                       'two-col wrapper exists');
ok(/flex-wrap:wrap/.test(src.match(/id="gpts-2col"[^>]*/)[0]), 'wrapper is flex-wrap (stacks when narrow)');
ok(/gpts-2col[^]*?flex:1 1 300px[^]*?kingBlock\(\)/.test(src), 'LEFT column holds kingBlock');
// right column: opens after kingBlock's div, closes after accumBlock
ok(/kingBlock\(\)\+'<\/div>'\+\s*'<div style="flex:1 1 300px;min-width:0">'/.test(src), 'RIGHT column opens beside King');
ok(/accumBlock\(\);[^]*?close right column \+ two-col wrapper/.test(src), 'RIGHT column closes after accumBlock (Deflections+NodeMap)');
// deflections live inside accumBlock -> right column by construction
ok(/html\+=deflectionBlock\(\);/.test(src),          'deflections render inside accumBlock (right col)');
// suppression panel + feed status remain OUTSIDE the columns (full width)
var gateIdx=src.indexOf('html+=outOfSyncBlock'); var colIdx=src.indexOf('id="gpts-2col"');
ok(gateIdx>0 && colIdx>0 && gateIdx<colIdx,          'sync-gate suppression stays full-width ABOVE the columns');
// width migration
ok(/gpts_2col_migr_v1/.test(src),                    'one-time width migration flag present');
ok(/wNow<620/.test(src) && /690px/.test(src),        'migrates narrow saved widths to 690px');
ok(/keep any later user resize sacred/.test(src),    'migration documented as one-time (user resizes respected after)');
ok(/width:'690px',/.test(src),                       'fresh-install default width is 690px');
// analysis view unaffected (its scroll container is separate)
ok(/gpts-analysis-scroll/.test(src),                 'analysis tab container untouched');

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
