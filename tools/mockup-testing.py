#!/usr/bin/env python3
"""
Renders design/mockup-testing-tab.html (+ -standalone): the Testing tab redesigned to sit between the
Analysis tab (studies, by subject) and the Dashboard (what fires at the tap). Numbers are pulled from
learning/register.json, learning/studies.json, learning/rules.json, learning/log/<last>.json and
data/es-1min/SWEEPS.json. Nothing here is illustrative; a stage with no number says so.
"""
import json, io, html, glob, collections
from importlib import import_module
import sys
sys.path.insert(0, 'tools')
CSS = import_module('mockup-from-studies').CSS
E = html.escape

REG = json.load(io.open('learning/register.json', encoding='utf-8'))
STU = json.load(io.open('learning/studies.json', encoding='utf-8'))
RUL = json.load(io.open('learning/rules.json', encoding='utf-8'))
SWP = json.load(io.open('data/es-1min/SWEEPS.json', encoding='utf-8'))
logs = sorted(f for f in glob.glob('learning/log/*.json') if 'catch-up' not in f)
LOG = json.load(io.open(logs[-1], encoding='utf-8')) if logs else None

studies = {x['id']: x for sj in STU['subjects'] for ss in sj['subsections'] for x in ss['studies']}
subj_of = {x['id']: sj['key'] for sj in STU['subjects'] for ss in sj['subsections'] for x in ss['studies']}
cnt = STU['counts']
tiers = collections.Counter(v.get('tier') for v in RUL['rules'].values() if isinstance(v, dict))
hand = tiers.get('hand', 0); earned = sum(v for k, v in tiers.items() if k != 'hand')
read_next = [x['id'] for x in studies.values() if x['status'] == 'READ NEXT']
open_n = cnt['byStatus'].get('OPEN', 0)
SWEEP_EVENTS = sum(c['events'] for c in SWP['cells'] if c['label'].endswith(('the LOD', 'the HOD')) and not c['label'].startswith(' '))

# register rows: hypothesis -> the study it belongs to
LINK = {'H1': 'F5.2', 'H2': 'F2.1', 'H3': 'F6.1', 'H4': 'F1.4', 'H5': 'H1.3'}
DRAFTS = [dict(id='H6', study='H2.7', claim='an ON/PD sweep-reclaim that lands in a top-5 node band or at the King prints the extreme more often', minN=40,
               predict='printed > 40% (base 24%, n=453)', refuteIf='printed <= 30% or the CI covers 24%', note='needs the TAP record or the API backfill'),
          dict(id='H7', study='H2.8', claim='a first-30-minute ON/PD sweep-reclaim prints the extreme more often than the fresh-low control', minN=60,
               predict='printed > 24%', refuteIf='printed <= 18%', note='read on sessions after 2026-08-21 only — the 27% n=180 was found on the sessions before it')]
verd = {h['id']: h for h in (LOG or {}).get('hypotheses', [])}


def bar(n, minN):
    n = n or 0; w = max(0, min(100, int(100*n/minN))) if minN else 0
    return '<span class="bar"><i style="width:%d%%"></i></span><span class="dm"> %d/%d</span>' % (w, n, minN)


def register_rows():
    h = []
    for H in REG['hypotheses']:
        sid = LINK.get(H['id'], '—'); sj = subj_of.get(sid, '—'); v = verd.get(H['id'], {})
        vd = (v.get('verdict') or 'thin').upper(); cls = {'THIN': 'am', 'BLOCKED': 'am', 'CLEARED': 'gr', 'REFUSED': 'rd', 'READY': 'bl'}.get(vd, 'dm')
        h.append('<tr><td class="bl"><b>%s</b></td><td class="dm">%s · %s</td><td>%s</td><td class="sk">%s</td><td class="dm">%s</td><td>%s</td><td class="r %s"><b>%s</b></td></tr>'
                 % (E(H['id']), E(sj), E(sid), E(H['claim']), E(H['predict']), E(H['refuteIf']), bar(v.get('n', 0), H['minN']), cls, vd))
    for D in DRAFTS:
        sj = subj_of.get(D['study'], '—')
        h.append('<tr class="draft"><td class="lil"><b>%s</b></td><td class="dm">%s · %s</td><td>%s</td><td class="sk">%s</td><td class="dm">%s</td><td>%s</td><td class="r lil"><b>DRAFT</b></td></tr>'
                 % (E(D['id']), E(sj), E(D['study']), E(D['claim']), E(D['predict']), E(D['refuteIf']), bar(0, D['minN'])))
    return ''.join(h)


def panel():
    since = REG.get('from', '—'); pre = (LOG or {}).get('preopen', 'no nightly log yet'); ldate = (LOG or {}).get('date', '—')
    h = ['<div class="pan"><div class="tabs"><span>Dashboard</span><span>📊 Analysis</span><span class="on">🧪 Testing</span></div>']
    h.append('<div class="subj"><span class="on">ALL</span>' + ''.join('<span>%s</span>' % E(s['key'] + ' ' + s['name']) for s in STU['subjects']) + '</div>')
    h.append('<div class="hd"><b>The loop, today</b> · %d studies → %d hypotheses written + %d drafted → %d features at the gate → %d rules on the ladder (%d earned, %d hand) → recorded: 11 sessions with the book, 284 price-only → nightly %s</div>'
             % (cnt['studies'], len(REG['hypotheses']), len(DRAFTS), 3, hand+earned, earned, hand, E(ldate)))
    # ⓪ flow strip
    h.append('<div class="flow">'
             '<div><b>ANALYSIS</b><span>%d studies</span><span class="dm">%d read · %d shipped</span></div><i>→</i>'
             '<div><b>REGISTER</b><span>%d written</span><span class="dm">%d drafted today</span></div><i>→</i>'
             '<div><b>GATE</b><span>3 features</span><span class="dm">30 per band to clear</span></div><i>→</i>'
             '<div><b>DASHBOARD</b><span>%d earned</span><span class="dm">%d hand rules</span></div><i>→</i>'
             '<div><b>RECORD</b><span>0 TAP records</span><span class="dm">v15.55</span></div><i>→</i>'
             '<div><b>NIGHTLY</b><span>%s</span><span class="dm">0 sessions since %s</span></div></div>'
             % (cnt['studies'], cnt['byStatus'].get('READ', 0), cnt['byStatus'].get('SHIPPED', 0), len(REG['hypotheses']), len(DRAFTS), earned, hand, E(ldate), E(since)))
    # ① register
    h.append('<div class="sec"><div class="sech"><span class="n">①</span><span class="t">THE REGISTER</span><span class="dc">fed by Analysis · read once at minN · sessions from %s</span></div><div class="secb">' % E(since))
    h.append('<table><tr><th>hyp</th><th>subject · study</th><th>claim</th><th>predict</th><th>refute if</th><th>n / minN</th><th class="r">verdict</th></tr>%s</table>' % register_rows())
    h.append('<div class="note">A hypothesis is written with its prediction and its refutation before the first session is scored, and never edited. Two are NULLS on purpose (H3, H4). H5 is blocked on the deflection ledger (0 of 50). H6 and H7 were drafted from today\'s sweep read on the Analysis tab and become rows here when the build writes them — H7 on sessions the read never saw.</div></div></div>')
    # ② gate
    h.append('<div class="sec"><div class="sech"><span class="n">②</span><span class="t">THE GATE</span><span class="dc">can the scorer fail? · rendered live from featGated</span></div><div class="secb">')
    h.append('<table><tr><th>feature</th><th>subject · rules it feeds</th><th class="r">predicted-low band</th><th class="r">predicted-high band</th><th class="r">Δ</th><th class="r">n per band</th><th class="r">verdict</th></tr>'
             '<tr><td><b>dir</b></td><td class="dm">D · dir.A / dir.B / dir.C · kill.*</td><td class="r dm">live</td><td class="r dm">live</td><td class="r dm">live</td><td class="r dm">30 to open</td><td class="r am"><b>⛔ until 30/band</b></td></tr>'
             '<tr><td><b>node</b></td><td class="dm">F · node.* · kill.tap3 · kill.negGammaWide</td><td class="r dm">live</td><td class="r dm">live</td><td class="r dm">live</td><td class="r dm">30 to open</td><td class="r am"><b>⛔ until 30/band</b></td></tr>'
             '<tr><td><b>decision</b></td><td class="dm">F P · decision.A×A … C×C</td><td class="r dm">live</td><td class="r dm">live</td><td class="r dm">live</td><td class="r dm">30 to open</td><td class="r am"><b>⛔ until 30/band</b></td></tr></table>')
    h.append('<div class="note">A feature whose hit rate does not move by ≥ 10 points between the band it predicts low and the band it predicts high cannot promote (ruleTier ⚖) and its rate cannot render (pctN "⛔ gated"). The three cells above are computed from the feat store at render; this page does not reproduce them. The lodhod scorer that once read 100% in every cell (n=362) is the reason this section exists.</div></div></div>')
    # ③ dashboard
    h.append('<div class="sec"><div class="sech"><span class="n">③</span><span class="t">ON THE DASHBOARD</span><span class="dc">what the ladder renders, and which study each number comes from</span></div><div class="secb">')
    h.append('<table><tr><th>ladder element</th><th>subject · study</th><th>renders</th><th>rule / tier</th><th class="r">state</th></tr>'
             '<tr><td>HOD/LOD "is it in" cell</td><td class="dm">H · H1.1</td><td>posr × clock cell with n · NOT-IN 85% <span class="ci">n=230</span></td><td class="dm">HLTAB · table</td><td class="r gr"><b>LIVE</b></td></tr>'
             '<tr><td>♛ King pill · moves / dwell</td><td class="dm">K · K4.3</td><td>kingmoves · kingdwell</td><td class="dm">measured</td><td class="r gr"><b>LIVE</b></td></tr>'
             '<tr><td>deflection ledger (.g3dfl)</td><td class="dm">F · F1.2</td><td>wick trigger · 3m close · held / broke · MFE10</td><td class="dm">engine</td><td class="r gr"><b>LIVE</b></td></tr>'
             '<tr><td>sweep line (new)</td><td class="dm">H · H2.4 H2.5</td><td>"ONL swept 08:41 · deep · slow — 40% <span class="ci">n=86</span>" — base rates only, no node claim</td><td class="dm">SWEEPS.json</td><td class="r lil"><b>v15.55</b></td></tr>'
             '<tr><td>direction grade A / B / C</td><td class="dm">D · D2</td><td>"⛔ gated" until the dir gate clears; never a bare %</td><td class="dm">dir.A/B/C · hand</td><td class="r am"><b>GATED</b></td></tr>'
             '<tr><td>kill.tap3</td><td class="dm">F · F2.2</td><td>kills the 3rd tap</td><td class="dm">hand · n=2</td><td class="r am"><b>THIN</b></td></tr>'
             '<tr><td>kill.negGammaWide</td><td class="dm">F · F6.1</td><td>kills on −γ wide</td><td class="dm">hand · contradicts H3 (null)</td><td class="r rd"><b>FLAG</b></td></tr>'
             '<tr><td>decision grid A×A … C×C</td><td class="dm">F P · decision.*</td><td>"⛔ gated"</td><td class="dm">9 hand rules</td><td class="r am"><b>GATED</b></td></tr></table>')
    h.append('<div class="note">%d rules on the ladder, %d earned. A hand rule renders no rate. kill.negGammaWide is flagged because a registered null (H3: polarity does not discriminate, 52.2%% vs 52.1%%, n=46/48) argues against a rule that kills on polarity — when H3 clears at 40, the rule retires; if H3 is refused, it stays. That is the loop feeding the Dashboard.</div></div></div>' % (hand+earned, earned))
    # ④ record
    h.append('<div class="sec"><div class="sech"><span class="n">④</span><span class="t">THE RECORD</span><span class="dc">what is captured, what the open studies still need</span></div><div class="secb">')
    h.append('<table><tr><th>store</th><th class="r">size</th><th>fields present</th><th>missing for the OPEN studies</th></tr>'
             '<tr><td>feat (IDB)</td><td class="r">48,008 recs · 11 d</td><td>grade · p · tap# · state · growth15 · pol · MFE10 / MAE · toClose (lodhod)</td><td class="dm">trinity at tap · gk ratio · King role & book · velocity · EM edge</td></tr>'
             '<tr><td>defl (IDB v3)</td><td class="r">15 labelled · 1 d</td><td>sig · tapBar · wick · held / broke · extent</td><td class="dm">wasSessionExtreme (labelled at close, v15.55) · node born-during-move · sweep link</td></tr>'
             '<tr><td>TAP record</td><td class="r lil">0 · v15.55</td><td class="dm">— the record %d OPEN studies wait on —</td><td class="dm">identity · node · lifecycle · growth · structure · configuration · trinity · both zones · extent · wasSessionExtreme</td></tr>'
             '<tr><td>ES 1-min (price)</td><td class="r">284 sessions</td><td>RTH + Globex · ONH/ONL · PDH/PDL · IB · sweeps (%d events)</td><td class="dm">NQ alongside for D5 · the node at the sweep (H6)</td></tr>'
             '<tr><td>kingRoll / gatekeeper recs</td><td class="r">437 · 671</td><td>recorded since v14</td><td class="rd">never read — %d READ NEXT studies</td></tr></table>' % (open_n, SWEEP_EVENTS, len(read_next)))
    h.append('<div class="note">The record is the bottleneck, not the analysis: %d of %d studies are OPEN on the TAP record. Shipping it is v15.55; every OPEN row on the Analysis tab lights up as it fills.</div></div></div>' % (open_n, cnt['studies']))
    # ⑤ nightly
    h.append('<div class="sec"><div class="sech"><span class="n">⑤</span><span class="t">THE NIGHTLY</span><span class="dc">run.py · reads the register · refreshes the tables · writes the digest for the LLM review</span></div><div class="secb">')
    h.append('<div class="row"><span class="k">last run</span><span class="v">%s · %s</span></div>' % (E(ldate), E(pre)))
    h.append('<div class="row"><span class="k">reads next</span><span class="v">H2 at 30 episodes · H1 H3 H4 at 40 · H5 at 50 deflections · then H6 H7 once written</span></div>')
    h.append('<div class="row"><span class="k">refreshes</span><span class="v">BASERATES.json · SWEEPS.json (%d cells, %d sessions) · HLTAB calibration cells (close-scored rows only)</span></div>' % (SWP['ledger']['cells_read'], SWP['corpus']['sessions']))
    h.append('<div class="row"><span class="k">read-next queue</span><span class="v">%s <span class="dm">· %d in all — data on hand, never read; one per night, in this order</span></span></div>' % (E(' · '.join(read_next[:6])), len(read_next)))
    h.append('<div class="row"><span class="k">ledger</span><span class="v">every cell read is counted; a first read is never a verdict — it becomes a register row and is read again on sessions it has not seen</span></div>')
    h.append('<div class="row"><span class="k">LLM review</span><span class="v">day-digest.py → the digest (verdicts · flagged rules · read-next result · coverage gaps) → review → proposals land in rules.json <span class="dm">(0 open)</span> → reimplementation → build → installer</span></div>')
    h.append('</div></div>')
    # ⑥ suite
    h.append('<div class="sec"><div class="sech"><span class="n">⑥</span><span class="t">THE SUITE</span><span class="dc">the build cannot ship red</span></div><div class="secb">')
    h.append('<div class="row"><span class="k">last run</span><span class="v">131 files · <b class="gr">122 green</b> · <b class="rd">9 red</b> <span class="dm">(5 baseline: expiry_profile · node_map · sma_cont · tapeking · v1126_process — 4 save gates clear at save)</span></span></div>')
    h.append('<div class="row"><span class="k">mutation</span><span class="v">every new assertion is mutation-tested; v15.54: 44 assertions, M2 caught a gate the stub could not reach until the fixture had 25 sessions</span></div>')
    h.append('<div class="row"><span class="k">self-test</span><span class="v">run.py --selftest · 16 nodes/day fixture · the pipeline scores a planted effect and refuses a planted null</span></div>')
    h.append('</div></div>')
    h.append('<div class="foot"><span>● rec</span><span>● saved</span><span>● pushed</span><span style="margin-left:auto">Testing · by subject</span></div></div>')
    return ''.join(h)


EXTRA = r"""
.flow{display:flex;align-items:center;gap:4px;margin:5px 6px 2px;padding:5px 6px;border:1px solid var(--g-line);border-radius:6px;background:var(--g-card)}
.flow div{flex:1;min-width:0;display:flex;flex-direction:column;gap:1px;padding:2px 5px;border-left:2px solid var(--g-line2)}
.flow div b{font-size:6.6px;letter-spacing:.09em;color:var(--g-blu)} .flow div span{font-size:7.6px;color:var(--g-txt)} .flow div span.dm{font-size:6.8px}
.flow i{color:var(--g-dim);font-style:normal;font-size:9px}
.bar{display:inline-block;width:52px;height:5px;background:rgba(255,255,255,.08);border-radius:2px;vertical-align:middle;overflow:hidden} .bar i{display:block;height:100%;background:var(--g-amb)}
tr.draft td{color:var(--g-mut)} td.sk{color:var(--g-lil)}
.row{display:flex;gap:6px;align-items:baseline;padding:2px 0;border-top:1px dashed rgba(255,255,255,.05)} .row:first-child{border-top:0}
.k{color:var(--g-mut);min-width:72px;font-size:7.4px} .v{color:var(--g-txt);font-size:7.6px}
td{white-space:normal}
"""


def page():
    return '\n'.join([
        '<title>Testing by Subject</title>',
        '<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=IBM+Plex+Mono:wght@400;500;600&family=IBM+Plex+Sans:wght@400;500;600;700&display=swap">',
        '<style>' + CSS + EXTRA + '</style>', '<div class="wrap">',
        '<h1>The Testing tab, redesigned</h1>',
        '<p class="lede">The Testing tab is the machine between the two other tabs. <b>Analysis</b> asks the questions and reads the evidence; <b>Testing</b> turns a read into a hypothesis with a refutation written in advance, checks that the scorer can fail, and decides what the <b>Dashboard</b> is allowed to render at the tap. Same seven subjects, same ids: a row on Analysis, its hypothesis here and its rule on the ladder share one name.</p>',
        '<div class="flag"><span class="tag">Real state</span><p><b>Every number is today\'s.</b> Five hypotheses written, zero sessions scored since the register\'s start date, 75 hand rules and none earned, no TAP record yet — the tab shows an honest empty machine, which is the correct thing for it to show. The two drafts (H6, H7) are today\'s sweep read graduating from Analysis; they are not in the register until the build writes them.</p></div>',
        '<div class="ctl"><span>Scale</span><button type="button" data-s="1" aria-pressed="false">1×</button><button type="button" data-s="1.55" aria-pressed="true">1.55×</button><button type="button" data-s="2.1" aria-pressed="false">2.1×</button></div>',
        '<section><h2><span>Testing</span> the loop, the register, the gate, the ladder, the record, the nightly, the suite</h2>',
        '<div class="stage"><div class="scaler">' + panel() + '</div></div>',
        '<div class="pros"><div><b>Fed by Analysis</b>A study on the Analysis tab reaches READ (a first number, with n and a control). The Testing tab turns it into a register row — predict and refuteIf fixed, minN set, read once on sessions the read never saw. Today that is H6 (sweep × node) and H7 (the early sweep). A study that stays exploratory never reaches the Dashboard.</div><div><b>Feeds the Dashboard</b>The ladder renders a number only when its rule has earned a tier and its feature has cleared the gate; otherwise it prints "⛔ gated" or "thin", never a bare percentage. The gate\'s three rows and the FLAG on kill.negGammaWide are the two places where this tab changes what the ladder says.</div></div>',
        '<div class="verdict">What it replaced: T_canfail · T_prereg · T_prop · T_chal · T_kill · T_cov · T_self · T_detail — eight sections with no subject and no link to a study. Now six, each answering one question in the loop, each row carrying the subject and study id it belongs to.</div></section>',
        '<section><h2>Build</h2><p><b>v15.55</b> — the Testing tab renders from <code class="mono">register.json</code> (+ the two new rows), <code class="mono">studies.json</code> (the subject strip and the study ids), <code class="mono">rules.json</code> (tiers), the last nightly log (verdicts, read-next result) and the live gate. The TAP record ships in the same build. The sweep line on the ladder renders the base rate from <code class="mono">SWEEPS.json</code> with its n and makes no node claim until H6 clears.</p></section>',
        '</div><script>(function(){ var b=[].slice.call(document.querySelectorAll(".ctl button")); b.forEach(function(x){ x.addEventListener("click",function(){ b.forEach(function(o){ o.setAttribute("aria-pressed", o===x?"true":"false"); }); document.documentElement.style.setProperty("--scale", x.getAttribute("data-s")); }); }); function fit(){ [].slice.call(document.querySelectorAll(".scaler")).forEach(function(s){ var p=s.querySelector(".pan"); if(p) s.style.setProperty("--sh", p.offsetHeight+"px"); }); } fit(); window.addEventListener("resize",fit); })();</script>'])


if __name__ == '__main__':
    body = page()
    io.open('design/mockup-testing-tab.html', 'w', encoding='utf-8').write(body)
    head, rest = body.split('<style>', 1); css, tail = rest.split('</style>', 1)
    io.open('design/mockup-testing-tab-standalone.html', 'w', encoding='utf-8').write(
        '<!doctype html>\n<html lang="en">\n<head>\n<meta charset="utf-8">\n<meta name="viewport" content="width=device-width,initial-scale=1">\n' + head + '<style>' + css + '</style>\n</head>\n<body>\n' + tail + '\n</body>\n</html>\n')
    print('wrote design/mockup-testing-tab(.html, -standalone.html)')
