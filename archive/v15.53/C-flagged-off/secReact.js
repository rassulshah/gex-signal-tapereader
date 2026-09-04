// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function secReact(sym){
  var h='<div class="g3b">';
  // (v11.46) REACTION READS THE NODE, NOT THE LEVEL. Under the node rule the trade is at a node, so
  // "is it holding?" has to be asked of the node being traded. Asking it of the nearest LEVEL answered
  // a question nobody was about to act on.
  var pb=null; try{ pb=pbEntryPick(sym); }catch(e0){}
  var nodeK=(pb&&pb.ok&&pb.level!=null)?pb.level:null;
  var L=G3_AT_LEVEL;
  var subj=(nodeK!=null)?nodeK:(L?(L.und!=null?L.und:L.k):null);
  var rr=1; try{ rr=dispIsFut()?dispR():1; }catch(e1){}
  var isNode=(nodeK!=null);

  h+='<div class="g3rx"'+g3tip('What is being watched, and why that? Under the node rule the trade is at a NODE, so the reaction that matters is the node\'s. When there is no node in play this falls back to the nearest level and says so — but a level reacting is context, not a trigger.')+
     '><em>WATCH</em><span>'+(subj!=null
        ? ((isNode?'node ':'level ')+'<b>'+dispNum(subj*rr)+'</b>'+(isNode&&pb.rule?(' · '+g3esc(pb.rule)):'')+(isNode?'':' — context only, not a trigger'))
        : 'nothing in range')+'</span></div>';

  // is the thing being watched growing or bleeding, in dollars
  var ac=null; try{ ac=accumAsym(sym); }catch(e2){}
  var nodeState=null;
  try{ if(nodeK!=null){ var acn=accumCanon(sym,nodeK); nodeState=(acn&&acn.m15)?acn.m15.label:null; } }catch(e3){}
  // (v13.1) SKYLIT'S OWN DOLLARS FIRST. The accumulation read below stays as corroboration — two
  // independent readings agreeing is worth more than one, and when they DISAGREE that is a finding.
  var RD=null; try{ RD=reactDefence(sym, (subj!=null)?subj*rr:null); }catch(eRD){}
  if(RD){
    h+='<div class="g3rx"'+g3tip('Is the level defending itself, in dollars? This is Skylit\'s own published change for the exact node price is testing. A node ADDING size while price sits on it is being defended as it is tested — the setup this whole process exists to find. One SHEDDING size as price arrives is being walked away from. Flat is not a trade, however good the location looked.')+
       '><em>DEFENCE</em><span><b class="'+RD.cls+'" style="font-size:12px">'+RD.verdict+'</b>'+
       ' \u00b7 5m '+velD(RD.v.d5).txt+' \u00b7 15m '+velD(RD.v.d15).txt+
       ' <span style="color:#5b6675">on '+dispNum(RD.node.es)+'</span>'+
       (RD.stale?' <span style="color:#f2b45a">\u00b7 aged '+Math.round((RD.age||0)/1000)+'s</span>':'')+
       '</span></div>';
    if(RD.caveat){
      h+='<div class="g3rx"><em></em><span style="color:#f2b45a">\u26a0 '+g3esc(RD.caveat)+'</span></div>';
    }
  }
  var nodeTxt;
  if(isNode && nodeState) nodeTxt='the node is <b>'+g3esc(String(nodeState).toLowerCase())+'</b>'+(ac&&ac.txt?(' · book '+g3esc(ac.txt)):'');
  else if(ac) nodeTxt=g3esc(ac.txt);
  else nodeTxt='%King only — no dollars';
  h+='<div class="g3rx"'+g3tip('Is it being defended or abandoned? A node ACCUMULATING as price arrives is dealers adding — the level is being reinforced exactly when it is tested, and that is the setup this whole process exists to find. One bleeding as price approaches is being walked away from and will probably not hold.')+
     '><em>NODE</em><span>'+nodeTxt+'</span></div>';

  var rj=null; try{ rj=paReject(sym, subj); }catch(e4){}
  var pa=null; try{ pa=paRead(sym); }catch(e5){}
  var ptxt;
  if(rj) ptxt=g3esc(rj.txt)+' — <span class="g3ok">rejected</span>';
  else if(subj!=null) ptxt='at '+dispNum(subj*rr)+', no rejection bar yet';
  else ptxt=(pa&&pa.ok)?('nothing in range · '+g3esc(pa.label)+' ('+pa.clv+')'):'nothing in range';
  h+='<div class="g3rx"'+g3tip('Did it push price back? A rejection is a bar that trades THROUGH and closes back on the side it came from. Without order flow this is the confirmation available, and it is the trigger — price merely arriving is not a signal. A close beyond the zone is the opposite reading.')+
     '><em>PRICE</em><span>'+ptxt+'</span></div>';

  try{
    if(subj!=null){
      var Cx=levelDepth(sym), t=confTier(Cx, subj);
      if(t){
        var dTxt=(t.d>=0.55)?'heavy':((t.d>=0.25)?'moderate':'thin');
        var gTxt=(t.g>=0.55)?'heavy':((t.g>=0.25)?'moderate':'thin');
        h+='<div class="g3rx"'+g3tip('What is standing behind it? Dealer delta and gamma at this strike, against the heaviest in their book. Heavy delta means a lot of hedging has to happen for price to pass; thin means it is mostly a gamma effect and cheaper to break.')+
           '><em>DEPTH</em><span>delta <b>'+dTxt+'</b> · gamma <b>'+gTxt+'</b>'+(t.tier>1?' — <span class="g3ok">both loaded</span>':'')+'</span></div>';
      }
    }
  }catch(e6){}

  if(pa&&pa.ok){
    h+='<div class="g3rx"'+g3tip('Who is winning the bars right now? Where each bar CLOSES inside its own range, the closest honest stand-in for a TICK reading. A run of upper-third closes is buyers finishing each bar in control; the structure tag breaks the tie when closes are mid-range.')+
       '><em>PRESSURE</em><span><b>'+pa.upBars+'</b> upper / <b>'+pa.dnBars+'</b> lower of '+pa.bars+' · '+g3esc(pa.struct)+'</span></div>';
  }
  h+='</div>';
  return h;
}
