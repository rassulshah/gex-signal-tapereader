#!/usr/bin/env python3
"""Generate install.bat from the working tree.

WHY THIS EXISTS (2026-08-23). install.bat was hand-edited every build and drifted: the v11.86 installer
announced "GEX Tapereader installer - v11.49", named its temp files gex-v1149-payload, committed with the
message "v11.79 ...", and told the user the companion was "@version 1.8, unchanged" when it was 1.13.
Four stale strings from three different builds, in the one artefact the user actually runs. The PAYLOAD
was correct every time, which is exactly why nobody noticed.

Every version string here is READ FROM THE FILES. Nothing is typed twice.

  python3 tools/build-installer.py "v11.87: one-line commit message"

Payload rules (tools/install-template.md): base64 after `exit /b 0`, extracted with `more +<HDRLINES>`
then `certutil -f -decode` then `tar -xzf`. NO PowerShell anywhere - Avast flags it (IDP.HELU.PSE88).
HDRLINES is computed here, never guessed, and asserted against the emitted file before it is written.
"""
import base64, gzip, io, os, re, subprocess, sys, tarfile, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

def ver(path):
    with io.open(path, encoding='utf-8', errors='replace') as fh:
        for line in fh.read().split('\n')[:40]:
            m = re.search(r'@version\s+([0-9.]+)', line)
            if m:
                return m.group(1)
    raise SystemExit('no @version in ' + path)

SCRIPT = 'current/gex-signal-tapereader.user.js'
COMPAN = 'current/gex-if-levels.user.js'
V   = ver(SCRIPT)
VC  = ver(COMPAN)
TAG = 'v' + V.replace('.', '')          # v11.87 -> v1187, used for temp filenames
MSG = sys.argv[1] if len(sys.argv) > 1 else ('v' + V)

# --- did the companion actually change against origin/main? -------------------------------------
# Linking a script that did NOT change makes Tampermonkey offer "Reinstall" instead of "Update",
# which reads exactly like a failed push. So the installer must say which one moved.
def changed(path):
    try:
        r = subprocess.run(['git', 'diff', '--quiet', 'origin/main', '--', path])
        return r.returncode != 0
    except Exception:
        return True
COMPAN_NOTE = 'companion, @version %s' % VC
if not changed(COMPAN):
    COMPAN_NOTE += ', UNCHANGED this release'

# --- the payload ---------------------------------------------------------------------------------
FILES = []
for pat in ['current/gex-signal-tapereader.user.js', 'current/gex-if-levels.user.js',
            'changelog/CHANGELOG.md', '.gitignore', '.gex-config.json',
            'learning/rules.json', 'data/README.json', 'skills/gex/SKILL.md',
            'setup-gex-autopull.bat']:
    if os.path.exists(pat):
        FILES.append(pat)
for d in ['session-state', 'tools', 'mockups']:
    for f in sorted(os.listdir(d)):
        p = os.path.join(d, f)
        if os.path.isfile(p) and not f.endswith('.log'):
            FILES.append(p)
FILES += sorted(f for f in os.listdir('.') if f.startswith('test_') and f.endswith('.js'))

buf = io.BytesIO()
# mtime=0 so an unchanged tree produces an identical payload - a diffable installer
with gzip.GzipFile(fileobj=buf, mode='wb', mtime=0) as gz:
    with tarfile.open(fileobj=gz, mode='w') as tf:
        for p in FILES:
            ti = tf.gettarinfo(p); ti.mtime = 0; ti.uid = ti.gid = 0
            ti.uname = ti.gname = ''
            with open(p, 'rb') as fh:
                tf.addfile(ti, fh)
b64 = base64.b64encode(buf.getvalue()).decode('ascii')
B64_LINES = [b64[i:i+76] for i in range(0, len(b64), 76)]

HDR = """@echo off
setlocal EnableDelayedExpansion
set REPO=C:\\Dev\\gex-signal-tapereader
set SELF=%~f0

echo ============================================================
echo   GEX Tapereader installer - v{V}
echo ============================================================
echo.

if not exist "%REPO%\\.git" (
  echo ERROR: repo not found at %REPO%
  echo Clone it first: git clone https://github.com/rassulshah/gex-signal-tapereader.git "%REPO%"
  pause
  exit /b 1
)

echo Extracting payload...
set HDRLINES={HDRLINES}
more +%HDRLINES% "%SELF%" > "%TEMP%\\gex-{TAG}-payload.b64"
certutil -f -decode "%TEMP%\\gex-{TAG}-payload.b64" "%TEMP%\\gex-{TAG}-payload.tar.gz" >nul
if errorlevel 1 (
  echo ERROR: certutil decode failed. Payload is corrupt or certutil is unavailable.
  del "%TEMP%\\gex-{TAG}-payload.b64" >nul 2>&1
  pause
  exit /b 1
)

pushd "%REPO%"
tar -xzf "%TEMP%\\gex-{TAG}-payload.tar.gz"
if errorlevel 1 (
  echo ERROR: tar extract failed.
  popd
  del "%TEMP%\\gex-{TAG}-payload.b64" >nul 2>&1
  del "%TEMP%\\gex-{TAG}-payload.tar.gz" >nul 2>&1
  pause
  exit /b 1
)
popd

del "%TEMP%\\gex-{TAG}-payload.b64" >nul 2>&1
del "%TEMP%\\gex-{TAG}-payload.tar.gz" >nul 2>&1

echo.
echo Files installed into %REPO% ({NFILES} files):
echo   current\\gex-signal-tapereader.user.js   (@version {V})
echo   current\\gex-if-levels.user.js           ({COMPAN_NOTE})
echo   changelog\\CHANGELOG.md
echo   session-state\\  (resume note, DECISIONS, INSIDERFINANCE, PROJECT-CONSTANTS)
echo   learning\\rules.json
echo   the test_*.js suite and tools\\
echo.

set GIT=
where git >nul 2>&1 && set GIT=git
if not defined GIT if exist "C:\\Program Files\\Git\\cmd\\git.exe" set GIT=C:\\Program Files\\Git\\cmd\\git.exe
if not defined GIT if exist "C:\\Program Files (x86)\\Git\\cmd\\git.exe" set GIT=C:\\Program Files (x86)\\Git\\cmd\\git.exe
if not defined GIT if exist "%LOCALAPPDATA%\\Programs\\Git\\cmd\\git.exe" set GIT=%LOCALAPPDATA%\\Programs\\Git\\cmd\\git.exe
if not defined GIT (
  for /d %%D in ("%LOCALAPPDATA%\\GitHubDesktop\\app-*") do if exist "%%D\\resources\\app\\git\\cmd\\git.exe" set GIT=%%D\\resources\\app\\git\\cmd\\git.exe
)

if not defined GIT (
  echo ============================================================
  echo   GIT NOT FOUND
  echo ============================================================
  echo Files are installed at %REPO% but were NOT committed or pushed.
  echo Install Git for Windows, or open GitHub Desktop and commit/push
  echo this folder manually, then push origin.
  pause
  exit /b 0
)

pushd "%REPO%"
"!GIT!" add -A
"!GIT!" diff --cached --quiet
if errorlevel 1 (
  "!GIT!" commit -m "{MSG}"
  if errorlevel 1 (
    echo ============================================================
    echo   COMMIT FAILED
    echo ============================================================
    echo Files are installed but git commit failed. Check the output above,
    echo then commit and push manually from %REPO%.
    popd
    pause
    exit /b 1
  )
  "!GIT!" push
  if errorlevel 1 (
    echo ============================================================
    echo   PUSH FAILED
    echo ============================================================
    echo Committed locally but git push failed. Run "git push" manually
    echo from %REPO%, or push via GitHub Desktop.
    popd
    pause
    exit /b 1
  )
  echo ============================================================
  echo   PUSHED
  echo ============================================================
  echo Committed and pushed v{V} to GitHub.
) else (
  echo ============================================================
  echo   NOTHING TO COMMIT
  echo ============================================================
  echo Working tree already matches this release. Nothing new was pushed.
)
popd

echo.
echo Done.
echo IMPORTANT: raw.githubusercontent.com caches responses for FIVE minutes.
echo Wait five minutes, then reload the Tampermonkey tab in your browser and
echo check for "Update" (not "Reinstall").
pause
exit /b 0
"""

def render(hdrlines):
    return HDR.format(V=V, TAG=TAG, HDRLINES=hdrlines, MSG=MSG,
                      COMPAN_NOTE=COMPAN_NOTE, NFILES=len(FILES))

# HDRLINES is the number of header lines `more +N` must skip. Solve it: the value appears INSIDE the
# header, so a naive count can be off by the digits it adds. Iterate to a fixed point, then ASSERT.
n = render(0).count('\n')
for _ in range(6):
    n2 = render(n).count('\n')
    if n2 == n:
        break
    n = n2
header = render(n)
assert header.count('\n') == n, 'HDRLINES did not converge'

out = header.replace('\n', '\r\n') + '\r\n'.join(B64_LINES) + '\r\n'
with open('install.bat', 'wb') as fh:
    fh.write(out.encode('ascii'))

# --- ROUND-TRIP THE FILE WE JUST WROTE, the way the .bat will ------------------------------------
lines = open('install.bat', 'rb').read().split(b'\r\n')
payload = b''.join(l.strip() for l in lines[n:])
tf = tarfile.open(fileobj=io.BytesIO(gzip.decompress(base64.b64decode(payload))))
names = tf.getnames()
bad = []
for nm in names:
    m = tf.extractfile(nm)
    if m is None:
        continue
    if m.read() != open(nm, 'rb').read():
        bad.append(nm)
assert not bad, 'payload differs from working tree: ' + ', '.join(bad)
assert len(names) == len(FILES), 'file count mismatch %d vs %d' % (len(names), len(FILES))

txt = open('install.bat', 'r', encoding='ascii').read()
assert 'powershell' not in txt.lower(), 'PowerShell in installer - Avast flags IDP.HELU.PSE88'
assert txt.count('v' + V) >= 2, 'version banner missing'
assert not re.search(r'v11\.(?!' + re.escape(V.split('.', 1)[1]) + r')\d+', txt.split('exit /b 0')[0]), \
    'a STALE version string survived in the header'

print('install.bat  %d bytes  HDRLINES=%d  %d files  script v%s  companion v%s'
      % (os.path.getsize('install.bat'), n, len(names), V, VC))
print('round-trip: every file byte-identical to the working tree')
