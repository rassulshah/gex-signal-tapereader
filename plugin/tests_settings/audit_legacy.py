#!/usr/bin/env python3
"""audit_legacy.py - static settings audit of IRT indicators that are NOT yet on the SettingsSpec.h layer (2026-10-18).
usage: python audit_legacy.py <plugin folder> [Name.cpp ...]
Per indicator: the window rows (from set*Parameter calls in setup), each line's estimated width vs the 400 px budget (same rule as
SettingsSpec.h), colour rows filled / read with the INTEGER API (black swatch), positional parameter indices, '?' / range text in
labels, missing parmsUpdt, dashed drawing. Prints a report; exit 0 always (the gate is settings_contract)."""
import os, re, sys

BUDGET = 400
CALL = re.compile(r'set(Bool|Integer|Float|Color|List|Label|String)Parameter\s*\(\s*"([^"]*)"([^;]*)\);')

def label_px(s): return len(s) * 6 + 8
def ctl_px(kind, label, width):
    if kind == 'Bool': return 18 + label_px(label)
    if kind == 'Label': return min(width or 400, len(label) * 6 + 8)
    dflt = {'Color': 40, 'List': 110, 'String': 160}.get(kind, 60)
    return label_px(label) + (width if width else dflt)

def audit(path):
    src = open(path, encoding='latin-1').read()
    name = os.path.basename(path)
    out = []
    rows = []
    for m in CALL.finditer(src):
        kind, label, rest = m.group(1), m.group(2), m.group(3)
        line_start = src.rfind('\n', 0, m.start()) + 1
        if src[line_start:m.start()].lstrip().startswith('//'): continue
        same = 'kParmAppendSameLine' in rest or re.search(r'\bSL\b|\bSAME\b', rest) is not None
        nums = [int(x) for x in re.findall(r'(?<![\w.])(\d+)(?![\w.])', rest)]
        width = 0
        if kind in ('Integer', 'Float', 'Color', 'Label'):
            # setXParameter(name, default, width, flags): the 2nd number after the default
            parts = [p.strip() for p in rest.split(',')]
            if len(parts) >= 3 and parts[2].isdigit(): width = int(parts[2])
        elif kind in ('List',):
            parts = [p.strip() for p in rest.split(',')]
            if len(parts) >= 4 and parts[3].isdigit(): width = int(parts[3])
        elif kind == 'String':
            parts = [p.strip() for p in rest.split(',')]
            if len(parts) >= 4 and parts[3].isdigit(): width = int(parts[3])
        rows.append((kind, label, same, width))
    lines = []
    for r in rows:
        if not r[2] or not lines: lines.append([])
        lines[-1].append(r)
    out.append(f'== {name}: {len(rows)} rows on {len(lines)} lines')
    for L in lines:
        px = sum(ctl_px(k, l, w) for k, l, s, w in L) + 10 * (len(L) - 1)
        flag = '  CUT OFF' if px > BUDGET else ''
        out.append(f'   ~{px:3d}px  ' + ' | '.join(f'{k}:{l}' for k, l, s, w in L) + flag)
    issues = []
    for k, l, s, w in rows:
        if '?' in l: issues.append(f"label '{l}' contains '?'")
        if re.search(r'\(\s*\d+\s*-\s*\d+', l): issues.append(f"label '{l}' carries its range (check it matches the code)")
    # colour rows touched with the integer API
    col_idx = set(re.findall(r'(\w+)\s*=\s*pc\+\+;\s*setColorParameter', src)) | set(re.findall(r'(\w+)\]\s*=\s*getParameterCount\(\);\s*setColorParameter', src))
    for m in re.finditer(r'(setIntegerValue|getIntegerValue)\s*\(([^,)]*)', src):
        arg = m.group(2).strip()
        if re.search(r'COL|Col|col|Bg\b|TableBg|ProjUp|ProjDn|MarkLod|MarkHod|cp\[', arg) and 'FONT' not in arg:
            issues.append(f"{m.group(1)}({arg}) on what looks like a COLOUR row - use setParameterColor / getParameterColor")
    if re.search(r'getIntegerValue\(\s*\d+\s*\)|setIntegerValue\(\s*\d+\s*,|getListIndex\(\s*\d+\s*\)|isBoxChecked\(\s*\d+\s*\)|checkBox\(\s*\d+\s*,', src):
        issues.append('controls addressed by literal position (0, 1, 2 ...) - adding a row shifts every later one')
    if re.search(r'static const int \w+_INDEX\s*=\s*\d+', src) or re.search(r'_PARM\s*=\s*\d+', src):
        issues.append('controls addressed by hard-coded index constants')
    if rows and 'parmsUpdt' not in src: issues.append('no parmsUpdt (group masters cannot work)')
    if 'dashRect' in src or 'P_DASH' in src: issues.append('draws DASHED lines (his rule: solid only) - check which setting controls it')
    if 'ss::registerAll(' in src: issues = ['ON THE LAYER (settings_contract checks it)']
    for i in sorted(set(issues)): out.append('   - ' + i)
    return '\n'.join(out)

if __name__ == '__main__':
    d = sys.argv[1] if len(sys.argv) > 1 else '.'
    names = sys.argv[2:] or sorted(f for f in os.listdir(d) if f.endswith('.cpp') and not f.startswith(('test_', 'fuzz_', 'first_')))
    for n in names:
        p = os.path.join(d, n)
        if os.path.exists(p): print(audit(p))
