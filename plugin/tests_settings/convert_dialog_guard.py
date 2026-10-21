"""(2026-10-21, SettingsGuard 1.1.0) Put SS_SAFE_DIALOG into a plugin: the plugin's own parmsLoad / parmsApply / parmsUpdt become
parmsLoadBody / parmsApplyBody / parmsUpdtBody (same code), the class gets the macro (guarded window calls + safe symbol reads), and
SettingsGuard.h is included after irtsdk.h. A missing Load / Apply keeps the SDK default (RTX_FAIL), a missing Updt RTX_OK.
Usage: python3 convert_dialog_guard.py <Plugin.cpp> [<Class>]   (idempotent)"""
import re, sys

def convert(path, cls=None):
    raw = open(path, 'rb').read()
    crlf = b'\r\n' in raw
    s = raw.decode('utf-8', errors='surrogateescape').replace('\r\n', '\n')
    if 'SS_SAFE_DIALOG(' in s:
        return 'already'
    if cls is None:
        m = re.search(r'class\s+(\w+)\s*(?:final\s*)?:\s*public\s+cppExtension\b', s)
        if not m:
            return 'no cppExtension class'
        cls = m.group(1)
    has = {k: bool(re.search(r'\b' + k + r'\s*\(', s)) for k in ('parmsLoad', 'parmsApply', 'parmsUpdt')}
    if not any(has.values()):
        return 'no window calls'
    for k in has:
        s = re.sub(r'\b' + k + r'(\s*\()', k + r'Body\1', s)
    # an override that no longer overrides
    s = re.sub(r'(parms(?:Load|Apply|Updt)Body\s*\([^)]*\))\s*override\b', r'\1', s)
    m = re.search(r'class\s+' + cls + r'\s*(?:final\s*)?:\s*public\s+cppExtension\s*\{', s)
    if not m:
        return 'class not found'
    pub = s.find('public:', m.end())
    if pub < 0:
        return 'no public:'
    add = '\n    SS_SAFE_DIALOG(' + cls + ', "' + cls + '")   // (SettingsGuard 1.1.0) every window call guarded + logged; a chart-less copy opens filled'
    if not has['parmsLoad']:
        add += '\n    int parmsLoadBody(void) { return RTX_FAIL; }      // the SDK default'
    if not has['parmsApply']:
        add += '\n    int parmsApplyBody(void) { return RTX_FAIL; }     // the SDK default'
    if not has['parmsUpdt']:
        add += '\n    int parmsUpdtBody(unsigned int) { return RTX_OK; }   // the SDK default'
    at = pub + len('public:')
    s = s[:at] + add + s[at:]
    if '#include "SettingsGuard.h"' not in s:
        inc = re.search(r'#include\s+"irtsdk\.h"[^\n]*\n', s)
        if not inc:
            return 'no irtsdk.h include'
        s = s[:inc.end()] + '#include "SettingsGuard.h"   // (2026-10-21) SS_SAFE_DIALOG: the settings window never faults, never opens blank\n' + s[inc.end():]
    out = s.replace('\n', '\r\n') if crlf else s
    open(path, 'wb').write(out.encode('utf-8', errors='surrogateescape'))
    return 'converted ' + cls

if __name__ == '__main__':
    print(sys.argv[1], convert(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else None))
