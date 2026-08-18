// (v10.50) ZONE ROW — the deflection-zone building blocks: colored `g` by polarity,
// ✓/✗ reaction, S/Q/V confluence order, the ACTIVITY tag port, and the decision-fold
// take/pass gate. Tests the standalone helpers deflZonesBlock composes.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function grab(name){ var i=src.indexOf('function '+name+'('); if(i<0){ console.error('MISSING '+name); process.exit(1); }
  var d=0,st=false,j=i; for(;j<src.length;j++){var c=src[j]; if(c==='{'){d++;st=true;} else if(c==='}'){d--; if(st&&d===0){j++;break;}}} return src.slice(i,j); }
let p=0,f=0; function ok(c,m,g){ if(c){p++;} else {f++; console.log('  FAIL: '+m+(g!==undefined?' -> '+g:''));} }

global.PAL={longAccent:'#2ec27e',shortAccent:'#f0616d',blue:'#5aa9ff',amber:'#f2b45a',gold:'#f2cc60',sub:'#8b98a9',line:'#21262d',ink:'#e6edf3'};
global.GPOL_TIP='What kind of node?';
global.DEFLECT_CONFIRM=2; global.EP_DEFL_HANDOFF=3;
global.STATE={SPY:{setups:{}}};
eval(['zonePolCol','zoneGGlyph','zoneConfHtml','nodeBOchip','nodeActivityWord'].map(grab).join('\n'));

// ---- colored `g` by polarity: yellow (+γ) / purple (−γ) ----
ok(zonePolCol({pos:true})==='#f2cc60','+γ g is YELLOW (gold)', zonePolCol({pos:true}));
ok(zonePolCol({pos:false})==='#a371f7','−γ g is PURPLE', zonePolCol({pos:false}));
ok(/#f2cc60/.test(zoneGGlyph({pos:true})) && />g<\/span>/.test(zoneGGlyph({pos:true})),'g glyph is a colored g');
ok(/#a371f7/.test(zoneGGlyph({pos:false})),'−γ g glyph is purple');

// ---- confluence S/Q/V order, ✓ / ✗ / – ----
ok(zoneConfHtml({q:true,v:true,s:true}).indexOf('S✓')<zoneConfHtml({q:true,v:true,s:true}).indexOf('Q✓'),'order is S before Q');
ok(/Q✓ V✓/.test(zoneConfHtml({q:true,v:true,s:true})),'Q and V ✓ when they agree');
ok(/S✓/.test(zoneConfHtml({q:true,v:true,s:true})),'S✓ when SPXW agrees');
ok(/S✗/.test(zoneConfHtml({q:false,v:false,s:false})),'S✗ when SPXW disagrees');
ok(/S–/.test(zoneConfHtml({q:true,v:true,s:null})),'S– when no SPXW header captured');
ok(/Q✗ V✗/.test(zoneConfHtml({q:false,v:false,s:null})),'Q/V ✗ when they do not agree');

// ---- ACTIVITY tag port (Pull / Push / Defl / BO·FT / BOw) ----
ok(nodeActivityWord('SPY',{ep:{state:'Pull'},k:773})==='Pull','Pull tag');
ok(nodeActivityWord('SPY',{ep:{state:'Push'},side:'below',k:773})==='Push ↑','Push off a node BELOW price = ↑ (bounce)');
ok(nodeActivityWord('SPY',{ep:{state:'Push'},side:'above',k:773})==='Push ↓','Push off a node ABOVE price = ↓');
ok(nodeActivityWord('SPY',{ep:{},deflection:{dir:1,bars:1},k:773})==='Defl ↑','fresh Defl ↑ takes precedence');
ok(nodeActivityWord('SPY',{ep:{},k:773})==='','no activity -> empty');

// BO chip from STATE.setups: live setup at the strike, stage BO -> BOw, FT -> BO·FT
STATE.SPY.setups={ s1:{strike:773, stage:'BO', updated:10} };
ok(nodeBOchip('SPY',773)==='BOw','initial break -> BOw', nodeBOchip('SPY',773));
STATE.SPY.setups={ s1:{strike:773, stage:'FT', updated:10} };
ok(nodeBOchip('SPY',773)==='BO·FT','follow-through -> BO·FT', nodeBOchip('SPY',773));
STATE.SPY.setups={ s1:{strike:773, stage:'FT', outcome:'FAILED', updated:10} };
ok(nodeBOchip('SPY',773)==='','terminal setups are skipped', nodeBOchip('SPY',773));

// ---- decision-fold + take/pass gate live in deflZonesBlock source ----
var dz=grab('deflZonesBlock');
ok(/entry '\+fmtNum\(L\.k\)/.test(dz),'r3 folds the decision frame (entry = the node)');
ok(/gateOK=\(ng\.grade==='A'\|\|ng\.grade==='B'\) && !\/stand aside\//.test(dz),'take/pass gated: grade>=B AND cell != stand aside');
ok(/tgtAir/.test(dz) && /invalAir/.test(dz),'air tag computed for BOTH tgt and inval');
ok(!/· px /.test(dz) && !/sparkline/.test(dz),'header drops `· px`; no sparkline');
ok(!/Dir grade<\/b> = /.test(dz) && !/Node grade<\/b> = /.test(dz),'gray legend line dropped');
ok(/— SPY '\+fmtNum\(px\)\+' —/.test(dz),'price divider between above/below');

console.log('test_zone_row: '+p+' passed, '+f+' failed');
process.exit(f?1:0);
