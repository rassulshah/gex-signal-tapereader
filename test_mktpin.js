// ============================================================================================
// test_mktpin.js — THE PANEL READS THE SPX BOOK UNLESS TOLD OTHERWISE.
//
// Operator, 2026-08-30: "put a guard on this application so it always shows spxw by default,
// unless another market in the settings is selected."
//
// ⚠⚠ WHY THIS IS A GUARD AND NOT A PREFERENCE. Everything measured in this project stands on the
// SPX/SPY book: the 284-session HOD/LOD corpus, the GREEN/RED rule (76% on 80% of days), the
// deflection geometry calibrated against his own charts, the last-session latch. Following the
// CHART meant one click onto a QQQ or ES tab silently swapped the panel onto a book with none of
// that behind it — which is precisely how SPX-scale strikes ended up drawn against a 716 QQQ price
// and collapsed into a nine-pixel band on 2026-08-30.
// ============================================================================================
const fs=require('fs');
const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
const grab=(n)=>{ const i=src.indexOf('function '+n+'('); if(i<0) return ''; let d=0,st=false;
  for(let j=i;j<src.length;j++){ const c=src[j];
    if(c==='{'){d++;st=true;} else if(c==='}'){d--; if(st&&d===0) return src.slice(i,j+1);} } return ''; };

ok(/\n\s*mkt: 'SPX',/.test(src), 'm1 the default is SPX, not auto');
const AS=grab('activeSym').replace(/\/\/[^\n]*/g,'');
ok(/CFG && CFG\.mkt/.test(AS), 'm2 activeSym reads the setting');
ok(/if\(m==='SPX'\) return 'SPY';/.test(AS), "m3 SPX resolves to the SPY underlying this panel reads");
ok(/if\(m==='QQQ'\) return 'QQQ';/.test(AS), 'm4 QQQ can be pinned too');
// ⚠ the pin must be checked BEFORE the chart fallback, or it never takes effect
ok(AS.indexOf("CFG.mkt")>0 && AS.indexOf("CFG.mkt") < AS.indexOf('FUTMODE'),
   'm5 ...and the pin is read BEFORE the follow-the-chart fallback');
ok(/FUTMODE && FUTMODE\.underlying==='QQQ'/.test(AS), 'm6 auto still follows the chart');

// ⚠⚠ A SILENT PIN IS WORSE THAN NO PIN: every number would look like the chart's.
const MP=grab('mktPinned').replace(/\/\/[^\n]*/g,'');
ok(!!MP, 'm7 mktPinned exists');
ok(/if\(m==='auto'\) return null;/.test(MP), 'm8 ...auto is never a pin, so it never warns');
ok(/pin!==chart/.test(MP), 'm9 ...and it only reports when the pin DISAGREES with the chart');
ok(/book \(chart: /.test(src), 'm10 the disagreement is on the FACE, naming both books');

// persistence + the control
ok(/o\.mkt==='SPX'\|\|o\.mkt==='QQQ'\|\|o\.mkt==='auto'/.test(src), 'm11 the setting persists, validated');
ok(/class="gpts-mkt"/.test(src), 'm12 there is a Market control in settings');
ok(/CFG\.mkt=mkx\.value; saveCfg\(\)/.test(src), 'm13 ...wired to save and re-render');
ok(/SPX \/ SPY \(default\)/.test(src), 'm14 ...and the default option says it is the default');

console.log('test_mktpin: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
