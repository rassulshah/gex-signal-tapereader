"""Static source contract for ReaderState (0.6.4: one state per fixed-market DLL, timer-safe)."""
from pathlib import Path
import re

source = (Path(__file__).resolve().parent / "IRTReader.cpp").read_text()
checks = {
    'ReaderState is defined': 'struct ReaderState {' in source,
    'state is owned by the DLL object, not a host slot': 'ReaderState* own_ = nullptr;' in source and 'setUserData' not in source,
    'new state is deleted explicitly': 'delete p;\n    if (own_ == p) own_ = nullptr;' in source,
    'freeUserData is not called for new state': re.search(r'\bfreeUserData\s*\(', source) is None,
    'done releases current host state': 'int cppExtension::done(void)\n{\n    // The SDK calls done()' in source and 'static_cast<IRTReader*>(this)->releaseState();' in source,
    'destroy releases current host state': 'int cppExtension::destroy(void)\n{\n    static_cast<IRTReader*>(this)->releaseState();' in source,
    'timer IDs are state-specific': 'createTimer(timerIdFor(&state()), 1000)' in source and 'timerIdFor(p)' in source,
    'global cross-host other-market notice removed': 'static bool saidOthers' not in source and 'state().saidOthers' in source,
}
failed = []
for name, result in checks.items():
    print(('ok   ' if result else 'FAIL ') + name)
    if not result:
        failed.append(name)
print(f'\n{len(checks) - len(failed)} passed, {len(failed)} failed')
raise SystemExit(bool(failed))
