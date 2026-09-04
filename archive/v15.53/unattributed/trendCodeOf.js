// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function trendCodeOf(state){
  return state==='up'?'UP':state==='dn'?'DN':
         state==='up-broken'?'UP-BRK':state==='dn-broken'?'DN-BRK':
         state==='flat'?'SIDE':'\u2013';
}
