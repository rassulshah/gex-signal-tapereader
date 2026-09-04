// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function convergenceSvg(sym,st){
  var day=A_day(); var snaps=(day.snaps&&day.snaps[sym])||[];
  var gaps=[]; snaps.forEach(function(s){ if(typeof s.king==='number'&&typeof s.px==='number') gaps.push({i:gaps.length,g:s.king-s.px}); });
  if(gaps.length<2) return '<svg width="280" height="50" viewBox="0 0 280 50" style="display:block;width:100%"><text x="6" y="28" fill="'+PAL.sub+'" font-size="11">not enough bars yet</text></svg>';
  var fitW=_bodyW(); var W=Math.max(fitW, gaps.length*6), H=72, mid=36, padL=4, padR=4;
  var mx=1; gaps.forEach(function(p){ mx=Math.max(mx,Math.abs(p.g)); });
  function X(i){ return padL+(i/(gaps.length-1))*(W-padL-padR); }
  function Y(g){ return mid - (g/mx)*(mid-10); }
  var svg='<svg width="'+W+'" height="'+H+'" viewBox="0 0 '+W+' '+H+'" style="display:block;'+(W<=fitW?'width:100%':'')+'">';
  svg+='<line x1="0" y1="'+mid+'" x2="'+W+'" y2="'+mid+'" stroke="'+PAL.line+'" stroke-dasharray="3 3"/>';
  svg+='<text x="4" y="11" fill="'+PAL.sub+'" font-size="8">King above price (+)</text>';
  svg+='<text x="4" y="'+(H-4)+'" fill="'+PAL.sub+'" font-size="8">King below price (\u2212)</text>';
  // shade area between gap line and zero (green when converging toward 0 handled visually by proximity)
  var p=''; gaps.forEach(function(pt){ p+=(p?' L ':'M ')+X(pt.i).toFixed(1)+' '+Y(pt.g).toFixed(1); });
  svg+='<path d="'+p+'" fill="none" stroke="#b58ce0" stroke-width="2"/>';
  svg+='</svg>'; return svg;
}
