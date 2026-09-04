// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function fmtKd(kd){                       // kd arrives in $K units from the tape
  if(typeof kd!=='number') return null;
  if(kd>=1e6) return '$'+(kd/1e6).toFixed(2)+'B';
  if(kd>=1e3) return '$'+Math.round(kd/1e3)+'M';
  return '$'+Math.round(kd)+'K';
}
