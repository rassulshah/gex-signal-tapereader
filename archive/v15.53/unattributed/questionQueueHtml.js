// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function questionQueueHtml(){
  var qs=[];
  try{ qs=seedQuestions(); }catch(e){ qs=[]; }
  if(!qs.length) return tabEmpty('no questions in the queue — the queue is DERIVED from FEATURES[].questions, so it is empty only if nothing is enrolled.');
  var byState={ answered:[], testing:[], proposed:[] };
  qs.forEach(function(q){
    var st=null; try{ st=ruleLocalRate(q.feature); }catch(e1){ st=null; }
    var en=st?st.effN:0;
    var rate=st?st.rate:null;
    var promoted=false; try{ promoted=rulePromoted(q.feature); }catch(e2){}
    var state = (promoted && en>=RULE_UNLOCK_N) ? 'answered' : (en>0 ? 'testing' : 'proposed');
    byState[state].push({ q:q, en:en, rate:rate, n:st?st.n:0 });
  });
  var h='';
  [['answered','📊 ANSWERED',PAL.longAccent],['testing','◐ TESTING',PAL.blue],['proposed','⚖ PROPOSED',PAL.sub]].forEach(function(g){
    var arr=byState[g[0]];
    h+='<div style="font-size:8px;font-weight:700;color:'+g[2]+';margin-top:4px">'+g[1]+' ('+arr.length+')</div>';
    if(!arr.length){ h+=tabEmpty('none in this state.'); return; }
    arr.forEach(function(x){
      var need=Math.max(0, RULE_UNLOCK_N-x.en);
      h+='<div title="'+String(x.q.note||'').replace(/"/g,'&quot;')+'" style="font-size:8.5px;line-height:1.35;border-top:1px solid rgba(255,255,255,.04);padding:2px 0;white-space:normal">'+
        '<b style="color:'+PAL.ink+'">'+x.q.id+'</b> <span style="color:'+PAL.sub+'">'+x.q.feature+'</span>'+
        '<div style="color:'+PAL.sub+'">condition: '+((x.q.when||[]).map(function(w){ return w.f+'='+w.v; }).join(' & ')||'—')+' → outcome '+(x.q.outcome||'—')+'</div>'+
        '<div style="color:'+PAL.sub+'">'+(x.rate==null?('● recording · '+nTxt(x.n)):(x.rate+'% · '+nTxt(x.n)))+
          (need>0?(' · need '+need+' more effective observations (~'+Math.ceil(need*FEAT_FWD/100)+' more sessions)'):' · unlocked')+'</div>'+
      '</div>';
    });
  });
  return h;
}
