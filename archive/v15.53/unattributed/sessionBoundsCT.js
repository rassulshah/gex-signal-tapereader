// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function sessionBoundsCT(){
  var d=ctNow();
  function atCT(h,m){
    // Build an epoch ms for today's h:m America/Chicago. ctNow() already returns
    // a Date whose fields are CT wall-clock; reconstruct via the same offset.
    var real=new Date();
    var ctMs=ctNow().getTime();
    var offset=real.getTime()-ctMs;              // realUTC - CTwall
    var wall=new Date(d.getFullYear(), d.getMonth(), d.getDate(), h, m, 0, 0).getTime();
    return wall+offset;
  }
  return { start:atCT(8,30), end:atCT(15,0) };
}
