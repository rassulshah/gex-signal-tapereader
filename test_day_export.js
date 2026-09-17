// (v16.37) GATE A — the DAY section of the export, END TO END: gammaProfileBuild() with a pinned morning (bars, base,
// EM pin, the READ) -> the DAYACT / DAYEXP / EXPMODEL / CONDE / DAYSE / DAYSA rows AND the audit's AU.day block, then
// tools/day-derive.js re-derives every row from that audit and must reproduce the CSV — the same check the live
// regression (gp-regress.py --indicator daymodel|daystats) runs on a real export.
//   node test_day_export.js          (cp current/gex-signal-tapereader.user.js v10.js first; tools/regress.py does)
const fs=require('fs'), cp=require('child_process'), path=require('path'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);if(!m) throw new Error('no fn '+n);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
function v(n){ const m=src.match(new RegExp('\\nvar '+n+'\\s*=[^\\n]*?;')); if(!m) throw new Error('no var '+n); return m[0].replace(/\/\/.*$/,'')+'\n'; }
function vObj(n){ const i=src.indexOf('\nvar '+n+'='); if(i<0) throw new Error('no var '+n); let d=0,j=src.indexOf('{',i),st=false; for(;j<src.length;j++){ if(src[j]==='{'){d++;st=true;} else if(src[j]==='}'){d--; if(st&&d===0) break;} } return src.slice(i+1,j+1)+';\n'; }

global.window={__gptsDebug:{}};
eval(['GPTS_VERSION','GP_FILE','GP_SPXWR_KEY','GP_LAST','GP_AUDIT_FILE','GP_AUDIT','GP_FLIP_BUFFER_PTS','GP_NEAR_PTS',
 'REGIME_TREND_SKEW','REGIME_WHIP_EDGEMID','REGIME_RAINBOW_MIN','REGIME_SIG_PCT','GRID_STACK_STEPS','GRID_STACK_MIN_PCT','GRID_STACK_MAX_PCT','GRID_RUG_FLOOR_STEPS',
 'GP_IF_FILE','GP_IF_BUILT','GP_SCALEREF_ROW'].map(v).join(''));
eval(vObj('GP_EM_MODEL')+vObj('GP_COND_FALLBACK'));
var PANEL_MUTED=false, GP_DEPTH_T0=0.70, GP_DEPTH_TW=0.70, GP_SLOPE_STEEP=0.10;
eval(['gpF2','gpN1','gpI','gpDur','gpClk','gpShownDate','gpDow','gpDate','sum3','gridStep','gridSetups','gpRegime','gpEsOfSpx','gpEmRows','gpWallDepth','gpSlopeWord','gpLevelRows',
      'gpOpenWindow','hodlodCondE','gpDayModel','gammaProfileBuildIF','gammaProfileBuild'].map(ex).join('\n'));

// ---- the world: 2026-09-16, the export at 10:33 CT (123 min after the open), the SPXW tape of 09:47 for the gamma section
var LS={}; global.localStorage={ getItem:k=>(k in LS?LS[k]:null), setItem:(k,val)=>{LS[k]=String(val);} };
global.CFG={ nodeThresh:20, irt:{ on:true, secs:180 } }; global.SKY_FUT={ ES:'ES1' }; global.mul=(a,b)=>a*b;
const OPEN=8*3600+30*60; let NOW=OPEN+123*60;
global.ctNowSecOfDay=()=>NOW; global.ctTodayStr=()=>'2026-09-16'; global.hlDayShown=()=>'2026-09-16';
global.irtResolveAutoSym=()=>{}; global.FUTMODE={ fam:'ES', r:10.107, live:true };
const RATIO=1.0094, SPY_RATIO=10.10757;
global.skylitFutPx=(fut, book, px)=>{ const r=(book==='SPY')?SPY_RATIO:RATIO; return { px:px*r, ratio:r, src:'payload' }; };
global.LASTFUTDER={ ES1:{ j:{ derived:[ { source:'SPY', ratio:SPY_RATIO, levels:[{s:7690.5,t:1789572780}] }, { source:'SPXW', ratio:RATIO, levels:[{s:7690.5,t:1789572780}] } ] } } };
const TAPE={ '7695.00':1,'7690.00':-18,'7685.00':100,'7680.00':16,'7675.00':31,'7670.00':0,'7665.00':12,'7660.00':63,'7655.00':-10,'7650.00':-18,'7645.00':-4,'7640.00':4,'7635.00':3,'7630.00':-11,'7625.00':-2,'7620.00':8,'7615.00':-28,'7610.00':-49,'7605.00':-54,'7600.00':-40 };
global.tapeMapLive=(sym)=>(sym==='SPXW'?{ pct:TAPE, king:7685, kingNeg:false, ladderSrc:'trinity' }:null);
global.ladderFor=()=>({ price:undefined }); global.readTrinityHeaders=()=>({ SPXW:{ px:7619.45 }, SPY:{ px:760.27 }, QQQ:{ px:709.99 } });
let IFC={ err:null, stale:false, ageMin:2, spot:7619.9, payloadT:'2026-09-16T15:30:00Z', dte0:{ exps:[20260916], gf:{ flip:7620.72, netAtSpot:-35638037, sDn:-0.02, sUp:0.01 }, lv:{ cr:7675, ps:7600, netGEX:440821085 } } };
global.ifChain=(s)=>(s==='SPX'?IFC:null);
global.KTRK={ day:'2026-09-16', SPX:[{so:2,es:7504.19,strike:7500}], SPY:[], QQQ:[], NDX:[] }; global.KTRK_NOW={ SPX:{es:7689.14,strike:7685,pct:100}, SPY:null, QQQ:null, NDX:null };
global.KTRK_BOOKS=[ { book:'SPX', fam:'ES' }, { book:'SPY', fam:'ES' }, { book:'QQQ', fam:'NQ' }, { book:'NDX', fam:'NQ' } ]; global.ktrkSample=()=>{};
// the day: 3-minute bars 08:33..10:33 on the ES chart (scale 1). Open 7690, opening 30 min ranges 7683..7712 (open near the
// bottom), the first hour 7683..7724 (HOD 7724 at 09:18), a low of 7676 at 10:12, close 7702.
const bars=[]; { let px=7690; for(let so=OPEN+180; so<=NOW; so+=180){ const m=(so-OPEN)/60; let h,l,c;
  if(m<=30){ c=7690+m*0.7; h=c+3; l=Math.min(7690,c)-2; if(m===3){ l=7683; } }
  else if(m<=60){ c=7711+(m-30)*0.4; h=(m===48)?7724:Math.min(c+2,7722); l=c-2; }
  else if(m<=102){ c=7723-(m-60)*1.1; h=c+2; l=(m===102)?7676:c-2; }
  else { c=7677+(m-102)*1.2; h=c+2; l=c-1; }
  bars.push({ so, o:px, h, l, c }); px=c; } }
const HI=Math.max(...bars.map(b=>b.h)), LO=Math.min(...bars.map(b=>b.l)), CLOSE=bars[bars.length-1].c;
global.activeSym=()=>'ES'; global.dispMarket=()=>'ES'; global.ptUsd=()=>50; global.irtRatio=()=>({ r:RATIO });
global.measureBars=()=>({ bars, scale:1, src:'test' }); global.hlToolBars=(b)=>b;
global.sessionBody=()=>({ open:7690, close:CLOSE, hi:HI, lo:LO });
const D={ ok:true, first:'HOD', firstT:OPEN+48*60, hodT:OPEN+48*60, lodT:OPEN+102*60, took:48, bop:3, wick:9, wend:OPEN+9*60, wickPct:18, mud:54, second:'LOD', secondT:OPEN+102*60, gap:54,
          rngPts:HI-LO, rngUsd:Math.round((HI-LO)*50), posr:0.55, reclaimed:true, clock:NOW, far:0.3 };
global.hodLod=()=>D;
global.futSessionBars=(n)=>({ rth:[[0,7600,7660,7600,7650],[1,7650,7655,7602,7610]] });   // prior day range 58 (unused past 30 min)
const E={ n:300, rngPts:52, rngUsd:2600, rngP25:34.4, rngP75:75.7, tookMin:38, gapMin:93, firstClock:OPEN+38*60, secondClock:OPEN+131*60, lodFirstPct:48,
          wick:{ bop:12, wick:50, wickPct:23, mud:201 }, basis:{ dow:'Wed', n:60, pooled:false },
          predict:{ exante:{ a:41.95, b:0.351 }, open30:{ a:26.48, b:1.441 }, open60:{ a:25.47, b:1.099 } },
          condstats:{ n:300, t1Med:24, t2Med:286.5, lodPct:52, t2LastHrPct:39, t2Last30Pct:29,
            pos30:[{n:114,t1:21,t2:286.5,lodPct:59},{n:81,t1:33,t2:294,lodPct:58},{n:105,t1:21,t2:282,lodPct:39}],
            pos60:[{n:108,t1:16.5,t2:303,lodPct:66},{n:93,t1:36,t2:306,lodPct:53},{n:99,t1:21,t2:267,lodPct:35}] } };
global.hodlodBaseFor=()=>E;
global.gpDayEmPin=()=>({ emSpx:38.2, est:false, atMin:2, date:'2026-09-16' });   // pinned at the open: 38.2 SPX -> 38.56 ES
global.gpDayRecordTake=()=>null; global.sweepEventsShown=()=>[]; global.lodhodCall=(d)=>({ in:true, notIn:false, p:88, n:41, first:d.first });
global.scaleRef=7690.5; global.spotSpxP=7619.9; global.flipSpx=7620.72;

function rows(csv){ const R={}; csv.trim().split('\r\n').forEach(l=>{ const t=l.split(','); (R[t[0]]=R[t[0]]||[]).push(t.slice(1)); }); return R; }
const B=gammaProfileBuild(); ok(!!B && B.csv, '1.0 builds');
const R=rows(B.csv), AU=B.audit;
// GPTS_DUMP=<dir>: write this synthetic export as a fixture pair (the live runner's own test: tools/gp-regress.py --indicator ...)
if(process.env.GPTS_DUMP){ const d=process.env.GPTS_DUMP; fs.mkdirSync(d,{recursive:true}); AU.ct=NOW; AU.v=GPTS_VERSION; fs.writeFileSync(path.join(d,'synth-1033.csv'), B.csv); fs.writeFileSync(path.join(d,'synth-1033.audit.json'), JSON.stringify(AU,null,1)); }
// ---- 1. the rows
ok(R.DAYACT && R.DAYACT[0].join(',')===['7690.00',HI.toFixed(2),LO.toFixed(2),CLOSE.toFixed(2)].join(','), '1.1 DAYACT = the session body', R.DAYACT);
const EM=R.EXPMODEL && R.EXPMODEL[0];
ok(EM && EM[0]==='em-open60', '1.2 at 123 min with a pinned EM the basis is em-open60', EM);
const emEs=38.2*RATIO, or60=7724-7683, expRng=GP_EM_MODEL.open60.a+GP_EM_MODEL.open60.b*or60+GP_EM_MODEL.open60.c*emEs;
ok(EM && Math.abs(parseFloat(EM[1])-expRng)<0.06, '1.3 range = a + b*IB60 + c*EM = '+expRng.toFixed(1), EM);
const pos60=(7690-7683)/or60, expF=Math.max(0.05, Math.min(0.95, GP_EM_MODEL.place60.a+GP_EM_MODEL.place60.b*pos60));
ok(EM && Math.abs(parseFloat(EM[3])-expF)<0.006, '1.4 placement f from the open\'s position in the first hour ('+pos60.toFixed(2)+' -> '+expF.toFixed(2)+')', EM);
ok(EM && EM[4]===emEs.toFixed(1) && EM[5]==='pin', '1.5 EM in ES points and the pin state', EM);
ok(EM && EM[2]==='1', '1.6 the opening drive is up (close(30) > open)', EM);
const DE=R.DAYEXP && R.DAYEXP[0];
ok(DE && Math.abs(parseFloat(DE[1])-(7690+expF*expRng))<0.1 && Math.abs(parseFloat(DE[2])-(7690-(1-expF)*expRng))<0.1, '1.7 DAYEXP hi/lo placed by f', DE);
const CE=R.CONDE && R.CONDE[0];
ok(CE && CE[0]==='pos60-bottom+orclock' && CE[1]==='3.0', '1.8 CONDE: open at the bottom of the first hour -> the 1ST clock is the window low\'s clock (3 min)', CE);
ok(CE && CE[3]==='66' && CE[5]==='39' && CE[6]==='29', '1.9 CONDE: 66% LOD-first, the 2ND ladder 39 / 29', CE);
const SE=R.DAYSE && R.DAYSE[0];
ok(SE && SE[0]==='LOD' && SE[2]===String(OPEN+3*60) && SE[9]==='HOD' && SE[11]===String(OPEN+303*60), '1.10 DAYSE: LOD first at 08:33, HOD at 13:33 (t2 303)', SE);
ok(SE && SE[3]==='3.0' && SE[12]==='300.0', '1.11 DAYSE took 3.0 / gap 300.0', SE);
const SA=R.DAYSA && R.DAYSA[0];
ok(SA && SA[0]==='HOD' && SA[1]===HI.toFixed(2) && SA[2]===String(OPEN+48*60) && SA[3]==='48.0' && SA[9]==='LOD' && SA[13]===(HI-LO).toFixed(1), '1.12 DAYSA = the actual measurement', SA);
ok(SE && SA && SE[2]!==SA[2], '1.13 (16.36) the E row does NOT copy the actual 1ST clock after the READ');
// ---- 2. the audit block
const Y=AU.day;
ok(Y && !Y.err && Y.open===7690 && Y.elapsed===123 && Y.dow==='Wed', '2.1 AU.day: open, elapsed, weekday', Y && { open:Y.open, elapsed:Y.elapsed, dow:Y.dow, err:Y.err });
ok(Y && Y.w30 && Y.w60 && Y.w60.complete && Y.w60.h===7724 && Y.w60.l===7683 && Math.abs(Y.w60.pos-pos60)<1e-9, '2.2 AU.day carries both opening windows', Y && Y.w60);
ok(Y && Y.emEs!=null && Math.abs(Y.emEs-emEs)<1e-6 && Y.em && Y.em.est===false && Y.em.emSpx===38.2, '2.3 AU.day carries the EM pin', Y && Y.em);
ok(Y && Y.model && Y.model.basis==='em-open60' && Math.abs(Y.model.f-expF)<1e-9 && Y.model.drive===1, '2.4 AU.day carries the model output', Y && Y.model);
ok(Y && Y.cond && Y.cond.basis==='pos60-bottom+orclock' && Y.cond.readIn==='HOD', '2.5 AU.day carries the conditional E row and the READ note', Y && Y.cond);
ok(Y && Y.actual && Y.actual.first==='HOD' && Y.actual.took===48 && Y.call && Y.call.in===true, '2.6 AU.day carries the actual day and the call', Y && Y.actual);
ok(Y && Y.base && Y.base.hasCond===true && Y.base.n===60 && Y.base.rngPts===52 && Y.base.predict, '2.7 AU.day carries the base (n, range, predict, condstats)', Y && Y.base);
// ---- 3. THE RE-DERIVATION — tools/day-derive.js on this audit must reproduce every day row of the CSV
const tmp=path.join(require('os').tmpdir(), 'gpts-day-audit-'+process.pid+'.json'); fs.writeFileSync(tmp, JSON.stringify(AU));
let DER=null; try{ DER=JSON.parse(cp.execFileSync('node', [path.join(__dirname,'tools','day-derive.js'), tmp], { encoding:'utf8' })); }catch(e){ ok(false, '3.0 day-derive.js runs', String(e.message).slice(0,200)); }
try{ fs.unlinkSync(tmp); }catch(e){}
if(DER){
  ok(!DER.err, '3.0 day-derive.js runs', DER.err);
  for(const k of ['DAYACT','DAYEXP','EXPMODEL','CONDE','DAYSE','DAYSA']){
    const csvRow=R[k] && R[k][0], der=DER[k];
    ok(csvRow && der && csvRow.join(',')===der.join(','), '3.'+k+' re-derived from the audit == the CSV row', { csv:csvRow, derived:der });
  }
}
// ---- 4. before 30 min: pre-open stage, symmetric placement, exante EM range, pooled CONDE
NOW=OPEN+12*60; const bars12=bars.filter(b=>b.so<=NOW); global.measureBars=()=>({ bars:bars12, scale:1 }); global.hodLod=()=>({ ok:false });
const B4=gammaProfileBuild(); const R4=rows(B4.csv); const EM4=R4.EXPMODEL[0], CE4=R4.CONDE[0];
ok(EM4[0]==='em-exante' && EM4[3]==='0.50' && EM4[2]==='0' && Math.abs(parseFloat(EM4[1])-(GP_EM_MODEL.exante.a+GP_EM_MODEL.exante.b*emEs))<0.06, '4.1 at 12 min: em-exante, f 0.50, no drive', EM4);
ok(CE4[0]==='pre-open' && CE4[1]==='24.0' && CE4[3]==='52', '4.2 at 12 min: the pooled medians', CE4);
ok(B4.audit.day && B4.audit.day.actual===null && B4.audit.day.call===null && B4.audit.day.elapsed===12, '4.3 AU.day before the measurement: actual/call null, elapsed 12', B4.audit.day && { a:B4.audit.day.actual, c:B4.audit.day.call, e:B4.audit.day.elapsed });
ok(!R4.DAYSA, '4.4 no DAYSA row before hodLod is ready');
{ const tmp4=path.join(require('os').tmpdir(), 'gpts-day-audit4-'+process.pid+'.json'); fs.writeFileSync(tmp4, JSON.stringify(B4.audit));
  let D4=null; try{ D4=JSON.parse(cp.execFileSync('node', [path.join(__dirname,'tools','day-derive.js'), tmp4], { encoding:'utf8' })); }catch(e){}
  try{ fs.unlinkSync(tmp4); }catch(e){}
  ok(D4 && D4.EXPMODEL.join(',')===EM4.join(',') && D4.CONDE.join(',')===CE4.join(',') && D4.DAYEXP.join(',')===R4.DAYEXP[0].join(','), '4.5 re-derivation matches at 12 min too', D4 && { EXPMODEL:D4.EXPMODEL, CONDE:D4.CONDE }); }
// ---- 5. no EM (IF down): the study's own stages carry the range, placement still applies
NOW=OPEN+123*60; global.measureBars=()=>({ bars, scale:1 }); global.hodLod=()=>D; global.gpDayEmPin=()=>null;
const B5=gammaProfileBuild(); const R5=rows(B5.csv); const EM5=R5.EXPMODEL[0];
ok(EM5[0]==='open60' && EM5[4]==='' && EM5[5]==='' && Math.abs(parseFloat(EM5[1])-(25.47+1.099*or60))<0.06 && Math.abs(parseFloat(EM5[3])-expF)<0.006, '5.1 without an EM: open60 range from E.predict, placement unchanged', EM5);
ok(B5.audit.day.emEs===null && B5.audit.day.em===null, '5.2 AU.day: no EM recorded');
{ const tmp5=path.join(require('os').tmpdir(), 'gpts-day-audit5-'+process.pid+'.json'); fs.writeFileSync(tmp5, JSON.stringify(B5.audit));
  let D5=null; try{ D5=JSON.parse(cp.execFileSync('node', [path.join(__dirname,'tools','day-derive.js'), tmp5], { encoding:'utf8' })); }catch(e){}
  try{ fs.unlinkSync(tmp5); }catch(e){}
  ok(D5 && D5.EXPMODEL.join(',')===EM5.join(',') && D5.DAYEXP.join(',')===R5.DAYEXP[0].join(','), '5.3 re-derivation matches without an EM', D5 && D5.EXPMODEL); }
// ---- 6. mutation: a panel that forgot to place the candle (f fixed at 0.5) fails 1.4 / 1.7 / 3.DAYEXP
{ const mut=ex('gpDayModel').replace('out.f=clampF(M.place60.a+M.place60.b*w60.pos);','out.f=0.5;'); ok(mut!==ex('gpDayModel'), '6.0 mutation applies');
  const keep=global.gpDayModel; eval(mut); global.gpDayModel=gpDayModel; global.gpDayEmPin=()=>({ emSpx:38.2, est:false, atMin:2, date:'2026-09-16' });
  const Bm=gammaProfileBuild(); const EMm=rows(Bm.csv).EXPMODEL[0];
  ok(EMm[3]==='0.50' && Math.abs(parseFloat(EMm[3])-expF)>0.05, '6.1 mutation: f 0.50 would fail 1.4 (the assertions bite)', EMm);
  global.gpDayModel=keep; }
console.log((fail?'FAIL ':'PASS ')+pass+'/'+(pass+fail)+' assertions (day export end to end + audit re-derivation, v16.37)');
process.exit(fail?1:0);
