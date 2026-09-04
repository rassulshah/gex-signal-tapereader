// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function lvlRows(L){
  if(!L) return [];
  var rows=[];
  ['cr','cr0','hvl','mag','ps0','ps'].forEach(function(id){ var v=L[id]; if(v && v.k!=null) rows.push({id:id, k:v.k, dist:v.dist, distPct:v.distPct, src:v.src}); });
  rows.sort(function(a,b){ return b.k-a.k; });
  return rows;
}
