// (v10.50) LAYOUT — single-column dashboard. The two-column King-console layout was
// retired (v10.44); this test was stale. Updated to the current render: header cluster →
// drift → READ → deflZonesBlock (the single ladder) → footer. deflectionBlock() is RETIRED
// from the live render (kept for Analysis only).
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
function ok(c,m){ if(c){pass++; console.log('PASS '+m);} else {fail++; console.log('FAIL '+m);} }
function grab(name){ var i=src.indexOf('function '+name+'('); if(i<0) return '';
  var d=0,st=false,j=i; for(;j<src.length;j++){var c=src[j]; if(c==='{'){d++;st=true;} else if(c==='}'){d--; if(st&&d===0){j++;break;}}} return src.slice(i,j); }

// single-column wrapper (the two-col wrapper is gone)
ok(/id="gpts-1col"/.test(src),                       'single-column wrapper exists');
ok(!/id="gpts-2col"/.test(src),                      'legacy two-col wrapper removed');

// render order: header → drift → READ → accumBlock (zones)
var iHdr=src.indexOf('html+=kingHeaderBlock()');
var iRead=src.indexOf("html+=readBlock44(");   // (v10.55 PART G) the READ is keyed by the ACTIVE underlying, not the literal 'SPY'
var iAcc=src.indexOf('html+=accumBlock()');
ok(iHdr>0 && iRead>iHdr,                              'header cluster renders before the READ');
ok(iAcc>iRead,                                        'zone ladder (accumBlock) renders after the READ');

// the single ladder IS deflZonesBlock, reached via nodeMapBlock's early return
var nmb=grab('nodeMapBlock');
ok(/deflZonesBlock\(sym\)/.test(nmb) && /return __zonesEarly/.test(nmb), 'nodeMapBlock returns deflZonesBlock (the single ladder)');
ok(/function deflZonesBlock/.test(src),              'deflZonesBlock is defined');

// legacy "Deflections" section RETIRED from the live render (accumBlock no longer calls it)
var acc=grab('accumBlock');
ok(!/html\+=deflectionBlock\(\)/.test(acc),          'deflectionBlock() NOT rendered inside accumBlock');
ok(/function deflectionBlock/.test(src),             'deflectionBlock kept defined (Analysis/history only)');

// (v10.52) footer = the four PIPELINE stages + version. The v10.50 feed/vex/rec health
// dots were replaced: they could all be green while the day's data never left the
// browser. Feed/vex liveness now lives in the `rec` stage hover.
var foot=grab('feedStatusHtml');
ok(/data-pipe="'\+sg\.key\+'"/.test(foot) && /pipeStages\(\)/.test(foot) && /pipeColor\(sg\.state\)/.test(foot),
   'footer renders the pipeStages() strip with a coloured dot per stage');
ok(/>v10\.57<\/span>/.test(foot),                  'footer keeps the version at the right (10.56)');
ok(/white-space:nowrap/.test(foot),                 'footer strip is nowrap \u2014 one line at 250px');
ok(/\uD83D\uDCBE/.test(foot) && /\uD83D\uDCC1/.test(foot), 'footer keeps the export and folder-pick buttons');
ok(/feedLive\?'live':'waiting'/.test(foot) && /vexLive\?'capturing':'waiting'/.test(foot),
   'feed/vex health is preserved inside the rec hover (no signal dropped)');

// sync-gate banner stays ABOVE the single column (full width)
var gateIdx=src.indexOf('html+=syncBannerHtml'); var colIdx=src.indexOf('id="gpts-1col"');
ok(gateIdx>0 && colIdx>0 && gateIdx<colIdx,          'sync banner stays above the column');

// analysis view unaffected (its scroll container is separate)
ok(/gpts-analysis-scroll/.test(src),                 'analysis tab container untouched');

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
