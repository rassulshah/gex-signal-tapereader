// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function nodeChartHtml(sym){
  try{
    sym=sym||'SPY';
    var cs=closedCandles(sym)||[];
    if(cs.length<4) return '';
    var now=Date.now();
    var t1=cs[cs.length-1].t||now, t0=t1-NCHART_MIN*60000;
    var bars=cs.filter(function(c){ return (c.t||0)>=t0; });
    if(bars.length<3) bars=cs.slice(-30);
    if(bars.length<3) return '';
    t0=bars[0].t; t1=bars[bars.length-1].t;
    if(!(t1>t0)) return '';
    var lo=Infinity, hi=-Infinity;
    bars.forEach(function(c){ if(c.l<lo) lo=c.l; if(c.h>hi) hi=c.h; });
    var H=(HIST[sym]||{}), keys=[];
    for(var k in H){
      var kk=parseFloat(k); if(!isFinite(kk)) continue;
      var seq=(H[k]&&H[k].seq)||[]; if(!seq.length) continue;
      if(kk < lo-(hi-lo)*0.9 || kk > hi+(hi-lo)*0.9) continue;
      var peak=0;
      for(var q=0;q<seq.length;q++){ var vv=seq[q]&&seq[q].v; if(typeof vv==='number' && Math.abs(vv)>peak) peak=Math.abs(vv); }
      keys.push({k:kk, seq:seq, peak:peak});
    }
    keys.forEach(function(o){ if(o.k<lo) lo=o.k; if(o.k>hi) hi=o.k; });
    var pad=(hi-lo)*0.06 || 0.2; lo-=pad; hi+=pad;
    if(!(hi>lo)) return '';

    // (v11.38) THREE ZONES. Left is InsiderFinance STRUCTURE (net GEX, then net DEX) — the chain,
    // refreshed once a day, telling you where the walls are. Right is Skylit FLOW — live node strength
    // with 60m and 15m growth ticks, telling you what is happening to them now. The middle is price,
    // and level labels sit ON their own lines so neither gutter is spent on text.
    var W=416, HGT=NCHART_H+38, TOP=8, BOT=24;
    // (v13.0, user-directed) TWO ZONES, not three. The InsiderFinance GEX and DEX profiles are gone;
    // the Skylit NODE flow profile takes the left gutter they occupied, and the price plot takes back
    // the whole right side. ⚠ THE BARS NOW GROW RIGHTWARD from the left edge, so they still point IN
    // toward price — anchoring them at the gutter's right edge would have pointed them away from it.
    var SX=3, SW=56;                             // flow (NODES), left — was right
    var PL=64, PR=410;                           // price, taking the space the flow column vacated
    var ih=HGT-TOP-BOT, iw=PR-PL;
    function X(t){ return PL + ((t-t0)/(t1-t0))*iw; }
    function Y(p){ return TOP + (1-((p-lo)/(hi-lo)))*ih; }
    var rr=1; try{ rr=dispIsFut()?dispR():1; }catch(e9){}
    var g='';

    // ---------- (v13.0) THE IF STRUCTURE PROFILES ARE REMOVED ----------
    // Two gutter columns of once-a-day open-interest stock, competing for width with the live
    // tape. `ifChain`/`ifLadder` still feed the LEVELS engine and the depth tiers; only these two
    // drawn profiles are gone, so nothing that reads the chain lost its input.

    // ---------- RIGHT: Skylit flow, pointing INWARD, drawn as growth segments ----------
    // (v11.39) Two corrections. The bars now grow LEFTWARD from the right edge, so both profiles point
    // in toward price and the chart reads as a spine rather than two unrelated columns. And growth is
    // the SEGMENTS, not a tick mark: the oldest portion is dim, 60m→15m is mid, and the last 15 minutes
    // is bright. A node that is accumulating shows a bright leading edge; one that is bleeding shows
    // dim length with nothing new on the front. That is the brightest-newest encoding from the design.
    var flowN=0, SXA=SX;                          // LEFT edge — bars anchor here and grow toward price
    g+='<line x1="'+(SX+SW)+'" y1="'+TOP+'" x2="'+(SX+SW)+'" y2="'+(TOP+ih)+'" stroke="#1e2530" stroke-width="0.7"/>';
    (function(){
      function at(seq, msBack){
        var cut=t1-msBack, best=null;
        for(var q=0;q<seq.length;q++){ var pt=seq[q]; if(!pt||typeof pt.v!=='number') continue; if(pt.t<=cut) best=pt.v; }
        return best;
      }
      // Normalise across CURRENT AND HISTORICAL values. Scaling on current alone means a node that has
      // shrunk has a past larger than the axis, so its history clips to full width and the whole bar
      // renders as one dim block — decay becomes invisible, which is the opposite of the point.
      var mx=0;
      keys.forEach(function(o){
        var lastv=null;
        for(var q=o.seq.length-1;q>=0;q--){ if(o.seq[q]&&typeof o.seq[q].v==='number'){ lastv=o.seq[q].v; break; } }
        o.now=lastv;
        o.v60=at(o.seq,60*60000); o.v15=at(o.seq,15*60000);
        [lastv,o.v60,o.v15].forEach(function(v){ if(v!=null && Math.abs(v)>mx) mx=Math.abs(v); });
      });
      if(!(mx>0)) return;
      var maxW=SW-4;
      keys.forEach(function(o){
        if(o.now==null) return;
        var av=Math.abs(o.now); if(av<3) return;
        var y=Y(o.k)-2.2, col=(o.now>0)?'#a371f7':'#e3c341';
        var wNow=Math.max(1.2,(av/mx)*maxW);
        var v60=o.v60, v15=o.v15;
        // RAW widths first. Clamping these to wNow would erase the only case the shrink marker exists
        // for — a node that is smaller now than it was 15 minutes ago.
        var r60=(v60==null)?0:Math.max(0,Math.min(maxW,(Math.abs(v60)/mx)*maxW));
        var r15=(v15==null)?r60:Math.max(0,Math.min(maxW,(Math.abs(v15)/mx)*maxW));
        var w60=Math.min(r60,wNow), w15=Math.max(w60,Math.min(r15,wNow));
        function seg(from,to,op){
          if(to<=from) return;
          g+='<rect x="'+(SXA+from).toFixed(1)+'" y="'+y.toFixed(1)+'" width="'+(to-from).toFixed(1)+
             '" height="4.4" fill="'+col+'" opacity="'+op+'"/>';
        }
        seg(0,   w60, '0.30');                     // held for over an hour
        seg(w60, w15, '0.58');                     // added between 60m and 15m ago
        seg(w15, wNow,'0.95');                     // added in the last 15 minutes — the live edge
        // a node that SHRANK has its lost ground marked, so decay is visible rather than merely absent
        if(r15>wNow+0.6){
          g+='<line x1="'+(SXA+Math.min(maxW,r15)).toFixed(1)+'" y1="'+(y+0.4).toFixed(1)+'" x2="'+(SXA+wNow).toFixed(1)+
             '" y2="'+(y+3.8).toFixed(1)+'" stroke="'+col+'" stroke-width="0.7" opacity="0.35"/>';
        }
        flowN++;
      });
    })();

    // ---------- CENTRE: node bands, candles, rolls, centred level labels ----------
    // (v11.46) A NODE MUST BE VISIBLE WITH ONE SAMPLE. This drew a marker only where a history sample
    // fell inside the CANDLE window — so after any reload you saw a growing sliver, and with the market
    // closed (candles stop at the last close while sampleTapeHistory keeps writing "now") every sample
    // fell outside and NOTHING drew at all. Absence of history was rendering as absence of node, which
    // is the opposite of the truth. History still draws the lifecycle; the node's CURRENT state now
    // always draws at the right edge regardless.
    var drawn=0;
    keys.forEach(function(o){
      var y=Y(o.k); o.shown=0;
      o.seq.forEach(function(pt){
        if(pt==null||typeof pt.v!=='number') return;
        if(pt.t<t0-90000||pt.t>t1+90000) return;
        var x=X(Math.max(t0,Math.min(t1,pt.t))), av=Math.abs(pt.v);
        if(av<3) return;
        var op=Math.max(0.14,Math.min(0.95,av/100)), put=(pt.v>0), col=put?'#a371f7':'#e3c341', w=2.5, hh=2.1;
        g+= put
          ? '<path d="M'+(x-w)+' '+(y-hh)+' L'+(x+w)+' '+(y-hh)+' L'+x+' '+(y+hh)+' Z" fill="'+col+'" opacity="'+op.toFixed(2)+'"/>'
          : '<path d="M'+(x-w)+' '+(y+hh)+' L'+(x+w)+' '+(y+hh)+' L'+x+' '+(y-hh)+' Z" fill="'+col+'" opacity="'+op.toFixed(2)+'"/>';
        drawn++; o.shown++;
      });
      // the live edge: whatever the node reads NOW, drawn at the right of the plot
      var nowv=null;
      for(var q=o.seq.length-1;q>=0;q--){ if(o.seq[q]&&typeof o.seq[q].v==='number'){ nowv=o.seq[q].v; break; } }
      if(nowv!=null && Math.abs(nowv)>=3){
        var av2=Math.abs(nowv), op2=Math.max(0.20,Math.min(0.95,av2/100));
        var put2=(nowv>0), col2=put2?'#a371f7':'#e3c341', xE=PR-3, w2=2.9, h2=2.4;
        g+= put2
          ? '<path d="M'+(xE-w2)+' '+(y-h2)+' L'+(xE+w2)+' '+(y-h2)+' L'+xE+' '+(y+h2)+' Z" fill="'+col2+'" opacity="'+op2.toFixed(2)+'"/>'
          : '<path d="M'+(xE-w2)+' '+(y+h2)+' L'+(xE+w2)+' '+(y+h2)+' L'+xE+' '+(y-h2)+' Z" fill="'+col2+'" opacity="'+op2.toFixed(2)+'"/>';
        drawn++; o.shown++;
      }
    });
    var bw=Math.max(1.2, Math.min(5, (iw/bars.length)*0.62));
    bars.forEach(function(c){
      var x=X(c.t), up=(c.c>=c.o), col=up?'#2ec27e':'#f0616d';
      g+='<line x1="'+x.toFixed(1)+'" y1="'+Y(c.h).toFixed(1)+'" x2="'+x.toFixed(1)+'" y2="'+Y(c.l).toFixed(1)+'" stroke="'+col+'" stroke-width="0.7" opacity="0.74"/>';
      var yo=Y(c.o), yc=Y(c.c), yt=Math.min(yo,yc), hb=Math.max(0.8,Math.abs(yc-yo));
      g+='<rect x="'+(x-bw/2).toFixed(1)+'" y="'+yt.toFixed(1)+'" width="'+bw.toFixed(1)+'" height="'+hb.toFixed(1)+'" fill="'+col+'" opacity="0.8"/>';
    });
    // (v11.38) THE 50-SMA IS THE PRIMARY READ AND IT WAS NEVER ON OUR CHART — the verdict came from us
    // while the line itself was only on Atlas. contSMAAtTodayIdx walks the CONTINUOUS series at each of
    // today's closed-bar indices, which is what trendVerdict reads, so the line and the call cannot
    // disagree. `bars` is a tail slice of closedCandles, so the index offset has to be carried through.
    try{
      var period=(function(){ var mp=parseInt(CFG.trendMA[sym],10); return (isNaN(mp)||mp<1)?50:mp; })();
      var allC=closedCandles(sym)||[], off=allC.length-bars.length;
      var prev=null;
      bars.forEach(function(c,ix){
        var v=contSMAAtTodayIdx(sym, period, off+ix);
        if(typeof v!=='number' || v<lo || v>hi){ prev=null; return; }
        var pt=[X(c.t), Y(v)];
        if(prev) g+='<line x1="'+prev[0].toFixed(1)+'" y1="'+prev[1].toFixed(1)+'" x2="'+pt[0].toFixed(1)+'" y2="'+pt[1].toFixed(1)+'" stroke="#e06c4f" stroke-width="1.2" opacity="0.9"/>';
        prev=pt;
      });
    }catch(eSm){}
    // rolls
    var RL=null; try{ RL=rollDetect(sym); }catch(eR){}
    var rollLines=[];
    if(RL && !RL.err){
      [['ceil','#f0616d','g3ad'],['flr','#2ec27e','g3au']].forEach(function(cfg){
        var e=RL[cfg[0]]; if(!e||!e.toward) return;
        if(e.from<lo||e.from>hi||e.to<lo||e.to>hi) return;
        var yF=Y(e.from), yT=Y(e.to); if(Math.abs(yT-yF)<6) return;
        var xa=PL+iw*0.30, xb=PL+iw*0.62, dn=(yT>yF);
        g+='<path d="M'+xa.toFixed(1)+' '+(yF+(dn?3:-3)).toFixed(1)+' C'+(xa+8).toFixed(1)+' '+((yF+yT)/2).toFixed(1)+
           ' '+(xb-8).toFixed(1)+' '+((yF+yT)/2).toFixed(1)+' '+xb.toFixed(1)+' '+(yT+(dn?-4:4)).toFixed(1)+
           '" fill="none" stroke="'+cfg[1]+'" stroke-width="1.5" opacity="0.9" marker-end="url(#'+cfg[2]+')"/>';
        g+='<text x="'+(xb+4).toFixed(1)+'" y="'+(yF+2.2).toFixed(1)+'" font-size="7" fill="'+cfg[1]+'" font-weight="500" opacity="0.45">'+dispNum(e.from*rr)+'</text>';
        g+='<text x="'+(xb+4).toFixed(1)+'" y="'+(yT+2.2).toFixed(1)+'" font-size="8" fill="'+cfg[1]+'" font-weight="800">'+dispNum(e.to*rr)+'</text>';
        rollLines.push({side:cfg[0], col:cfg[1], from:e.from, to:e.to, fromPct:e.fromPct, toPct:e.toPct});
      });
    }
    // centred level labels — the line BREAKS around the text instead of running under it
    // `p` positions the line on the UNDERLYING scale; `disp` is what gets printed. They are passed
    // separately on purpose: an IF row already carries a display value computed from the live SPX->ES
    // basis, and re-deriving it here through the SPY->ES ratio is a second conversion path that
    // disagrees by a point or two. Never recompute a number the source already gave you.
    function centreLvl(p, disp, name, col){
      if(p==null||p<lo||p>hi) return;
      var y=Y(p), txt=(name?name+' ':'')+dispNum(disp), tw=txt.length*4.0+6, cx=(PL+PR)/2;
      g+='<line x1="'+PL+'" y1="'+y.toFixed(1)+'" x2="'+(cx-tw/2).toFixed(1)+'" y2="'+y.toFixed(1)+'" stroke="'+col+'" stroke-width="0.7" stroke-dasharray="5,4" opacity="0.5"/>';
      g+='<line x1="'+(cx+tw/2).toFixed(1)+'" y1="'+y.toFixed(1)+'" x2="'+PR+'" y2="'+y.toFixed(1)+'" stroke="'+col+'" stroke-width="0.7" stroke-dasharray="5,4" opacity="0.5"/>';
      g+='<text x="'+cx.toFixed(1)+'" y="'+(y+2.5).toFixed(1)+'" font-size="7.5" fill="'+col+'" font-weight="700" text-anchor="middle">'+g3esc(txt)+'</text>';
    }
    // one centred-label pass so IF levels, strong nodes and price cannot overprint each other
    var taken=[];
    function free(y){ for(var i=0;i<taken.length;i++){ if(Math.abs(taken[i]-y)<8) return false; } taken.push(y); return true; }
    var px0=(STATE[sym]||{}).price;
    if(typeof px0==='number' && px0>=lo && px0<=hi) taken.push(Y(px0));   // price wins the middle
    var IL=null; try{ IL=ifLadder(sym); }catch(e0){}
    if(IL && !IL.err && IL.rows){
      IL.rows.slice().sort(function(a,b2){ return Math.abs(a.und-(px0||0))-Math.abs(b2.und-(px0||0)); })
      .forEach(function(r){
        if(r.und==null||r.und<lo||r.und>hi) return;
        if(!free(Y(r.und))) return;
        var isC=/CR/.test(r.id), isP=/PS/.test(r.id);
        centreLvl(r.und, r.disp, r.id.split('·')[0], isC?'#f0616d':(isP?'#2ec27e':'#8b98a9'));
      });
    }
    // (v11.38) STRONG NODES KEEP THEIR PRICE. The redesign moved labels off the gutters and briefly took
    // the node prices with them — a band you cannot read the price of is a shape, not a level.
    keys.slice().sort(function(a,b2){ return b2.peak-a.peak; }).forEach(function(o){
      if(o.peak<25 || !o.shown) return;
      if(!free(Y(o.k))) return;
      var lastv=null;
      for(var q=o.seq.length-1;q>=0;q--){ if(o.seq[q]&&typeof o.seq[q].v==='number'){ lastv=o.seq[q].v; break; } }
      centreLvl(o.k, o.k*rr, '', (lastv>0)?'#a371f7':'#e3c341');   // nodes are underlying-scale
    });
    var px=(STATE[sym]||{}).price;
    if(typeof px==='number' && px>=lo && px<=hi){
      var yp=Y(px), cx=(PL+PR)/2;
      g+='<line x1="'+PL+'" y1="'+yp.toFixed(1)+'" x2="'+PR+'" y2="'+yp.toFixed(1)+'" stroke="#f3f6fa" stroke-width="0.8" opacity="0.8"/>';
      g+='<rect x="'+(cx-18)+'" y="'+(yp-5).toFixed(1)+'" width="36" height="10" rx="2" fill="#0b0e14" opacity="0.9"/>';
      g+='<text x="'+cx+'" y="'+(yp+2.6).toFixed(1)+'" font-size="7.5" fill="#f3f6fa" font-weight="800" text-anchor="middle">'+dispNum(px*rr)+'</text>';
    }
    // ---------- AXES ----------
    // (v11.42) A chart without scales is a picture. Price ticks run down the inside of the plot's left
    // edge and time along the bottom, both dim enough to read past. Ticks land on ROUND numbers, chosen
    // from the range rather than by dividing it, so they stay stable as the window scrolls instead of
    // renumbering on every bar.
    (function(){
      var span=hi-lo;
      var steps=[1,2,2.5,5,10,20,25,50,100];
      var raw=span/4, stepv=steps[steps.length-1];
      for(var si=0;si<steps.length;si++){ if(steps[si]>=raw){ stepv=steps[si]; break; } }
      var first=Math.ceil(lo/stepv)*stepv;
      for(var pv=first; pv<=hi; pv+=stepv){
        var y=Y(pv);
        if(y<TOP+6 || y>TOP+ih-4) continue;
        g+='<line x1="'+PL+'" y1="'+y.toFixed(1)+'" x2="'+(PL+4)+'" y2="'+y.toFixed(1)+'" stroke="#2a3140" stroke-width="0.7"/>';
        g+='<text x="'+(PL+6)+'" y="'+(y+2.2).toFixed(1)+'" font-size="6" fill="#5b6675">'+dispNum(pv*rr)+'</text>';
      }
      // time: a mark every 30 minutes of the window, labelled in the chart's own clock
      var mins=(t1-t0)/60000, stepM=(mins>75)?30:15;
      var d0=new Date(t0);
      var m0=d0.getMinutes(), roll=(stepM-(m0%stepM))%stepM;
      for(var tt=t0+roll*60000; tt<=t1; tt+=stepM*60000){
        var x=X(tt);
        if(x<PL+10||x>PR-10) continue;
        g+='<line x1="'+x.toFixed(1)+'" y1="'+(TOP+ih)+'" x2="'+x.toFixed(1)+'" y2="'+(TOP+ih+3)+'" stroke="#2a3140" stroke-width="0.7"/>';
        var dt=new Date(tt), hh=dt.getHours(), mm=dt.getMinutes();
        g+='<text x="'+x.toFixed(1)+'" y="'+(TOP+ih+9)+'" font-size="6" fill="#5b6675" text-anchor="middle">'+
           (hh<10?'0':'')+hh+':'+(mm<10?'0':'')+mm+'</text>';
      }
      g+='<line x1="'+PL+'" y1="'+(TOP+ih)+'" x2="'+PR+'" y2="'+(TOP+ih)+'" stroke="#1e2530" stroke-width="0.7"/>';
    })();
    // zone captions
    g+='<text x="'+SX+'" y="'+(HGT-5)+'" font-size="6" fill="#8b98a9" font-weight="700">NODES</text>';
    g+='<text x="'+(PR-2)+'" y="'+(HGT-5)+'" font-size="6" fill="#5b6675" text-anchor="end">Skylit · flow</text>';

    var tip='What is this picture telling you? LEFT is Skylit NODE FLOW — live node strength, drawn as growth segments. '+
            'A bar is a level the market is holding options against, and its LENGTH is how much. Bars grow toward price, so the longest reach nearest the candles. '+
            'The dim portion is what the node has held for over an hour, the mid portion is what it added between 60 and 15 minutes ago, and the bright leading edge is the last 15 minutes. '+
            'A bright front edge is accumulation happening NOW; a bar with no bright edge is bleeding, and a thin diagonal marks ground it has actually lost. '+
            'That is the level being defended or abandoned, in real time. '+
            'RIGHT is price on the same vertical scale, so a candle meeting a bar is a test of that node. '+
            'Node rows brighten as they strengthen and an arrow appears when mass transfers between strikes. '+
            'KEY — yellow node markers are call-dominant and purple put-dominant; '+
            'the orange line is the 50-SMA; on the flow bars the dim portion is what a node has held for over an hour, the mid portion is what it added between 60 and 15 minutes ago, '+
            'and the bright leading edge is the last 15 minutes. A bright front edge is accumulation happening now.';
    var defs='<defs>'+
      '<marker id="g3ad" viewBox="0 0 8 8" refX="4" refY="4" markerWidth="5" markerHeight="5" orient="auto"><path d="M0 0 L8 4 L0 8 z" fill="#f0616d"/></marker>'+
      '<marker id="g3au" viewBox="0 0 8 8" refX="4" refY="4" markerWidth="5" markerHeight="5" orient="auto"><path d="M0 0 L8 4 L0 8 z" fill="#2ec27e"/></marker></defs>';
    var out='<div class="g3b" style="padding-top:2px">';
    out+='<svg viewBox="0 0 '+W+' '+HGT+'" width="100%" height="'+HGT+'" preserveAspectRatio="none"'+g3tip(tip)+'>'+defs+g+'</svg>';
    // (v11.39) the legend row is gone — the panel has no vertical space to spend on a key.
    // Everything it said is in the chart hover, which is where a legend belongs.
    rollLines.forEach(function(r){
      var down=(r.side==='ceil');
      out+='<div style="display:flex;align-items:center;gap:6px;font-size:8.5px;color:#8b98a9;margin-top:3px;padding:2px 5px;border-radius:3px;background:'+
           (down?'rgba(240,97,109,.09)':'rgba(46,194,126,.09)')+'"'+
           g3tip('Is mass moving between strikes? One node is dissipating while another on the same side accumulates — that is a wall relocating, not just weakening. Measured in dollars over '+ROLL_WIN_MIN+' minutes and against the session median, because the typical strike grows 10-15% every half hour on its own. Direction is NOT yet a vote: on the days measured it had no proven edge and every archived day was a down day, so nothing could be settled.')+
           '><b style="color:'+r.col+';font-weight:800">'+(down?'CEIL ROLL ↓ ':'FLOOR ROLL ↑ ')+dispNum(r.from*rr)+' → '+dispNum(r.to*rr)+'</b>'+
           '<span>'+r.fromPct+'% / +'+r.toPct+'%</span>'+
           '<span style="margin-left:auto;color:#8b98a9">shadow · not voting</span></div>';
    });
    if(!drawn) out+='<div style="text-align:center;font-size:7.5px;color:#8b98a9;margin-top:1px">node history still filling — it rebuilds from empty after a reload</div>';
    out+='</div>';
    return out;
  }catch(e){ swallow('nodeChartHtml', e); return ''; }
}
