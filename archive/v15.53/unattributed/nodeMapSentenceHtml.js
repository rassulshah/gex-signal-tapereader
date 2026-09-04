// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function nodeMapSentenceHtml(m, sym, tagFn, tip){
  var r=(m&&m.ok)?nodeMapSentence(m, sym, tagFn):{verdict:'NONE',text:''};
  // (v10.47b) nothing engaged => no sentence at all (space); the ⑤ icon rides on the column header
  if(r.verdict==='NONE' || r.verdict==='NO NODE IN PLAY') return '';
  var col={CONT:PAL.longAccent, REV:PAL.shortAccent, DEFL:PAL.shortAccent, TBD:PAL.gold, 'NO NODE IN PLAY':PAL.sub, NONE:PAL.sub}[r.verdict]||PAL.sub;
  if(r.node && r.node.side==='below' && (r.verdict==='CONT')) col=PAL.shortAccent;   // continuing DOWN through a floor = bearish
  if(r.node && r.node.side==='below' && (r.verdict==='REV'||r.verdict==='DEFL')) col=PAL.longAccent; // bouncing off a floor = bullish
  return '<div title="'+(tip||'').replace(/"/g,'')+' Full text: '+(r.verdict+' '+r.text).replace(/<[^>]+>/g,'').replace(/"/g,'')+'" style="margin:4px 2px 3px;font-size:9.5px;line-height:1.4;color:'+PAL.ink+';display:-webkit-box;-webkit-line-clamp:3;-webkit-box-orient:vertical;overflow:hidden">'+
    '<b style="color:'+col+'">'+r.verdict+'</b> '+r.text+'</div>';
}
