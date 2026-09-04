// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function stepState(sym){
  // (v13.0) FOUR steps: TREND, LOCATION, REACTION, EXECUTE.
  // ⚠ THE GAMMA BOOK IS STILL A PRECONDITION, it just is not a STEP. Regime decides which playbook is
  // legal; it is not something the trader waits to complete. It gates the `wait` message and nothing
  // else, so a missing gamma book still says so instead of silently reading as "trend not ready".
  var st=[false,false,false,false], wait='';
  try{
    var R=regime2D(sym), B=biasVotes(sym), U=null;
    try{ U=lvlUnified(sym); }catch(e){}
    st[0]=(B.live>=3 && B.dir!==0);
    st[1]=!!(U&&U.rows&&U.rows.length && G3_AT_LEVEL);
    var rj=null; try{ rj=G3_AT_LEVEL?paReject(sym,G3_AT_LEVEL.k):null; }catch(e){}
    st[2]=!!rj;
    var pb=null; try{ pb=pbEntryPick(sym); }catch(e){}
    st[3]=!!(pb && pb.ok && pb.level!=null);   // (v11.45) a trade is off a NODE — `entry` never existed
    if(R.g==null) wait='waiting on the <b>gamma book</b>';
    else if(!st[0]) wait=(B.live<3)?'waiting on <b>more inputs</b> — '+B.live+' of 6 live':'trend is <b>neutral</b> — no side yet';
    else if(!st[1]) wait='price is <b>between levels</b> — no trade location';
    else if(!st[2]) wait='at <b>'+g3esc(G3_AT_LEVEL?fmtLvl(G3_AT_LEVEL.k):'the level')+'</b> — waiting on a reaction';
    else if(!st[3]) wait='reaction confirmed — <b>waiting on a node</b>';
    else wait='<b>armed</b> — all four steps satisfied';
  }catch(e){}
  var cur=0; for(var i=0;i<st.length;i++){ if(st[i]) cur=i+1; else break; }
  return { done:st, cur:cur, wait:wait };
}
