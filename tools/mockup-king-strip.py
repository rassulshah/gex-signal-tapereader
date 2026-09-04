#!/usr/bin/env python3
"""
MOCKUP v3 — feature 1 (the node row) and feature 2 (the King zone), simplified on his review, from the dashboard conversation of 2026-09-03.

    "three things to look at are if the node recently grew so it's a new node, whether there was a roll from a
     prior node, and growth." · "all three kings are important and can cause deflections. sometimes one will
     disagree because it is leading and the others are lagging … we don't know which node it will deflect on.
     what we do know is all 3 can pull and push (deflect) price."            - operator, 2026-09-03

REAL NUMBERS: the book at 12:48 CT on 2026-09-03 from his day file — the SPXW vendor rows (size, d15), the trinity
(the SPY / QQQ / SPXW Kings, their sizes at 12:33 and 12:48, their paths through the day), the roll event at 12:30,
the ES price. The SPY / QQQ Kings' taps are placeholders (the per-book ledger is v15.63) and are drawn as such.

Writes design/mockup-king-strip.html (+ -standalone.html) and, through tools/shot, the png.
"""
import io, sys, json, datetime
sys.path.insert(0, 'tools')
from importlib import import_module
M = import_module('mockup-from-studies')

ES = 1.00108
j = json.load(io.open('data/2026-09-03.json', encoding='utf-8'))
def hm(ms): return datetime.datetime.utcfromtimestamp(ms / 1000 - 5 * 3600).strftime('%H:%M')
snaps = j['snaps']['SPY']
snap = [s for s in snaps if hm(s['bar']) == '12:48'][0]
prev = [s for s in snaps if hm(s['bar']) == '12:33'][0]
rows = sorted([r for r in snap['vend']['rows'] if r[0] >= 1000], key=lambda r: -abs(r[1]))
king = rows[0][0]; kmax = abs(rows[0][1])
tri, tri0 = snap['tri'], prev['tri']
PX_ES = 7756.0
SPY_PX = snap['xm']['SPY']['px']; QQQ_PX = snap['xm']['QQQ']['px']
K = {
    'SPX': dict(strike=7750, es=7750 * ES, kd=tri['SPXW']['kd'], kd0=tri0['SPXW']['kd'], since='10:03', moved='⇄ from 7745 at 12:30 (+$20M) · crown since 10:03', lastMove='12:30', taps='2 held · 0 broke · last 12:06', ph=False),
    'SPY': dict(strike=773, es=773 * PX_ES / SPY_PX, kd=tri['SPY']['kd'], kd0=tri0['SPY']['kd'], since='12:27', moved='up 771 → 773 at 12:27', lastMove='12:27', taps='— · the per-book ledger is v15.63', ph=True),
    'QQQ': dict(strike=718, es=PX_ES * 718 / QQQ_PX, kd=tri['QQQ']['kd'], kd0=tri0['QQQ']['kd'], since='11:12', moved='718 since 11:12', lastMove='11:12', taps='— · v15.63', ph=True),
}
for b, d in K.items():
    d['g'] = round(100.0 * (d['kd'] - d['kd0']) / d['kd0']); d['dM'] = (d['kd'] - d['kd0']) / 1000.0
born = {7755: 1}
tapped = {7750: '2 held · 0 broke', 7740: '1 held · 0 broke'}
roll = {7750: '<span class="chip roll up">▲ from 7745 · 12:30</span>', 7745: '<span class="chip roll up">▲ to 7750 · 12:30</span>'}   # the 12:30 ROLL event: mass rolled UP out of 7745 into 7750 — a roll is between any two nodes, King or not
setup = {7740: '<span class="chip stk">PIKA STACK · 7735</span>', 7735: '<span class="chip stk">PIKA STACK · 7740</span>'}   # the setup column: pika stack · rug · reverse rug · air pocket · gatekeeper (the registry's S-setups) — only what is present now

def esq(k): return k * ES
def pct(cur): return round(100 * abs(cur) / kmax)
def grow(cur, d15):
    base = abs(cur) - d15 if cur >= 0 else abs(cur) + d15
    return None if base <= 0 else round(100 * d15 / base)

CSS = M.CSS.replace('--scale:1.55', '--scale:1.4') + r"""
.pan{width:700px}
/* ---- the King zone strip: three identical cells, six labelled lines each ---- */
.kstrip{display:grid;grid-template-columns:1fr 1fr 1fr;gap:5px;margin:6px 6px 3px}
.kc{border:1px solid var(--g-line);border-radius:6px;background:var(--g-card);padding:5px 7px 4px}
.kc .kh{display:flex;align-items:center;gap:5px;margin-bottom:3px}
.kc .bk{font-size:6.8px;letter-spacing:.1em;font-weight:900}
.kc .pos{margin-left:auto;font-size:6.2px;letter-spacing:.08em;font-weight:900;padding:1px 5px;border-radius:3px}
.kc .pos.ab{background:rgba(240,97,109,.18);color:#f0616d;box-shadow:inset 0 0 0 1px rgba(240,97,109,.5)} .kc .pos.be{background:rgba(46,194,126,.16);color:#2ec27e;box-shadow:inset 0 0 0 1px rgba(46,194,126,.5)}
.kc .kx{font-size:12px;font-weight:900;line-height:1.1;letter-spacing:-.01em}
.kc .kx small{font-size:7px;font-weight:700;color:var(--g-dim);margin-left:4px;letter-spacing:.04em}
.kc .kl{display:grid;grid-template-columns:44px 1fr;gap:4px;font-size:7px;color:var(--g-mut);line-height:1.35;padding:1.5px 0;border-top:1px dashed rgba(255,255,255,.06)}
.kc .kl:first-of-type{border-top:0}
.kc .kl em{font-style:normal;font-size:6.2px;letter-spacing:.08em;color:var(--g-dim);font-weight:800;text-transform:uppercase;padding-top:1px}
.kc .kl b{color:var(--g-txt);font-weight:700}
.kc.spx{border-color:rgba(227,195,65,.55)} .kc.spx .bk,.kc.spx .kx{color:#e3c341}
.kc.spy{border-color:rgba(205,180,250,.55)} .kc.spy .bk,.kc.spy .kx{color:#cdb4fa}
.kc.qqq{border-color:rgba(95,211,188,.55);border-style:dashed} .kc.qqq .bk,.kc.qqq .kx{color:#5fd3bc}
.up{color:#2ec27e} .dn{color:#f0616d} .fl{color:var(--g-dim)}
.ph{color:var(--g-dim);font-style:italic}
/* ---- the zone read: two labelled lines ---- */
.zread{margin:0 6px 5px;border:1px solid var(--g-line);border-left:3px solid #7cc7ff;border-radius:6px;background:var(--g-card);padding:4px 7px}
.zread .zl{display:grid;grid-template-columns:44px 1fr;gap:4px;font-size:7.6px;color:var(--g-txt);white-space:normal;line-height:1.4;padding:1.5px 0}
.zread .zl em{font-style:normal;font-size:6.2px;letter-spacing:.1em;font-weight:900;color:#7cc7ff;padding-top:2px}
.zread .zl .dm{font-size:6.6px}
/* ---- the ladder: a fixed grid, one column per tell ---- */
.lad{margin:0 6px 6px;border:1px solid var(--g-line);border-radius:6px;background:var(--g-card);overflow:hidden}
.grid{display:grid;grid-template-columns:62px 68px 140px 54px 96px 76px 1fr;gap:5px;align-items:center;padding:0 7px}
.lh{font-size:6.2px;letter-spacing:.1em;color:var(--g-dim);font-weight:800;border-bottom:1px solid var(--g-line2);text-transform:uppercase;padding-top:4px;padding-bottom:3px}
.lh span{white-space:nowrap}
.lr{min-height:19px;border-top:1px dashed rgba(255,255,255,.05);font-size:7.6px}
.lr.now{background:rgba(255,255,255,.045)}
.lr.zone{background:rgba(124,199,255,.045)}
.lv{font-size:6.6px;color:var(--g-dim);white-space:nowrap;line-height:1.25} .lv b{color:var(--g-txt)}
.lv.zt{color:#7cc7ff;font-weight:900;letter-spacing:.06em;font-size:6.2px}
.px{font-weight:800;color:var(--g-txt);font-variant-numeric:tabular-nums;white-space:nowrap}
.px small{font-weight:600;color:var(--g-dim);font-size:6.6px;margin-left:3px} .px.sub{color:var(--g-dim);font-weight:600}
.bar{display:flex;align-items:center;height:11px;border-radius:2px;padding:0 4px;font-size:7px;font-weight:900;color:#2a2408;background:#e3c341;box-sizing:border-box;min-width:14px}
.bar.neg{background:#a371f7;color:#1b1030} .bar.king{height:14px}
.bar.sub{background:transparent;box-shadow:inset 0 0 0 1px #e3c341;color:#e3c341;opacity:.55} .bar.sub.neg{box-shadow:inset 0 0 0 1px #a371f7;color:#a371f7}
.kchip{display:inline-flex;align-items:center;gap:4px;height:14px;padding:0 6px;border-radius:3px;font-size:7px;font-weight:900;letter-spacing:.04em;white-space:nowrap}
.kchip.spx{background:#e3c341;color:#2a2408} .kchip.spy{background:#cdb4fa;color:#1b1030} .kchip.qqq{background:transparent;color:#5fd3bc;box-shadow:inset 0 0 0 1px #5fd3bc}
.kchip small{font-weight:700;opacity:.8}
.c{white-space:nowrap;font-size:7.2px} .c.mut{color:var(--g-dim)}
.chip{display:inline-block;font-size:6.4px;letter-spacing:.06em;font-weight:900;padding:1px 5px;border-radius:3px;white-space:nowrap}
.chip.new{background:#2ec27e;color:#04120a} .chip.roll{background:rgba(124,199,255,.16);color:#7cc7ff;box-shadow:inset 0 0 0 1px rgba(124,199,255,.5)} .chip.roll.dn{background:rgba(240,97,109,.14);color:#f0616d;box-shadow:inset 0 0 0 1px rgba(240,97,109,.5)}
.chip.tap{background:rgba(255,255,255,.07);color:var(--g-txt)} .chip.stk{color:#cdb4fa;box-shadow:inset 0 0 0 1px rgba(205,180,250,.5)}
.g{font-size:7.6px;font-weight:800;white-space:nowrap} .g small{font-weight:600;color:var(--g-dim);font-size:6.4px;margin-left:3px}
.nowpill{display:inline-block;background:#fff;color:#0d1117;border-radius:8px;font-size:9px;font-weight:900;padding:1px 8px}
.leg{display:flex;gap:12px;flex-wrap:wrap;padding:4px 7px 5px;border-top:1px solid var(--g-line);font-size:6.6px;color:var(--g-dim);white-space:normal;line-height:1.5}
.leg b{color:var(--g-mut);font-weight:800}
.legend2{font-size:12.5px;color:var(--ink2);max-width:900px;margin:12px 0 0} .legend2 b{color:var(--ink)}
"""

def gtxt(g, dM=None, cls_only=False):
    if g is None: return '<span class="g fl">—</span>'
    cls = 'up' if g >= 3 else ('dn' if g <= -3 else 'fl'); arrow = '▲' if g >= 3 else ('▼' if g <= -3 else '▸')
    return '<span class="g %s">%s %+d%%</span>' % (cls, arrow, g)

def bar(cur, p, is_king=False, sub=False):
    w = max(14, int(p / 100 * 132)); cls = 'bar' + (' neg' if cur < 0 else '') + (' king' if is_king else '') + (' sub' if sub else '')
    return '<span class="%s" style="width:%dpx">%s</span>' % (cls, w, ('%d%%' % p) if p >= 6 else '')

def cell(html='', cls='c'): return '<span class="%s">%s</span>' % (cls, html)

def node_row(r, zone=False, lv=''):
    k, cur, d5, d15, d60 = r[0], r[1], r[2], r[3], r[4]; p = pct(cur); is_king = (k == king); sub = p < 20
    cls = 'grid lr' + (' zone' if zone else '')
    if sub:
        return ('<div class="%s">%s<span class="px sub">%.1f<small>%d</small></span>%s%s%s%s%s</div>'
                % (cls, cell(lv, 'lv'), esq(k), k, bar(cur, p, False, True), cell(), cell(), cell(), cell('context row', 'c mut')))
    g = grow(cur, d15)
    node = ('<span class="kchip spx">♛ SPX KING <small>$%dM</small></span>' % round(kmax / 1e6)) if is_king else bar(cur, p)
    return ('<div class="%s">%s<span class="px">%.1f<small>%d</small></span>%s%s%s%s%s</div>' % (
        cls, cell(lv, 'lv'), esq(k), k, node,
        cell('<span class="chip new">NEW %db</span>' % born[k] if k in born else ''),
        cell(roll.get(k, '')),
        cell(gtxt(g)),
        cell(setup.get(k, ''))))

def king_row(b, lv=''):
    d = K[b]; label = {'SPX': '♛ SPX KING', 'SPY': '♛ SPY KING', 'QQQ': '♛ QQQ KING ≈'}[b]
    rolled = {'SPX': '<span class="chip roll up">▲ from 7745 · 12:30</span>', 'SPY': '<span class="chip roll up">▲ 771 → 773 · 12:27</span>', 'QQQ': ''}[b]
    return ('<div class="grid lr zone">%s<span class="px">%.1f<small>%d</small></span><span class="kchip %s">%s</span>%s%s%s%s</div>' % (
        cell(lv, 'lv zt'), d['es'], d['strike'], b.lower(), label,
        cell(''), cell(rolled), cell(gtxt(d['g'])), cell('bearing' if b == 'QQQ' else '', 'c mut')))

def king_cell(b, name, cls, lead=False):
    d = K[b]
    above = d['es'] > PX_ES
    badge = '<span class="pos %s">%s · %.1f</span>' % ('ab' if above else 'be', 'ABOVE' if above else 'BELOW', abs(d['es'] - PX_ES))
    rolled = {'SPX': '<span class="up">▲</span> up · from 7745 at 12:30', 'SPY': '<span class="up">▲</span> up · 771 → 773 at 12:27', 'QQQ': '<span class="fl">—</span> none today'}[b]
    return ('<div class="kc %s"><div class="kh"><span class="bk">%s</span>%s</div>'
            '<div class="kx">%.1f<small>%s %d</small></div>'
            '<div class="kl"><em>growth</em><span>%s <span class="dm">15 min</span></span></div>'
            '<div class="kl"><em>rolled</em><span>%s</span></div></div>' % (
        cls, name, badge, d['es'], 'SPXW' if b == 'SPX' else b, d['strike'], gtxt(d['g']), rolled))

strip = '<div class="kstrip">' + king_cell('SPY', 'SPY KING', 'spy', True) + king_cell('SPX', 'SPX KING · flow', 'spx', False) + king_cell('QQQ', 'QQQ KING ≈', 'qqq', False) + '</div>'
zread = ('<div class="zread">'
         '<div class="zl"><em>ZONE</em><span>price <b>7756.0 is inside the King zone 7755.7 – 7759.7</b> — three layers, SPY · SPX · QQQ, 4.0 points, all +γ. The zone grew <b>−$48M</b> in 15 min: SPY K shrinking (−13%), SPX K stalled (0%), QQQ K growing (+5%).</span></div>'
         '<div class="zl"><em>LEAD</em><span>SPY K moved last — up 771 → 773 at 12:27 — the lead is <b>up</b>; SPX K rolled up from 7745 at 12:30 (+$20M); QQQ K unchanged since 11:12. <span class="dm">descriptive — K1.3 (which King leads) is READ NEXT; the deflection score (v15.63) replaces these words with a number and its n</span></span></div>'
         '<div class="zl"><em>NEXT</em><span>above the zone <b>7755 is NEW this bar</b> (+27%/15m, 7763.4); below it the pullback floor 7740 · 7735 (+26% · +33%/15m) held at 12:36.</span></div></div>')

R = {r[0]: r for r in rows}
lad = ('<div class="lad"><div class="grid lh"><span>level</span><span>price · strike</span><span>node · %king</span><span>new</span><span>⇄ roll</span><span>▲ growth 15m</span><span>setup</span></div>')
lad += node_row(R[7765]) + node_row(R[7760]) + node_row(R[7755])
lad += king_row('QQQ', 'KING ZONE')
lad += king_row('SPX', '<b>EMH 7743</b> ↑')
lad += ('<div class="grid lr now zone"><span class="lv zt">NOW</span><span class="px"><span class="nowpill">%.2f</span></span><span class="c mut">SPY %.2f · QQQ %.2f · SPXW %.2f</span>%s%s%s<span class="c mut">inside the King zone</span></div>' % (PX_ES, SPY_PX, QQQ_PX, snap['xm']['SPXW']['px'], cell(), cell(), cell()))
lad += king_row('SPY', '3 layers · 4.0 pts')
lad += node_row(R[7745]) + node_row(R[7740], lv='IB30H 7726') + node_row(R[7735]) + node_row(R[7730]) + node_row(R[7725]) + node_row(R[7720])
lad += ('<div class="leg"><span><b>NEW</b> crossed the 20% threshold within the last 30 bars — new money / hedging coming in — age in bars</span><span><b>⇄ ROLL</b> ▲ rolled up · ▼ rolled down — "from" on the node that received it, "to" on the node it left; any node, King or not</span>'
        '<span><b>▲ GROWTH</b> the node lighting up — its change over the window as a share of itself; the window (5 · 15 · 30 min) is under test</span><span><b>SETUP</b> pika stack · rug · reverse rug · air pocket · gatekeeper, when present</span>'
        '<span><b>KING ZONE</b> the three Kings at their ES prices, three layers of one zone</span></div></div>')

panel = ('<div class="pan"><div class="tabs"><span class="on">Dashboard</span><span>📊 Analysis</span><span>🧪 Testing</span><span>📚 Learn</span><span>⚙ Architecture</span><span>🗺 Roadmap</span><span>📌 Open Items</span></div>'
         '<div class="hd"><b>2026-09-03 · 12:48 CT</b> · ES 7756.00 · the book 12 minutes after the pullback bounce, 24 minutes before the high · <b>real numbers</b> from the day file</div>'
         + strip + lad +
         '<div class="foot"><span>● rec</span><span>● saved</span><span>● pushed</span><span style="margin-left:auto">mockup v3 · features 1 + 2 · simplified on his review</span></div></div>')

page = '\n'.join([
    '<title>The King zone and the node row</title>',
    '<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=IBM+Plex+Mono:wght@400;500;600&family=IBM+Plex+Sans:wght@400;500;600;700&display=swap">',
    '<style>' + CSS + '</style>', '<div class="wrap">',
    '<h1>The King zone and the node row — v3</h1>',
    '<p class="lede">Third draft, simplified on his review: no dollar amounts anywhere, the King cells down to growth and the roll with an ABOVE / BELOW price badge, the roll column with its own up / down arrow, a SETUP column for pika stack · rug · reverse rug in place of taps and zone, and the zone read parked. Drawn on the real book of 2026-09-03 at 12:48 CT: twelve minutes after the pullback bounce you circled, twenty-four minutes before the high printed at the fresh 7755.</p>',
    '<div class="flag"><span class="tag">His words</span><p><b>"all three kings are important and can cause deflections. sometimes one will disagree because it is leading and the others are lagging … we don\'t know which node it will deflect on."</b> So the Kings are not three candidates for one crown — they are three layers of one zone, and the zone is what the ladder brackets, the strip describes and the ledger records (which layer price touched first, whether it went through to the next).</p></div>',
    '<div class="ctl"><span>Scale</span><button type="button" data-s="1" aria-pressed="false">1×</button><button type="button" data-s="1.4" aria-pressed="true">1.4×</button><button type="button" data-s="2" aria-pressed="false">2×</button></div>',
    '<div class="stage"><div class="scaler" style="width:700px">' + panel + '</div></div>',
    '<p class="legend2"><b>What lines up now.</b> Eight fixed columns: level · price and strike · the node bar · NEW · ⇄ roll · ▲ growth · taps · zone/stack. A tell that is absent leaves its column empty rather than shifting the others; a row is read left to right in the same order every time.</p>',
    '<p class="legend2"><b>The King zone.</b> QQQ K ≈ 7759.7 (a bearing, dashed), SPX K 7758.4 (the flow book), SPY K 7755.7 — price at 7756.0 is inside it, on the second layer. The strip’s three cells carry the same six lines: the price, the size, the 15-minute growth, the distance from price, the last move, the taps today. LEAD marks the King that moved last (SPY, up at 12:27) — descriptive until K1.3 is read.</p>',
    '</div><script>(function(){ var b=[].slice.call(document.querySelectorAll(".ctl button")); b.forEach(function(x){ x.addEventListener("click",function(){ b.forEach(function(o){ o.setAttribute("aria-pressed", o===x?"true":"false"); }); document.documentElement.style.setProperty("--scale", x.getAttribute("data-s")); }); }); })();</script>'
])
io.open('design/mockup-king-strip.html', 'w', encoding='utf-8').write(page)
title = 'The King zone and the node row'
body = page.replace('<title>%s</title>\n' % title, '', 1)
io.open('design/mockup-king-strip-standalone.html', 'w', encoding='utf-8').write('<!doctype html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>%s</title></head><body>%s</body></html>' % (title, body))
print('wrote design/mockup-king-strip.html (+ standalone)')
