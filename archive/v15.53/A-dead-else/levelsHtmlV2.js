// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function levelsHtmlV2(sym){
  try{
    var U=lvlUnified(sym); if(!U || !U.rows.length) return '';
    var srcTxt = (U.src==='week') ? ('to Fri · '+(U.nExps||'?')+' exps · '+(U.n||'?')+' strikes')
               : (U.src==='0dte')  ? ('0DTE only · '+(U.n||'?')+' strikes')
               : ('feed only · '+(U.n||'?')+' strikes · top-60');
    var regTxt = U.regime==='posGamma'?'<span style="color:'+PAL.longAccent+'">+γ</span> damp'
               :(U.regime==='negGamma'?'<span style="color:'+PAL.shortAccent+'">−γ</span> amp':'');
    var tip=('Levels computed from our own feed, calls and puts separated: the payload carries v (TOTAL gamma) '+
      'and net (NET gamma), so call=(v-net)/2 and put=(v+net)/2. CR is Call Resistance on CALL gamma and PS is '+
      'Put Support on PUT gamma — the same definitions a third-party GEX page uses. '+
      (U.src==='chain'
        ? ('Read from a self-fetched full-chain request: '+(U.nExps||'?')+' expirations, '+(U.n||'?')+' strikes'+(U.ageMin!=null?(', '+U.ageMin+'m old'):'')+'.')
        : 'Read from the passive feed, which is a ranked top-60 subset over four expirations — the walls are real but far levels may be outside it.')+
      (U.ratio!=null?(' Call/put ratio '+U.ratio+' — compare against a third-party header to check the decomposition.'):'')+
      ' Every value is on this chart\'s instrument. Descriptive only; scored nightly, no weight until earned.').replace(/"/g,'');
    var h='<div style="padding:2px 7px 3px;margin:0 0 1px">';
    h+='<div title="'+tip+'" style="display:flex;align-items:center;gap:6px;font-size:10px;font-weight:800;color:'+PAL.sub+';white-space:nowrap">'+
       '<span data-glvl="open" style="cursor:pointer;color:'+PAL.ink+'">'+(LVL_UI.open?'▾':'▸')+' LEVELS</span>'+
       '<span style="font-weight:600">'+srcTxt+'</span>'+
       (U.degenerate?('<span title="The newest feed frames carry no call/put decomposition — every strike reads net = +/-v. That happens outside regular hours, when the latest bar is still forming. The walls below are read from magnitude only and the call/put split is NOT measured. Treat them as provisional." style="color:'+PAL.gold+';font-weight:800">⚠ no split</span>'):'')+
       (U.snapBack?('<span title="Stepped back '+U.snapBack+' frame(s) to the most recent snapshot that carries a real decomposition." style="color:'+PAL.sub+'">−'+U.snapBack+'f</span>'):'')+
       (regTxt?('<span style="font-weight:700">'+regTxt+'</span>'):'')+
       '<span data-glvl="ifman" title="InsiderFinance levels, entered by hand — their page blocks cross-origin reads." style="margin-left:auto;cursor:pointer;color:'+(IFMAN?PAL.blue:PAL.sub)+';font-size:9px">IF</span>'+
       '<span data-glvl="chart" style="cursor:pointer;color:'+(LVL_UI.chart?PAL.blue:PAL.sub)+';font-size:9px">chart</span>'+
       '</div>';
    if(LVL_UI.open){
      h+='<div style="margin-top:2px">';
      U.rows.forEach(function(r){
        var base=r.id.split('·')[0].replace('0','');
        var c=(r.id.indexOf('·')>0)?PAL.gold:(LVL_COL[base.toLowerCase()]||LVL_COL[r.id.toLowerCase()]||PAL.sub);
        var z=!!r.tag;
        var dc=(r.dist>0)?PAL.longAccent:((r.dist<0)?PAL.shortAccent:PAL.sub);
        h+='<div title="'+((LVL_WHAT[base.toLowerCase()]||'')+(r.gex?(' Gamma here: '+Math.round(r.gex/1e6)+'M.'):'')).replace(/"/g,'')+'" '+
           'style="display:flex;align-items:center;gap:6px;font-size:10.5px;line-height:1.5;white-space:nowrap">'+
           '<span style="display:inline-block;min-width:30px;color:'+c+';font-weight:800;opacity:'+(z?0.75:1)+'">'+r.id+'</span>'+
           '<span style="color:'+PAL.ink+';font-weight:700;font-size:11.5px;font-variant-numeric:tabular-nums">'+lvlFmt(r.k)+'</span>'+
           (r.gex?('<span style="color:'+PAL.sub+';font-size:9px">'+Math.round(r.gex/1e6)+'M</span>'):'')+
           '<span style="margin-left:auto;color:'+dc+';font-weight:700;font-size:9.5px;font-variant-numeric:tabular-nums">'+((r.dist>0?'+':'')+lvlSpanFmt(r.dist))+'</span>'+
           '<span style="color:'+PAL.sub+';font-size:9px;width:36px;text-align:right">'+((r.distPct>0?'+':'')+r.distPct+'%')+'</span>'+
           '</div>';
      });
      if(U.absent && U.absent.length){
        h+='<div title="Every level on the roster is accounted for. A level with no value is a FINDING, not a gap: a 0DTE book with no call-dominant strike above spot has no call wall, and a published GEX page reports N/A in exactly that case. \'same as CR\' means the 0DTE wall landed on the identical strike, so it is not printed twice." '+
           'style="display:flex;align-items:center;gap:5px;font-size:9px;line-height:1.5;color:'+PAL.sub+';white-space:nowrap;overflow:hidden">'+
           '<span style="font-weight:800">—</span>'+
           '<span style="overflow:hidden;text-overflow:ellipsis">'+
           U.absent.map(function(a){ return '<span style="color:'+(LVL_COL[a.id.replace('0','').toLowerCase()]||PAL.sub)+';font-weight:700">'+a.id+'</span> '+a.why; }).join(' · ')+
           '</span></div>';
      }
      if(U.crSuppressed || U.psSuppressed){
        var sup=U.crSuppressed?'CR':'PS', shr=U.crSuppressed?U.callShare:U.putShare;
        h+='<div title="No wall is named on this side because the side barely exists: '+(U.crSuppressed?'call':'put')+' gamma is '+shr+'% of the book. On expiry day almost all gamma is on one side, and the heaviest strike on the empty side is a rounding artefact rather than a level — the strike it would have named was '+((U.crSuppressed?U.crSuppressed.k:U.psSuppressed.k))+'. A third-party GEX page reports N/A in the same situation." '+
           'style="display:flex;align-items:center;gap:6px;font-size:10px;line-height:1.45;white-space:nowrap">'+
           '<span style="display:inline-block;min-width:30px;color:'+(U.crSuppressed?LVL_COL.cr:LVL_COL.ps)+';font-weight:800;opacity:.55">'+sup+'</span>'+
           '<span style="color:'+PAL.sub+'">—</span>'+
           '<span style="color:'+PAL.sub+';font-size:9px;overflow:hidden;text-overflow:ellipsis">'+(U.crSuppressed?'call':'put')+' side only '+shr+'% of book</span></div>';
      }
      if(U.hvlMissing){
        h+='<div title="The FLIP is the gamma flip — where cumulative gamma crosses zero and dealers stop dampening and start amplifying. It is absent because the cumulative never changes sign across the strikes we hold. That is a fact about the book, not a gap in the read." '+
           'style="display:flex;align-items:center;gap:6px;font-size:10px;line-height:1.45;white-space:nowrap">'+
           '<span style="display:inline-block;width:30px;color:'+LVL_COL.hvl+';font-weight:800;opacity:.55">FLIP</span>'+
           '<span style="color:'+PAL.sub+'">—</span>'+
           '<span style="color:'+PAL.sub+';font-size:9px;overflow:hidden;text-overflow:ellipsis">'+
           (U.hvlFar?('nearest flip '+U.hvlFar.pct+'% away — tail, not a flip'):'no flip in these strikes')+'</span></div>';
      }
      h+='</div>';
      if(LVL_UI.chart){ var sv=null; try{ sv=levelsChartV2(sym,U); }catch(eC){} if(sv) h+='<div style="margin-top:3px;border:1px solid '+PAL.line+';border-radius:3px;overflow:hidden">'+sv+'</div>'; }
      // their numbers, when entered, kept visibly separate
      // (v11.25) THEIR levels, computed from their own chain by the companion script. Auto when present;
      // the hand-entered lane below is the fallback for when the companion is not installed.
      try{
        var CH=ifChainRows(sym,'toFri'), CH0=ifChainRows(sym,'dte0');
        if(CH && CH.rows.length){
          h+='<div style="border-top:1px dotted '+PAL.line+';margin:3px 0 1px"></div>';
          h+='<div title="InsiderFinance levels, recomputed from the option chain their page embeds — gamma x open interest, not scraped from their header. Window: '+(CH.exps||[]).join(', ')+'. Call/put ratio '+(CH.lv.ratio)+', Max Pain '+(CH.lv.maxPain)+', put/call OI '+(CH.lv.pcOI)+'. THIS IS A DIFFERENT MEASUREMENT FROM OURS: theirs is where exposure SITS in the chain, ours is live dealer positioning that accumulates intraday. Neither is a check on the other." '+
             'style="display:flex;align-items:center;gap:6px;font-size:9px;color:'+PAL.sub+';white-space:nowrap">'+
             '<span style="font-weight:800;color:'+PAL.blue+'">CHAIN · IF</span>'+
             '<span>to Fri · r '+(CH.lv.ratio==null?'–':CH.lv.ratio)+'</span>'+
             '<span style="margin-left:auto">'+(CH.stale?'⚠ ':'')+CH.ageMin+'m</span></div>';
          CH.rows.forEach(function(r){
            var base=(r.id==='MP'||r.id==='MP*')?'mag':r.id.toLowerCase();
            var c=(r.id==='MP'||r.id==='MP*')?PAL.sub:(LVL_COL[base]||PAL.sub);
            var d0=+(r.k-U.px).toFixed(2), dc=(d0>0)?PAL.longAccent:((d0<0)?PAL.shortAccent:PAL.sub);
            var alt=(CH0&&CH0.rows.filter(function(x){return x.id===r.id;})[0]);
            h+='<div title="'+(r.id==='MP*'?'Max Pain, RECOMPUTED BY US from their chain over our expiry window — NOT the max pain InsiderFinance publish, which covers all expiries and is a different number (7350 on 2026-08-21). The strike minimising total payout to option holders at expiry. Needs open interest, which is why it is impossible from the Skylit feed and free here.':('InsiderFinance '+r.id+', from their chain.'))+'" '+
               'style="display:flex;align-items:center;gap:5px;font-size:10px;line-height:1.4;white-space:nowrap;opacity:.85">'+
               '<span style="display:inline-block;min-width:30px;color:'+c+';font-weight:700;font-style:italic">'+r.id+'</span>'+
               '<span style="color:'+PAL.ink+';font-weight:600;font-variant-numeric:tabular-nums">'+lvlFmt(r.k)+'</span>'+
               ((alt&&Math.abs(alt.k-r.k)>0.005)?('<span style="color:'+PAL.sub+';font-size:9px">0dte '+lvlFmt(alt.k)+'</span>'):'')+
               '<span style="margin-left:auto;color:'+dc+';font-weight:700;font-size:9px;font-variant-numeric:tabular-nums">'+((d0>0?'+':'')+lvlSpanFmt(d0))+'</span></div>';
          });
          if(CH.lv.crSuppressed) h+='<div style="font-size:9px;color:'+PAL.sub+';padding-left:2px">CR — none, call side '+CH.lv.crSuppressed.share+'% of book</div>';
        }
      }catch(eCH){}
      try{
        var IFL=(ifChain(sym)?null:ifManLevels());
        if(IFL && !IFL.err){
          var agTx=(IFL.ageMin<60)?(IFL.ageMin+'m'):(Math.round(IFL.ageMin/60)+'h');
          h+='<div style="border-top:1px dotted '+PAL.line+';margin:3px 0 1px"></div>';
          h+='<div style="display:flex;align-items:center;gap:6px;font-size:9px;color:'+PAL.sub+';white-space:nowrap">'+
             '<span style="font-weight:800;color:'+PAL.blue+'">INSIDERFINANCE</span><span>'+IFL.scale+' · '+agTx+'</span>'+
             '<span style="margin-left:auto;cursor:pointer" data-glvl="ifman">edit</span></div>';
          [['CR',IFL.cw,LVL_COL.cr],['FLIP',IFL.zg,LVL_COL.hvl],['Mag',IFL.mag,LVL_COL.mag],['PS',IFL.pw,LVL_COL.ps]]
            .filter(function(r){ return r[1]!=null; }).sort(function(a,b){ return b[1]-a[1]; })
            .forEach(function(r){
              var d=+(r[1]-U.px).toFixed(2), dc2=(d>0)?PAL.longAccent:PAL.shortAccent;
              h+='<div style="display:flex;align-items:center;gap:6px;font-size:10px;line-height:1.4;white-space:nowrap;opacity:.8">'+
                 '<span style="display:inline-block;width:30px;color:'+r[2]+';font-weight:700;font-style:italic">'+r[0]+'</span>'+
                 '<span style="color:'+PAL.ink+';font-weight:600;font-variant-numeric:tabular-nums">'+lvlFmt(r[1])+'</span>'+
                 '<span style="margin-left:auto;color:'+dc2+';font-weight:700;font-size:9px">'+((d>0?'+':'')+lvlSpanFmt(d))+'</span></div>';
            });
        } else if(IFL && IFL.err){ h+='<div style="font-size:9px;color:'+PAL.sub+';margin-top:2px">IF: '+IFL.err+'</div>'; }
      }catch(eIF2){}
    }
    h+='</div>';
    return h;
  }catch(e){ return ''; }
}
