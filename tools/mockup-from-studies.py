#!/usr/bin/env python3
"""
Renders design/mockup-analysis-by-subject.html (+ -standalone) FROM learning/studies.json and
data/es-1min/SWEEPS.json - so the mockup and the registry cannot drift. One panel per subject, every
subsection expanded, every study a row; the sweep subsection carries its real table.
"""
import json, io, html

S = json.load(io.open('learning/studies.json', encoding='utf-8'))
W = json.load(io.open('data/es-1min/SWEEPS.json', encoding='utf-8'))
E = html.escape

STATUS_CLS = {'SHIPPED': 'gr', 'READ': 'bl', 'READ NEXT': 'bl2', 'THIN': 'am', 'OPEN': 'dm', 'REFUSED': 'rd',
              'REGISTERED': 'pu', 'BLOCKED': 'am', 'DRAFT': 'lil'}

CSS = r"""
:root{ --ink:#141821; --ink2:#3d4557; --mut:#6b7488; --rule:#d9dde5; --paper:#f4f5f8; --card:#ffffff; --acc:#8a5a2b; --good:#1d6b4a; --bad:#9b2f38;
  --g-bg:#0b0e14; --g-panel:#0f131b; --g-card:#12161f; --g-line:#1e2530; --g-line2:#3a4150; --g-txt:#e6edf3; --g-mut:#8b98a9; --g-dim:#6c7889;
  --g-grn:#2ec27e; --g-red:#f0616d; --g-amb:#f2b45a; --g-yel:#e3c341; --g-pur:#a371f7; --g-blu:#7cc7ff; --g-lil:#cdb4fa; --g-teal:#5fd3c4; --scale:1.55; }
@media (prefers-color-scheme:dark){:root:not([data-theme="light"]){ --ink:#e7eaf0; --ink2:#aab3c4; --mut:#7d8798; --rule:#2a3040; --paper:#101319; --card:#171b24; --acc:#d9a066; --good:#4cc38a; --bad:#e5707a; }}
:root[data-theme="dark"]{ --ink:#e7eaf0; --ink2:#aab3c4; --mut:#7d8798; --rule:#2a3040; --paper:#101319; --card:#171b24; --acc:#d9a066; --good:#4cc38a; --bad:#e5707a; }
*{box-sizing:border-box}
body{margin:0;background:var(--paper);color:var(--ink);font-family:"IBM Plex Sans",ui-sans-serif,system-ui,sans-serif;line-height:1.55;-webkit-font-smoothing:antialiased}
.wrap{max-width:1080px;margin:0 auto;padding:38px 26px 90px}
h1{font-size:29px;font-weight:600;letter-spacing:-.015em;margin:0 0 6px;text-wrap:balance}
h2{font-size:12px;font-weight:600;letter-spacing:.10em;text-transform:uppercase;color:var(--mut);margin:0 0 14px;padding-bottom:7px;border-bottom:1px solid var(--rule)}
h2 span{color:var(--ink);font-weight:700;letter-spacing:0;text-transform:none;font-size:15px;margin-right:10px}
p{margin:0 0 13px;max-width:68ch;color:var(--ink2)}
.lede{font-size:16px;color:var(--ink2);max-width:70ch;margin-bottom:22px}
section{margin-top:52px}
.mono{font-family:"IBM Plex Mono",ui-monospace,monospace}
.flag{display:flex;gap:13px;align-items:flex-start;border:1px solid var(--rule);border-left:3px solid var(--acc);background:var(--card);padding:13px 16px;border-radius:3px;margin-bottom:26px}
.flag p{margin:0;font-size:13.5px} .flag b{color:var(--ink)}
.tag{display:inline-block;font-family:"IBM Plex Mono",monospace;font-size:10px;letter-spacing:.09em;text-transform:uppercase;padding:2px 7px;border:1px solid currentColor;border-radius:2px;color:var(--acc);white-space:nowrap}
.ctl{display:flex;gap:7px;align-items:center;margin:0 0 16px;font-size:12px;color:var(--mut)}
.ctl button{font-family:"IBM Plex Mono",monospace;font-size:11px;padding:4px 10px;cursor:pointer;background:var(--card);color:var(--ink2);border:1px solid var(--rule);border-radius:2px}
.ctl button[aria-pressed="true"]{background:var(--ink);color:var(--paper);border-color:var(--ink)}
.stage{background:var(--g-bg);border:1px solid var(--g-line2);border-radius:4px;padding:14px;overflow-x:auto;margin-bottom:4px}
.scaler{width:608px;transform:scale(var(--scale));transform-origin:top left;margin-bottom:calc((var(--scale) - 1) * var(--sh,400px))}
.pan{width:608px;background:var(--g-panel);color:var(--g-txt);font-family:ui-sans-serif,system-ui,"Segoe UI",Roboto,sans-serif;font-variant-numeric:tabular-nums;font-size:8.4px;line-height:1.35}
.tabs{display:flex;gap:2px;background:#12161f;border-bottom:1px solid var(--g-line2);padding:4px 4px 0}
.tabs span{padding:4px 10px;font-size:9.5px;font-weight:600;color:var(--g-dim)} .tabs span.on{color:var(--g-txt);font-weight:800;border-bottom:2px solid var(--g-blu)}
.hd{padding:5px 7px 4px;border-bottom:1px solid var(--g-line);font-size:7.6px;color:var(--g-mut)} .hd b{color:var(--g-txt);font-size:9px}
.subj{display:flex;gap:3px;padding:4px 6px 0;flex-wrap:wrap} .subj span{font-size:7.2px;font-weight:800;letter-spacing:.06em;padding:2px 6px;border:1px solid var(--g-line2);border-radius:3px;color:var(--g-dim)} .subj span.on{color:var(--g-txt);border-color:var(--g-blu);background:#141a24}
.sec{margin:5px 6px;border:1px solid var(--g-line);border-radius:6px;background:var(--g-card);overflow:hidden}
.sech{display:flex;align-items:center;gap:6px;padding:4px 7px;background:#141a24;border-bottom:1px solid var(--g-line)}
.sech .n{font-size:8.6px;font-weight:900;color:var(--g-blu);min-width:16px} .sech .t{font-size:8.4px;font-weight:800;letter-spacing:.03em;color:var(--g-txt)}
.sech .dc{margin-left:auto;font-size:6.6px;letter-spacing:.08em;color:var(--g-teal);font-weight:800;white-space:nowrap} .sech .ct{font-size:7px;color:var(--g-dim);white-space:nowrap}
.sech .car{color:var(--g-dim);font-size:8px}
.secb{padding:4px 7px 5px}
.note{font-size:7.2px;color:var(--g-mut);font-style:italic;padding:3px 0 2px;border-top:1px dashed rgba(255,255,255,.06);margin-top:3px}
.sc{display:flex;gap:6px;padding:2.5px 0 0;font-size:7.8px;align-items:baseline;border-top:1px dashed rgba(255,255,255,.05)} .sc:first-child{border-top:0}
.sc .id{font-family:ui-monospace,monospace;color:var(--g-dim);min-width:30px} .sc .qq{color:var(--g-txt);flex:1;min-width:0}
.sc .de{font-size:6.2px;letter-spacing:.07em;font-weight:800;color:var(--g-teal);min-width:54px;text-align:right;white-space:nowrap}
.sc .cl{color:var(--g-dim);min-width:40px;font-size:7px;white-space:nowrap} .sc .cp{color:var(--g-dim);min-width:74px;font-size:7px;white-space:nowrap}
.sc .vd{min-width:56px;text-align:right;font-weight:900;letter-spacing:.05em;font-size:6.8px;white-space:nowrap}
.rs{padding:0 0 2.5px 36px;font-size:7.4px;color:var(--g-mut)} .rs b{color:var(--g-txt);font-weight:600} .rs .ci{color:var(--g-dim);font-style:italic}
.gr{color:var(--g-grn)} .rd{color:var(--g-red)} .am{color:var(--g-amb)} .ye{color:var(--g-yel)} .pu{color:var(--g-pur)} .bl{color:var(--g-blu)} .bl2{color:#a9dbff} .te{color:var(--g-teal)} .dm{color:var(--g-dim)} .mu{color:var(--g-mut)} .lil{color:var(--g-lil)}
table{border-collapse:collapse;width:100%;font-size:7.6px;margin:3px 0 2px}
th{font-size:6.4px;font-weight:800;letter-spacing:.07em;color:var(--g-dim);text-transform:uppercase;text-align:left;padding:2px 4px 3px;border-bottom:1px solid var(--g-line2)}
td{padding:1.5px 4px;border-top:1px solid rgba(255,255,255,.04);vertical-align:top;white-space:nowrap} td.r,th.r{text-align:right}
td.sub{padding-left:12px;color:var(--g-mut)}
.foot{padding:5px 7px;border-top:1px solid var(--g-line);font-size:7.4px;color:var(--g-dim);display:flex;gap:9px}
.pros{display:grid;grid-template-columns:1fr 1fr;gap:14px;margin-top:12px;max-width:900px}
.pros div{font-size:13px;color:var(--ink2);border-top:2px solid var(--rule);padding-top:8px}
.pros b{display:block;font-size:11px;letter-spacing:.08em;text-transform:uppercase;color:var(--mut);margin-bottom:4px}
.verdict{margin-top:12px;font-size:13.5px;color:var(--ink);border-left:3px solid var(--acc);padding:6px 12px;background:var(--card);max-width:900px}
.legend{font-size:12.5px;color:var(--ink2);max-width:900px;margin:0 0 18px}
.legend code{font-family:"IBM Plex Mono",monospace;font-size:11.5px;color:var(--ink)}
@media (prefers-reduced-motion:reduce){*{transition:none!important}}
@media (max-width:700px){.wrap{padding:26px 15px 70px} h1{font-size:23px} .pros{grid-template-columns:1fr}}
"""


def chips(s):
    return ' · '.join(x.strip() for x in s.split('·'))


def row(x):
    st_ = x['status']; cls = STATUS_CLS.get(st_, 'dm')
    was = (' <span class="dm">(' + E(x['was']) + ')</span>') if x.get('was') else ''
    h = ('<div class="sc"><span class="id">%s</span><span class="qq">%s%s</span><span class="de">%s</span>'
         '<span class="cl">%s</span><span class="cp">%s</span><span class="vd %s">%s</span></div>'
         % (E(x['id']), E(x['q']), was, E(chips(x['decides'])), E(x['claim']), E(x['corpus']), cls, E(st_)))
    if x.get('result'):
        scr = (' <span class="ci">· ' + E(x['script']) + '</span>') if x.get('script') else ''
        by = (' <span class="ci nt">· by the nightly, %s</span>' % E(x.get('asOf') or '')) if x.get('by') == 'nightly' else ''
        h += '<div class="rs">→ <b>%s</b>%s%s</div>' % (E(x['result']), scr, by)
    # (v15.72) the nightly's count so far under a row it is still counting toward — the panel drew this line since v15.68
    # (studyRowHtml); the generator is the look's source, so it draws it too (test_v1562 2e pins them equal on subject K)
    if x.get('nightly') and x.get('nightly') != x.get('result'):
        h += '<div class="rs nt">⟳ <b>%s</b></div>' % E(x['nightly'])
    return h


def sweep_table():
    cells = {c['label'].strip(): c for c in W['cells']}
    order = ['ONL sweep-reclaim -> printed the LOD', 'ONH sweep-reclaim -> printed the HOD', 'PDL sweep-reclaim -> printed the LOD',
             'PDH sweep-reclaim -> printed the HOD', 'IBL sweep-reclaim -> printed the LOD', 'IBH sweep-reclaim -> printed the HOD',
             'reclaim within 5 bars', 'reclaim in 6-30 bars', 'depth <= 3 pts', 'depth 3-8 pts', 'depth > 8 pts']
    names = {'ONL sweep-reclaim -> printed the LOD': 'ONL → LOD', 'ONH sweep-reclaim -> printed the HOD': 'ONH → HOD',
             'PDL sweep-reclaim -> printed the LOD': 'PDL → LOD', 'PDH sweep-reclaim -> printed the HOD': 'PDH → HOD',
             'IBL sweep-reclaim -> printed the LOD': 'IBL → LOD', 'IBH sweep-reclaim -> printed the HOD': 'IBH → HOD',
             'reclaim within 5 bars': 'reclaim ≤ 5 bars (the poke)', 'reclaim in 6-30 bars': 'reclaim 6–30 bars (the flush)',
             'depth <= 3 pts': 'depth ≤ 3 pts', 'depth 3-8 pts': 'depth 3–8 pts', 'depth > 8 pts': 'depth > 8 pts'}
    h = ['<table><tr><th>sweep-and-reclaim</th><th class="r">events</th><th class="r">accepted</th><th class="r">printed it</th><th class="r">rate [95%]</th><th class="r">fresh-low control</th><th class="r">lift</th><th class="r">pays (med.)</th></tr>']
    for k in order:
        c = cells[k]
        lift = c['lift_fresh']; lc = 'gr' if lift >= 0.08 else ('rd' if lift <= -0.08 else 'mu')
        h.append('<tr><td%s>%s</td><td class="r">%d</td><td class="r dm">%d</td><td class="r">%d</td><td class="r"><b>%.0f%%</b> <span class="ci">[%.0f–%.0f]</span></td><td class="r dm">%.0f%%</td><td class="r %s">%+.0fpp</td><td class="r">%s</td></tr>'
                 % (' class="sub"' if k.startswith(('reclaim', 'depth')) else '', E(names[k]), c['events'], c['accepted'], c['printed'],
                    100*c['rate'], 100*c['ci'][0], 100*c['ci'][1], 100*c['fresh'], lc, 100*lift,
                    ('%.1f pts' % c['pay_far_med']) if c['pay_far_med'] is not None else '—'))
    early = [cells[k] for k in ['ONL · 08:30-09:00', 'ONH · 08:30-09:00', 'PDL · 08:30-09:00', 'PDH · 08:30-09:00']]
    ek = sum(c['printed'] for c in early); en = sum(c['reclaimed'] for c in early)
    ef = sum(c['fresh']*c['reclaimed'] for c in early)/en
    h.append('<tr><td class="sub">first 30 minutes, ON+PD pooled</td><td class="r">%d</td><td class="r dm">—</td><td class="r">%d</td><td class="r"><b>%.0f%%</b></td><td class="r dm">%.0f%%</td><td class="r gr">%+.0fpp</td><td class="r">—</td></tr>'
             % (en, ek, 100*ek/en, 100*ef, 100*(ek/en-ef)))
    h.append('</table>')
    h.append('<div class="note">%d sessions %s → %s · fresh-low control = a bounce off a session low printed in the last 30 bars at the same minute, at any level or none · %d cells read, none pre-registered · pays = median points from the sweep to the far extreme when the sweep WAS the extreme</div>'
             % (W['corpus']['sessions'], W['corpus']['first'], W['corpus']['last'], W['ledger']['cells_read']))
    return ''.join(h)


def panel(sj):
    n_st = sum(len(s['studies']) for s in sj['subsections'])
    by = {}
    for s in sj['subsections']:
        for x in s['studies']:
            by[x['status']] = by.get(x['status'], 0) + 1
    cnt = ' · '.join('%d %s' % (v, k.lower()) for k, v in sorted(by.items(), key=lambda kv: -kv[1]))
    h = ['<div class="pan"><div class="tabs"><span>Dashboard</span><span class="on">📊 Analysis</span><span>🧪 Testing</span></div>']
    h.append('<div class="subj">' + ''.join('<span%s>%s</span>' % (' class="on"' if s['key'] == sj['key'] else '', E(s['key'] + ' ' + s['name'])) for s in S['subjects']) + '</div>')
    h.append('<div class="hd"><b>%s</b> · %s · %d subsections · %d studies · %s</div>' % (E(sj['name']), E(sj['strap']), len(sj['subsections']), n_st, cnt))
    for s in sj['subsections']:
        h.append('<div class="sec"><div class="sech"><span class="car">▾</span><span class="n">%s</span><span class="t">%s</span><span class="dc">decides %s</span><span class="ct">· %d</span></div><div class="secb">'
                 % (E(s['key']), E(s['name']), E(chips(s['decides'])), len(s['studies'])))
        for x in s['studies']:
            h.append(row(x))
        if s['key'] == 'H2':
            h.append(sweep_table())
        if s.get('note'):
            h.append('<div class="note">%s</div>' % E(s['note']))
        h.append('</div></div>')
    h.append('<div class="foot"><span>● rec</span><span>● saved</span><span>● pushed</span><span style="margin-left:auto">subject %s</span></div></div>' % E(sj['key']))
    return ''.join(h)


def page():
    c = S['counts']
    out = ['<title>Analysis by Subject</title>',
           '<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=IBM+Plex+Mono:wght@400;500;600&family=IBM+Plex+Sans:wght@400;500;600;700&display=swap">',
           '<style>' + CSS + '</style>', '<div class="wrap">',
           '<h1>The Analysis tab, by subject</h1>',
           '<p class="lede">Seven subjects, split the way a trader thinks about them, %d studies. Every study is a question whose answer changes an action at the tap — <b>size, side, target, stop, skip, time, level, wait</b> — and nothing is on the tab for the sake of being known. This page is rendered from <code class="mono">learning/studies.json</code>, the one registry the tab, the Testing tab and the nightly will read.</p>' % c['studies'],
           '<div class="flag"><span class="tag">Real numbers</span><p><b>Every number here is measured</b> — there are no illustrative cells on this page. The 284-session ES corpus (0.879, NOT-IN 85%% n=230, the range and clock base rates), the 11-day book corpus (94 episodes; tap decay 73%% n=22 vs 47%% n=70; H3 and H4 nulls), and the <b>sweep study run today</b> (919 sweep events over 284 sessions, 34 cells). A study with no number says <i>thin</i>, <i>open</i> or <i>read next</i>. Counts: %s.</p></div>' % E(' · '.join('%d %s' % (v, k.lower()) for k, v in sorted(c['byStatus'].items(), key=lambda kv: -kv[1]))),
           '<p class="legend">How to read a row: <code>id</code> · the question · <code>DECIDES</code> what it changes at the tap · the doctrine claim it tests (<code>C-n</code>, or <i>lore</i> for trader folklore, <i>ours</i> for our own) · the corpus it needs · its status. A second line carries the result, always with its <code>n</code>. <code>tap record</code> means the v15.55 TAP record (not yet shipped); <code>API backfill</code> means Skylit\'s historical heatmap (his decision, ~15–20k credits).</p>',
           '<div class="ctl"><span>Scale</span><button type="button" data-s="1" aria-pressed="false">1×</button><button type="button" data-s="1.55" aria-pressed="true">1.55×</button><button type="button" data-s="2.1" aria-pressed="false">2.1×</button></div>']
    for sj in S['subjects']:
        out.append('<section><h2><span>%s · %s</span> %s</h2>' % (E(sj['key']), E(sj['name'].title() if sj['name'] != 'HOD / LOD' else 'HOD / LOD'), E(sj['strap'])))
        out.append('<div class="stage"><div class="scaler" data-panel="%s">%s</div></div>' % (E(sj['key']), panel(sj)))
        if sj['key'] == 'H':
            out.append('<div class="pros"><div><b>What the sweep study says</b>The level\'s <i>name</i> does not matter. A bounce off the overnight low, the prior-day low or the IB low is the day\'s low exactly as often as a bounce off any fresh low at that minute (29% vs 28%, n=113; 23% vs 24%, n=87; 20% vs 26%, n=143). What matters is <b>when</b> (first 30 minutes: 27% vs 18%, n=180), <b>how deep</b> (>8 pts: 40% vs 24%, n=86) and <b>how slow the reclaim</b> (6–30 bars: 40% vs 24%, n=90). A shallow quick poke is <i>not</i> the low 86% of the time (n=228).</div><div><b>What it means for the trade</b>Do not buy the first reclaim of a level because it is <i>the ONL</i>; buy it because it flushed deep, took its time, and came early — and, the whole point of this application, because a top-5 node or the King sits there. That last clause is unmeasured: it is <b>H6</b>, the register entry the tap record exists to answer. When the sweep <i>is</i> the extreme, the far extreme is a median 60–89 pts away.</div></div>')
            out.append('<div class="verdict">First read, 34 cells, not pre-registered. The cells that stand out are the ones a trader would have named in advance (early, deep, slow), and they point the same way across four independent levels — but they are read on the corpus they were found in. H7 re-reads the early-sweep claim on sessions after 2026-08-21; H6 adds the node. Neither is traded until then.</div>')
        out.append('</section>')
    out.append('''<section><h2>What this changes in the build</h2>
<p><b>v15.55.</b> The tab renders <code class="mono">learning/studies.json</code> exactly as drawn: the subject strip, each subsection collapsible, each study a row with its status, and a result line only where a number exists. Studies with a script name link to the script; studies marked <i>tap record</i> light up as the TAP record fills. The sweep table is computed by <code class="mono">tools/study-sweeps.py</code> in the nightly and written to <code class="mono">data/es-1min/SWEEPS.json</code>, which the tab fetches like <code class="mono">BASERATES.json</code>.</p>
<p><b>The register grows by subject.</b> H6 (sweep × node) and H7 (the early sweep, re-read on new sessions) are drafted here and written to <code class="mono">learning/register.json</code> in the build — with <code class="mono">predict</code> and <code class="mono">refuteIf</code> fixed before the first new session is scored. The Testing tab is redesigned next to show the register, the gate and the promoted rules <b>by the same subjects</b>, so a row on Analysis, its hypothesis on Testing and its rule on the Dashboard share one id.</p>
<p><b>Merges.</b> The old S-A/S-B/S-C/S-D ids are carried in each row as <i>(was …)</i>; the old list retires when the tab ships.</p></section>''')
    out.append('</div><script>(function(){ var b=[].slice.call(document.querySelectorAll(".ctl button")); b.forEach(function(x){ x.addEventListener("click",function(){ b.forEach(function(o){ o.setAttribute("aria-pressed", o===x?"true":"false"); }); document.documentElement.style.setProperty("--scale", x.getAttribute("data-s")); }); }); function fit(){ [].slice.call(document.querySelectorAll(".scaler")).forEach(function(s){ var p=s.querySelector(".pan"); if(p) s.style.setProperty("--sh", p.offsetHeight+"px"); }); } fit(); window.addEventListener("resize",fit); })();</script>')
    return '\n'.join(out)


if __name__ == '__main__':
    body = page()
    io.open('design/mockup-analysis-by-subject.html', 'w', encoding='utf-8').write(body)
    head, rest = body.split('<style>', 1)
    css, tail = rest.split('</style>', 1)
    standalone = ('<!doctype html>\n<html lang="en">\n<head>\n<meta charset="utf-8">\n<meta name="viewport" content="width=device-width,initial-scale=1">\n'
                  + head + '<style>' + css + '</style>\n</head>\n<body>\n' + tail + '\n</body>\n</html>\n')
    io.open('design/mockup-analysis-by-subject-standalone.html', 'w', encoding='utf-8').write(standalone)
    print('wrote design/mockup-analysis-by-subject(.html, -standalone.html)', len(standalone), 'bytes')
