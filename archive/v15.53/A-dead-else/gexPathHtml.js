// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function gexPathHtml(sym){
  try{
    var P=gexPath(sym); if(!P || !P.mag) return '';
    var col=(P.state==='pinned')?PAL.gold:((P.dir>0)?PAL.longAccent:PAL.shortAccent);
    var tip=('Where the gamma structure points. Mag is the strike with the largest absolute gamma — the one dealers hedge around hardest, so price tends to be drawn to it. PS is the floor and CR the ceiling: the two places a deflection is most likely. '+
      (P.cage?('Price sits '+P.cage.pos+'% of the way up a '+P.cage.span+'-wide cage from '+fmtLvl(P.cage.lo)+' to '+fmtLvl(P.cage.hi)+'. '):'')+
      (P.through?('The magnet lies BEYOND '+P.through+', so that edge gets tested on the way. '):'')+
      (P.agree===false?'Recent bars are moving AWAY from the magnet, so the pull is not confirmed by the tape. ':'')+
      (P.agree===true?'Recent bars agree with the direction. ':'')+
      'Descriptive only, never an instruction, and it carries no weight in the direction vote until it has earned one.').replace(/"/g,'');
    var h='<div title="'+tip+'" style="display:flex;align-items:center;gap:6px;font-size:10.5px;line-height:1.5;padding:1px 7px 2px;white-space:nowrap">'+
      '<span style="color:'+PAL.sub+';font-weight:800;font-size:9px">PATH</span>'+
      '<span style="color:'+col+';font-weight:800">'+P.line+'</span>'+
      (P.magDist!=null&&P.state!=='pinned'?('<span style="color:'+PAL.sub+';font-size:9px">'+(P.magDist>0?'+':'')+fmtSpan(P.magDist)+'</span>'):'')+
      (P.agree===false?('<span style="color:'+PAL.gold+';font-size:9px">tape disagrees</span>'):'')+
      (P.reversalAt?('<span style="margin-left:auto;color:'+PAL.sub+';font-size:9px">'+P.reversalAt+' '+fmtLvl(P.reversalK)+(P.reversalDist!=null?(' · '+fmtSpan(Math.abs(P.reversalDist))):'')+'</span>'):'')+
      '</div>';
    if(P.cage){
      var pos=P.cage.pos;
      h+='<div title="The cage: PS below, CR above, price where the marker sits." style="padding:0 7px 3px">'+
         '<div style="position:relative;height:4px;border-radius:2px;background:linear-gradient(90deg,'+LVL_COL.ps+'55,'+PAL.line+','+LVL_COL.cr+'55)">'+
         '<div style="position:absolute;left:'+pos+'%;top:-2px;width:2px;height:8px;background:'+PAL.time+';border-radius:1px;transform:translateX(-1px)"></div>'+
         '</div></div>';
    }
    return h;
  }catch(e){ return ''; }
}
