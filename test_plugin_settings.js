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
ok(/short cy = y;/.test(gp) && /textLJc\(\(short\)\(leftX \+ gpl::PILL_PAD\), y, tag\.c_str\(\)/.test(gp), '7 the depth pill is centred on y with its text (it used to sit low to match the low text)');
ok((gp.match(/textLJc\(c[12], p\.v,/g)||[]).length===2, '8 the tape strip (strike and %King) is centred on its bar on both rails');

// ---- (GP 0.73) the SPY strip carries the ES price in the dimmer ink; each strip is sized to its own text
ok(/textLJc\(\(short\)\(c1 \+ TW\.skW \+ TW\.spaceW\), p\.v, es, primary \? C_DIMES : C_GREY/.test(gp), '9 the ES price is drawn one space after the SPY strike, centred on the bar, in C_DIMES (grey on a sub-threshold row)');
ok(/measureOne\(strikesSpy, S, true, tw\[1\]\)/.test(gp) && /measureOne\(strikes, S, S\.book == 2, tw\[0\]\)/.test(gp), '10 the SPY rail (Both) and a single SPY rail show the ES price; the SPX rail does not');
ok(!/paneR - colW - 2|paneL \+ 2 \+ colW\)/.test(gp.replace(/anchor = \(short\)\(paneL \+ 2 \+ colW\);/,'')), '11 no line / band stops at "the other strip" by THIS strip\'s width any more — stripL / stripR, each strip\'s own');

// ---- (GP 0.74) Hide nodes under % hides the node on both rails; the King line keeps the Line style at any width
ok(/float ab = std::fabs\(s\.pct\);\s*if \(gpl::nodeHidden\(s\.pct, s\.king, S\.hideu\)\) continue;/.test(gp), '12 the bar loop (both rails render through it) skips a hidden node before anything of it is drawn');
ok(/if \(s\.since < 0\) continue;\s*if \(gpl::nodeHidden/.test(gp), '13 ...and it gets no band');
ok(/hlinePx\(a\.v, a\.h, b\.h, col, penOf\(style\), wpx\)/.test(gp) && /gpl::dashSegments\(lx, rx, ps == P_DOT \? 1 : 2, w\)/.test(gp), '14 every level line (the King included) goes through hlinePx, which draws a wide dotted / dashed line as segments');

// ---- (GP 0.75) three line families, each its own style; labels only; top nodes as lines, width by %King
ok(/PX\.lstyle = pc\+\+; setListParameter   \("King line style"/.test(gp), '15 the King has its own "King line style" row');
ok(/PX\.tnmode = pc\+\+/.test(gp) && /PX\.tnstyle= pc\+\+/.test(gp) && /PX\.tnwidth= pc\+\+/.test(gp) && /PX\.ifstyle= pc\+\+/.test(gp) && /PX\.magline= pc\+\+/.test(gp), '16 the 0.75 rows are all still declared');
ok(/"Solid;Dot;Dash;None \(labels only\)"/.test(gp) && /if \(gpl::styleDrawsLine\(style\)\) hlinePx/.test(gp), '17 the IF level style has "None (labels only)": the label draws, the line does not');
ok((gp.match(/false, ifs, 1, lp, S\)/g)||[]).length===6, '18 CW, PW, Flip, EM-H, EM-L and Mag all use the IF level style, not the King style');
ok(/lvl\[0\], kc, "", S\.extk, S\.lstyle, S\.klinew/.test(gp) && /hlinePx\(kp\.v, kx1, kx2, kc, penOf\(S\.lstyle\), S\.klinew\)/.test(gp), '19 both King lines use the King style at the King width');
{ const k=gp.indexOf('if (S.tnmode == 1) {'), blk=gp.slice(k, k+700);
  ok(k>0 && blk.indexOf('if (s.king) continue;')>0 && blk.indexOf('penOf(S.tnstyle), gpl::nodeLineWidth(s.pct, S.tnwidth == 1, S.klinew)')>0, '20 Lines mode: the top node style, 1 px or By %King, the King not drawn twice'); }

// ---- (GP 0.76) IF / King tags ON TOP of the strip row; symmetric pills
ok(/if \(wtag && tagTop\) \{ topW = wtag; topWc = wcol; wtag = 0; \}/.test(gp), '21 a CW / PW / MAG tag that fits above its row is NOT drawn beside the bar tip any more');
ok(/if \(s\.king && tagTop\) rn = "";/.test(gp), '22 the King name leaves the bar when it goes above the row; G / C / F stay in the bar');
ok(/gpl::tagRow\(c1, kw \+ dw \+ ww \+ gpl::BOLD_OVERRUN, TW\.spaceW, pw, leftLim, \(int\)paneR - 2\)/.test(gp) && /gpl::tagCentreY\(p\.v, S\.font\)/.test(gp), '23 the tag starts on the strike, pill one space after, 1 px above the row text, shifted left at the pane edge');
ok(/gpl::pillWidth\(getTextWidth/.test(gp) && /textLJc\(\(short\)\(leftX \+ gpl::PILL_PAD\)/.test(gp), '24 every pill: the same pad left and right of its text');
ok(/textLJc\(stripC1, \(short\)\(fp\.v - \(S\.font - 1\) \/ 2 - 2\), fl, C_FLIPC/.test(gp), '25 FLIP: tag above its tick on the strike column when the tape strip is on');

// ---- (GP 0.77) THE REGROUP: version 8, every row kept, a v7 instance is caught and reset to HIS settings
const setupSrc=gp.slice(gp.indexOf('int cppExtension::setup(void)'), gp.indexOf('return RTX_OK;', gp.indexOf('int cppExtension::setup(void)')));
const order=[...setupSrc.matchAll(/PX\.(\w+)\s*=\s*pc\+\+/g)].map(m=>m[1]);
const V7=['book','width','side','thick','filter','thresh','below','scale','cpos','cneg','cmid','kingcol','hideu','font','rank','type','rankpos','rankscope','kinglabel','kline','cw','pw','flip','em','extk','lstyle','lpos','roles','regime','panelpos','tapecols','lvllabels','spywidth','bands','bandh','klinew','tnmode','tnstyle','tnwidth','ifstyle','magline'];
ok(/setParameterVersion\(8\)/.test(gp), '26 parameter version 8');
ok(order.length===V7.length && V7.every(k=>order.includes(k)) && new Set(order).size===order.length, '27 every v7 row is still there, once — nothing removed, nothing added', order.length);
ok(order.join()==='book,side,width,spywidth,thick,font,tapecols,scale,filter,thresh,below,hideu,rank,rankpos,type,rankscope,kinglabel,roles,cpos,cneg,cmid,kingcol,kline,extk,lstyle,klinew,bands,tnmode,bandh,tnstyle,tnwidth,cw,pw,flip,magline,em,ifstyle,lpos,lvllabels,regime,panelpos', '28 the agreed section order (Book & layout, Nodes, Colours, King line, Top nodes, IF levels, Read panel)');
// his v7 values (09-19 dialog screenshot), by v7 position -> what each NEW slot reads on first load
const HIS={book:4,width:90,side:0,thick:2,filter:1,thresh:20,below:0,scale:0,cpos:0x41c3e3,cneg:0xaf3bc4,cmid:0x5c9a1f,kingcol:0,hideu:0,font:10,rank:1,type:1,rankpos:0,rankscope:0,kinglabel:3,kline:1,cw:1,pw:1,flip:1,em:1,extk:0,lstyle:1,lpos:0,roles:1,regime:1,panelpos:1,tapecols:1,lvllabels:1,spywidth:120,bands:0,bandh:3,klinew:2,tnmode:0,tnstyle:1,tnwidth:0,ifstyle:0,magline:0};
const readAs=k=>HIS[V7[order.indexOf(k)]];
ok(readAs('width')<20 && /w < 20 \|\| w > 1000/.test(gp), '29 his v7 instance is CAUGHT: the new SPX-width slot reads his old Side ('+readAs('width')+') -> scrambled() -> the one-time reset', readAs('width'));
const mig=gp.slice(gp.indexOf('void GammaProfile::migrateScrambled()'), gp.indexOf('int GammaProfile::parmsLoad'));
const want={'setListIndex(PX.thick, 2)':1,'setListIndex(PX.kinglabel, 3)':1,'checkBox(PX.em, true)':1,'setListIndex(PX.lstyle, 1)':1,'checkBox(PX.bands, false)':1,'setIntegerValue(PX.width, 90)':1,'setIntegerValue(PX.spywidth, 120)':1,'setListIndex(PX.panelpos, 1)':1,'setListIndex(PX.tnstyle, 1)':1};
ok(Object.keys(want).every(k=>mig.includes(k)), '30 the reset writes HIS settings (Medium, GPOC, EM on, King Dot, bands off, 90 / 120, Bottom-C, top node Dot)');
ok(order.every(k=>new RegExp('PX\\.'+k+'\\b').test(mig)), '31 the reset covers every row', order.filter(k=>!new RegExp('PX\\.'+k+'\\b').test(mig)));
ok(!/setLabelParameter\s*\(/.test(setupSrc), '32 no label rows in the dialog (they shift the numbering)');

// ---- (GP 0.78) the settings are checked and read on EVERY draw; the v7 repair has no once-gate
const gpfn=(name)=>{ const i=gp.indexOf(name); if(i<0) return ''; const b=gp.indexOf('{',i); let d=0; for(let k=b;k<gp.length;k++){ if(gp[k]==='{')d++; else if(gp[k]==='}'){ d--; if(d===0) return gp.slice(i,k+1); } } return ''; };
ok(/^int GammaProfile::draw\(void\)\s*\{\s*syncSettings\(\);/.test(gpfn('int GammaProfile::draw(void)')), '33 GP draw() syncs the settings FIRST (the DS 0.16 pattern)');
ok(['parmsLoad(void)','parmsApply(void)','parmsUpdt(unsigned int)'].every(n=>/\{\s*syncSettings\(\);\s*return RTX_OK;\s*\}/.test(gpfn('int GammaProfile::'+n))), '34 all three GP callbacks go through syncSettings');
ok(!/if \(repaired\) return;/.test(gpfn('void GammaProfile::migrateScrambled()')) && /migrateScrambled\(\);/.test(gpfn('void GammaProfile::syncSettings()')), '35 the v7 repair re-checks on every sync — no once-gate (his second open shipped the old values past it)');
ok(/if \(f < 1 \|\| f > 200\) return;/.test(gpfn('void GammaProfile::syncSettings()')), '36 unpopulated values (font 0) are skipped, retried next draw');

console.log('test_plugin_settings: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
