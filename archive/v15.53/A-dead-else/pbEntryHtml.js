// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function pbEntryHtml(sym){
  try{
    var pe=pbEntryPick(sym); if(!pe.ok || pe.level==null) return '';
    var st=null; try{ st=featStatsCached(sym); }catch(e){}
    var b30=(st&&st.byKey&&st.byKey['pbEntry'])||{n:0,hit:0}, b60=(st&&st.byKey&&st.byKey['pbEntry.60'])||{n:0,hit:0};
    var r30=b30.n?Math.round(100*b30.hit/b30.n):null, r60=b60.n?Math.round(100*b60.hit/b60.n):null;
    var e30=effN(b30.n), e60=effN(b60.n);
    var gcol=pe.grade==='A'?PAL.longAccent:(pe.grade==='B'?PAL.blue:PAL.amber);
    var gbg=pe.grade==='A'?'rgba(46,194,126,.15)':(pe.grade==='B'?'rgba(74,144,217,.15)':'rgba(242,180,90,.15)');
    var above=(pe.level>pe.px);
    var lvlCol=above?PAL.longAccent:PAL.shortAccent;
    var ptsTxt=(above?'+':'−')+fmtSpan(pe.dist)+' pts';
    var stTxt=pe.state==='acm'?'<span style="color:'+PAL.longAccent+'">Acm</span>':(pe.state==='dec'?'<span style="color:'+PAL.shortAccent+'">Dec</span>':(pe.state==='gone'?'<span style="color:'+PAL.sub+'">gone</span>':''));
    var deflTxt=(pe.dir<0?'defl ↓':'defl ↑')+(pe.nextStop!=null?(' → '+fmtLvl(pe.nextStop)):'');
    var zoneTxt=(pe.zoneLo!=null&&pe.zoneHi!=null&&pe.rule==='leg.pbZone')?(' (zone '+fmtLvl(pe.zoneLo)+'–'+fmtLvl(pe.zoneHi)+', forming)'):'';
    // (v11.3.1) stacked pullback nodes: name the zone across the stack so the level is read as an AREA
    try{ var cxS=_pbCtx(sym,pe); if(cxS&&cxS.stack&&cxS.stack.n>=2){ var mks=cxS.stack.members.map(function(mm){return mm.k;});
      zoneTxt+=' (stack of '+cxS.stack.n+': '+fmtLvl(Math.min.apply(null,mks))+'–'+fmtLvl(Math.max.apply(null,mks))+')'; } }catch(eZs){}
    var latch=pe.latched?(' '+pe.latched):'';
    var tip=('PB Entry — where to look for the pullback. '+pe.why+' — '+fmtSpan(pe.dist)+' '+(above?'above':'below')+' price; the expected move off it is '+(pe.dir<0?'DOWN':'UP')+(pe.nextStop!=null?(' toward the Next Stop '+fmtLvl(pe.nextStop)):'')+'. Node state '+(pe.state||'—')+(pe.pol?(', polarity '+pe.pol+'γ'):'')+'. '+
      'Confidence '+pe.grade+' (B = leg active, node accumulating, SMA-50 agrees, within reach; C = structure-only / chop / mid-range; A only after promotion). '+
      'Measured here: touched-and-deflected within 30m '+(r30!=null&&e30>=RULE_UNLOCK_N?(r30+'% (eff n '+e30+')'):('— (eff n '+e30+', need '+RULE_UNLOCK_N+')'))+
      ' · within 60m '+(r60!=null&&e60>=RULE_UNLOCK_N?(r60+'% (eff n '+e60+')'):('— (eff n '+e60+')'))+
      '. Scored as: price touched the level (contact zone) and then moved '+DIR_PTS+' in the deflection direction before closing through it. Rule-based until measured; graded nightly by rule, state and polarity. Descriptive — a level to watch, never an instruction.').replace(/"/g,'');
    // (v11.3.2, user) TWO LINES: the level line stays tight; the gray context (deflection target, zone,
    // stack, measured rate) moves to a smaller second line that WRAPS instead of running off the panel.
    var line2=('· '+deflTxt)+zoneTxt+latch+
      (r30!=null&&e30>=RULE_UNLOCK_N?(' · '+r30+'% @30m'):'');
    return '<div title="'+tip+'" style="padding:1px 7px;margin:0 0 1px">'+
      '<div style="display:flex;align-items:center;gap:6px;font-size:11px;font-weight:800;color:'+PAL.ink+';white-space:nowrap">'+
        '<span style="color:'+PAL.sub+'">PB Entry:</span>'+
        '<span style="font-size:12px;color:'+lvlCol+'">'+(above?'↑ ':'↓ ')+fmtLvl(pe.level)+'</span>'+
        '<span style="color:'+lvlCol+';font-weight:700;font-size:9.5px">'+ptsTxt+'</span>'+
        (stTxt?('<span style="font-weight:700;font-size:9.5px">· '+stTxt+'</span>'):'')+
        '<span style="margin-left:auto;color:'+gcol+';background:'+gbg+';padding:0 5px;border-radius:3px;font-size:10px">'+pe.grade+'</span>'+
      '</div>'+
      '<div style="font-size:8.5px;font-weight:600;color:'+PAL.sub+';white-space:normal;line-height:1.35;margin-top:1px">'+line2+'</div>'+
    '</div>';
  }catch(e){ return ''; }
}
