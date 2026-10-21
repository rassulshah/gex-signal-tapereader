#!/usr/bin/env python3
"""(2026-10-21, SettingsGuard 1.1.0) Every plugin with a settings window goes through SS_SAFE_DIALOG - no exceptions, no new plugin
without it. The 1.0.0 guard was put into 3 of 30 plugins and lsSessionVWAP still faulted on a chart-less window copy (msglog
2026-10-21 09:51). Fails when a plugin source (or a header with a cppExtension class) defines parmsLoad / parmsApply / parmsUpdt
itself instead of parms*Body, lacks the macro, or lacks the SettingsGuard.h include; and when a plugin built by gex-build.bat
does not hash SettingsGuard.h."""
import re, sys
from pathlib import Path
root = Path(__file__).resolve().parent.parent
bat = (root.parent / 'tools' / 'gex-build.bat').read_text(errors='replace')
bad, checked = [], 0
srcs = [p for p in sorted(root.glob('*.cpp')) + sorted(root.glob('*.h'))
        if not p.name.startswith('test_') and 'sentinel' not in p.name and 'regression' not in p.name and p.name not in ('SettingsGuard.h',)]
for p in srcs:
    s = p.read_text(errors='replace')
    code = re.sub(r'//[^\n]*', '', s)
    if not re.search(r'class\s+\w+[^{;]*:\s*public\s+(cppExtension|off::MboPane<[^>]*>)', code):
        continue
    raw = re.findall(r'\b(?:virtual\s+)?int\s+(?:\w+::)?parms(?:Load|Apply|Updt)\s*\(', code)
    body = re.search(r'parms(?:Load|Apply|Updt)Body\s*\(', code)
    if not raw and not body:
        continue
    checked += 1
    if raw:
        bad.append(f'{p.name}: defines parmsLoad / parmsApply / parmsUpdt itself - name it parms*Body and add SS_SAFE_DIALOG(Class, "Name")')
    if 'SS_SAFE_DIALOG(' not in code:
        bad.append(f'{p.name}: no SS_SAFE_DIALOG in its class')
    if '#include "SettingsGuard.h"' not in s:
        bad.append(f'{p.name}: does not include SettingsGuard.h')
    if p.suffix == '.cpp':
        for line in bat.splitlines():
            parts = line.split()
            if line.startswith('call :one ') and len(parts) > 2 and (parts[2] + '.cpp' == p.name or p.name in parts[4:]) and 'SettingsGuard.h' not in parts[4:]:
                bad.append(f'gex-build.bat: {parts[2]} builds {p.name} but does not hash SettingsGuard.h')
for b in bad:
    print('FAIL', b)
print(f'dialog guard contract: {checked} plugins with a settings window, {len(bad)} problems')
sys.exit(1 if bad or checked < 25 else 0)
