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
ok(/Push '\+parr\+/.test(src) && />Pull </.test(src), 'Pull/Push chips rendered');
ok(/Range <b>'\+fmtNum\(m\.range\.lo\)/.test(src), 'range chip');
ok(/out\.flr=flr; out\.ceil=ceil;/.test(src), 'nearest-strong Flr/Ceil in model');

// --- layout ---
ok(/id="gpts-1col"/.test(src) && !/id="gpts-2col"/.test(src), 'single column');
ok(!/'<div style="flex:1 1 300px;min-width:0">'\+kingBlock\(\)/.test(src), 'kingBlock no longer rendered');
ok(/function kingBlock\(\)/.test(src) && /function projScorecard\(\)/.test(src), 'King code retained (recording continues)');
ok(/readBlock44\('SPY'\)/.test(src), 'READ ▸ block rendered');
ok(/function studyRun\(/.test(src) && /STUDY_BASELINE/.test(src), 'study module present');
ok(/function repoUpsertSnaps\(/.test(src) && /indexedDB\.open\(REPO_DB_NAME/.test(src), 'IndexedDB repository');
ok(/function readTrinityHeaders\(/.test(src) && /xm:\(function/.test(src), 'cross-market headers recorded');
ok(/gpts_kd_open_v1/.test(src), '%KCH baseline persisted');
ok(/@version\s+10\.44/.test(src), 'version 10.44');
ok((src.match(/^function render\(\)/gm)||[]).length===1 && /\}\)\(\);\s*$/.test(src), 'file shape rule 2.4');
console.log('test_magnet_v1044: '+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
