// (v10.49 A) AUTH CAPTURE — the blocking v10.48 fix. The self-fetch of the non-displayed
// book 401'd because it sent cookies only. These tests cover all four capture paths
// (Headers object / plain object / Request input / XHR setRequestHeader) and prove the
// captured token is replayed on selfFetch and that a 401 is swallowed, not looped on.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m+(g!==undefined?' -> '+g:''));} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+g:''));} };

global.LASTAUTH=null;
global.LASTFEEDURL=null;
global.LASTFEED={SPY:null,QQQ:null};
global.LASTVEX={SPY:null,QQQ:null};
global.LASTDISP={SPY:null,QQQ:null};
global.SELF_MIN_MS=4000;
global.SELF_LAST={};
global.FEED_STALE_MS=12000;
global.observeFeedCadence=function(){};
global.onFeed=function(){};
global.feedTypeFromUrl=function(u){ return /data_type=vanna/.test(u)?'vanna':'gamma'; };
global.symFromUrl=function(u){ return /symbol=QQQ/.test(u)?'QQQ':'SPY'; };

eval(['authFromHeaders','captureAuth','selfFetch'].map(ex).join('\n'));

const TOKEN='Bearer eyJhbGciOi.TESTTOKEN.sig';

// ================= 1. authFromHeaders: every header shape =================
// (a) a real Headers object (has .get)
function HeadersLike(map){ this._m={}; for(var k in map) this._m[k.toLowerCase()]=map[k]; }
HeadersLike.prototype.get=function(k){ var v=this._m[String(k).toLowerCase()]; return v===undefined?null:v; };
ok(authFromHeaders(new HeadersLike({Authorization:TOKEN}))===TOKEN, '1a Headers object via .get()');
ok(authFromHeaders(new HeadersLike({'content-type':'application/json'}))===null, '1b Headers object without the header -> null');
// (b) a plain object, ANY case
ok(authFromHeaders({Authorization:TOKEN})===TOKEN,   '1c plain object, capitalised key');
ok(authFromHeaders({authorization:TOKEN})===TOKEN,   '1d plain object, lower-case key');
ok(authFromHeaders({AUTHORIZATION:TOKEN})===TOKEN,   '1e plain object, upper-case key');
ok(authFromHeaders({'AuThOrIzAtIoN':TOKEN})===TOKEN, '1f plain object, mixed case');
ok(authFromHeaders({'x-api-key':'nope'})===null,     '1g plain object without the header -> null');
// (c) the [[k,v],...] init form
ok(authFromHeaders([['Content-Type','application/json'],['authorization',TOKEN]])===TOKEN, '1h entry-array form');
// (d) junk
ok(authFromHeaders(null)===null && authFromHeaders(undefined)===null, '1i null/undefined -> null, no throw');
ok(authFromHeaders({})===null, '1j empty object -> null');

// ================= 2. captureAuth: init.headers AND Request input =================
global.LASTAUTH=null;
captureAuth('https://app.skylit.ai/tv/api/gex/levels?symbol=SPY', {headers:new HeadersLike({authorization:TOKEN})});
ok(global.LASTAUTH===TOKEN, '2a captured from fetch(url, {headers: Headers})', global.LASTAUTH);
global.LASTAUTH=null;
captureAuth('u', {headers:{Authorization:TOKEN}});
ok(global.LASTAUTH===TOKEN, '2b captured from fetch(url, {headers: plain object})');
global.LASTAUTH=null;
captureAuth({url:'u', headers:new HeadersLike({authorization:TOKEN})}, undefined);
ok(global.LASTAUTH===TOKEN, '2c captured from a Request input (input.headers.get)');
global.LASTAUTH=null;
captureAuth({url:'u', headers:new HeadersLike({authorization:TOKEN})}, {headers:{}});
ok(global.LASTAUTH===TOKEN, '2d init has no auth -> falls back to the Request input');
// a request WITHOUT the header must not wipe a good token
global.LASTAUTH=TOKEN;
captureAuth('u', {headers:{'content-type':'application/json'}});
ok(global.LASTAUTH===TOKEN, '2e a header-less request never clears a previously captured token');
captureAuth(null, null);
ok(global.LASTAUTH===TOKEN, '2f null args are inert');

// ================= 3. the XHR path (setRequestHeader wrap) =================
// installFeedObserver patches XMLHttpRequest.prototype; drive it with a fake XHR class.
function FakeXHR(){ this.status=200; this.responseText=''; this._h={}; this._l={}; }
FakeXHR.prototype.open=function(m,u){ this._m=m; this._u=u; };
FakeXHR.prototype.send=function(){ this._sent=true; };
FakeXHR.prototype.setRequestHeader=function(k,v){ this._h[k]=v; };
FakeXHR.prototype.addEventListener=function(ev,fn){ (this._l[ev]=this._l[ev]||[]).push(fn); };
global.window={ XMLHttpRequest:FakeXHR };
global.document={ visibilityState:'visible' };
global.ensureFeeds=function(){};
global.setInterval=function(){ return 0; };
eval(ex('installFeedObserver'));
installFeedObserver();
ok(FakeXHR.prototype.__gptsHooked===true, '3a XHR prototype hooked');
ok(FakeXHR.prototype.setRequestHeader!==undefined, '3b setRequestHeader still present after wrapping');

global.LASTAUTH=null; global.LASTFEEDURL=null;
var x=new FakeXHR();
x.open('GET','https://app.skylit.ai/tv/api/gex/levels?symbol=SPY&data_type=gamma');
x.setRequestHeader('Content-Type','application/json');
x.setRequestHeader('Authorization',TOKEN);
x.send();
ok(x._h['Authorization']===TOKEN, '3c the wrapper still forwards the header to the real XHR');
ok(global.LASTAUTH===TOKEN,       '3d LASTAUTH captured on send() for a gex/levels URL', global.LASTAUTH);
ok(global.LASTFEEDURL.indexOf('gex/levels')>=0, '3e LASTFEEDURL captured too');
// a NON-feed XHR carrying some other token must not overwrite LASTAUTH
var y=new FakeXHR();
y.open('GET','https://app.skylit.ai/tv/api/unrelated');
y.setRequestHeader('authorization','Bearer OTHER');
y.send();
ok(global.LASTAUTH===TOKEN, '3f a non-feed XHR never overwrites the feed token', global.LASTAUTH);
// lower-case header name is captured too
global.LASTAUTH=null;
var z=new FakeXHR();
z.open('GET','https://app.skylit.ai/tv/api/gex/levels?symbol=SPY&data_type=vanna');
z.setRequestHeader('authorization',TOKEN);
z.send();
ok(global.LASTAUTH===TOKEN, '3g lower-case authorization on XHR is captured');

// ================= 4. selfFetch REPLAYS the token =================
var CALLS=[];
global.fetch=function(url, init){ CALLS.push({url:url, init:init});
  return { then:function(f){ try{ f({ok:true, status:200, json:function(){ return {then:function(){ return {catch:function(){}}; }}; }}); }catch(e){} return {catch:function(){ return null; }}; } }; };
global.LASTFEEDURL='https://app.skylit.ai/tv/api/gex/levels?symbol=SPY&data_type=gamma&v=1';
global.LASTAUTH=TOKEN; global.SELF_LAST={};
selfFetch('SPY','vanna');
ok(CALLS.length===1, '4a selfFetch issued one request', CALLS.length);
ok(CALLS[0].init.credentials==='include', '4b credentials:include kept');
ok(CALLS[0].init.headers && CALLS[0].init.headers.Authorization===TOKEN, '4c Authorization replayed on the self-fetch', JSON.stringify(CALLS[0].init.headers));
ok(/data_type=vanna/.test(CALLS[0].url) && /symbol=SPY/.test(CALLS[0].url), '4d URL rewritten to the missing book', CALLS[0].url);
ok(!/[?&]v=1(&|$)/.test(CALLS[0].url), '4e cache-buster v= refreshed');
// no token yet -> empty headers object, NOT a broken/undefined header
global.LASTAUTH=null; global.SELF_LAST={}; CALLS=[];
selfFetch('QQQ','gamma');
ok(CALLS.length===1 && CALLS[0].init.headers && CALLS[0].init.headers.Authorization===undefined,
   '4f with no captured token the request still goes out with an empty headers object');
// throttled per (sym,type)
CALLS=[]; global.SELF_LAST={}; global.LASTAUTH=TOKEN;
selfFetch('SPY','vanna'); selfFetch('SPY','vanna');
ok(CALLS.length===1, '4g throttled: the second immediate call for the same (sym,type) is dropped', CALLS.length);
// no URL template captured yet -> no request at all
CALLS=[]; global.LASTFEEDURL=null; global.SELF_LAST={};
selfFetch('SPY','vanna');
ok(CALLS.length===0, '4h without a captured feed URL, selfFetch does nothing');

// ================= 5. a 401 is swallowed, never looped on =================
var parsed=0;
global.LASTFEEDURL='https://app.skylit.ai/tv/api/gex/levels?symbol=SPY&data_type=gamma';
global.LASTAUTH=TOKEN; global.SELF_LAST={};
global.fetch=function(){ return { then:function(f){ f({ok:false, status:401, json:function(){ parsed++; return {then:function(){return {catch:function(){}};}}; }}); return {catch:function(){ return null; }}; } }; };
var threw=false; try{ selfFetch('SPY','vanna'); }catch(e){ threw=true; }
ok(!threw, '5a a 401 response does not throw');
ok(parsed===0, '5b a 401 body is never parsed / fed into onFeed', parsed);
ok(global.LASTAUTH===TOKEN, '5c a 401 does NOT clear the token (the next real request refreshes it)');

// ================= 6. source-level guards =================
ok(/var LASTAUTH = null;/.test(src), '6a LASTAUTH declared alongside LASTFEEDURL');
ok(/captureAuth\(input, init\)/.test(src), '6b the fetch hook calls captureAuth');
ok(/XP\.setRequestHeader = function/.test(src), '6c the XHR prototype setRequestHeader is wrapped');
ok(/headers:hdrs/.test(src) && /LASTAUTH \? \{Authorization:LASTAUTH\} : \{\}/.test(src), '6d selfFetch builds the auth header');
ok(/vex ⏳/.test(src), '6e footer shows a dim "vex ⏳" instead of pretending VEX is live');

console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
