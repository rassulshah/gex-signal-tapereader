// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function nmVerdictColor(v, side){
  // Bounce off a floor(below) = bullish(green); off a ceiling(above) = bearish(red).
  // Break-through DOWN through support = bearish(red); UP through resistance = bullish(green).
  if(v==='Bounce')        return side==='below'?PAL.longAccent:PAL.shortAccent;
  if(v==='Break-through') return side==='below'?PAL.shortAccent:PAL.longAccent;
  if(v==='Pullback')      return PAL.blue;
  return PAL.sub;   // Forming
}
