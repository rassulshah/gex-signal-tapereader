// (v11.47) EVERY FUNCTION THE RENDERERS CALL MUST EXIST.
//
// v11.46 shipped a TRADE LOCATION block that called tradeNodes() — a function that was never added,
// because the edit which would have added it aborted and the failure went unnoticed. The block's own
// try/catch swallowed the ReferenceError, so the NODES section simply did not appear and nothing was
// logged. Defensive try/catch is what makes a missing function invisible; this test is the counterweight.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };

// every top-level function name declared in the file
const declared=new Set();
{ const re=/^function\s+([A-Za-z_$][\w$]*)\s*\(/gm; let m; while((m=re.exec(src))!==null) declared.add(m[1]); }
// plus locally-declared helpers and the browser/host surface
const builtin=new Set(['Math','JSON','Object','Array','String','Number','Boolean','Date','RegExp','Error',
 'parseInt','parseFloat','isNaN','isFinite','encodeURIComponent','decodeURIComponent','setTimeout',
 'setInterval','clearInterval','clearTimeout','fetch','alert','console','document','window','localStorage',
 'requestAnimationFrame','atob','btoa','Promise','Set','Map','Infinity','NaN','undefined','if','for','while',
 'switch','catch','return','typeof','function','new','delete','void','in','of','do','else','try','throw']);

// Comments and string literals are prose: "to next Fri (rolled)" is not a call to Fri(). Strip both
// before scanning, or the check drowns in false positives and gets switched off — which is how a real
// missing function would then slip through.
function codeOnly(t){
  return t
    .replace(/\/\*[\s\S]*?\*\//g,' ')
    .replace(/(^|[^:])\/\/[^\n]*/g,'$1 ')
    .replace(/'(?:[^'\\\n]|\\.)*'/g,"''")
    .replace(/"(?:[^"\\\n]|\\.)*"/g,'""');
}
function bodyOf(name){
  const i=src.indexOf('function '+name+'(');
  if(i<0) return '';
  let j=src.indexOf('{', i), d=0, e=-1;
  for(let k=j;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){ d--; if(d===0){ e=k; break; } } }
  return src.slice(i, e+1);
}

// the render path: anything the five sections and the chart invoke
const renderers=['secFrame','secBias','secLoc','secReact','secExec','nodeChartHtml','panelV3','stepState',
                 'ifLadder','levelDepth','confTier','tradeNodes','pbNodeK','biasVotes','rollDetect'];
renderers.forEach(function(r){
  ok(declared.has(r), r+'() is declared');
});

let missing=[];
renderers.forEach(function(r){
  const body=codeOnly(bodyOf(r));
  if(!body) return;
  const local=new Set();
  { const re=/\b(?:var|function)\s+([A-Za-z_$][\w$]*)/g; let m; while((m=re.exec(body))!==null) local.add(m[1]); }
  { const re=/function\s*\(([^)]*)\)/g; let m; while((m=re.exec(body))!==null) m[1].split(',').forEach(a=>local.add(a.trim())); }
  { const m=body.match(/^function\s+\w+\s*\(([^)]*)\)/); if(m) m[1].split(',').forEach(a=>local.add(a.trim())); }
  // BARE calls only. A method call is preceded by a dot and belongs to whatever object it is on;
  // `rgba(` and friends live inside CSS strings. Neither can be a missing top-level function.
  const re=/(^|[^.\w$])([A-Za-z_$][\w$]*)\s*\(/g; let m;
  while((m=re.exec(body))!==null){
    const n=m[2];
    if(declared.has(n)||local.has(n)||builtin.has(n)) continue;
    if(/^(rgba|rgb|url|translate|scale|calc|var)$/.test(n)) continue;   // CSS, not JavaScript
    missing.push(r+' -> '+n+'()');
  }
});
// dedupe
missing=[...new Set(missing)];
ok(missing.length===0,'no renderer calls a function that does not exist — a swallowed ReferenceError renders as a MISSING SECTION, not an error',missing.slice(0,12));

// ---- THE SWALLOW MUST BE RECORDED ----
// A regex scan for undeclared IDENTIFIERS was tried and abandoned: it flagged keywords, regex-literal
// contents and inner-function parameters, and a check that drowns in false positives gets switched off,
// which is worse than not having it. JavaScript needs a parser, not a pattern. So instrument the catch
// instead — record what was eaten and let the smoke test fail on it. That catches this whole class,
// including the cases nobody predicted.
{
  ok(/function swallow\(tag, e\)/.test(src),'a recorder exists for swallowed render errors');
  ok(/__gptsDebug\.renderErrors/.test(src),'and it is exposed so the smoke test and the live panel can read it');
  ['secLoc.nodes','nodeChart','nodeChartHtml'].forEach(function(tag){
    ok(src.indexOf('swallow("'+tag+'"')>=0 || src.indexOf("swallow('"+tag+"'")>=0,
       'the '+tag+' catch reports rather than eats');
  });
  ok(/swallow\('section'\+\(j\+1\), eS\)/.test(src),'and every section wrapper does too');
}

// and the specific regression
ok(declared.has('tradeNodes'),'tradeNodes exists (the v11.46 miss)');
ok(/var rr=1; try\{ rr=dispIsFut\(\)\?dispR\(\):1; \}/.test(bodyOf('secLoc')),'secLoc declares its own rr (the v11.48 miss)');
ok(/tradeNodes\(sym\)/.test(bodyOf('secLoc')),'and TRADE LOCATION calls it');
console.log('\n'+pass+' pass / '+fail+' fail');
