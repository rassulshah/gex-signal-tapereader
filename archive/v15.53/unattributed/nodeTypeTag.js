// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function nodeTypeTag(L){
  if(L.pos===true){
    return '<span title="Positive-gamma node \u2014 dealers dampen moves here (pinning / mean-reverting). Yellow." style="color:'+PAL.gold+';font-size:8.5px;font-weight:800">+\u03b3</span>';
  }
  if(L.pos===false){
    return '<span title="Negative-gamma node \u2014 dealers amplify moves here (accelerant / breakout-prone). Purple." style="color:#b58bff;font-size:8.5px;font-weight:800">\u2212\u03b3</span>';
  }
  return '';
}
