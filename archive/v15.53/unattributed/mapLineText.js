// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function mapLineText(sym, legR, trendConfirmed){
  try{ var m=mapSentence(sym, legR, trendConfirmed, {}); return { s:m.s, html:m.html, lean:m.lean, id:m.id }; }catch(e){ return { s:'', html:'', lean:'none' }; }
}
