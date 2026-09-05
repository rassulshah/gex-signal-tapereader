// ============================================================================================
// test_origin_guard.js — (v15.74b) THE INSTALLER MUST NEVER CARRY A STALE COPY OF A FILE HIS MACHINE WRITES.
//   2026-09-04 23:41 CT: the v15.74 payload overwrote his machine's 9/4 nightly outputs (run 22:35, pushed 22:36)
//   with the cloud's earlier copies; the Analysis tab named the cloud; "double check .. look at analysis".
//   tools/origin-guard.py adopts origin's bytes where the cloud never touched a file (or his machine rewrote it with
//   the same numbers) and refuses on a real conflict; build-installer.py runs it read-only and refuses the build.
//   The logic is EXECUTED by the guard's own selftest (a temp repo with a base, "his machine" and "the cloud");
//   this file runs that selftest, then pins the wiring.
// ============================================================================================
const fs=require('fs'), cp=require('child_process');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,300):''));} };
const r=cp.spawnSync('python3',['tools/origin-guard.py','--selftest'],{encoding:'utf8'});
ok(r.status===0 && /origin-guard selftest ok/.test(r.stdout||''),'1a the guard\'s selftest passes: adopt what the cloud never touched, adopt his machine\'s same-numbers rewrite, refuse a real conflict, keep-mine, the installer-push case',(r.stderr||'').slice(-400));
const g=fs.readFileSync('tools/origin-guard.py','utf8');
ok(/NIGHTLY_WRITES\s*=\s*\(/.test(g) && ['learning/log/','learning/results.json','learning/studies.json','learning/recommendations.json','learning/deflections/examples.json','learning/items.json','learning/requests.json','data/es-1min/SWEEPS.json','data/es-1min/SWEEPS-BOOK.json'].every(p=>g.indexOf("'"+p+"'")>=0),'1b the files his machine writes are all named');
ok(/STAMPS\s*=\s*\('ranOn', 'ranAt', 'writtenBy'\)/.test(g),'1c the runner stamps are his to set (ignored by same_numbers)');
const b=fs.readFileSync('tools/build-installer.py','utf8');
ok(/origin-guard\.py/.test(b) && /_og\.check\(FILES, '\.', 'origin\/main', _keep, write=False\)/.test(b) && /raise SystemExit\(2\)/.test(b.slice(b.indexOf('ORIGIN GUARD'))),'2a build-installer.py runs the guard read-only on the manifest and refuses the build when anything is left to adopt');
ok(/--no-origin-guard/.test(b) && /--keep-mine=/.test(b),'2b …with the two explicit overrides, never a silent one');
ok(b.indexOf("if '--list' in sys.argv:")<b.indexOf('ORIGIN GUARD'),'2c --list (what the tests read) never runs the guard');
const c=fs.readFileSync('tools/BUILD-CHECKLIST.md','utf8');
ok(/origin-guard\.py` BEFORE THE COMMIT/.test(c) && /test_origin_guard\.js/.test(c),'3a BUILD-CHECKLIST §1a: the guard runs before the commit, pinned here');
const m=cp.spawnSync('python3',['tools/build-installer.py','--list'],{encoding:'utf8'});
ok((m.stdout||'').split('\n').indexOf('tools/origin-guard.py')>=0,'3b the guard rides the installer (tools/)');
console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
