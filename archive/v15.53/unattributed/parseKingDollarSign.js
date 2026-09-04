// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function parseKingDollarSign(txt){
  var t=(''+txt).trim(); if(!/\$[\d,]+K/.test(t)) return null;
  return /^[\-\u2212]/.test(t) ? false : true;
}
