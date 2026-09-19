// (DS 0.16) THE SAVED SETTINGS REACH THE DRAW. Operator, 2026-09-19: Day Stats opened at the top-left although its dialog said
// Top-center; re-selecting Top-center + Apply fixed it until the next reopen. IRT can call parmsLoad before a restored
// instance's values exist (the font reads 0), the guard skips readSettings, and the constructor's defaults draw until an Apply.
// Pins: DayStats reads its settings through ONE guarded routine, from all three callbacks AND at the top of every draw.
const fs=require('fs');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
const ds=fs.readFileSync('plugin/DayStats.cpp','utf8');
const fn=(name)=>{ const i=ds.indexOf(name); if(i<0) return ''; const b=ds.indexOf('{',i); let d=0; for(let k=b;k<ds.length;k++){ if(ds[k]==='{')d++; else if(ds[k]==='}'){ d--; if(d===0) return ds.slice(i,k+1); } } return ''; };
const draw=fn('int DayStats::draw(void)'), sync=fn('void DayStats::syncSettings()');
ok(/^int DayStats::draw\(void\)\s*\{\s*syncSettings\(\);/.test(draw), '1 draw() reads the saved settings FIRST — the Corner applies on the first draw after a reopen, no Apply needed', draw.slice(0,90));
ok(/getIntegerValue\(PX\.font\)/.test(sync) && /readSettings\(cfg\)/.test(sync) && /p >= 1 && p <= 200/.test(sync), '2 syncSettings keeps the font guard (values not yet populated -> skip, retry next draw)', sync.slice(0,160));
ok(['parmsLoad(void)','parmsApply(void)','parmsUpdt(unsigned int)'].every(n=>/\{\s*syncSettings\(\);\s*return RTX_OK;\s*\}/.test(fn('int DayStats::'+n))), '3 all three dialog callbacks go through the same routine (one place to get it right)');
ok(!/int DayStats::parms\w+\([^)]*\)\s*\{[^}]*readSettings\(cfg\)/.test(ds), '4 no callback reads the settings on its own any more');
// ---- (GP 0.73) labels anchored at a price are CENTRED on it. IRT's rect text lands ~0.45 x font below y (measured, GP 0.55);
// at Medium thickness the strip's price and %King sat visibly under their bars (operator 2026-09-19, both rails).
const gp=fs.readFileSync('plugin/GammaProfile.cpp','utf8');
const lowOnPrice=(gp.match(/\btext(LJ|RJ)\([^;]*\bp\.v\b[^;]*\);/g)||[]);
ok(lowOnPrice.length===0, '5 no label is drawn at a bar\'s price (p.v) with the low-baseline helpers — every one goes through textLJc / textRJc', lowOnPrice.slice(0,3));
ok(/static short lowShift\(int sz\) \{ return \(short\)\(sz \* 0\.45f \+ 0\.5f\); \}/.test(gp), '6 the shift is the measured 0.45 x font (the offset the depth pill was built around in 0.55)');
ok(/short cy = y;/.test(gp) && /textLJc\(\(short\)\(leftX \+ 6\), y, tag\.c_str\(\)/.test(gp), '7 the depth pill is centred on y with its text (it used to sit low to match the low text)');
ok((gp.match(/textLJc\(c[12], p\.v,/g)||[]).length===2, '8 the tape strip (strike and %King) is centred on its bar on both rails');

// ---- (GP 0.73) the SPY strip carries the ES price in the dimmer ink; each strip is sized to its own text
ok(/textLJc\(\(short\)\(c1 \+ TW\.skW \+ TW\.spaceW\), p\.v, es, primary \? C_DIMES : C_GREY/.test(gp), '9 the ES price is drawn one space after the SPY strike, centred on the bar, in C_DIMES (grey on a sub-threshold row)');
ok(/measureOne\(strikesSpy, S, true, tw\[1\]\)/.test(gp) && /measureOne\(strikes, S, S\.book == 2, tw\[0\]\)/.test(gp), '10 the SPY rail (Both) and a single SPY rail show the ES price; the SPX rail does not');
ok(!/paneR - colW - 2|paneL \+ 2 \+ colW\)/.test(gp.replace(/anchor = \(short\)\(paneL \+ 2 \+ colW\);/,'')), '11 no line / band stops at "the other strip" by THIS strip\'s width any more — stripL / stripR, each strip\'s own');

console.log('test_plugin_settings: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
