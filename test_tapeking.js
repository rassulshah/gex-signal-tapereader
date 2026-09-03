// Regression test for v10.22 King / table-selection fix.
// Reproduces the real 2026-08-12 bug: a FLOW BUCKET popup (King $K on 775) sits
// EARLIER in the DOM than the true GEX ladder (King $K on 773, 96%). Old code
// picked the popup -> crowned 775. Fixed code must pick the ladder -> 773.
// Skylit renders both as flat "div grid" cells (Path B), which we mirror here.
const { JSDOM } = require('jsdom');
const fs = require('fs');

const src = fs.readFileSync(__dirname + '/v10.js', 'utf8');
function grab(name){
  const start = src.indexOf('function ' + name + '(');
  if(start < 0) throw new Error('missing fn ' + name);
  let i = src.indexOf('{', start), depth = 0;
  for(; i < src.length; i++){
    if(src[i]==='{') depth++;
    else if(src[i]==='}'){ depth--; if(depth===0) return src.slice(start, i+1); }
  }
  throw new Error('unbalanced ' + name);
}
function grabVar(decl){ const s=src.indexOf(decl); return src.slice(s, src.indexOf('\n', s)); }

fs.writeFileSync(__dirname + '/_harness.js', [
  grabVar('var KING_DOLLAR_RE ='), grabVar('var TAPE_REJECT_RE ='),
  grabVar('var ISO_DATE_RE ='), grabVar('var TAPE_TOK_RE ='),
  grab('tapeStrikeRowCount'), grab('findTapeTable'), grab('leadTok'),
  grab('tapeCells'), grab('leadSignedPct'),
  grab('kingResolve'), grab('readTapeFromDOM'),
  'module.exports = { findTapeTable, readTapeFromDOM };'
].join('\n'));

// flat-grid cell builder (real Skylit markup)
function grid(id, dates, rows){
  let c = '<span>Strike</span>' + dates.map(d=>'<span>'+d+'</span>').join('');
  rows.forEach(r=>{ c += '<span>'+r.s+'.0</span><span>'+r.p+'</span>' + (r.k?'<span>'+r.k+'</span>':''); });
  return '<div id="'+id+'">'+c+'</div>';
}
const pct = (n)=> (n<0? n : '+'+n) + '%';

// popup: King $K decoy on 775; only one date; short list + "FLOW BUCKET" marker
const flowPopup = '<div id="flowwrap"><span>FLOW BUCKET</span><span>Top contracts</span><span>Pick range end</span>' +
  grid('flow', ['2026-08-12'], [
    {s:776,p:'16%'},{s:775,p:'39%',k:'$215,273K'},{s:774,p:'20%'},
    {s:773,p:'10%'},{s:772,p:'8%'},{s:771,p:'5%'}
  ]) + '</div>';

// true GEX ladder: King $K on 773 (96%); multiple expiry dates; deep list
const ladderRows = [
  {s:780,p:pct(-15)},{s:779,p:pct(10)},{s:778,p:pct(2)},{s:777,p:pct(10)},
  {s:776,p:'16%'},{s:775,p:pct(41)},{s:774,p:pct(-1)},
  {s:773,p:'96%',k:'$207,395K'},
  {s:772,p:pct(35)},{s:771,p:'28%'},{s:770,p:pct(31)},{s:769,p:pct(-9)},
  {s:768,p:'20%'},{s:767,p:pct(-9)},{s:766,p:'7%'},{s:765,p:pct(-3)}
];
const gexLadder = grid('gex', ['2026-08-12','2026-08-13','2026-08-14'], ladderRows);

const dom = new JSDOM('<!DOCTYPE html><body>'+flowPopup+gexLadder+'</body>');
global.document = dom.window.document;
const H = require('./_harness.js');

let pass = true;
function check(name, got, want){
  const ok = String(got)===String(want); if(!ok) pass=false;
  console.log((ok?'PASS':'FAIL')+' '+name+' -> got '+JSON.stringify(got)+(ok?'':'  WANT '+JSON.stringify(want)));
}

const t = H.findTapeTable();
check('findTapeTable picks GEX ladder, not flow popup',
      t ? (t.id==='gex'?'gex':(t.id==='flow'?'flow':t.id)) : 'null', 'gex');

const d = H.readTapeFromDOM('SPY');
check('parse succeeded', !!d, 'true');
check('King strike = 773 (true GEX King)', d && d.king, 773);
check('King is NOT 775 (flow-popup decoy)', d && d.king!==775, 'true');
check('773 %King preserved as 96 (not clobbered to 100)', d && d.pct['773.00'], 96);
check('775 present as normal 41% node', d && d.pct['775.00'], 41);
check('no year token polluted the map (2026 absent)', d && !('2026.00' in d.pct), 'true');
check('kingSrc = dollar (tag agrees with data here)', d && d.kingSrc, 'dollar');

// ---- cross-check: mis-tagged $K on a weaker strike must be overridden ----
const badRows = ladderRows.map(r=>{
  if(r.s===773) return {s:773,p:'96%'};                 // strip $K off the true king
  if(r.s===775) return {s:775,p:pct(41),k:'$207,395K'}; // mis-tag $K onto weaker 775
  return r;
});
const dom2 = new JSDOM('<!DOCTYPE html><body>'+grid('gex',['2026-08-12'],badRows)+'</body>');
global.document = dom2.window.document;
delete require.cache[require.resolve('./_harness.js')];
const H2 = require('./_harness.js');
const d2 = H2.readTapeFromDOM('SPY');
check('cross-check overrides mis-tagged $K (773 wins)', d2 && d2.king, 773);
check('cross-check sets kingConflict flag', d2 && d2.kingConflict, true);
check('cross-check records the wrong tag (775)', d2 && d2.kingTagged, 775);
check('cross-check kingSrc = maxpct-override', d2 && d2.kingSrc, 'maxpct-override');

console.log(pass ? '\nALL PASS' : '\nSOME FAILED');
process.exit(pass?0:1);
