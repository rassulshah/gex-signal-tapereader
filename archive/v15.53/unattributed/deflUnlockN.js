// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function deflUnlockN(perDayCount){
  if(!perDayCount || perDayCount<=0) return DEFL_UNLOCK_MIN;
  var n=Math.round(perDayCount*3);               // ~3 trading days' worth
  return Math.max(DEFL_UNLOCK_MIN, Math.min(DEFL_UNLOCK_MAX, n));
}
