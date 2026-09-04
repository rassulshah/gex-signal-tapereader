// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function nextStopHtml(sym){
  try{
    var ns=nextStopPick(sym); if(!ns.ok || ns.level==null) return '';
    var st=null; try{ st=featStatsCached(sym); }catch(e){}
    var b30=(st&&st.byKey&&st.byKey['nextStop'])||{n:0,hit:0}, b60=(st&&st.byKey&&st.byKey['nextStop.60'])||{n:0,hit:0};
    var r30=b30.n?Math.round(100*b30.hit/b30.n):null, r60=b60.n?Math.round(100*b60.hit/b60.n):null;
    var e30=effN(b30.n), e60=effN(b60.n);
    var gcol=ns.grade==='A'?PAL.longAccent:(ns.grade==='B'?PAL.blue:PAL.amber);
    var gbg=ns.grade==='A'?'rgba(46,194,126,.15)':(ns.grade==='B'?'rgba(74,144,217,.15)':'rgba(242,180,90,.15)');
    // (v11.1.1) the level is GREEN when above price, RED when below; the distance is signed points
    var upSide=(ns.dir>0);
    var lvlCol=upSide?PAL.longAccent:PAL.shortAccent;
    var ptsTxt=(upSide?'+':'−')+fmtSpan(ns.dist)+' pts';
    var tip=('Why this level? '+ns.why+' — '+fmtSpan(ns.dist)+' '+(ns.dir>0?'above':'below')+' price. Confidence '+ns.grade+' (B = leg and SMA-50 agree and the level is within ~30 min of travel; C = structure-only, chop or mid-range; A only after promotion). '+
      'Measured here: reached within 30m '+(r30!=null&&e30>=RULE_UNLOCK_N?(r30+'% (eff n '+e30+')'):('— (eff n '+e30+', need '+RULE_UNLOCK_N+')'))+
      ' · within 60m '+(r60!=null&&e60>=RULE_UNLOCK_N?(r60+'% (eff n '+e60+')'):('— (eff n '+e60+')'))+
      '. Rule-based until measured; the nightly review grades each rule and proposes re-ordering through the promotion bar. Descriptive — a level, never an instruction.').replace(/"/g,'');
    return '<div title="'+tip+'" style="display:flex;align-items:center;gap:6px;font-size:11px;font-weight:800;color:'+PAL.ink+';padding:1px 7px;margin:2px 0 1px;white-space:nowrap">'+
      '<span style="color:'+PAL.sub+'">Next Stop:</span>'+
      '<span style="font-size:12px;color:'+lvlCol+'">'+(upSide?'↑ ':'↓ ')+fmtLvl(ns.level)+'</span>'+
      '<span style="color:'+lvlCol+';font-weight:700;font-size:9.5px">'+ptsTxt+'</span>'+
      '<span style="color:'+PAL.sub+';font-weight:600;font-size:9px">· '+ns.horizon+'</span>'+
      (r30!=null&&e30>=RULE_UNLOCK_N?('<span style="color:'+PAL.sub+';font-weight:600;font-size:9px">· '+r30+'% @30m</span>'):'')+
      '<span style="margin-left:auto;color:'+gcol+';background:'+gbg+';padding:0 5px;border-radius:3px;font-size:10px">'+ns.grade+'</span>'+
    '</div>';
  }catch(e){ return ''; }
}
