// (v10.54) ZONE ROW — the deflection-zone building blocks, plus the four things v10.54
// changed about the in-play card:
//   · ABSOLUTE VALUE at the boundary: a −85% node is a real magnet. It used to arrive
//     with a NEGATIVE pct, so every magnitude test downstream (Flr/Ceil selection,
//     ★Mag, the zone sort) read the heaviest −γ walls on the board as the weakest.
//   · R:R on row 3: rr = |tgt−k| / |k−inval|, rendered, and below 2:1 the decision text
//     itself becomes "skip · R:R X:1 (below the 3:1 floor)".
//   · The take/pass buttons are gated on the DECISION CELL, not the node grade, and are
//     hidden entirely below 2:1.
//   · IN PLAY MEANS IN CONTACT (DEFLECT_ZONE = 0.50). Nothing in contact = "watching —
//     not in contact", no frame, no buttons.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function grab(name){ var i=src.indexOf('function '+name+'('); if(i<0){ console.error('MISSING '+name); process.exit(1); }
  var d=0,st=false,j=i; for(;j<src.length;j++){var c=src[j]; if(c==='{'){d++;st=true;} else if(c==='}'){d--; if(st&&d===0){j++;break;}}} return src.slice(i,j); }
let p=0,f=0; function ok(c,m,g){ if(c){p++;} else {f++; console.log('  FAIL: '+m+(g!==undefined?' -> '+g:''));} }

global.PAL={longAccent:'#2ec27e',shortAccent:'#f0616d',blue:'#5aa9ff',amber:'#f2b45a',gold:'#f2cc60',sub:'#8b98a9',line:'#21262d',ink:'#e6edf3'};
global.GPOL_TIP='What kind of node?';
global.DEFLECT_CONFIRM=2; global.EP_DEFL_HANDOFF=3;
global.STATE={SPY:{setups:{}}};
global.DEFLECT_ZONE=0.50;
global.FRAME_HALF=0.25; global.FRAME_FALLBACK=0.5;
global.RR_FLOOR=3; global.RR_MIN=2;
global.FLRCEIL_MIN_PCT=15; global.FLRCEIL_EDGE_PCT=40; global.FLRCEIL_FAR=6;
global.fmtNum=function(x){ return (Math.round(x*100)/100).toString(); };
eval(['zonePolCol','zoneGGlyph','zoneConfHtml','nodeBOchip','nodeActivityWord',
      'frameRR','rrText','zoneMeaningful','zoneRole','inPlayBand'].map(grab).join('\n'));

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

// ---- R:R is pure geometry off the numbers already on the row ----
ok(frameRR({k:773, tgt:776, inval:772})===3, 'R:R 3 = 3 up / 1 risk', frameRR({k:773,tgt:776,inval:772}));
ok(frameRR({k:773, tgt:773.7, inval:772})===0.7, 'R:R below 1 is reported honestly', frameRR({k:773,tgt:773.7,inval:772}));
ok(frameRR({k:773, tgt:776, inval:773})===null, 'zero risk = no R:R, never Infinity');
ok(frameRR({k:773, tgt:null, inval:772})===null && frameRR(null)===null, 'a missing leg = no R:R');
ok(frameRR({k:776, tgt:772, inval:777})===4, 'a DOWNWARD frame measures the same way', frameRR({k:776,tgt:772,inval:777}));
ok(rrText(2.4)==='R:R 2.4:1', 'rendered as R:R 2.4:1', rrText(2.4));

// ---- IN PLAY MEANS IN CONTACT: the band is DEFLECT_ZONE, not 1.20 strikes ----
ok(inPlayBand()===0.50, 'the in-play band is DEFLECT_ZONE (0.50), the same band a TAP uses', inPlayBand());
ok(/INPLAY_BAND=\(typeof DEFLECT_ZONE/.test(src), 'the constant is derived from DEFLECT_ZONE in source');
var ipz=grab('inPlayZoneOf');
ok(/inContact:true/.test(ipz) && /inContact:false/.test(ipz), 'inPlayZoneOf reports whether anything is actually in contact');
ok(/watch/.test(ipz), '...and returns the nearest edge only as a level to WATCH');

// ---- ABSOLUTE VALUE AT THE BOUNDARY ----
var fss=grab('futureStructureSummary');
ok(/pct:\(function\(\)\{ var v=\(typeof tapePct==='number'\)\?tapePct:w\.pct; return \(typeof v==='number'\)\?Math\.abs\(v\):v; \}\)\(\)/.test(fss),
   'futureStructureSummary assigns row.pct = Math.abs(...)');
ok(/pos:w\.pos/.test(fss), '...and polarity still lives ONLY in row.pos');
ok(/pctSigned/.test(fss), '...with the signed value kept beside it, not thrown away');
// a −85 node is a magnet: with abs() it clears every magnitude threshold downstream
var neg={ k:770, pct:Math.abs(-85), pos:false, side:'below' };
ok(neg.pct>=FLRCEIL_MIN_PCT && neg.pct>=FLRCEIL_EDGE_PCT, 'a −85% node clears the Flr/Ceil magnitude thresholds', neg.pct);
ok(zoneMeaningful({isStrongMag:true, pct:85}), '...and can be a ★Mag');
ok(zoneMeaningful({isCeil:true}) && zoneMeaningful({isFlr:true}), '...or a Ceil / Flr');
ok(zoneRole({isCeil:true})==='Ceil' && zoneRole({isFlr:true})==='Flr' && zoneRole({isStrongMag:true})==='Mag',
   '...and is labelled Ceil / Flr / Mag like any other magnet');
ok(zonePolCol({pos:false})==='#a371f7', '...while STILL rendering as −γ (purple g), because pos kept the polarity');

// ---- decision-fold, R:R and the gates live in deflZonesBlock source ----
var dz=grab('deflZonesBlock');
ok(/entry '\+fmt(Num|Lvl)\(L\.k\)/.test(dz),'r3 folds the decision frame (entry = the node; v10.55 formats it through fmtLvl so a futures chart reads in its own points)');
ok(/rr=frameRR\(fr\)/.test(dz) && /rrText\(rr\)/.test(dz), 'row 3 computes and RENDERS the R:R');
ok(/thin=\(rr!=null && rr<RR_MIN\)/.test(dz), 'below RR_MIN the row is flagged thin');
ok(/'skip · '\+rrText\(rr\)\+' \(below the '\+RR_FLOOR\+':1 floor\)'/.test(dz),
   "below 2:1 the decision text reads \"skip · R:R X:1 (below the 3:1 floor)\"");
ok(/gateOK = inContact && !thin && !\/stand aside\|skip\|wait\|watching\//.test(dz),
   'take/pass gated on the DECISION CELL (not the node grade), and hidden below 2:1');
ok(!/ng\.grade==='A'\|\|ng\.grade==='B'/.test(dz), '...the old node-grade gate is gone');
ok(/watching — not in contact/.test(dz), 'nothing in contact -> the card says "watching — not in contact"');
ok(/frTxt='nearest zone '/.test(dz), '...and shows the distance instead of a frame');
ok(/tgtAir/.test(dz) && /invalAir/.test(dz),'air tag computed for BOTH tgt and inval');
ok(!/· px /.test(dz) && !/sparkline/.test(dz),'header drops `· px`; no sparkline');
ok(!/Dir grade<\/b> = /.test(dz) && !/Node grade<\/b> = /.test(dz),'gray legend line dropped');
ok(/— '\+divLab\+' '\+fmt(Num|Lvl)\(px\)\+' —/.test(dz),'price divider between above/below (v10.55 labels the DISPLAYED instrument)');
// the frame vocabulary stays descriptive: no order words anywhere in the row
ok(!/\b(buy|sell|long|short|stop loss|position size)\b/i.test(dz), 'row 3 carries no order words');

console.log('test_zone_row: '+p+' passed, '+f+' failed');
process.exit(f?1:0);
