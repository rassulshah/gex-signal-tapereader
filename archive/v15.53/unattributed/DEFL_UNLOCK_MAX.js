// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
var DEFL_UNLOCK_MAX = 25;    // cap for the auto-tuned unlock sample size

// Classify a confirmed deflection at level L into a setup label + confluence chips.
// dir: +1 = deflected UP off the node (bounce), -1 = deflected DOWN off it (rejection).
function classifyDeflection(sym, L, dir){
  var chips=[];
