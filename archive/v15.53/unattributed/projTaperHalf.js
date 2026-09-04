// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function projTaperHalf(b, etaBars, barsLeft, halfFn){
  if(etaBars==null || b<=etaBars){
    // no ETA: still cap growth at the 10-bar envelope so a flat projection
    // cannot blow the y-domain.
    return (etaBars==null && b>10) ? halfFn(10) : halfFn(b);
  }
  var hEta=halfFn(etaBars);
  var span=Math.max(1,(barsLeft-etaBars));
  var frac=Math.min(1,(b-etaBars)/span);
  return Math.max(0.5, hEta*(1-0.5*frac));
}
