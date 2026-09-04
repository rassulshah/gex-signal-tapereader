// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function nodeChip(nd){
  var v=nd&&nd.vel; if(!v) return null;
  var d5=v.d5, d15=v.d15, d60=v.d60;
  if(typeof d5!=='number'||typeof d15!=='number'||typeof d60!=='number') return null;
  var above=(nd.side==='above');
  if(d5<0 && d15<0 && d60>0) return { txt:'TURNING \u2193 \u00b7 5m then 15m', cls:'g3cTurn' };
  if(d5>0 && d15>0 && d60<0) return { txt:'TURNING \u2191 \u00b7 5m then 15m', cls:'g3cTurn' };
  if(d60>0) return above ? { txt:'RESISTANCE BUILDING', cls:'g3cBear' }
                         : { txt:'SUPPORT BUILDING',    cls:'g3cBull' };
  if(d60<0) return above ? { txt:'RESISTANCE FAILING',  cls:'g3cBull' }
                         : { txt:'SUPPORT FAILING',     cls:'g3cBear' };
  return { txt:(above?'CAP':'FLOOR')+' \u00b7 HOLDING', cls:'g3cNeut' };
}
