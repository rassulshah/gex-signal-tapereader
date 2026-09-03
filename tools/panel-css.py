#!/usr/bin/env python3
"""
THE MOCKUP'S CSS IS THE PANEL'S CSS — one source, scoped.  (v15.62)

    "for the analysis and testing tabs, use the look and feel that you gave me with the mockups."
                                                                        - operator, 2026-09-03 (third time)

Twice the tabs were rebuilt "as mocked" by re-typing the mockup's look into inline styles, and twice they drifted:
a header the mockup never had, a section header in another shape, smaller type. The fix is not a third re-typing.
The panel's Analysis / Testing / Learn tabs now render with the mockup generators' OWN stylesheet — the `.pan …`
rules of tools/mockup-from-studies.py and the EXTRA of tools/mockup-testing.py — scoped under `#gpts-body .g3pan`,
and `PANEL_CSS` in the userscript is pinned equal to this script's output by test_v1562.js. Change the mockup and
the suite fails until the panel follows; change the panel's copy and it fails until the mockup follows.

Run:  python3 tools/panel-css.py            -> prints the scoped CSS (one line)
      python3 tools/panel-css.py --splice   -> writes it into current/gex-signal-tapereader.user.js (PANEL_CSS)
"""
import io, re, sys, os
from importlib import import_module

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.getcwd())

# the rules that ARE the panel (everything else in the mockup CSS is the page around it)
PANEL_SELECTORS = ('.pan', '.tabs', '.hd', '.subj', '.sec', '.sech', '.secb', '.note', '.sc', '.rs',
                   '.gr', '.rd', '.am', '.ye', '.pu', '.bl', '.bl2', '.te', '.dm', '.mu', '.lil',
                   'table', 'th', 'td', '.foot', '.flow', '.bar', 'tr.draft', '.row', '.k', '.v')
SCOPE = '#gpts-body .g3pan'


def rules(css):
    """(selector, declarations) pairs, in order; @-blocks skipped (the page's media rules are not the panel's)."""
    out = []
    depth, buf = 0, ''
    i = 0
    text = css
    while i < len(text):
        ch = text[i]
        if ch == '@':                       # skip an @-rule block whole
            j = text.find('{', i)
            k, d = j, 0
            while k < len(text):
                if text[k] == '{': d += 1
                elif text[k] == '}':
                    d -= 1
                    if d == 0: break
                k += 1
            i = k + 1
            continue
        j = text.find('{', i)
        if j < 0: break
        k = text.find('}', j)
        sel = text[i:j].strip()
        dec = text[j + 1:k].strip()
        if sel: out.append((sel, dec))
        i = k + 1
    return out


def scoped(sel):
    parts = [s.strip() for s in sel.split(',')]
    res = []
    for s in parts:
        if s == '.pan':
            res.append(SCOPE)
        elif s.startswith('.pan '):
            res.append(SCOPE + s[4:])
        else:
            res.append(SCOPE + ' ' + s)
    return ','.join(res)


def keep(sel):
    first = sel.split(',')[0].strip()
    return any(first == p or first.startswith(p + ' ') or first.startswith(p + '.') or first.startswith(p + ':')
               for p in PANEL_SELECTORS)


def build():
    M = import_module('mockup-from-studies')
    T = import_module('mockup-testing')
    css = M.CSS
    extra = getattr(T, 'EXTRA', '')
    # the --g-* variables from :root become the panel's own
    root = re.search(r':root\{(.*?)\}', css, re.S).group(1)
    gvars = ';'.join(v.strip() for v in root.split(';') if v.strip().startswith('--g-'))
    out = [SCOPE + '{' + gvars + '}']
    for sel, dec in rules(css) + rules(extra):
        if not keep(sel):
            continue
        dec = re.sub(r'\s*\n\s*', '', dec)
        out.append(scoped(sel) + '{' + dec + '}')
    # the panel fills its host; the tab BODY is scaled by the user's choice (the mockup page's 1× / 1.55× / 2.1×)
    out.append(SCOPE + '{width:auto}' + SCOPE + '.g3scaled{zoom:var(--g-scale,1)}')
    # the app's own tab bar and the guide rows sit in the mockup's chrome too
    out.append(SCOPE + ' .tabs span{cursor:pointer}' + SCOPE + ' .sech[data-gsec]{cursor:pointer}' + SCOPE + ' .qq,' + SCOPE + ' .v,' + SCOPE + ' .note,' + SCOPE + ' .rs{white-space:normal}')
    return ''.join(out)


if __name__ == '__main__':
    css = build()
    if '--splice' in sys.argv:
        p = 'current/gex-signal-tapereader.user.js'
        s = io.open(p, encoding='utf-8').read()
        i = s.index('var PANEL_CSS=')
        j = s.index('\n', i)
        # a JS single-quoted literal: escape backslashes and single quotes
        lit = "'" + css.replace('\\', '\\\\').replace("'", "\\'") + "'"
        s = s[:i] + 'var PANEL_CSS=' + lit + ';' + s[j:]
        io.open(p, 'w', encoding='utf-8').write(s)
        print('spliced PANEL_CSS ·', len(css), 'chars')
    else:
        print(css)
