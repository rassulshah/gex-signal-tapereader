// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function esTick(x){
  if(typeof x!=='number' || !isFinite(x)) return null;
  var q=Math.round(x*4)/4;
  // (v14.0, user-directed) a trailing .00 is noise — 7709.00 reads as 7709. A REAL tick keeps its
  // decimals: 7706.50 stays 7706.50, distances like -22.75 are untouched.
  var s=q.toFixed(2);
  return (s.slice(-3)==='.00') ? s.slice(0,-3) : s;
}
