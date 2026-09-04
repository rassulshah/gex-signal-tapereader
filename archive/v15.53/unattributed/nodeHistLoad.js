// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function nodeHistLoad(){
  if(NODEHIST) return NODEHIST;
  try{ NODEHIST=JSON.parse(localStorage.getItem(NODEHIST_KEY)||'null'); }catch(e){ NODEHIST=null; }
  if(!NODEHIST || typeof NODEHIST!=='object') NODEHIST={ v:1, sym:{} };
  if(!NODEHIST.sym) NODEHIST.sym={};
  return NODEHIST;
}
