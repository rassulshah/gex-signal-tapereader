// (v16.29) GATE A REGRESSION — the CSV the panel writes for lsGammaProfile, row by row, from a pinned Atlas tape.
//   Operator, 2026-09-16: "a thorough regression build for the gamma profile … ensure that skylit tape and atlas chart
//   match IRT". This is the Skylit -> CSV half (Gate A of design/IRT-VS-SKYLIT-TESTING-PLAN.md). The CSV -> chart half
//   is plugin/test_gammaprofile_logic.cpp; the live same-moment run is tools/gp-regress.py.
//   Fixture A = the live SPXW ladder of 2026-09-16 09:47:09 CT (row-for-row verified against Atlas and the IRT rail),
//   with the ES1 payload spot 7613.50, the SPX->ES ratio 1.000558, the companion's 0DTE flip 7620.72 / walls 7675 / 7600.
//   THE TEST RUNS THE CODE (eval(ex('gammaProfileBuild')) with stubs), and the mutation block at the end proves the
//   assertions fire when the code is wrong.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);if(!m) throw new Error('no fn '+n);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
function v(n){ const m=src.match(new RegExp('\\nvar '+n+'\\s*=[^\\n]*?;')); if(!m) throw new Error('no var '+n); return m[0].replace(/\/\/.*$/,'')+'\n'; }

// ---- the code under test, verbatim from the script ----
global.window={__gptsDebug:{}};
eval(['GPTS_VERSION','GP_FILE','GP_SPXWR_KEY','GP_LAST','GP_AUDIT_FILE','GP_AUDIT','GP_FLIP_BUFFER_PTS','GP_NEAR_PTS',
 'REGIME_TREND_SKEW','REGIME_WHIP_EDGEMID','REGIME_RAINBOW_MIN','REGIME_SIG_PCT',
 'GRID_STACK_STEPS','GRID_STACK_MIN_PCT','GRID_STACK_MAX_PCT','GRID_RUG_FLOOR_STEPS'].map(v).join(''));
var PANEL_MUTED=false;
eval(['GP_IF_FILE','GP_IF_BUILT'].map(v).join(''));
eval(['gpF2','gpN1','gpI','gpDur','gpClk','gpShownDate','gpDow','gpDate','sum3','gridStep','gridSetups','gpRegime','gpEsOfSpx','gammaProfileBuildIF','gammaProfileBuild'].map(ex).join('\n'));
// the helpers the code calls must be reachable from a mutant built with new Function (global scope) — see section 8
Object.assign(global, { gpF2, gpN1, gpI, gpDur, gpClk, gpShownDate, gpDow, gpDate, sum3, gridStep, gridSetups, gpRegime, gpEsOfSpx, gammaProfileBuildIF, GP_IF_FILE, GP_IF_BUILT, GPTS_VERSION, GP_FILE, GP_SPXWR_KEY, GP_LAST, GP_AUDIT_FILE, GP_AUDIT, GP_FLIP_BUFFER_PTS, GP_NEAR_PTS, REGIME_TREND_SKEW, REGIME_WHIP_EDGEMID, REGIME_RAINBOW_MIN, REGIME_SIG_PCT, GRID_STACK_STEPS, GRID_STACK_MIN_PCT, GRID_STACK_MAX_PCT, GRID_RUG_FLOOR_STEPS, PANEL_MUTED });

// ---- the world, stubbed to the 09:47 CT snapshot ----
var LS={}; global.localStorage={ getItem:k=>(k in LS?LS[k]:null), setItem:(k,val)=>{LS[k]=String(val);} };
global.CFG={ nodeThresh:20, irt:{ on:true, secs:180 } };
global.SKY_FUT={ ES:'ES1' };
global.mul=(a,b)=>a*b;
global.ctNowSecOfDay=()=>35229;   // 09:47:09
global.ctTodayStr=()=>'2026-09-16';
global.hlDayShown=()=>'2026-09-16';
global.activeSym=()=>null;        // no day section in this test (the day model has its own tests)
global.irtResolveAutoSym=()=>{};
global.FUTMODE={ fam:'ES', r:10.107, live:true };
const RATIO=1.000558, SPY_RATIO=10.10757;
global.skylitFutPx=(fut, book, px)=>{ const r=(book==='SPY')?SPY_RATIO:RATIO; return { px:px*r, ratio:r, src:'payload' }; };
global.LASTFUTDER={ ES1:{ j:{ derived:[ { source:'SPY', ratio:SPY_RATIO, levels:[{s:7613.5}] }, { source:'SPXW', ratio:RATIO, levels:[{s:7613.5}] } ] } } };
// fixture A: the tape as tapeMapLive('SPXW') returns it
const TAPE_A={ '7695.00':1,'7690.00':-18,'7685.00':100,'7680.00':16,'7675.00':31,'7670.00':0,'7665.00':12,'7660.00':63,'7655.00':-10,'7650.00':-18,
  '7645.00':-4,'7640.00':4,'7635.00':3,'7630.00':-11,'7625.00':-2,'7620.00':8,'7615.00':-28,'7610.00':-49,'7605.00':-54,'7600.00':-40,
  '7595.00':-3,'7590.00':-14,'7585.00':-25,'7580.00':-10,'7575.00':3,'7570.00':-7,'7565.00':-6,'7560.00':-6,'7555.00':-8,'7550.00':6,
  '7545.00':-6,'7540.00':-6,'7535.00':-10,'7530.00':2,'7525.00':7,'7520.00':27,'7515.00':29,'7510.00':0,'7505.00':5,'7500.00':68 };
let TT={ pct:TAPE_A, king:7685, kingNeg:false, ladderSrc:'trinity' };
global.tapeMapLive=(sym)=>(sym==='SPXW'?TT:null);
global.ladderFor=(sym)=>({ price:undefined });          // the real ladder object carries NO price (the 11:5x lesson)
let TRI={ SPXW:{ px:7609.45 }, SPY:{ px:759.27 }, QQQ:{ px:709.99 } };
global.readTrinityHeaders=()=>TRI;
let IFC={ err:null, stale:false, ageMin:2, spot:7609.9, dte0:{ exps:[20260916], gf:{ flip:7620.72, netAtSpot:-35638037 }, lv:{ cr:7675, ps:7600, netGEX:440821085 } } };
global.ifChain=(s)=>(s==='SPX'?IFC:null);
global.KTRK={ day:'2026-09-16', SPX:[{so:2,es:7504.19,strike:7500},{so:4746,es:7604.25,strike:7600},{so:33608,es:7688.37,strike:7685}], SPY:[{so:2},{so:34478},{so:35006}], QQQ:[], NDX:[] };
global.KTRK_NOW={ SPX:{es:7689.14,strike:7685,pct:100}, SPY:{es:7645.26,strike:763,pct:-100}, QQQ:null, NDX:null };
global.KTRK_BOOKS=[ { book:'SPX', fam:'ES' }, { book:'SPY', fam:'ES' }, { book:'QQQ', fam:'NQ' }, { book:'NDX', fam:'NQ' } ];
global.ktrkSample=()=>{};

function rows(csv){ const R={}; csv.trim().split('\r\n').forEach(l=>{ const t=l.split(','); (R[t[0]]=R[t[0]]||[]).push(t.slice(1)); }); return R; }
const es=(spx)=>Math.round(spx*RATIO/0.25)*0.25;

// ======================= 1. the ladder -> STRIKE rows =======================
const B=gammaProfileBuild(); ok(!!B && B.csv, '1.0 builds');
const R=rows(B.csv);
ok((R.STRIKE||[]).length===40, '1.1 forty STRIKE rows, one per tape strike', (R.STRIKE||[]).length);
let esOk=true, spxOk=true, pctOk=true;
R.STRIKE.forEach(t=>{ const spx=parseFloat(t[5]); if(Math.abs(parseFloat(t[0])-es(spx))>0.001) esOk=false; if(!(String(spx) in {}) && !(spx.toFixed(2) in TAPE_A)) spxOk=false;
  const tp=TAPE_A[spx.toFixed(2)]; const exp=(spx===7685)?100:Math.round(tp); if(parseInt(t[1],10)!==exp) pctOk=false; });
ok(esOk, '1.2 every ES price = round(SPX x Skylit ratio, 0.25) — the SPX->ES mapping Atlas uses');
ok(spxOk, '1.3 the 7th field is the raw SPXW strike (the tape columns print it)');
ok(pctOk, '1.4 every %King is the tape\'s own signed value (King forced to 100)');
const kingRow=R.STRIKE.find(t=>t[3]==='1'); ok(kingRow && parseFloat(kingRow[5])===7685 && kingRow[2]==='1' && kingRow[1]==='100', '1.5 King = 7685, rank 1, flag 1, +100 (tape sign)');
const byRank=R.STRIKE.slice().sort((a,b)=>parseInt(a[2])-parseInt(b[2])).map(t=>[parseFloat(t[5]),parseInt(t[1])]);
ok(byRank[1][0]===7500 && byRank[2][0]===7660 && byRank[3][0]===7605 && byRank[4][0]===7610, '1.6 ranks 2..5 = 7500(+68) 7660(+63) 7605(-54) 7610(-49): magnitude, not sign', byRank.slice(0,6));
ok(R.KING && Math.abs(parseFloat(R.KING[0][0])-es(7685))<0.001, '1.7 KING row = ES of 7685 (7689.25)', R.KING);
ok(R.SCALEREF && parseFloat(R.SCALEREF[0][0])===7613.5, '1.8 SCALEREF = the ES1 payload spot (7613.50), the front price the ladder is scaled to', R.SCALEREF);

// GP_DUMP=1 writes fixture A's CSV + audit to testing/gamma-profile/fixtures/ (the sentinel the live runner and the
// eyes-on checklist use); the ASOF row is fixed at 09:47:09 so the runner's timing check is deterministic.
if(process.env.GP_DUMP){ const dir='testing/gamma-profile/fixtures'; fs.mkdirSync(dir,{recursive:true});
  fs.writeFileSync(dir+'/fixtureA-0947.csv', B.csv); fs.writeFileSync(dir+'/fixtureA-0947.audit.json', JSON.stringify(B.audit,null,1)); console.log('dumped fixture A'); }

// ======================= 2. the pattern tags (6th field) =======================
const tagOf={}; R.STRIKE.forEach(t=>{ if(t[4]) tagOf[parseFloat(t[5])]=t[4]; });
ok(tagOf[7605]==='BARNEY' && tagOf[7600]==='BARNEYM' && tagOf[7610]==='BARNEYM', '2.1 barney stack 7600-7610 named on 7605 (-54, the biggest), members marked', tagOf);
ok(!tagOf[7615], '2.2 7615 (-28) is NOT a member: under 30% of the King breaks the run (S6)');
ok(!tagOf[7520] && !tagOf[7515], '2.3 7520/7515 (+27/+29) are not a pika stack: members must be >= 30%');
ok(!Object.values(tagOf).some(x=>x==='RUG'||x==='RRUG'), '2.4 no rug on fixture A (no yellow directly over purple with no floor)');
ok(Object.keys(tagOf).length===3, '2.5 exactly three tags on the board', tagOf);

// ======================= 3. FLIP / CW / PW rows =======================
ok(R.FLIP && Math.abs(parseFloat(R.FLIP[0][0])-es(7620.72))<0.001 && R.FLIP[0][1]==='7620.72' && R.FLIP[0][2]==='0DTE' && R.FLIP[0][3]==='calc', '3.1 FLIP = companion dte0.gf.flip on the ES scale, tagged 0DTE calc', R.FLIP);
ok(R.CW && Math.abs(parseFloat(R.CW[0][0])-es(7675))<0.001 && R.CW[0][1]==='7675' && R.CW[0][2]==='0DTE', '3.2 CW = dte0.lv.cr 7675', R.CW);
ok(R.PW && Math.abs(parseFloat(R.PW[0][0])-es(7600))<0.001 && R.PW[0][1]==='7600' && R.PW[0][2]==='0DTE', '3.3 PW = dte0.lv.ps 7600', R.PW);

// ======================= 4. REGIME row =======================
ok(R.REGIME && R.REGIME[0][0]==='NEG', '4.1 sign NEG: spot 7609.45 is 11 pts below the 0DTE flip 7620.72', R.REGIME);
ok(R.REGIME && R.REGIME[0][1]==='WHIPSAW', '4.2 type WHIPSAW: both edges stand (7500 / 7685), the three largest nodes within 30 pts of spot are -gamma (7605 7610 7600), no 1.8x skew', R.REGIME);
ok(R.REGIME && R.REGIME[0][3]==='0', '4.3 no conflict: IF says negative, Skylit\'s near-price polarity is negative');
ok(R.REGIME && R.REGIME[0][4]==='7620.72', '4.4 the flip strike rides on the row');
ok(R.REGIME && /rolls 2/.test(R.REGIME[0].slice(5).join(',')), '4.5 velocity = 2 confirmed King rolls today');

// ======================= 5. the audit sidecar =======================
const AU=B.audit;
ok(AU && AU.king===7685 && AU.kingNeg===false && AU.tapeCount===40, '5.1 audit carries the tape as read (King, polarity, count)', AU&&{king:AU.king,n:AU.tapeCount});
ok(AU && Math.abs(AU.ratio.SPXW-RATIO)<1e-9 && Math.abs(AU.ratio.SPY-SPY_RATIO)<1e-9, '5.2 audit carries both ratios (SPX->ES, SPY->ES)');
ok(AU && AU.spot.src==='trinity' && AU.spot.spx===7609.45 && AU.scaleRef===7613.5, '5.3 audit names the spot source and SCALEREF');
ok(AU && AU.ifc && AU.ifc.dte0.gf.flip===7620.72 && AU.ifc.dte0.cr===7675, '5.4 audit carries the companion\'s 0DTE numbers');
ok(AU && AU.regime && AU.regime.type==='WHIPSAW' && AU.rolls.SPX===2, '5.5 audit carries the regime read and the roll count');

// ======================= 6. fixture B: rug + pika + RANGE + POS =======================
const TAPE_B={ '7705.00':50,'7700.00':45,'7690.00':-100,'7660.00':60,'7655.00':-45,'7640.00':3,'7620.00':55,'7580.00':-30,'7540.00':70 };
TT={ pct:TAPE_B, king:7690, kingNeg:true, ladderSrc:'trinity' };
TRI={ SPXW:{ px:7641.0 } };
IFC={ err:null, stale:false, ageMin:1, spot:7641, dte0:{ exps:[20260916], gf:{ flip:7600.0, netAtSpot:2e9 }, lv:{ cr:7705, ps:7540 } } };
const B2=gammaProfileBuild(); const R2=rows(B2.csv); const tag2={}; R2.STRIKE.forEach(t=>{ if(t[4]) tag2[parseFloat(t[5])]=t[4]; });
ok(tag2[7660]==='RUG', '6.1 RUG on 7660: yellow (+60) directly over purple 7655 (-45), no yellow floor within 3 strikes below, spot 7641 under the yellow', tag2);
ok(tag2[7705]==='PIKA' && tag2[7700]==='PIKAM', '6.2 pika stack 7700-7705 named on 7705 (+50)');
const kr2=R2.STRIKE.find(t=>t[3]==='1'); ok(kr2 && kr2[1]==='-100', '6.3 a negative King writes -100 (tape sign)');
ok(R2.REGIME[0][0]==='POS' && R2.REGIME[0][1]==='RANGE', '6.4 spot 7641 above the flip 7600 -> POS; both edges standing with +gamma in charge near price -> RANGE', R2.REGIME);
ok(R2.REGIME[0][3]==='0', '6.5 no conflict');
ok(R2.CW[0][1]==='7705' && R2.PW[0][1]==='7540', '6.6 walls follow the chain');

// ======================= 7. the rules that must NOT be crossed =======================
IFC={ err:null, stale:true, ageMin:45, spot:7641, dte0:{ gf:{ flip:7600 }, lv:{ cr:7705, ps:7540 } } };
const B3=gammaProfileBuild(); const R3=rows(B3.csv);
ok(!R3.FLIP && !R3.CW && !R3.PW, '7.1 a STALE chain writes NO flip / walls (absent, never a different window\'s answer)');
ok(R3.REGIME && R3.REGIME[0][0]==='NA', '7.2 ...and the regime sign is NA, the type still read', R3.REGIME);
IFC={ err:null, stale:false, ageMin:1, spot:7641, dte0:{ gf:{ flip:7600 }, lv:{ cr:7705, ps:7540 } } };
TRI={ SPXW:{ px:7602.0 } };
const B4=gammaProfileBuild(); const R4=rows(B4.csv);
ok(R4.REGIME[0][0]==='AT', '7.3 spot within 3 pts of the flip -> AT (the buffer that stops the label flapping)', R4.REGIME);
TRI=null; global.ladderFor=()=>null;
const B5=gammaProfileBuild(); const R5=rows(B5.csv);
ok(R5.REGIME && R5.REGIME[0][0]!=='NA' && B5.audit.spot.src==='ifchain', '7.4 no Trinity header, no ladder price -> the companion\'s chain spot is used (never "no spot" while a spot exists)', {row:R5.REGIME, src:B5.audit.spot.src});

// ======================= 9. THE IF BOOK (v16.30) — GammaProfile-IF.csv from the companion's 0DTE chain =======================
// fixture: the operator's console paste of 2026-09-16 ~10:20 CT (dte0.lv.gexProf, [strike, call $M, put $M<0]) — 69 strikes
const GEXPROF=[[7380,0,-50.7],[7400,1.3,-67.2],[7405,0.1,-55.1],[7440,0.2,-54.5],[7450,3.3,-86.3],[7460,1.5,-55.9],[7465,0.2,-45.7],[7470,1.5,-69.2],[7475,1.6,-82.8],[7480,1.5,-67.7],[7485,1.1,-39.2],[7490,2.7,-81.4],[7495,4.9,-56.5],[7500,20.8,-350.5],[7505,8.3,-55.7],[7510,12.8,-100.3],[7515,12.7,-87.5],[7520,145.8,-189.8],[7525,161.3,-464.6],[7530,19.1,-184],[7535,14,-151.1],[7540,18.4,-198],[7545,12.3,-126.2],[7550,39.9,-345.6],[7555,24.4,-90.2],[7560,46.8,-197.2],[7565,26,-143.3],[7570,67.5,-300],[7575,98.5,-454.3],[7580,180.8,-425.1],[7585,314.4,-401],[7590,294.4,-393],[7595,226.8,-428.4],[7600,560.8,-1003.2],[7605,196.6,-141.2],[7610,274.4,-262.3],[7615,267.5,-95.3],[7620,619.2,-181.1],[7625,407,-1345.2],[7630,273.8,-154.7],[7635,338.8,-245.9],[7640,572,-116.5],[7645,334.5,-224.5],[7650,1972.1,-1581.2],[7655,473.1,-267.5],[7660,500.8,-108.6],[7665,313.1,-100.3],[7670,497.6,-232.7],[7675,1324.4,-261.1],[7680,390.1,-204.8],[7685,485.2,-74.3],[7690,251.2,-49.1],[7695,204,-20],[7700,488.8,-120.7],[7705,138.2,-276.3],[7710,169,-11.3],[7715,85.8,-134.5],[7720,206.8,-19.7],[7725,129,-8.7],[7730,197.3,-131.6],[7735,61.3,-2.8],[7740,98,-66.2],[7745,35.3,-5.7],[7750,68.2,-12.2],[7755,94,-1.6],[7765,57.7,-0.8],[7775,49.1,-1.4],[7780,77.2,-0.5],[7790,45.6,-0.4]];
TT={ pct:TAPE_A, king:7685, kingNeg:false, ladderSrc:'trinity' }; TRI={ SPXW:{ px:7620.52 } }; global.ladderFor=()=>({});
IFC={ err:null, stale:false, ageMin:2, spot:7620.52, payloadT:'2026-09-16T15:20:00Z', dte0:{ exps:[20260916], gf:{ flip:7620.72, netAtSpot:-35638037 }, lv:{ cr:7675, ps:7600, gexProf:GEXPROF, gexProfCoverage:96.9 } } };
const B9=gammaProfileBuild(); const IFCSV=GP_IF_BUILT; ok(!!IFCSV, '9.0 the IF book builds alongside the Skylit file');
const R9=rows(IFCSV||''); const IFI=B9.audit.ifProf;
ok((R9.STRIKE||[]).length===69 && IFI.n===69, '9.1 one STRIKE row per gexProf strike (69)', (R9.STRIKE||[]).length);
const k9=R9.STRIKE.find(t=>t[3]==='1'); ok(k9 && parseFloat(k9[5])===7675 && k9[1]==='100' && k9[2]==='1', '9.2 IF King = 7675 (net +$1063M, the largest |net|), +100, rank 1', k9);
const pct9={}; R9.STRIKE.forEach(t=>{ pct9[parseFloat(t[5])]=parseInt(t[1],10); });
ok(pct9[7625]===-88 && pct9[7600]===-42 && pct9[7500]===-31 && pct9[7640]===43, '9.3 %King = 100 x net / max|net|, puts negative: 7625 -88, 7600 -42, 7500 -31, 7640 +43', {7625:pct9[7625],7600:pct9[7600],7500:pct9[7500],7640:pct9[7640]});
ok(R9.STRIKE.every(t=>Math.abs(parseFloat(t[0])-es(parseFloat(t[5])))<0.001), '9.4 every IF ES price uses the SAME SPX->ES mapping as the Skylit file');
ok(R9.KING && Math.abs(parseFloat(R9.KING[0][0])-es(7675))<0.001, '9.5 KING row = ES of 7675');
ok(R9.SCALEREF && parseFloat(R9.SCALEREF[0][0])===7613.5, '9.6 SCALEREF shared with the Skylit file (same anchor -> same rail)');
ok(R9.FLIP && R9.CW && R9.PW && R9.CW[0][1]==='7675' && R9.PW[0][1]==='7600', '9.7 FLIP / CW / PW rows identical in source to the Skylit file');
ok(R9.BOOK && R9.BOOK[0][0]==='IF0DTE', '9.8 BOOK row says IF0DTE (the plugin header prints "IF 0DTE")');
ok(R9.REGIME && R9.REGIME[0][0]==='AT', '9.9 REGIME sign: spot 7620.52 is within 3 pts of the flip 7620.72 -> AT (same flip, same rule)', R9.REGIME);
ok(!R9.KINGTRACK && !R9.DAYACT, '9.10 no KINGTRACK / day rows in the IF file (those plugins read GammaProfile.csv)');
const ranks9=R9.STRIKE.slice().sort((a,b)=>parseInt(a[2])-parseInt(b[2])).slice(0,4).map(t=>parseFloat(t[5]));
ok(ranks9[0]===7675 && ranks9[1]===7625 && ranks9[2]===7640 && ranks9[3]===7600, '9.11 ranks 1..4 = 7675, 7625, 7640, 7600 (by |%King|)', ranks9);
ok(IFI.ok && IFI.king===7675 && IFI.coverage===96.9 && IFI.spotSrc==='panel', '9.12 audit.ifProf carries king / coverage / spot source', IFI);
IFC={ err:null, stale:true, ageMin:45, spot:7620.52, dte0:{ gf:{ flip:7620.72 }, lv:{ cr:7675, ps:7600, gexProf:GEXPROF } } };
gammaProfileBuild(); ok(GP_IF_BUILT===null && B9.audit.ifProf.ok===true, '9.13 a STALE chain -> no IF file this export (absent, never an old book drawn as live)');
IFC={ err:null, stale:false, ageMin:1, spot:7620.52, dte0:{ gf:{ flip:7620.72 }, lv:{ cr:7675, ps:7600 } } };
gammaProfileBuild(); ok(GP_IF_BUILT===null, '9.14 a chain without gexProf (companion < 1.12) -> no IF file');
TT=null; IFC={ err:null, stale:false, ageMin:1, spot:7620.52, dte0:{ gf:{ flip:7620.72 }, lv:{ cr:7675, ps:7600, gexProf:GEXPROF } } };
const B9b=gammaProfileBuild(); ok(!!GP_IF_BUILT && /BOOK,IF0DTE/.test(GP_IF_BUILT), '9.15 the IF book still builds with NO Skylit tape at all (its own spot fallback)');
TT={ pct:TAPE_A, king:7685, kingNeg:false, ladderSrc:'trinity' }; IFC={ err:null, stale:false, ageMin:2, spot:7609.9, dte0:{ gf:{ flip:7620.72 }, lv:{ cr:7675, ps:7600 } } };

// ======================= 8. mutation check — the assertions must fire =======================
// a MUTANT of gammaProfileBuild, built from the mutated source in global scope; `run` receives it explicitly
function withSrc(mut, run){ const msrc=mut(ex('gammaProfileBuild')); if(msrc===ex('gammaProfileBuild')) throw new Error('mutation did not apply'); const fn=new Function('return ('+msrc+')')(); return run(fn); }
TT={ pct:TAPE_A, king:7685, kingNeg:false, ladderSrc:'trinity' }; TRI={ SPXW:{ px:7609.45 } }; global.ladderFor=()=>({}); IFC={ err:null, stale:false, ageMin:2, spot:7609.9, dte0:{ gf:{ flip:7620.72 }, lv:{ cr:7675, ps:7600 } } };
let m1=withSrc(s=>s.replace("var es=esOfSpx(k); if(es==null){ noRatio=true; return; }","var es=k;"), (fn)=>{ const RR=rows(fn().csv); return RR.STRIKE.every(t=>Math.abs(parseFloat(t[0])-es(parseFloat(t[5])))<0.001); });
ok(m1===false, '8.1 MUTATION: writing raw SPX instead of ES is caught by 1.2');
let m2=withSrc(s=>s.replace("var pv = isK ? (((p<0)||kingNeg)?-100:100) : Math.round(p);","var pv = isK ? 100 : Math.round(p);"), (fn)=>{ TT={ pct:TAPE_B, king:7690, kingNeg:true }; const RR=rows(fn().csv); TT={ pct:TAPE_A, king:7685, kingNeg:false }; return RR.STRIKE.find(t=>t[3]==='1')[1]; });
ok(m2!=='-100', '8.2 MUTATION: dropping the King\'s sign is caught by 6.3', m2);
let m3=withSrc(s=>s.replace("if(IFC && !IFC.err && !IFC.stale && IFC.dte0){","if(IFC && !IFC.err && IFC.dte0){"), (fn)=>{ IFC={ err:null, stale:true, ageMin:45, dte0:{ gf:{ flip:7600 }, lv:{ cr:7705, ps:7540 } } }; const RR=rows(fn().csv); IFC={ err:null, stale:false, ageMin:2, spot:7609.9, dte0:{ gf:{ flip:7620.72 }, lv:{ cr:7675, ps:7600 } } }; return !!RR.FLIP; });
ok(m3===true, '8.3 MUTATION: ignoring staleness writes a flip from a stale chain — 7.1 would catch it');

console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
