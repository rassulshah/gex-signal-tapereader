// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function deflGrade(rate){
  if(rate>=75) return {g:'A+',col:PAL.longAccent};
  if(rate>=68) return {g:'A', col:PAL.longAccent};
  if(rate>=58) return {g:'B', col:PAL.blue};
  if(rate>=45) return {g:'C', col:PAL.amber};
  return {g:'D', col:PAL.shortAccent};
}
