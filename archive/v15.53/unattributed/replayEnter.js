// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function replayEnter(){
  if(REPLAY.on) return;
  REPLAY.on=true;
  var start=null; try{ start=sessionDayStr(); }catch(e){}
  replayLoadDays(function(days){
    var d=start;
    if(!days.length){ REPLAY.err='the repository holds no recorded days'; try{ render(); }catch(e2){} return; }
    if(days.indexOf(d)<0) d=days[days.length-1];
    replayLoadDay(d);
  });
  try{ render(); }catch(e3){}
}
