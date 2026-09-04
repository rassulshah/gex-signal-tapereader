// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function secExec(sym){
  var h='<div class="g3b">';
  var R=regime2D(sym), B=biasVotes(sym);
  var pb=null; try{ pb=pbEntryPick(sym); }catch(e){}
  var L=G3_AT_LEVEL;
  // ================= THE NODE RULE (user-mandated 2026-08-22) =================
  // A TRADE IS OFF A NODE. Levels give CONTEXT — they tell you where structure sits and what the day
  // is shaped like — but the entry itself has to be at a node, preferably a PULLBACK node. Price sitting
  // at a level with no node behind it is information, not a setup.
  // This section previously read `pb.entry`, a field pbEntryPick has never returned (it returns `level`),
  // so EXECUTE could not arm at ALL: a valid node-based entry sat in the object while the face showed
  // "no setup". Dead since v11.26 and silent, because reading an absent property is not an error.
  var hasNode=!!(pb && pb.ok && pb.level!=null);
  var isPB=!!(pb && /pb/i.test(pb.rule||''));
  var entry=hasNode?pb.level:null;
  // stop sits beyond the far edge of the node's own zone; target is the next structural stop
  var stop=null, target=null, rr=null;
  try{
    if(hasNode){
      var pad=0.1; try{ var av=atr(sym); if(av>0) pad=Math.max(av*0.35,0.05); }catch(e0){}
      stop = (pb.dir>0) ? ((pb.zoneLo!=null?pb.zoneLo:entry)-pad) : ((pb.zoneHi!=null?pb.zoneHi:entry)+pad);
      target = (pb.nextStop!=null)?pb.nextStop:null;
      if(target!=null && stop!=null){
        var risk=Math.abs(entry-stop), rew=Math.abs(target-entry);
        if(risk>0) rr=+(rew/risk).toFixed(2);
      }
    }
  }catch(e1){}
  var rr2=1; try{ rr2=dispIsFut()?dispR():1; }catch(e2){}
  var blocked=null;
  if(!hasNode){
    blocked={ why:'NO NODE — NO TRADE',
              sub:(L? 'price is at '+dispNum((L.und!=null?L.und:L.k)*rr2)+', which is a level, not a node — context only'
                    : 'no pullback node in range') };
  } else if(R.g!=null && R.g<0 && !isPB){
    blocked={ why:'FADE BLOCKED — '+(R.danger?'−G/−V':'−G'),
              sub:'momentum regime · dealers hedge with the move' };
  } else if(rr!=null && rr<2.0){
    blocked={ why:'R:R UNDER FLOOR', sub:'R '+rr+' below the 2.0 floor'+(target==null?' · no target':'') };
  } else if(B.dir!==0 && pb.dir!==0 && pb.dir!==B.dir){
    blocked={ why:'AGAINST THE CALL', sub:'the node sets up '+(pb.dir>0?'long':'short')+' while the SMA reads '+B.verdict.toLowerCase() };
  }
  if(blocked){
    h+='<div class="g3blk"'+g3tip('Why is this refused? A trade has to be off a NODE — levels are context, the node is where the trade lives. Beyond that the regime can veto a fade, the reward-to-risk floor can veto a thin one, and a node setting up against the SMA is not a trade. A refusal is a result: the panel will not dress a bad setup as a good one to fill the space.')+
       '><b>'+g3esc(blocked.why)+'</b><span>'+g3esc(blocked.sub)+'</span></div>';
  } else {
    h+='<div class="g3arm"'+g3tip('What is armed, and what proves it wrong? The entry is a NODE — the '+(isPB?'pullback node the leg engine picked':'node in range')+' — with the stop beyond its own zone and the target at the next structural stop. Decide now whether you would actually take that stop, before price is moving.')+
       '><b>ARMED '+arrow(pb.dir)+' '+dispNum(entry*rr2)+'</b><span>'+
       (isPB?'pullback node':'node')+(pb.state?(' · '+pb.state):'')+(pb.grade?(' · '+pb.grade):'')+
       ' · stop '+(stop!=null?dispNum(stop*rr2):'—')+
       ' · target '+(target!=null?dispNum(target*rr2):'—')+
       (rr!=null?(' · '+rr+'R'):'')+'</span></div>';
  }
  // what the node is doing, always — armed or not
  if(hasNode){
    h+='<div class="g3alt"'+g3tip('Which node, and is it building or bleeding? A pullback node that is accumulating as price comes back to it is the setup this whole process exists to find. One that is decaying is a node the market is walking away from.')+
       '>node <b>'+dispNum(pb.level*rr2)+'</b>'+(pb.rule?(' · '+g3esc(pb.rule)):'')+
       (pb.dist!=null?(' · '+dispNum(pb.dist*rr2)+' away'):'')+'</div>';
  }
  var alt=[];
  try{ if(B.path&&B.path.line) alt.push(g3esc(B.path.line)); }catch(e3){}
  if(alt.length) h+='<div class="g3alt"'+g3tip('Where is the gamma structure pointing? Context for the trade above, not a signal on its own.')+'>path: <b>'+alt.join(' · ')+'</b></div>';
  h+='</div>';
  return h;
}
