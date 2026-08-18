#!/usr/bin/env node
// ============================================================================
// (v10.53 E) SYNTHETIC DAY — a test for the REVIEW, not for the panel.
//
// The weekly learning run is an LLM. Nothing else in this repo checks whether it can
// actually do arithmetic honestly. This generator writes ONE day file with three
// properties PLANTED at known strengths; `docs/REVIEW-ACCEPTANCE.md` states what a
// competent review must report about it, and `test_review_selftest.js` asserts the
// planted properties really are in the file (so the acceptance doc is truthful).
//
//   (a) TRUE EDGE      `synth.edge`   — balanced UP/DOWN votes, ~75% hit, lift ~ +25.
//   (b) 1-WAY TRAP     `synth.trap`   — votes DOWN on ~95% of bars of a DOWN day.
//                                       Raw accuracy looks fine; re-weighted lift ~ 0.
//   (c) REGIME SPLIT   `synth.regime` — ~80% on trend-tagged bars, ~30% on chop-tagged
//                                       bars. Averages to ~55% and means nothing.
//
// Deterministic: no RNG, no clock. Same bytes every run.
// Shape = the panel's day export (`data/YYYY-MM-DD.json`): {version,date,snaps,events}
// with `snaps[SYM][].feat` per-bar feature records, plus the resolved outcome queue at
// `feat[SYM][]`, exactly as the recorder writes them.
//
// Usage:  node tools/synth_day.js [outPath]        (default data/_selftest.json)
// ============================================================================
'use strict';

var BARS       = 120;
var DIR_PTS    = 0.5;    // strikes price must travel for a direction call to count
var DATE       = '2026-08-15';
var PX0        = 770;
var TREND_BARS = 60;     // first half trend-tagged, second half chop-tagged

// ---- the tape: a DOWN day. 7 of every 10 bars resolve DOWN, 3 resolve UP. ----
// (this is what makes the 1-way trap look good for free)
function outcomeOf(i){ return ((i % 10) < 7) ? 'dn' : 'up'; }
function regimeOf(i){ return (i < TREND_BARS) ? 'trend' : 'chop'; }

function build(){
  var i, bars=[];
  for(i=0;i<BARS;i++){
    var o=outcomeOf(i);
    bars.push({ i:i, out:o, dirNum:(o==='up'?1:-1), regime:regimeOf(i),
                px:+(PX0 + (o==='up'?0.2:-0.2)*i*0.05).toFixed(2) });
  }
  var upBars=bars.filter(function(b){ return b.out==='up'; });   // 36
  var dnBars=bars.filter(function(b){ return b.out==='dn'; });   // 84

  // ---- (a) TRUE EDGE: 60 UP votes / 60 DOWN votes, 90 of 120 correct = 75% ----
  // UP votes on 33 up-bars (hit) + 27 dn-bars (miss); DOWN on 57 dn-bars (hit) + 3 up-bars.
  // With a balanced vote mix the re-weighted baseline is 50%, so the lift is real (+25).
  // Votes are spread across BOTH regime halves so the edge is regime-NEUTRAL (75/75):
  // the only planted regime dependence lives in `synth.regime`.
  var edge={};
  upBars.forEach(function(b,ix){ edge[b.i]=(ix%12===11)? -1 : 1; });   // 33 UP (hit) / 3 DOWN
  dnBars.forEach(function(b,ix){ edge[b.i]=((ix%3===0 && ix!==81))? 1 : -1; }); // 27 UP / 57 DOWN (hit)

  // ---- (b) 1-WAY TRAP: 114 DOWN votes / 6 UP votes on a 70%-down day ----
  // 82 of 120 correct = 68.3%, but the vote-mix-weighted baseline is also 68 → lift ~0.
  var trap={};
  upBars.forEach(function(b,ix){ trap[b.i]=(ix<2)? 1 : -1; });   // 2 UP (hit), 34 DOWN (miss)
  dnBars.forEach(function(b,ix){ trap[b.i]=(ix<4)? 1 : -1; });   // 4 UP (miss), 80 DOWN (hit)

  // ---- (c) REGIME SPLIT: 48/60 in trend (80%), 18/60 in chop (30%) ----
  var reg={};
  var tB=bars.filter(function(b){ return b.regime==='trend'; });
  var cB=bars.filter(function(b){ return b.regime==='chop'; });
  tB.forEach(function(b,ix){ reg[b.i]=(ix<48)? b.dirNum : -b.dirNum; });
  cB.forEach(function(b,ix){ reg[b.i]=(ix<18)? b.dirNum : -b.dirNum; });

  var snaps=[], queue=[];
  var t0=Date.parse(DATE+'T13:45:00Z');
  bars.forEach(function(b){
    var t=t0 + b.i*180000;
    var mfe=(b.out==='up')? 0.9 : 0.12;
    var mae=(b.out==='dn')? -0.9 : -0.12;
    var rgObj={ tag:b.regime, opex:false, event:false };
    var mk=function(vote, label){
      return { vote:vote, verdict:(vote>0?'UP':'DN'), label:label, regime:rgObj };
    };
    var feat={
      'dir':          { grade:(b.regime==='trend'?'B':'C'), verdict:(reg[b.i]>0?'UP':'DN'),
                        score:3, tier:'⚖', vote:reg[b.i], trendState:(b.regime==='trend'?'up':'flat'),
                        relation:(b.regime==='trend'?'confirmed':'tentative'),
                        session:'morning', opex:false, regime:rgObj },
      'synth.edge':   mk(edge[b.i], 'planted true edge'),
      'synth.trap':   mk(trap[b.i], 'planted 1-way trap'),
      'synth.regime': mk(reg[b.i],  'planted regime split')
    };
    snaps.push({ t:t, bar:b.i, px:b.px, king:775, rg:{ tag:b.regime, er:(b.regime==='trend'?0.62:0.18) },
                 inplay:null, nodes:[], feat:feat,
                 out5:null,
                 out10:{ mfe:mfe, mae:mae, net:+((b.out==='up'?0.6:-0.6)).toFixed(2),
                         pxEnd:+(b.px + (b.out==='up'?0.6:-0.6)).toFixed(2),
                         hitKing:false, revUp:false, revDn:false, n:10 } });
    Object.keys(feat).forEach(function(k){
      var rec=feat[k];
      var hit=null;
      if(typeof rec.vote==='number' && rec.vote!==0)
        hit=((rec.vote>0 && mfe>=DIR_PTS) || (rec.vote<0 && mae<=-DIR_PTS)) ? 1 : 0;
      queue.push({ key:k, t:t, bar:b.i, n:b.i, px:b.px, session:'morning',
                   rec:rec, hit:hit, mfe:mfe, mae:mae, resolved:true });
    });
  });

  return {
    schema:'gex-day/v1-selftest',
    version:'10.53',
    date:DATE,
    exportedAt:DATE+'T20:05:00Z',
    selftest:true,
    note:'SYNTHETIC. Not a trading day. Generated by tools/synth_day.js to test the WEEKLY '+
         'review’s rigor. Expected findings are stated in docs/REVIEW-ACCEPTANCE.md. '+
         'Never aggregate this file with real days.',
    planted:{
      edge:  { key:'synth.edge',   rate:75, votes:'balanced (60 up / 60 down)', expectedLift:'~ +25' },
      trap:  { key:'synth.trap',   rate:'~68', votes:'95% DOWN on a 70%-down day', expectedLift:'~ 0' },
      regime:{ key:'synth.regime', trend:80, chop:30, note:'averages to ~55% — meaningless without the split' }
    },
    syms:['SPY'],
    snaps:{ SPY:snaps, QQQ:[] },
    feat:{ SPY:queue, QQQ:[] },
    events:{ SPY:[], QQQ:[] }
  };
}

function main(){
  var fs=require('fs'), path=require('path');
  var out=process.argv[2] || path.join(__dirname, '..', 'data', '_selftest.json');
  var day=build();
  fs.mkdirSync(path.dirname(out), {recursive:true});
  fs.writeFileSync(out, JSON.stringify(day, null, 1)+'\n');
  console.log('wrote '+out+' — '+day.snaps.SPY.length+' bars, '+day.feat.SPY.length+' resolved records');
  return out;
}

module.exports = { build:build, main:main, BARS:BARS, DIR_PTS:DIR_PTS, DATE:DATE };
if(require.main === module) main();
