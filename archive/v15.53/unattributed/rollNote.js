// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function rollNote(sym){
  try{ var R=EXPSET_ROLL[sym]; if(!R) return null;
       return 'rolled to '+R.to.slice(5).replace('-','/'); }catch(e){ return null; }
}
