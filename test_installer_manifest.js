// ============================================================================================
// test_installer_manifest.js — EVERYTHING THE PANEL FETCHES FROM GITHUB RIDES THE INSTALLER.
//   The cloud cannot push (403), so a file the nightly or a study writes reaches the repo only inside
//   installvNNNN.bat. On 2026-09-03 the raw repo had panel v15.57 but studies.json / register.json /
//   requests.json / SWEEPS*.json / learning/log/*.json all returned 404: the manifest had never listed
//   them, and the Analysis tab ran on its seed while the READ quoted no rates. This test reads the
//   builder's own manifest (--list) and pins it against every pipeFetch path in the userscript.
// ============================================================================================
const fs=require('fs'), cp=require('child_process');
const src=fs.readFileSync('./current/gex-signal-tapereader.user.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,300):''));} };

const r=cp.spawnSync('python3',['tools/build-installer.py','--list'],{encoding:'utf8'});
ok(r.status===0,'m0 build-installer.py --list runs',(r.stderr||'').slice(-200));
const manifest=new Set((r.stdout||'').split('\n').map(s=>s.trim()).filter(Boolean));
ok(manifest.size>250,'m1 the manifest is the full payload ('+manifest.size+' files)');

// every STATIC path the panel fetches
const statics=[]; const re=/pipeFetch\(PIPE_RAW_BASE\+'(\/[^']+)'\)/g; let m;
while((m=re.exec(src))) statics.push(m[1].slice(1));
ok(statics.length>=5 && statics.indexOf('learning/studies.json')>=0 && statics.indexOf('learning/register.json')>=0 && statics.indexOf('data/es-1min/SWEEPS.json')>=0 && statics.indexOf('data/es-1min/SWEEPS-BOOK.json')>=0 && statics.indexOf('learning/rules.json')>=0,'m2 the panel fetches the registry, the register, the two sweep tables and the rules',statics);
const missing=statics.filter(p=>!manifest.has(p) && fs.existsSync(p));
ok(missing.length===0,'m3 every static pipeFetch path that exists in the tree is in the manifest',missing);
// the DYNAMIC directories: the nightly log (learning/log/<day>.json) must travel when it exists
const logs=fs.existsSync('learning/log')?fs.readdirSync('learning/log').filter(f=>/\.json$/.test(f)):[];
ok(logs.length===0 || logs.every(f=>manifest.has('learning/log/'+f)),'m4 every nightly log in the tree rides the installer',logs.filter(f=>!manifest.has('learning/log/'+f)));
// the TRACK path: requests.json must travel so the review's answers come back
ok(manifest.has('learning/requests.json'),'m5 learning/requests.json rides');
// and the pipeline's dynamic fetches are the ones we expect (a new dynamic fetch must be added to this list on purpose)
const dyn=(src.match(/pipeFetch\(PIPE_RAW_BASE\+'\/[^']+'\+/g)||[]).map(s=>s.replace(/^pipeFetch\(PIPE_RAW_BASE\+'/,'').replace(/'\+$/,''));
ok(dyn.every(d=>['/review/','/data/','/learning/log/'].indexOf(d)>=0),'m6 the dynamic fetch roots are review/, data/ and learning/log/ (data/<day>.json is HIS export and pushes from his machine)',dyn);

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
