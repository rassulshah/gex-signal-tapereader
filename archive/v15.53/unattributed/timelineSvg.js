// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function timelineSvg(sym,st){
  var day=A_day(); var snaps=(day.snaps&&day.snaps[sym])||[];
  if(snaps.length<2) return '<svg width="280" height="60" viewBox="0 0 280 60" style="display:block;width:100%"><text x="6" y="32" fill="'+PAL.sub+'" font-size="11">not enough bars yet \u2014 need \u22652</text></svg>';
  // Responsive width: fit the panel, but give each bar >=6px; scroll only if wider.
  var fitW=_bodyW(); var W=Math.max(fitW, snaps.length*6); var H=132, padT=10, padB=32, padL=4, padR=4;
  var ks=[],ps=[]; snaps.forEach(function(s){ if(typeof s.king==='number')ks.push(s.king); if(typeof s.px==='number')ps.push(s.px); });
  var all=ks.concat(ps); var lo=Math.min.apply(null,all), hi=Math.max.apply(null,all);
  // pad the range so a nearly-flat King/price don't collapse onto one line
  var span=hi-lo; if(span<0.5){ var mid=(hi+lo)/2; lo=mid-0.5; hi=mid+0.5; span=1; } else { lo-=span*0.12; hi+=span*0.12; }
  function X(i){ return padL + (i/(snaps.length-1))*(W-padL-padR); }
  function Y(v){ return padT + (1-(v-lo)/(hi-lo))*(H-padT-padB); }
  var bw=(W-padL-padR)/snaps.length;
  var svg='<svg width="'+W+'" height="'+H+'" viewBox="0 0 '+W+' '+H+'" style="display:block;'+(W<=fitW?'width:100%':'')+'">';
  // S/R dominance band per bar — VISIBLE tint (green support / red resistance)
  snaps.forEach(function(s,i){ if(!s.sig||!s.sig.srb) return;
    var c=(s.sig.srb.dom==='support')?'#2ec27e':(s.sig.srb.dom==='resistance')?'#f0616d':null; if(!c) return;
    svg+='<rect x="'+(X(i)-bw/2).toFixed(1)+'" y="'+padT+'" width="'+bw.toFixed(1)+'" height="'+(H-padT-padB)+'" fill="'+c+'" opacity="0.13"/>'; });
  // price line first (under King), light + slightly thicker so both read
  var pp=''; snaps.forEach(function(s,i){ if(typeof s.px!=='number')return; pp+=(pp?' L ':'M ')+X(i).toFixed(1)+' '+Y(s.px).toFixed(1); });
  svg+='<path d="'+pp+'" fill="none" stroke="#c8d3df" stroke-width="1.6" opacity="0.9"/>';
  // King staircase on top, gold
  var kp=''; snaps.forEach(function(s,i){ if(typeof s.king!=='number')return; kp+=(kp?' L ':'M ')+X(i).toFixed(1)+' '+Y(s.king).toFixed(1); });
  svg+='<path d="'+kp+'" fill="none" stroke="'+PAL.gold+'" stroke-width="2.2"/>';
  // crossover markers
  snaps.forEach(function(s,i){ if(s.sig&&s.sig.srb&&s.sig.srb.cross){ var c=s.sig.srb.cross==='bears'?PAL.shortAccent:PAL.longAccent;
    svg+='<circle cx="'+X(i).toFixed(1)+'" cy="'+Y(s.px||s.king).toFixed(1)+'" r="3.5" fill="'+c+'" stroke="#0b0e14" stroke-width="0.8"/>'; } });
  // compact single-row legend at the bottom
  var ly=H-9;
  svg+='<line x1="'+padL+'" y1="'+ly+'" x2="'+(padL+14)+'" y2="'+ly+'" stroke="'+PAL.gold+'" stroke-width="2.2"/><text x="'+(padL+18)+'" y="'+(ly+3)+'" fill="'+PAL.sub+'" font-size="9">King</text>'+
       '<line x1="'+(padL+50)+'" y1="'+ly+'" x2="'+(padL+64)+'" y2="'+ly+'" stroke="#c8d3df" stroke-width="1.6"/><text x="'+(padL+68)+'" y="'+(ly+3)+'" fill="'+PAL.sub+'" font-size="9">price</text>'+
       '<rect x="'+(padL+102)+'" y="'+(ly-4)+'" width="8" height="8" fill="'+PAL.longAccent+'" opacity="0.5"/><text x="'+(padL+113)+'" y="'+(ly+3)+'" fill="'+PAL.sub+'" font-size="9">sup</text>'+
       '<rect x="'+(padL+138)+'" y="'+(ly-4)+'" width="8" height="8" fill="'+PAL.shortAccent+'" opacity="0.5"/><text x="'+(padL+149)+'" y="'+(ly+3)+'" fill="'+PAL.sub+'" font-size="9">res</text>';
  svg+='</svg>'; return svg;
}
