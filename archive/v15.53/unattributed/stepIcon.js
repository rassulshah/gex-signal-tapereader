// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function stepIcon(n, extraStyle){
  return '<span class="gs-ico" data-gstep="'+n+'" title="Step '+n+' \u2014 click for the method" '+
    'style="width:16px;height:16px;border-radius:50%;border:1px solid '+PAL.sub+';color:'+PAL.sub+';'+
    'font-size:9.5px;font-weight:800;display:inline-flex;align-items:center;justify-content:center;cursor:pointer;'+(extraStyle||'')+'">'+n+'</span>';
}
