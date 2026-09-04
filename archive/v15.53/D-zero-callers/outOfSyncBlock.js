// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function outOfSyncBlock(r){
  var why={ 'no-consensus':'the three King sources disagree',
            'no-source':'no tape and no feed could be read',
            'single-source':'only one King source is available',
            'parse-invariant':'the tape parse failed an internal invariant' }[r.reason] || r.reason;
  var rows='';
  var names=['tag','feed','tapemax'];
  var label={tag:'tape $K tag', feed:'raw feed', tapemax:'tape max %King'};
  for(var i=0;i<names.length;i++){
    var v=r.votes?r.votes[names[i]]:null;
    var shown=(typeof v==='number')?v:'—';
    var colr=(typeof v==='number' && r.king!=null && v===r.king)?PAL.longAccent:PAL.shortAccent;
    rows+='<div style="display:flex;justify-content:space-between;font-size:10px;padding:1px 0">'+
      '<span style="color:'+PAL.sub+'">'+label[names[i]]+'</span>'+
      '<span style="color:'+colr+';font-weight:600">'+shown+'</span></div>';
  }
  return '<div style="border:1px solid '+PAL.shortAccent+';border-radius:6px;padding:7px 8px;margin:4px 0;background:rgba(240,97,109,0.07)">'+
    '<div style="color:'+PAL.shortAccent+';font-weight:700;font-size:11px;letter-spacing:0.3px">'+
      '⚠ STRUCTURAL READ SUPPRESSED</div>'+
    '<div style="color:'+PAL.sub+';font-size:10px;margin:3px 0 5px 0;line-height:1.35">'+
      'App is out of sync with the tape — '+why+'. Node strengths, King and targets are '+
      'hidden rather than shown wrong.'+
      (r.recurring?' <span style="color:'+PAL.amber+';font-weight:600">RECURRING ('+r.streak+' consecutive).</span>':'')+
    '</div>'+ rows +
    '<div style="color:'+PAL.sub+';font-size:9px;margin-top:5px;opacity:0.8">'+
      'Diagnose: __gptsDebug.syncReport()</div>'+
    '</div>';
}
