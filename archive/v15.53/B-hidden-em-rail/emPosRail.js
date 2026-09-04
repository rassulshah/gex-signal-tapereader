// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function emPosRail(B, v, RB){
  try{
    RB=RB||emRailBounds(B);
    var span=RB.hi-RB.lo; if(!(span>0)) return 0;
    return Math.max(0, Math.min(1, (v-RB.lo)/span))*100;
  }catch(e){ return 0; }
}
