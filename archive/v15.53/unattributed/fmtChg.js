// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function fmtChg(chg){ if(typeof chg!=='number') return null;
  return (chg>=0?'▲':'▼')+Math.abs(chg)+'%'; }
