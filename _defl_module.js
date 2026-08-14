// ============================================================================
// (v10.36) DEFLECTION SIGNALS  — section rendered ABOVE the Node Map.
// A deflection is only meaningful once price TAPS a node and REVERSES away
// (deflectionAt() already confirms the mechanics). This module:
//   1. classifies each confirmed deflection by SETUP TYPE + confluence
//      (King / Gate / Rug / Reverse-Rug / Pika / Barney / Floor / Ceiling,
//       plus the BO·FT-retest flavor and the FBO false-breakout flavor);
//   2. RECORDS every new confirmed deflection into the recorder day so its
//      FORWARD outcome (did price continue in the deflection direction?) can
//      be measured — grading is HONEST: hidden until enough real samples;
//   3. computes per-setup CONTINUATION RATE => letter grade;
//   4. renders the "⚡ Deflection Signals" section.
// Grades stay HIDDEN (recording state) until a setup crosses its unlock
// sample size. The unlock N is auto-recommended from observed daily volume
// (see deflUnlockN) rather than a guessed constant.
// ============================================================================

var DEFL_FWD_BARS   = 10;    // forward window (closed 3m bars) to score continuation
var DEFL_CONT_PTS   = 0.30;  // price must extend >= this many strikes in the deflect dir to count as "continued"
var DEFL_UNLOCK_MIN = 5;     // floor for the auto-tuned unlock sample size
var DEFL_UNLOCK_MAX = 25;    // cap for the auto-tuned unlock sample size

// Classify a confirmed deflection at level L into a setup label + confluence chips.
// dir: +1 = deflected UP off the node (bounce), -1 = deflected DOWN off it (rejection).
function classifyDeflection(sym, L, dir){
  var chips=[];
  var name, prio;
  // BO·FT retest flavor: the node had a breakout+follow-through, then price pulled
  // back to it and deflected — the highest-value flavor the user called out.
  var oc = nodeOutcome(sym, L.k);            // 'up'|'dn'|'held'|'false'|null
  var isFBO = (oc==='false');
  var brokeFT = (oc==='up'||oc==='dn');
  var pulledBack = true;                     // deflectionAt already proved a tap+reverse at the node

  if(L.isKing){        name='King deflection';    prio=90; chips.push({t:'King',c:'amber'}); }
  else if(L.isGatekeeper){ name='Gate deflection'; prio=85; chips.push({t:'Gate',c:'blue'}); }
  else if(L.isRugCeil||L.isRugFloor||L.isRugTarget){
    var rug=(nodeMapModel(sym).rug)||null;
    if(rug && rug.type==='reverse'){ name='Reverse Rug'; prio=80; chips.push({t:'RRug',c:'teal'}); }
    else { name='Rug'; prio=80; chips.push({t:'Rug',c:'teal'}); }
  }
  else if(L.role==='Pika'){ name='Pika deflection'; prio=60; chips.push({t:'Pika',c:'purple'}); }
  else if(L.role==='Barn'){ name='Barney deflection'; prio=60; chips.push({t:'Barn',c:'purple'}); }
  else if(dir>0){ name='Floor deflection'; prio=50; chips.push({t:'Flr',c:'green'}); }
  else { name='Ceiling deflection'; prio=50; chips.push({t:'Ceil',c:'red'}); }

  // BO·FT-retest is a flavor that STACKS on top of the base setup (user's marquee case).
  if(brokeFT && pulledBack){ chips.unshift({t:'BO\u00b7FT',c:'green'}); chips.push({t:'pullback',c:'amber'}); prio+=8; name='\u2b51 '+name+' (BO\u00b7FT retest)'; }
  if(isFBO){ chips.push({t:'FBO',c:'red'}); }

  chips.push({t:'deflect',c:'purple'});
  return { name:name, chips:chips, prio:prio, isFBO:isFBO, brokeFT:brokeFT };
}

// Stable key for a setup TYPE (so per-type stats aggregate across strikes/days).
function deflSetupKey(cls, dir){
  var base = cls.name.replace(/^\u2b51\s*/,'').replace(/\s*\(BO\u00b7FT retest\)$/,'');
  var ft = cls.brokeFT ? '+boft' : '';
  return base + ft + (dir>0?':up':':dn');
}

// Record any NEW confirmed deflection for this symbol into the recorder day.
// De-dupes on (setupKey, strike, tapBar-ish) so one deflection = one event.
function recordDeflections(sym){
  try{
    if(RECORDER_SYMS.indexOf(sym)<0 || !TODAY) return;
    var m=nodeMapModel(sym); if(!m || !m.ok || !m.levels) return;
    var S=STATE[sym]||{}; var px=S.price; if(px==null) return;
    var db=recorderLoad(); var day=recorderDay(db);
    if(!day.defl) day.defl={SPY:[],QQQ:[]};
    if(!day.defl[sym]) day.defl[sym]=[];
    var arr=day.defl[sym];
    var barN=(S.candles||[]).length;
    var changed=false;

    m.levels.forEach(function(L){
      var d=deflectionAt(sym, L.k);
      if(!d) return;
      var cls=classifyDeflection(sym, L, d.dir);
      var key=deflSetupKey(cls, d.dir);
      // signature: one event per (key, strike) per fresh tap — collapse re-fires within DEFL_FWD_BARS
      var sig=key+'@'+L.k.toFixed(2);
      var dup=arr.some(function(e){ return e.sig===sig && (barN - (e.tapBar||0)) < DEFL_FWD_BARS; });
      if(dup) return;
      arr.push({
        sig:sig, key:key, name:cls.name,
        strike:L.k, dir:d.dir, awayPts:d.awayPts, bars:d.bars,
        px0:px, tapBar:barN, t:Date.now(),
        chips:cls.chips.map(function(c){return c.t;}),
        cont:null, contBar:null          // forward outcome, filled by labelDeflectionOutcomes
      });
      changed=true;
    });
    if(arr.length>RECORDER_MAX_EVENTS) day.defl[sym]=arr.slice(arr.length-RECORDER_MAX_EVENTS);
    // score forward outcomes for events whose window has now elapsed
    if(labelDeflectionOutcomes(sym, day)) changed=true;
    if(changed) recorderSave(db);
  }catch(e){}
}

// For each recorded deflection whose forward window has elapsed, mark whether
// price CONTINUED in the deflection direction by >= DEFL_CONT_PTS.
function labelDeflectionOutcomes(sym, day){
  var S=STATE[sym]||{}; var cs=S.candles||[]; var n=cs.length;
  var arr=(day.defl&&day.defl[sym])||[]; var changed=false;
  arr.forEach(function(e){
    if(e.cont!=null) return;                       // already scored
    var elapsed = n - (e.tapBar||0);
    if(elapsed < DEFL_FWD_BARS) return;            // window not closed yet
    var startIdx=e.tapBar, endIdx=Math.min(n, e.tapBar+DEFL_FWD_BARS);
    var ext=0;
    for(var i=startIdx;i<endIdx;i++){ var b=cs[i]; if(!b||b.c==null) continue;
      var move=(b.c - e.px0)*e.dir;               // positive = moved in deflect dir
      if(move>ext) ext=move;
    }
    e.cont = (ext >= DEFL_CONT_PTS) ? 1 : 0;
    e.contBar = endIdx;
    changed=true;
  });
  return changed;
}

// Aggregate ALL recorded days into per-setup continuation stats for a symbol.
function deflStats(sym){
  var out={ perKey:{}, totalResolved:0, daysSeen:0, perDayCount:0 };
  try{
    var db=recorderLoad(); var days=db.days||{};
    var dayKeys=Object.keys(days); var totalEvents=0;
    dayKeys.forEach(function(dk){
      var day=days[dk]; var arr=(day.defl&&day.defl[sym])||[];
      if(arr.length) out.daysSeen++;
      totalEvents+=arr.length;
      arr.forEach(function(e){
        var b=out.perKey[e.key]||(out.perKey[e.key]={n:0,hit:0,pending:0,name:e.name,dir:e.dir});
        if(e.cont==null){ b.pending++; }
        else { b.n++; if(e.cont) b.hit++; out.totalResolved++; }
      });
    });
    out.perDayCount = out.daysSeen>0 ? +(totalEvents/out.daysSeen).toFixed(1) : 0;
  }catch(e){}
  return out;
}

// Auto-tuned unlock sample size: the more setups you record per day, the higher
// the bar we can afford (cheap to reach) — capped both ways. Recommended, not final.
function deflUnlockN(perDayCount){
  if(!perDayCount || perDayCount<=0) return DEFL_UNLOCK_MIN;
  var n=Math.round(perDayCount*3);               // ~3 trading days' worth
  return Math.max(DEFL_UNLOCK_MIN, Math.min(DEFL_UNLOCK_MAX, n));
}

// Continuation rate -> letter grade (thresholds match the mockup legend).
function deflGrade(rate){
  if(rate>=75) return {g:'A+',col:PAL.longAccent};
  if(rate>=68) return {g:'A', col:PAL.longAccent};
  if(rate>=58) return {g:'B', col:PAL.blue};
  if(rate>=45) return {g:'C', col:PAL.amber};
  return {g:'D', col:PAL.shortAccent};
}

function _deflChipHtml(t){
  var map={amber:PAL.amber, blue:PAL.blue, teal:'#39c5cf', green:PAL.longAccent,
           red:PAL.shortAccent, purple:'#bc8cff'};
  var col=map[({ 'King':'amber','Gate':'blue','RRug':'teal','Rug':'teal','Pika':'purple',
    'Barn':'purple','Flr':'green','Ceil':'red','BO\u00b7FT':'green','pullback':'amber',
    'FBO':'red','deflect':'purple' })[t]] || PAL.sub;
  return '<span style="font-size:9px;font-weight:700;padding:0 5px;border-radius:999px;border:1px solid '+col+';color:'+col+'">'+t+'</span>';
}

// The Deflection Signals section (rendered above the Node Map).
function deflectionBlock(){
  var sym='SPY';
  var m=nodeMapModel(sym); if(!m||!m.ok) return '';
  var stats=deflStats(sym);
  var unlockN=deflUnlockN(stats.perDayCount);

  // collect LIVE confirmed deflections right now (for display rows)
  var rows=[];
  (m.levels||[]).forEach(function(L){
    var d=deflectionAt(sym, L.k); if(!d) return;
    var cls=classifyDeflection(sym, L, d.dir);
    var key=deflSetupKey(cls, d.dir);
    var b=stats.perKey[key]||{n:0,hit:0};
    rows.push({L:L, d:d, cls:cls, key:key, n:b.n, hit:b.hit});
  });
  rows.sort(function(a,b){ return b.cls.prio-a.cls.prio; });

  var hdr=sectionHdrRight('\u26a1 Deflection Signals',
    '<span style="font-size:9px;color:'+PAL.sub+'">unlock n\u2265'+unlockN+' \u00b7 '+stats.totalResolved+' resolved</span>',
    'A deflection = price taps a node and reverses away (\u2265'+DEFLECT_CONFIRM+' bars). Grade is the setup\u2019s measured forward continuation rate over '+DEFL_FWD_BARS+' bars \u2014 HIDDEN until n\u2265'+unlockN+' real outcomes.');
  var html=hdr;

  if(!rows.length){
    html+='<div style="color:'+PAL.sub+';font-size:11px;padding:6px 2px">No confirmed deflections right now \u2014 watching for taps + reversals.</div>';
    return html;
  }

  rows.forEach(function(r){
    var dirTxt=(r.d.dir>0)?'\u25b2 up':'\u25bc down';
    var dirCol=(r.d.dir>0)?PAL.longAccent:PAL.shortAccent;
    var chipsHtml=r.cls.chips.map(function(c){return _deflChipHtml(c.t);}).join(' ');
    // grade: HIDDEN until unlock; else earned letter + rate + n
    var gradeHtml;
    if(r.n>=unlockN){
      var rate=Math.round(100*r.hit/r.n);
      var g=deflGrade(rate);
      gradeHtml='<span style="font-size:17px;font-weight:800;color:'+g.col+'">'+g.g+'</span>'+
                '<span style="display:block;font-size:9px;color:'+PAL.sub+'">'+rate+'% \u00b7 n='+r.n+'</span>';
    } else {
      gradeHtml='<span style="display:inline-flex;align-items:center;gap:4px;font-size:10px;font-weight:700;color:'+PAL.sub+';border:1px dashed '+PAL.sub+';border-radius:999px;padding:1px 7px">\u25cf recording</span>'+
                '<span style="display:block;font-size:9px;color:'+PAL.sub+'">n='+r.n+'/'+unlockN+'</span>';
    }
    html+='<div style="display:grid;grid-template-columns:46px 1fr auto;gap:8px;align-items:center;padding:6px 4px;border-top:1px solid rgba(139,152,169,0.12)">'+
      '<div><div style="font-size:14px;font-weight:800;color:'+PAL.ink+'">'+fmtNum(r.L.k)+'</div><div style="font-size:9px;color:'+dirCol+'">'+dirTxt+'</div></div>'+
      '<div style="min-width:0"><div style="font-size:12px;font-weight:700;color:'+PAL.ink+'">'+r.cls.name+'</div><div style="margin-top:3px">'+chipsHtml+'</div></div>'+
      '<div style="text-align:right">'+gradeHtml+'</div>'+
    '</div>';
  });
  return html;
}
