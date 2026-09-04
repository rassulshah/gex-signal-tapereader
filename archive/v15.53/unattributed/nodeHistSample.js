// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function nodeHistSample(sym, m){
  try{
    sym=sym||'SPY';
    if(!m){ try{ m=nodeMapModel(sym); }catch(e1){ m=null; } }
    if(!m || !m.ok) return null;
    var row=nodeHistRow(m);
    if(row.px==null) return null;
    var H=nodeHistLoad();
    var arr=H.sym[sym]||(H.sym[sym]=[]);
    var bar=(STATE[sym]||{}).lastClosedB||0;
    var last=arr.length?arr[arr.length-1]:null;
    var rec={ d:TODAY, t:Date.now(), bar:bar, px:row.px, ceil:row.ceil, flr:row.flr };
    if(last && last.d===TODAY && last.bar===bar) arr[arr.length-1]=rec;
    else arr.push(rec);
    if(arr.length>NODEHIST_MAX) H.sym[sym]=arr.slice(arr.length-NODEHIST_MAX);
    nodeHistSave();
    var a2=H.sym[sym];
    return a2[a2.length-1];
  }catch(e){ return null; }
}
