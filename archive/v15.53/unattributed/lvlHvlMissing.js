// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function lvlHvlMissing(L){
  if(!L || (L.hvl && L.hvl.k!=null)) return null;
  if(L.nNat==null || L.nNat===0) return 'no native nodes read';
  return 'no flip in this book — cumulative γ never changes sign'+(L.regime==='posGamma'?' (net +γ throughout)':(L.regime==='negGamma'?' (net −γ throughout)':''));
}
