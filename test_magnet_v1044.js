// v10.44 tests — magnet frame: episode engine, lenient FT, hitKing fix, abs() guard,
// regime tag, Node Map identity/state/activity vocabulary, single column, READ block.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
function ok(c,msg){ if(c){pass++;} else {fail++; console.log('  FAIL:',msg);} }
function grab(name){const i=src.indexOf('function '+name+'(');if(i<0) return '';let d=0,j=src.indexOf('{',i);for(let k=j;k<src.length;k++){if(src[k]=='{')d++;if(src[k]=='}'){d--;if(!d)return src.slice(i,k+1);}}}
const consts=src.match(/var EP_[A-Z_]+\s*=\s*[^;]+;/g).join('\n')+'\nvar FLRCEIL_MIN_PCT=15;';
eval(consts+grab('nodeEpisode')+grab('regimeTag')+grab('parseKingDollarsK')+grab('parseKingDollarSign')+grab('_fwdStats'));

// --- episode engine ---
let cs=[];for(let i=0;i<12;i++){const c=778-i*0.2;cs.push({t:i,o:c+0.05,h:c+0.1,l:c-0.1,c:c});}
let e=nodeEpisode(cs,774,cs[11].c,0);
ok(e.state==='Pull' && e.tw>=60 && e.zone==='PULL', 'approach from 2 strikes with no tag => Pull in PULL zone ('+JSON.stringify(e)+')');
cs=[];for(let i=0;i<12;i++){const c=(i<3)?775.1:775.1+(i-2)*0.2;cs.push({t:i,o:c,h:c+0.15,l:c-0.15,c:c});}
e=nodeEpisode(cs,775,cs[11].c,0);
ok(e.state==='Push' && e.dir===1 && e.tagged!=null, 'tagged then moving away => Push up ('+JSON.stringify(e)+')');
ok(nodeEpisode(cs,cs[11].c+0.1,cs[11].c,0).state==='BOw', 'at the node => BOw');
ok(nodeEpisode(cs,cs[11].c+4,cs[11].c,0).zone==='OUT' && nodeEpisode(cs,cs[11].c+4,cs[11].c,0).state==='', 'beyond 3 strikes => OUT, no state');
ok(nodeEpisode(cs,cs[11].c+0.8,cs[11].c,0).zone==='ORBIT', '<=1 strike => ORBIT');
ok(nodeEpisode([],775,776,0).state==='' , 'no candles => empty');

// --- regime tag ---
ok(regimeTag(cs).tag==='trend', 'monotone => trend');
let chop=[];for(let i=0;i<20;i++){const c=775+((i%2)?0.3:-0.3);chop.push({t:i,o:c,h:c+0.1,l:c-0.1,c:c});}
ok(regimeTag(chop).tag==='chop', 'zigzag => chop');

// --- abs() guard + sign ---
ok(parseKingDollarsK('−$27,399K')===27399, 'signed $K -> magnitude');
ok(parseKingDollarsK('-$1,252,620K')===1252620, 'ascii minus -> magnitude');
ok(parseKingDollarSign('−$27,399K')===false && parseKingDollarSign('$77,617K')===true, 'sign extracted separately');

// --- hitKing fix ---
ok(_fwdStats({px:776,tking:775.5,king:5.28e8},[{px:775.9},{px:775.4},{px:776.2}]).hitKing===true, 'hitKing uses tking and range-cross');
ok(_fwdStats({px:776,king:5.28e8},[{px:775.9},{px:775.8}]).hitKing===false, 'magnitude king rejected, no false hit');

// --- lenient FT present in machine ---
ok(/lenientFT\s*=\s*\(prev\.c>prev\.o && last\.c>last\.o && prev\.c>k && last\.c>k && last\.c>prev\.c\)/.test(src), 'lenient FT long rule (2 progressing green closes)');
ok(/lenientFT\s*=\s*\(prev\.c<prev\.o && last\.c<last\.o && prev\.c<k && last\.c<k && last\.c<prev\.c\)/.test(src), 'lenient FT short rule');
ok(/\(holdBeyond \|\| lenientFT\)/.test(src), 'FT confirms on either rule');

// --- Node Map vocabulary ---
ok(/'Acm' : \(L\.state==='Fading' \? 'Dec' : 'Steady'\)/.test(src), 'STATE Acm/Dec/Steady');
ok(!/lab==='Diss'/.test(src.slice(src.indexOf('function nodeStatusTag'))), 'Diss retired');
ok(/label='\\u26F0 Flr'/.test(src) && /label='\\u2594 Ceil'/.test(src), 'Flr/Ceil identities');
ok(/label='\\u2605 Mag'/.test(src), '★ Mag identity');
ok(/if\(L\.pos===false\)\{ col='#b58bff'/.test(src), 'purple -γ identity');
ok(/var typeHtml = '';/.test(src), '±γ text tag dropped');
ok(/var chain = isFT \? 'BO\\u00b7FT' : 'BOw';/.test(src), 'only BOw / BO·FT chips');
ok(/Push '\+parr\+'<\/span>/.test(src) && />Pull<\/span>/.test(src), 'Pull/Push chips rendered (v10.47: no toward-share % on the chip)');
ok(/Range <b>'\+fmt(Num|Lvl)\(m\.range\.lo\)/.test(src), 'range chip (v10.55: through fmtLvl so a futures chart reads in its own points)');
ok(/out\.flr=flr; out\.ceil=ceil;/.test(src), 'nearest-strong Flr/Ceil in model');

// --- layout ---
ok(/id="gpts-1col"/.test(src) && !/id="gpts-2col"/.test(src), 'single column');
ok(!/'<div style="flex:1 1 300px;min-width:0">'\+kingBlock\(\)/.test(src), 'kingBlock no longer rendered');
ok(/function kingBlock\(\)/.test(src) && /function projScorecard\(\)/.test(src), 'King code retained (recording continues)');
ok(/readBlock44\((__asym|'SPY')\)/.test(src), 'READ ▸ block rendered (v10.55: for the ACTIVE underlying)');
ok(/function studyRun\(/.test(src) && /STUDY_BASELINE/.test(src), 'study module present');
ok(/function repoUpsertSnaps\(/.test(src) && /indexedDB\.open\(REPO_DB_NAME/.test(src), 'IndexedDB repository');
ok(/function readTrinityHeaders\(/.test(src) && /xm:\(function/.test(src), 'cross-market headers recorded');
ok(/gpts_kd_open_v1/.test(src), '%KCH baseline persisted');
// (v12.0) VERSION PINS ARE NUMERIC NOW. These were regex alternations listing every allowed major
// ('10.4x|10.5x|11.x'), so every major bump broke three unrelated suites at once — 11 -> 12 did it
// again. Parse the version and compare; a floor is what the assertion actually means.
function verAtLeast(src, min){
  var m=/@version\s+([0-9]+)\.([0-9]+)/.exec(src); if(!m) return false;
  var a=parseInt(m[1],10), b=parseInt(m[2],10);
  var p=String(min).split('.'), A=parseInt(p[0],10), B=parseInt(p[1]||'0',10);
  return (a>A) || (a===A && b>=B);
}
ok(verAtLeast(src,'10.44'), 'version is at least 10.44', (/@version\s+\S+/.exec(src)||[])[0]);
ok((src.match(/^function render\(\)/gm)||[]).length===1 && /\}\)\(\);\s*$/.test(src), 'file shape rule 2.4');
console.log('test_magnet_v1044: '+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
