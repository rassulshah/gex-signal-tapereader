// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function lvlSpanFmt(x){
  if(x==null) return '–';
  try{ if(typeof dispIsFut==='function' && dispIsFut()) return (typeof futMark==='function'?futMark():'')+String(Math.round(mul(x, dispR()))); }catch(e){}
  return fmtSpan(x);
}
