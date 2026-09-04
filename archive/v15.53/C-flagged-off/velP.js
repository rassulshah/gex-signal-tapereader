// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function velP(v){
  if(typeof v!=='number' || !isFinite(v)) return { txt:'\u2014', cls:'' };
  if(v===0) return { txt:'0%', cls:'g3flat' };
  var a=Math.abs(v), t=(a>=100)?Math.round(a)+'':(a>=10?a.toFixed(0):a.toFixed(1));
  return { txt:((v>0)?'+':'\u2212')+t+'%', cls:(v>0)?'g3up':'g3dn' };
}
