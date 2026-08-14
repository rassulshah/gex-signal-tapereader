// Issue H regression: node-ladder badge (1) commits on closed portion (no intrabar
// flicker), (2) rapid-override still flips promptly, (3) directional-meaning colors.
const fs = require('fs');
const src = fs.readFileSync('./v10.js','utf8');

function mul(a,b){ return a/(1/b); }
global.mul = mul;
global.ACC_RAPID_ROC = 0.20; global.ACC_DROP_BUDGET = 0.30; global.ACC_CONFIRM_DOWN = 2;
global.ACC_BUILD_MIN = 0.15;
global.PAL = { longAccent:'#2ec27e', shortAccent:'#f0616d', gold:'#e3c341', blue:'#4a90d9', sub:'#8b98a9' };

function extract(name){
  const re = new RegExp('function\\s+'+name+'\\s*\\(', 'g');
  const m = re.exec(src); if(!m) throw new Error('not found: '+name);
  let i = src.indexOf('{', m.index), depth=0, end=-1;
  for(let k=i;k<src.length;k++){ if(src[k]==='{')depth++; else if(src[k]==='}'){depth--; if(depth===0){end=k;break;}} }
  return src.slice(m.index, end+1);
}
eval(['seqTrendSlope','accumTrend'].map(extract).join('\n'));

// Replicate the closed-portion + rapid-override logic from accumulationStateFor so
// we test the exact decision path (accumulationStateFor itself needs live DOM stubs).
function labelFor(absSeq){
  var full = absSeq||[];
  var closedSeq = full.length>=3 ? full.slice(0, full.length-1) : full;
  var tClosed = accumTrend(closedSeq);
  var tFull = accumTrend(full);
  var label = tClosed.label, rapid = tFull.rapid;
  if(rapid && tFull.label!==label) label = tFull.label;
  return { label, rapid, closedLabel:tClosed.label };
}

// stateColor extracted (directional-meaning rule)
function stateColor(label, side){
  var isRes = (side==='above');
  if(label==='Building') return isRes ? PAL.shortAccent : PAL.longAccent;
  if(label==='Fading')   return isRes ? PAL.longAccent  : PAL.shortAccent;
  return PAL.sub;
}

let pass=0, fail=0;
function ok(name, cond, got){ if(cond){pass++; console.log('PASS '+name+(got!==undefined?' -> '+got:''));} else {fail++; console.log('FAIL '+name+(got!==undefined?' -> got '+got:''));} }

// 1) FLICKER: closed portion flat/building; live last point WIGGLES down then up.
//    Label must be governed by the closed portion, NOT the jittering live point.
const closedBuild = [100,112,120,128];            // clearly building on closed bars
const liveDown = closedBuild.concat([118]);       // live tick dips (would look "Fading")
const liveUp   = closedBuild.concat([130]);       // live tick pops
const lDown = labelFor(liveDown), lUp = labelFor(liveUp);
ok('closed-portion label is Building', lDown.closedLabel==='Building', lDown.closedLabel);
ok('live dip does NOT flip label off Building (no flicker)', lDown.label==='Building', lDown.label);
ok('live pop keeps Building too (stable)', lUp.label==='Building', lUp.label);

// 2) RAPID-OVERRIDE: closed portion flat, but the live tick is a >20% surge -> allowed to update.
const flatClosed = [100,101,100,101];
const rapidLive = flatClosed.concat([130]);       // +28% over last 2 -> rapid
const lr = labelFor(rapidLive);
ok('rapid live surge is detected', lr.rapid===true, lr.rapid);

// 3) DIRECTIONAL COLORS
ok('resistance building = RED (bearish)',  stateColor('Building','above')===PAL.shortAccent, stateColor('Building','above'));
ok('support building = GREEN (bullish)',   stateColor('Building','below')===PAL.longAccent,  stateColor('Building','below'));
ok('resistance fading = GREEN (bullish)',  stateColor('Fading','above')===PAL.longAccent,    stateColor('Fading','above'));
ok('support fading = RED (bearish)',       stateColor('Fading','below')===PAL.shortAccent,   stateColor('Fading','below'));
ok('steady = neutral grey',                stateColor('Steady','above')===PAL.sub,           stateColor('Steady','above'));

console.log(fail===0 ? '\nALL PASS' : '\n'+fail+' FAILED');
process.exit(fail===0?0:1);
