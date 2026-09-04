// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function firstStrengthPct(txt){
  if(!txt) return null;
  txt=(''+txt).replace(/\s+/g,' ').trim();
  var re=/([+\-\u2212]?)(\d{1,3})%/g, m, val=null;
  while((m=re.exec(txt))!==null){ if(m[1]==='') val=parseInt(m[2],10); }
  return val;
}
