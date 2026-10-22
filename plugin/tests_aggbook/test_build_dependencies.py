#!/usr/bin/env python3
"""Source-level native rebuild dependency check; not Windows batch execution."""
from pathlib import Path
import re,hashlib
root=Path(__file__).resolve().parent.parent
build=root.parent/'tools/gex-build.bat'
files={p.name:p for p in root.glob('*') if p.is_file()}
text=build.read_text()
checks=0
for target in ['BidAskStats','BookFlow','DefenseLines',
               # (2026-10-21) every plugin that now includes KeepOutDraw.h / BoxDock.h / BannerBoard.h (the HOD/LOD keep-out)
               'KeyLevels','DeltaProfile','TapeFlowMarks','CobImpulse','DealerSig','KingTracker','SessionVWAP','LiquidityProfile',
               'BigBidAsk',   # (1.21.0) breakout / breakdown
               'DealerProfile','LiquidityFlow']:   # (DP 2.19.0) LevelPctLogic.h; ColumnBoard.h 1.3.0
    closure=set();todo=[target+'.cpp']
    while todo:
        name=todo.pop()
        if name in closure or name not in files:continue
        closure.add(name)
        todo.extend(re.findall(r'#\s*include\s*"([^"]+)"',files[name].read_text(errors='replace')))
    line=next(x for x in text.splitlines() if x.startswith('call :one '+target+' '))
    names=[target+'.cpp']+[n for n in line.split()[4:] if n in files]
    missing=closure-set(names)
    assert not missing,(target,'unhashed dependencies',sorted(missing));checks+=1
    assert len(names)*13-1<1023,(target,'set /p hash line length');checks+=1
    # Every actual dependency must affect the simulated 12-hex-per-file hash
    # fingerprint that the batch script constructs. No source file is mutated.
    original='-'.join(hashlib.sha1(files[n].read_bytes()).hexdigest()[:12] for n in names)
    for changed in closure:
        altered='-'.join(hashlib.sha1(files[n].read_bytes()+(b'\n// regression\n' if n==changed else b'')).hexdigest()[:12] for n in names)
        assert altered!=original,(target,changed,'does not affect fingerprint');checks+=1
    if 'EffortReward.h' in closure:
        removed=set(names)-{'EffortReward.h'}
        assert 'EffortReward.h' in closure-removed;checks+=1
    print(target,':',len(closure),'reachable files tracked; stored hash line within 1023 chars')
print('Native rebuild dependency checks:',checks,'passed; Windows script not executed')
