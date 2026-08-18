// v10.56 PART E — SYNC BANNER GRACE. Live 2026-08-18: "⚠ Out of sync" flashed on a single
// 30-second tag-vote dropout (majority restored next poll). One failed check is a poll landing
// mid-repaint, not an out-of-sync app. The banner now needs SYNC_GRACE (2) CONSECUTIVE failed
// checks; a single failure is silent on the face but still counted + logged in RECON_STATE.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function grab(name){ var i=src.indexOf('function '+name+'('); if(i<0) throw new Error('MISSING '+name);
  var d=0,st=false,j=i; for(;j<src.length;j++){var c=src[j]; if(c==='{'){d++;st=true;} else if(c==='}'){d--; if(st&&d===0){j++;break;}}} return src.slice(i,j); }
let p=0,f=0;const ok=(n,c,g)=>{if(c){p++;console.log('PASS '+n);}else{f++;console.log('FAIL '+n+(g!==undefined?' -> '+JSON.stringify(g):''));}};

global.PAL={shortAccent:'#f0616d',amber:'#f2b45a',sub:'#8b98a9'};
global.RECON_MIN_AGREE=2; global.RECON_FAIL_ESCAL=3; global.RECON_LOG_MAX=20; global.SYNC_GRACE=2;
global.RECON_STATE={ SPY:{streak:0,last:null,log:[]}, QQQ:{streak:0,last:null,log:[]} };
// vote sources are mocked per check
let VOTES={tag:773,feed:773,tapemax:773};
global.kingFromTapeTag=()=>VOTES.tag; global.kingFromFeed=()=>VOTES.feed; global.kingFromTapeMax=()=>VOTES.tapemax;
global.tapeMap=()=>null;
eval(['reconcileVotes','tapeSync','syncBannerShow','syncBannerHtml'].map(grab).join('\n'));

// pure gate
ok('0a healthy result -> no banner', syncBannerShow({ok:true,streak:0})===false);
ok('0b ONE failure -> no banner (grace)', syncBannerShow({ok:false,streak:1})===false);
ok('0c TWO consecutive failures -> banner', syncBannerShow({ok:false,streak:2})===true);
ok('0d null/undefined -> no banner (never crashes)', syncBannerShow(null)===false && syncBannerShow(undefined)===false);
ok('0e syncBannerHtml is empty on a single failure', syncBannerHtml({ok:false,streak:1,reason:'no-consensus',votes:{}})==='');
ok('0f syncBannerHtml renders "⚠ Out of sync" at 2', /Out of sync/.test(syncBannerHtml({ok:false,streak:2,reason:'no-consensus',votes:{}})));

// live sequence through tapeSync: ok, ok, FAIL (tag dropout), ok, FAIL, FAIL
let r=tapeSync('SPY'); ok('1a healthy poll: streak 0, no banner', r.ok===true && r.streak===0 && !syncBannerShow(r), r);
VOTES={tag:null,feed:773,tapemax:null};            // only one source -> not ok
r=tapeSync('SPY'); ok('1b first dropout: counted (streak 1), LOGGED, but SILENT on the face', r.ok===false && r.streak===1 && RECON_STATE.SPY.log.length===1 && !syncBannerShow(r), r);
VOTES={tag:773,feed:773,tapemax:773};
r=tapeSync('SPY'); ok('1c majority restored next poll: streak resets to 0', r.ok===true && r.streak===0, r);
VOTES={tag:null,feed:773,tapemax:null};
r=tapeSync('SPY'); ok('1d second (non-consecutive) dropout: still silent (streak 1)', r.streak===1 && !syncBannerShow(r), r);
r=tapeSync('SPY'); ok('1e a CONSECUTIVE second failure: streak 2 -> banner shows', r.streak===2 && syncBannerShow(r)===true, r);
r=tapeSync('SPY'); ok('1f third: RECURRING flag at RECON_FAIL_ESCAL, banner still on', r.streak===3 && r.recurring===true && syncBannerShow(r)===true, r);
ok('1g every failure is still in the diagnostics log (4 failures logged)', RECON_STATE.SPY.log.length===4, RECON_STATE.SPY.log.length);
// the render path uses the gate
ok('2a render() shows the banner only through syncBannerShow(__sync)', /if\(syncBannerShow\(__sync\)\)\{ html\+=syncBannerHtml\(__sync\); \}/.test(src));
ok('2b syncBannerHtml itself refuses to render below the grace', /function syncBannerHtml\(r\)\{\s*if\(!syncBannerShow\(r\)\) return '';/.test(src));
console.log('test_sync_grace: '+p+' passed, '+f+' failed');
