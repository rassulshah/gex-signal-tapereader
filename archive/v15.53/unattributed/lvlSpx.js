// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function lvlSpx(x){
  if(x==null) return null;
  try{ var R=spxRatio(); if(!R.r) return null; return Math.round(x*R.r); }catch(e){ return null; }
}
