// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function briefBlockHtml(sym){
  try{
    var sb=sessionBucket();
    if(!BRIEF_FORCE && sb.bucket!=='pre-open') return '';
    var txt=briefLine(sym);
    var tip=('PRE-OPEN BRIEF — a descriptive snapshot of the map before the bell plus how yesterday’s grade-A direction calls '+
      'actually resolved. Click to expand. No forecast, no plan, no levels to act on: it is orientation only. Force it any time with __gptsDebug.brief().').replace(/"/g,'');
    return '<div class="gpts-brief" data-gbrief="1" title="'+tip+'" style="cursor:pointer;font-size:9px;color:'+PAL.sub+';background:'+PAL.card+';border:1px solid '+PAL.line+';border-radius:6px;padding:2px 6px;margin:2px 2px 3px;'+
      (BRIEF_OPEN?'white-space:normal;line-height:1.45':'overflow:hidden;white-space:nowrap;text-overflow:ellipsis')+'">'+
      (BRIEF_OPEN?'▾ ':'▸ ')+txt+'</div>';
  }catch(e){ return ''; }
}
