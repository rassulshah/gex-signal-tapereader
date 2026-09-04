// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function levelsHtml(sym){
  try{
    var L=null; try{ L=gLevels(sym); }catch(e){}
    if(!L) return '';
    var rows=lvlRows(L); if(!rows.length) return '';
    var reachTxt=({ '0dte':'today only', week:'≤1 week', month:'≤1 month', chain:'full chain', unknown:'reach unknown' })[L.reach]||L.reach;
    // (v11.10) "+γ dampening" is a claim about WHERE PRICE SITS relative to the flip. When there is no flip
    // the regime came from the net-gamma SIGN instead, which is a different (weaker) statement — so the card
    // now says which one answered rather than presenting both as the same read.
    var regSrcTxt=(L.regimeSrc==='hvl')?'':' <span style="font-weight:600;opacity:.75">(net)</span>';
    var regTxt=L.regime==='posGamma'?('<span style="color:'+PAL.longAccent+'">+γ</span> dampening'+regSrcTxt)
             :(L.regime==='negGamma'?('<span style="color:'+PAL.shortAccent+'">−γ</span> amplifying'+regSrcTxt):'');
    // The honesty line. Our chain is not MenthorQ's chain and the card says so on its face.
    var caveat=('These are computed from OUR tape, not scraped. CR/PS reach '+reachTxt+' ('+(L.nCols||1)+' expiry column'+((L.nCols||1)===1?'':'s')+
      (L.exps&&L.exps.length?(', '+L.exps[0]+'→'+L.exps[L.exps.length-1]):'')+') — MenthorQ and InsiderFinance aggregate the whole chain out years, so our CR/PS is the structural wall WE CAN SEE, not an all-expiration wall. '+
      'The 0DTE pair (dashed) is today\'s expiry alone. Basis '+(L.basis||'?')+(L.nSkipped?(', '+L.nSkipped+' derived lane'+(L.nSkipped===1?'':'s')+' excluded'):'')+'. '+
      'Descriptive only — every level here is scored nightly like any other factor and carries no weight until it earns one.').replace(/"/g,'');
    var h='<div style="padding:2px 7px 3px;margin:0 0 1px">';
    h+='<div title="'+caveat+'" style="display:flex;align-items:center;gap:6px;font-size:10px;font-weight:800;color:'+PAL.sub+';white-space:nowrap">'+
       '<span data-glvl="open" style="cursor:pointer;color:'+PAL.ink+'">'+(LVL_UI.open?'▾':'▸')+' LEVELS</span>'+
       '<span style="font-weight:600;color:'+PAL.sub+'">'+reachTxt+'</span>'+
       (regTxt?('<span style="font-weight:700">'+regTxt+'</span>'):'')+
       '<span data-glvl="ifman" title="InsiderFinance levels, entered by hand. Their server blocks cross-origin reads (verified in the console: BLOCKED Failed to fetch), and the @grant that would work sandboxes this script and kills the tape reader. Their walls barely move — the SPX call wall held 7900 all afternoon on 2026-08-20 while SPX fell 66 points — so hand entry costs little. Click to set or clear. SPX or SPY both work; the scale is detected." '+
         'style="margin-left:auto;cursor:pointer;color:'+(IFMAN?PAL.blue:PAL.sub)+';font-size:9px">IF</span>'+
       '<span data-glvl="chart" style="cursor:pointer;color:'+(LVL_UI.chart?PAL.blue:PAL.sub)+';font-size:9px">chart</span>'+
       '</div>';
    if(LVL_UI.open){
      h+='<div style="margin-top:2px">';
      rows.forEach(function(r){
        var c=LVL_COL[r.id]||PAL.sub, z=(r.id==='cr0'||r.id==='ps0');
        var dcol=(r.dist>0)?PAL.longAccent:((r.dist<0)?PAL.shortAccent:PAL.sub);
        var dtxt=(r.dist==null)?'':((r.dist>0?'+':'')+lvlSpanFmt(r.dist));
        var cashTip=''; try{ if(dispIsFut()) cashTip=' Cash level actually read: '+(+r.k).toFixed(2)+' (shown converted at ratio '+(+dispR()).toFixed(3)+', rounded).'; }catch(eCt){}
        // (v11.11) the SPX equivalent, from Skylit's own SPXW ratio — so this level can be put straight
        // beside an SPX gamma page without arithmetic. Scale only; see spxRatio() on why that is not the
        // same as claiming their SPX book has a wall here.
        try{ var sx=lvlSpx(r.k); if(sx!=null) cashTip+=' On the SPX scale: '+sx+' (via Skylit\'s own '+(spxRatio().src||'SPXW')+' ratio, scale conversion only — an SPX gamma page reads a different chain).'; }catch(eSx){}
        var alt=lvlAlt(r.k);
        var altTxt=alt?('<span style="color:'+PAL.sub+';font-size:9px;font-variant-numeric:tabular-nums">'+alt.label+' '+(alt.approx?'≈':'')+alt.txt+'</span>'):'';
        h+='<div title="'+(LVL_WHAT[r.id]||'').replace(/"/g,'')+' Source: '+(r.src||'—')+'.'+cashTip+'" '+
           'style="display:flex;align-items:center;gap:5px;font-size:10.5px;line-height:1.45;white-space:nowrap">'+
           '<span style="display:inline-block;width:26px;color:'+c+';font-weight:800;opacity:'+(z?0.72:1)+'">'+LVL_NAME[r.id]+'</span>'+
           '<span style="color:'+PAL.ink+';font-weight:700;font-variant-numeric:tabular-nums">'+lvlFmt(r.k)+'</span>'+
           altTxt+
           '<span style="margin-left:auto;color:'+dcol+';font-weight:700;font-size:9.5px;font-variant-numeric:tabular-nums">'+dtxt+'</span>'+
           '<span style="color:'+PAL.sub+';font-size:9px;width:36px;text-align:right">'+(r.distPct!=null?((r.distPct>0?'+':'')+r.distPct+'%'):'')+'</span>'+
           '</div>';
      });
      var hm=lvlHvlMissing(L);
      if(hm){
        h+='<div title="The FLIP is the gamma flip — the zero-gamma strike, where dealers stop dampening and start amplifying. It is the same thing MenthorQ calls the High Vol Level and InsiderFinance labels Support/Resistance with the note that dynamics change if breached. It is absent here because the cumulative gamma curve in this book never crosses zero, so there is no flip to place. That is a fact about the book, not a gap in the read." '+
             'style="display:flex;align-items:center;gap:5px;font-size:10.5px;line-height:1.45;white-space:nowrap">'+
             '<span style="display:inline-block;width:26px;color:'+LVL_COL.hvl+';font-weight:800;opacity:0.55">FLIP</span>'+
             '<span style="color:'+PAL.sub+';font-weight:700">—</span>'+
             '<span style="color:'+PAL.sub+';font-size:9px;overflow:hidden;text-overflow:ellipsis">'+hm+'</span>'+
             '</div>';
      }
      // ---- (v11.16) THE REAL WALLS, from the call/put decomposition. This is the block that answers
      //      "can we compute what they show" — CR here is Call Resistance on CALL gamma, their definition.
      try{
        var CP=cpLevels(sym);
        if(CP && CP.err){ h+='<div style="font-size:9px;color:'+PAL.sub+';margin-top:3px">call/put: '+CP.err+'</div>'; }
        else if(CP){
          var sxr=null; try{ sxr=spxRatio().r; }catch(eR2){}
          h+='<div style="border-top:1px dotted '+PAL.line+';margin:3px 0 1px"></div>';
          h+='<div title="Calls and puts separated from the feed itself: v is TOTAL gamma and net is NET gamma, so call=(v+net)/2 and put=(v-net)/2. Verified on the live book — |net| was never greater than v across the whole strike set. These are the walls on CALL and PUT gamma specifically, the same definitions a third-party GEX page uses. CAVEAT: the app requests the top '+CP.n+' nodes, a RANKED SUBSET of the chain, so the walls are real but the TOTALS are subset totals and will not equal a full-chain page. The ratio is shown so that gap stays visible." '+
             'style="display:flex;align-items:center;gap:6px;font-size:9px;color:'+PAL.sub+';white-space:nowrap">'+
             '<span style="font-weight:800;color:'+PAL.longAccent+'">CALL/PUT · derived</span>'+
             '<span>C '+(CP.totalCall/1e9).toFixed(1)+'B · P '+(CP.totalPut/1e9).toFixed(1)+'B · ratio '+CP.ratio+'</span>'+
             '<span style="margin-left:auto">top '+CP.n+' nodes</span></div>';
          [['CR',CP.callWall,LVL_COL.cr,CP.callWallGex],['FLIP',CP.zg,LVL_COL.hvl,null],['PS',CP.putWall,LVL_COL.ps,CP.putWallGex]]
            .filter(function(r){ return r[1]!=null; })
            .sort(function(a,b){ return b[1]-a[1]; })
            .forEach(function(r){
              var d=+(r[1]-CP.px).toFixed(2);
              var dc=(d>0)?PAL.longAccent:((d<0)?PAL.shortAccent:PAL.sub);
              var sx=(sxr?Math.round(r[1]*sxr):null);
              h+='<div title="'+(r[0]==='CR'?'Call Resistance — the strike carrying the most CALL gamma above spot. Their definition, computed from our own feed.':(r[0]==='PS'?'Put Support — the strike carrying the most PUT gamma below spot.':'Zero gamma from the net series.'))+'" '+
                 'style="display:flex;align-items:center;gap:5px;font-size:10.5px;line-height:1.45;white-space:nowrap">'+
                 '<span style="display:inline-block;width:26px;color:'+r[2]+';font-weight:800">'+r[0]+'</span>'+
                 '<span style="color:'+PAL.ink+';font-weight:700;font-variant-numeric:tabular-nums">'+lvlFmt(r[1])+'</span>'+
                 (sx?('<span style="color:'+PAL.sub+';font-size:9px">SPX '+sx+'</span>'):'')+
                 (r[3]?('<span style="color:'+PAL.sub+';font-size:9px">'+Math.round(r[3]/1e6)+'M</span>'):'')+
                 '<span style="margin-left:auto;color:'+dc+';font-weight:700;font-size:9.5px;font-variant-numeric:tabular-nums">'+((d>0?'+':'')+lvlSpanFmt(d))+'</span>'+
                 '</div>';
            });
        }
      }catch(eCP){}
      // ---- (v11.12) THEIR levels, kept visibly separate from ours ----
      try{
        var IFL=ifManLevels();
        if(IFL && IFL.err){ h+='<div style="font-size:9px;color:'+PAL.sub+';margin-top:3px">IF: '+IFL.err+'</div>'; }
        else if(IFL){
          var agTx=(IFL.ageMin<60)?(IFL.ageMin+'m'):(Math.round(IFL.ageMin/60)+'h');
          var stale=IFL.ageMin>180;
          h+='<div style="border-top:1px dotted '+PAL.line+';margin:3px 0 1px"></div>';
          h+='<div style="display:flex;align-items:center;gap:6px;font-size:9px;color:'+PAL.sub+';white-space:nowrap">'+
             '<span style="font-weight:800;color:'+(stale?PAL.sub:PAL.blue)+'">INSIDERFINANCE</span>'+
             '<span>'+IFL.scale+' · '+agTx+' old'+(stale?' ⚠':'')+'</span>'+
             '<span style="margin-left:auto;cursor:pointer;color:'+PAL.sub+'" data-glvl="ifman">edit</span></div>';
          [['CR',IFL.cw,LVL_COL.cr],['FLIP',IFL.zg,LVL_COL.hvl],['Mag',IFL.mag,LVL_COL.mag],['PS',IFL.pw,LVL_COL.ps]]
            .filter(function(r){ return r[1]!=null; })
            .sort(function(a,b){ return b[1]-a[1]; })
            .forEach(function(r){
              var d=(L.px!=null)?+(r[1]-L.px).toFixed(2):null;
              var dc=(d>0)?PAL.longAccent:((d<0)?PAL.shortAccent:PAL.sub);
              var al=lvlAlt(r[1]);
              h+='<div title="InsiderFinance '+r[0]+' — THEIR number, not ours. Full option chain, and their Call Wall uses call gamma specifically, which our feed does not decompose. Shown converted from '+IFL.scale+' to this chart\'s scale. Reference only: it never feeds direction and is not scored as ours." '+
                 'style="display:flex;align-items:center;gap:5px;font-size:10px;line-height:1.4;white-space:nowrap;opacity:.82">'+
                 '<span style="display:inline-block;width:26px;color:'+r[2]+';font-weight:700;font-style:italic">'+r[0]+'</span>'+
                 '<span style="color:'+PAL.ink+';font-weight:600;font-variant-numeric:tabular-nums">'+lvlFmt(r[1])+'</span>'+
                 (al?('<span style="color:'+PAL.sub+';font-size:9px">'+al.label+' '+(al.approx?'≈':'')+al.txt+'</span>'):'')+
                 '<span style="margin-left:auto;color:'+dc+';font-weight:700;font-size:9px;font-variant-numeric:tabular-nums">'+(d==null?'':((d>0?'+':'')+lvlSpanFmt(d)))+'</span>'+
                 '</div>';
            });
        }
      }catch(eIF){}
      h+='</div>';
      if(LVL_UI.chart){ var sv=levelsChartSvg(sym,L); if(sv) h+='<div style="margin-top:3px;border:1px solid '+PAL.line+';border-radius:3px;overflow:hidden">'+sv+'</div>'; }
      // the cross-check lane, only when it has something to say
      try{
        var X=L.ext;
        if(X && X.on){
          if(X.err){ h+='<div style="font-size:9px;color:'+PAL.sub+';margin-top:2px">IF cross-check: '+X.err+'</div>'; }
          else {
            var agTxt=X.ageMs!=null?(Math.round(X.ageMs/60000)+'m'):'';
            h+='<div title="InsiderFinance, all expirations. Cross-check only — it never overrides a native level." style="font-size:9px;color:'+(X.stale?PAL.sub:PAL.blue)+';margin-top:2px;white-space:nowrap">'+
               'IF chain: CR '+(X.cw!=null?X.cw:'–')+' · PS '+(X.pw!=null?X.pw:'–')+' · FLIP '+(X.zg!=null?X.zg:'–')+
               (X.callGex!=null&&X.putGex!=null?(' · C/P '+X.callGex+'B/'+X.putGex+'B'):'')+
               (agTxt?(' · '+agTxt+' old'):'')+(X.stale?' (stale)':'')+(X.suspect?(' ⚠ '+X.suspect):'')+'</div>';
          }
        }
      }catch(eX2){}
    }
    h+='</div>';
    return h;
  }catch(e){ return ''; }
}
