// ============================================================================================
// test_futbars.js — (v14.59) THE ES CORPUS FEED: courier -> day export -> corpus -> base rates
//
// Operator, 2026-08-27: "make sure we had a process in place that obtained es data daily and
// updated the data file ... The process should be extendable to other markets."
//
// ⚠ THESE TESTS EXECUTE. Every assertion below was MUTATION-TESTED — the guard was broken on
// purpose and the assertion confirmed to fire. A test that greps the source instead of running it
// is documentation (failure pattern #8), and this section has already shipped one hover reading
// "undefined of 1169" past forty green assertions.
// ============================================================================================
const fs = require('fs');
const src = fs.readFileSync('./v10.js', 'utf8');
let pass = 0, fail = 0;
const ok = (c, m, g) => { if (c) { pass++; console.log('PASS ' + m); }
                          else { fail++; console.log('FAIL ' + m + (g !== undefined ? ' -> ' + JSON.stringify(g) : '')); } };
function ex(n){ const re=new RegExp('function\\s+'+n+'\\s*\\(','g'); const m=re.exec(src);
  if(!m) return ''; let i=src.indexOf('{',m.index),d=0,e=-1;
  for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){d--; if(d===0){e=k;break;}} }
  return src.slice(m.index,e+1); }
function val(n){ const m=new RegExp('(?:var\\s+)?\\b'+n+'\\s*=\\s*([\\s\\S]*?);\\n').exec(src); return m?eval('('+m[1]+')'):undefined; }

// ---- harness: a real localStorage, because these functions are ABOUT localStorage -------------
const STORE = {};
global.localStorage = {
  getItem: k => (k in STORE ? STORE[k] : null),
  setItem: (k, v) => { STORE[k] = String(v); },
  removeItem: k => { delete STORE[k]; }
};
global.HODLOD_BASE = val('HODLOD_BASE');
global.HLBASE_KEY = 'gpts_hodlod_base_v1';
global.HLBASE_MIN_SESSIONS = val('HLBASE_MIN_SESSIONS');
global.HLBASE_MIN_BUCKET   = val('HLBASE_MIN_BUCKET');
global.FUTBARS_KEY = 'gpts_futbars_v1';
eval(ex('hlBaseNormalise')); eval(ex('hodlodBase')); eval(ex('hlTier'));
eval(ex('futBarsLoad')); eval(ex('futBarsHealth'));

// a well-formed BASERATES.json, the shape tools/study-hodlod.py actually writes
function baserates(over){
  const b = { corpus:{ sessions:284, first:'2025-06-02', last:'2026-08-27', min_bars:386 },
    sequence:{ pct_LOD_first:51 },
    expected:{ took_min:21, gap_min:237.5, rng_pts:56.5, rng_usd:2825,
               rng_p25:41.8, rng_p75:80.2, first_clock:31860, second_clock:48300 },
    ladder:{ both:{ '30':{n:1169,held:481,rate:41}, '60':{n:811,held:453,rate:56},
                    '90':{n:642,held:427,rate:67}, '120':{n:541,held:407,rate:75},
                    '180':{n:433,held:362,rate:84} } } };
  return Object.assign(b, over||{});
}
const put = j => { STORE[global.HLBASE_KEY] = JSON.stringify({ at: 1787877743000, base: j }); };
const clear = () => { delete STORE[global.HLBASE_KEY]; };

// ---- 1 · THE BAKED-IN FALLBACK ---------------------------------------------------------------
{
  clear();
  const B = hodlodBase();
  ok(B.src === 'baked', 'f1 with no courier payload the panel falls back to the compiled literal', B.src);
  ok(B.n === 284, 'f2 the fallback is the 284-session corpus', B.n);
  // ⚠ THE DEFECT THIS BUILD FIXED. v14.57/v14.58 shipped a ladder whose hover printed
  // "undefined of 1169" because the literal carried no `held`. Forty assertions passed over it.
  ok(B.ladder.every(L => typeof L.held === 'number' && L.held > 0 && L.held <= L.n),
     'f3 every baked rung carries held, and held <= n (the hover printed "undefined" for two releases)',
     B.ladder.map(L => L.held));
  ok(B.ladder.every(L => Math.round(100 * L.held / L.n) === L.rate),
     'f4 held/n reproduces the printed rate on every rung — the number and its evidence agree',
     B.ladder.map(L => [L.held, L.n, L.rate, Math.round(100 * L.held / L.n)]));
}

// ---- 2 · THE COURIER PAYLOAD IS PREFERRED, AND NORMALISED ------------------------------------
{
  put(baserates());
  const B = hodlodBase();
  ok(B.src === 'courier', 'f5 a valid couriered payload wins over the literal', B.src);
  ok(B.last === '2026-08-27', 'f6 and it carries its own corpus end date to the face', B.last);
  ok(B.ladder.length === 5 && B.ladder[0].w === 30 && B.ladder[4].w === 180,
     'f7 the study shape {ladder:{both:{w:{n,held,rate}}}} normalises to the display array, sorted by window',
     B.ladder.map(L => L.w));
  ok(B.ladder[4].rate === 84 && B.ladder[4].held === 362, 'f8 rates and held survive the normalisation');
  ok(B.rngPts === 56.5 && B.firstClock === 31860, 'f9 the E-row figures come across too');
  // the tier picker must read the LIVE base, or the table and the highlighted rung disagree
  ok(hlTier(100, B).w === 90, 'f10 a 100-minute extreme earns the 90m rung, never the 120m it has not reached',
     hlTier(100, B));
}

// ---- 3 · REFUSALS. Each of these must LOSE to the literal ------------------------------------
{
  const cases = [
    ['no corpus block',        baserates({ corpus: undefined })],
    ['zero sessions',          baserates({ corpus:{ sessions:0, first:'a', last:'b' } })],
    ['no ladder',              baserates({ ladder: {} })],
    ['a rung with no n',       baserates({ ladder:{ both:{ '30':{n:0,rate:41}, '60':{n:811,rate:56} } } })],
    ['a rung with no rate',    baserates({ ladder:{ both:{ '30':{n:1169}, '60':{n:811,rate:56} } } })]
  ];
  cases.forEach(([why, j]) => {
    put(j);
    ok(hodlodBase().src === 'baked', 'f11 REFUSED (' + why + ') — falls back rather than drawing it');
  });
}

// ---- 4 · THE NON-MONOTONE REFUSAL ------------------------------------------------------------
// The section's ONLY predictive claim is "the longer it stands, the likelier it is the day's".
// A corpus that stops supporting it must not be rendered.
{
  put(baserates({ ladder:{ both:{ '30':{n:1169,held:481,rate:41}, '60':{n:811,held:453,rate:56},
                                  '90':{n:642,held:300,rate:47},  // <- falls
                                  '120':{n:541,held:407,rate:75}, '180':{n:433,held:362,rate:84} } } }));
  ok(hodlodBase().src === 'baked',
     'f12 a NON-MONOTONE ladder is refused — the one claim the section makes would be unsupported');
}

// ---- 5 · THE DOWNGRADE FLOOR -----------------------------------------------------------------
// ⚠ THIS CAUGHT A REAL HOLE DURING THE BUILD. Two synthetic sessions produced 57/80/100/100/100 —
// monotone, well-formed, and complete nonsense. Monotonicity is not evidence.
{
  put(baserates({ corpus:{ sessions:2, first:'2026-08-26', last:'2026-08-27' },
    ladder:{ both:{ '30':{n:7,held:4,rate:57}, '60':{n:5,held:4,rate:80}, '90':{n:3,held:3,rate:100},
                    '120':{n:3,held:3,rate:100}, '180':{n:2,held:2,rate:100} } } }));
  ok(hodlodBase().src === 'baked',
     'f13 a 2-session corpus is refused even though its ladder is monotone and well-formed');
  // ⚠⚠ THE CASE ABOVE DOES NOT TEST THE SESSION FLOOR. Caught by mutation: deleting the
  // sessions>=120 guard left the suite GREEN, because that payload's rungs are also threadbare and
  // the per-rung floor was doing all the work. An assertion that cannot fail is worse than none —
  // third occurrence in this project. This payload has HEALTHY rungs and only a short corpus, so
  // the session floor is the only thing that can refuse it.
  put(baserates({ corpus:{ sessions:40, first:'2026-07-01', last:'2026-08-27' },
    ladder:{ both:{ '30':{n:280,held:115,rate:41}, '60':{n:200,held:112,rate:56},
                    '90':{n:160,held:107,rate:67}, '120':{n:140,held:105,rate:75},
                    '180':{n:110,held:92,rate:84} } } }));
  ok(hodlodBase().src === 'baked',
     'f13b a 40-session corpus with HEALTHY rungs is still refused — the session floor alone must catch it',
     hodlodBase().src);
  ok(global.HLBASE_MIN_SESSIONS >= 100 && global.HLBASE_MIN_BUCKET >= 20,
     'f14 the floors are real numbers, not zero', [global.HLBASE_MIN_SESSIONS, global.HLBASE_MIN_BUCKET]);
  // and one that clears the session floor but has a threadbare rung
  put(baserates({ ladder:{ both:{ '30':{n:1169,held:481,rate:41}, '60':{n:811,held:453,rate:56},
                                  '90':{n:642,held:427,rate:67}, '120':{n:541,held:407,rate:75},
                                  '180':{n:9,held:8,rate:84} } } }));
  ok(hodlodBase().src === 'baked', 'f15 a single threadbare rung (n=9) sinks the whole payload');
}

// ---- 6 · CORRUPTION MUST NOT REACH THE FACE --------------------------------------------------
{
  STORE[global.HLBASE_KEY] = '{not json';
  ok(hodlodBase().src === 'baked', 'f16 unparseable localStorage falls back instead of throwing');
  STORE[global.HLBASE_KEY] = JSON.stringify({ at: 1, base: null });
  ok(hodlodBase().src === 'baked', 'f17 a null base falls back');
  clear();
}

// ---- 7 · THE CORPUS FEED'S OWN HEALTH --------------------------------------------------------
{
  delete STORE[global.FUTBARS_KEY];
  let H = futBarsHealth();
  ok(H.ok === false && /companion/.test(H.why),
     'f18 no courier data REFUSES and names the likely cause — absence is not a reading', H);
  STORE[global.FUTBARS_KEY] = JSON.stringify({ _v:1, _at: Date.now() - 3*60000,
    ES:{ at:Date.now(), sym:'ES=F', rows:[[1,2,3,4,5,6],[7,8,9,10,11,12]] },
    NQ:{ at:Date.now(), sym:'NQ=F', rows:[[1,2,3,4,5,6]] },
    GC:{ at:Date.now(), err:'HTTP 404' } });
  H = futBarsHealth();
  ok(H.ok === true && H.markets.length === 3, 'f19 every market is reported, working or not', H.markets.length);
  ok(H.markets.find(m => m.k === 'GC').err === 'HTTP 404',
     'f20 a failed market carries its error rather than vanishing');
  ok(H.markets.find(m => m.k === 'ES').n === 2, 'f21 bar counts are reported per market');
  ok(H.ageMin !== null && H.ageMin >= 2 && H.ageMin <= 5,
     'f22 staleness is reported in minutes — a silent old corpus feed is the failure mode', H.ageMin);
  ok(futBarsLoad() !== null, 'f23 the raw payload is readable for the export');
}

// ---- 8 · EXTENSIBILITY IS ONE TABLE ROW, AND ND IS NOT INVENTED ------------------------------
{
  const comp = fs.readFileSync('./current/gex-if-levels.user.js', 'utf8');
  const m = /var FUT_MARKETS=\[([\s\S]*?)\];/.exec(comp);
  ok(!!m, 'f24 the companion carries a markets table');
  const keys = [...m[1].matchAll(/k:'([A-Z]+)'/g)].map(x => x[1]);
  ok(['ES','NQ','GC','CL'].every(k => keys.includes(k)),
     'f25 ES, NQ, GC and CL are all present — extending is one row', keys);
  // ⚠ THE OPERATOR NAMED "nd" AND I DO NOT KNOW WHICH CONTRACT THAT IS. A guessed symbol would put
  // the wrong series in the corpus under a right-looking name. It stays out until he says.
  ok(!keys.includes('ND'), 'f26 ND is NOT invented — an unknown contract must not be guessed', keys);
  ok(/@connect\s+query1\.finance\.yahoo\.com/.test(comp),
     'f27 the companion declares the Yahoo host — without it GM_xmlhttpRequest is refused');
  ok(/@connect\s+raw\.githubusercontent\.com/.test(comp),
     'f28 and the base-rate host');
  // @grant none in the PANEL is load-bearing; the tap must never migrate there
  ok(/\/\/ @grant\s+none/.test(src), 'f29 the panel still runs @grant none — the feed hooks depend on it');
  ok(!/query1\.finance\.yahoo\.com/.test(src),
     'f30 and the panel does NOT fetch Yahoo itself — measured BLOCKED from page context 2026-08-27');
}

// ---- 9 · THE DOCUMENT IS WIRED INTO `load gex`, AND STAYS WIRED ------------------------------
// ⚠ The operator's instruction, 2026-08-28: "place its details in the architecture document for this
// project so it is read every time load gex is done." A doc nobody reads is the disease this whole
// file exists to treat — every fact about who-fetches-what used to live in session-state/, which is
// rewritten in full every build. Prose cannot enforce that. This can.
{
  const cfg = JSON.parse(fs.readFileSync('./.gex-config.json', 'utf8'));
  const doc = 'design/DATA-ARCHITECTURE.md';
  ok(fs.existsSync('./' + doc), 'f31 the data architecture document exists');
  ok(cfg.canonicalFiles.projectFiles.includes(doc),
     'f32 it is in the config canonical list, so a load fetches it');
  ok(/DATA-ARCHITECTURE/.test(cfg.loadInstruction),
     'f33 and the load instruction names it explicitly');
  ok(!(cfg.knownMissingAtHead.files || []).includes(doc),
     'f34 it is no longer listed as missing at HEAD');
  ok(!!cfg.dataPipeline && /append-futures/.test(cfg.dataPipeline.summary),
     'f35 the config records the pipeline itself, not just a pointer to a file');
  ok(/ND/.test(cfg.dataPipeline.ND || ''), 'f36 and records that ND is deliberately unbuilt');
  const skill = fs.readFileSync('./skills/gex/SKILL.md', 'utf8');
  ok(/DATA-ARCHITECTURE\.md/.test(skill),
     'f37 the skill\'s load procedure names it too — the config alone is one point of failure');
  const d = fs.readFileSync('./' + doc, 'utf8');
  ok(/@grant none/.test(d) && /CANNOT PUSH|cannot push/.test(d),
     'f38 the doc states the two constraints that decide every routing question');
  ok(/query1\.finance\.yahoo\.com/.test(d) && /insiderfinance/i.test(d),
     'f39 it covers BOTH Yahoo and InsiderFinance — the operator asked for exactly this');
  ok(/append-futures\.py/.test(d) && /study-hodlod\.py/.test(d) && /gpts_futbars_v1/.test(d),
     'f40 and names the actual scripts and keys, so the next context can find them by grep');
  const old = fs.readFileSync('./design/architecture-design.md', 'utf8');
  ok(/SUPERSEDED ON DATA SOURCES/.test(old),
     'f41 the older architecture doc carries the supersede banner — two docs, one answer');
}

// ---- (v1.17) THE CADENCE IS RTH-AWARE, BECAUSE THESE BARS ARE NOW A LIVE INPUT ---------------
// Operator, 2026-09-01: "can you backfill the data in this hod lod section to ensure it is in sync
// with the day". These bars became a live input at panel v15.08, when hodLod started MEASURING the
// session off them on a futures chart — but the courier was still on the hourly cadence chosen when
// they only fed a nightly corpus. An hourly poll can leave HOD, LOD, range and PT an hour stale.
// ⚠ EXECUTED against the companion's own function, at fixed instants, rather than grepped.
{
  const cs = fs.readFileSync('./current/gex-if-levels.user.js', 'utf8');
  function exC(n){ const m=new RegExp('function\\s+'+n+'\\s*\\(','g').exec(cs);
    let i=cs.indexOf('{',m.index),d=0; for(let k=i;k<cs.length;k++){ if(cs[k]==='{')d++;
      else if(cs[k]==='}'){ d--; if(!d) return cs.slice(m.index,k+1); } } }
  const numC=n=>eval(new RegExp('var\\s+'+n+'\\s*=\\s*([0-9*]+)').exec(cs)[1]);
  global.FUT_POLL_RTH_MS=numC('FUT_POLL_RTH_MS');
  global.FUT_POLL_OFF_MS=numC('FUT_POLL_OFF_MS');
  eval(exC('futPollMs'));
  ok(FUT_POLL_RTH_MS < FUT_POLL_OFF_MS, 'f42 the RTH cadence is faster than the off-hours one',
     [FUT_POLL_RTH_MS, FUT_POLL_OFF_MS]);
  ok(FUT_POLL_RTH_MS <= 5*60*1000, 'f43 ...at most 5 minutes, so the HOD/LOD tail cannot go stale',
     FUT_POLL_RTH_MS);
  const RealDate = Date;
  function at(iso){                       // freeze the clock at a known CT instant
    global.Date = class extends RealDate {
      constructor(...a){ super(...(a.length?a:[iso])); }
      static now(){ return new RealDate(iso).getTime(); }
    };
    const v = futPollMs();
    global.Date = RealDate;
    return v;
  }
  // 2026-09-01 is a Tuesday. 14:00Z = 09:00 CT (RTH); 02:00Z = 21:00 CT Monday (closed).
  ok(at('2026-09-01T14:00:00Z')===FUT_POLL_RTH_MS,
     'f44 EXECUTED: inside RTH it polls on the fast cadence', at('2026-09-01T14:00:00Z'));
  ok(at('2026-09-01T02:00:00Z')===FUT_POLL_OFF_MS,
     'f45 ...and outside it, on the slow one', at('2026-09-01T02:00:00Z'));
  // ⚠ 2026-09-05 is a SATURDAY, not a Friday — my first cut asserted it was a session and the test
  // caught me. Friday is the 4th.
  ok(at('2026-09-04T14:00:00Z')===FUT_POLL_RTH_MS && at('2026-09-06T14:00:00Z')===FUT_POLL_OFF_MS,
     'f46 ...and a Sunday at the same hour is not a session',
     [at('2026-09-04T14:00:00Z'), at('2026-09-06T14:00:00Z')]);
  ok(at('2026-09-01T13:25:00Z')===FUT_POLL_OFF_MS && at('2026-09-01T13:35:00Z')===FUT_POLL_RTH_MS,
     'f47 ...and the boundary is the 08:30 CT open, not the top of the hour',
     [at('2026-09-01T13:25:00Z'), at('2026-09-01T13:35:00Z')]);
  // ⚠ AND THE POLL MUST ACTUALLY CONSULT IT. A cadence function that is correct and unused is the
  // "right requirement wired to the wrong place" failure (v13.1) — this SURVIVED its first mutation
  // because every assertion above tested futPollMs() in isolation and none tested the caller.
  const fc = exC('futCourier');
  ok(/futPollMs\(\)/.test(fc),
     'f47b the courier gates on futPollMs(), not on a fixed constant', fc.slice(0,140));
  ok(!/FUT_POLL_OFF_MS|FUT_POLL_RTH_MS/.test(fc),
     'f47c ...and reads no cadence constant of its own, so there is one decision, in one place');

  // the request must still ask for the whole window, or a faster poll would BACKFILL LESS
  ok(/interval=1m&range=5d/.test(cs),
     'f48 every poll still asks for 5 days, so opening late still fills the session from 08:30');
}

console.log('\n' + pass + ' passed, ' + fail + ' failed');
process.exit(fail ? 1 : 0);
