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
_MSG_RAW = sys.argv[1] if len(sys.argv) > 1 else ('v' + V)
# A .bat is written as ASCII (certutil + CRLF). An em dash in a commit message used to blow up at the
# final write with a UnicodeEncodeError 200 lines after the mistake was made. Fold to ASCII here, where
# the message is read, and say what was changed rather than failing or silently mangling it.
_SUBS = {'\u2014': ' - ', '\u2013': '-', '\u2018': "'", '\u2019': "'",
         '\u201c': '"', '\u201d': '"', '\u2026': '...', '\u00a0': ' ',
         '\u2212': '-', '\u26a0': '(!)', '\u2713': 'v', '\u2717': 'x'}
for _k, _v in _SUBS.items():
    _MSG_RAW = _MSG_RAW.replace(_k, _v)
MSG = _MSG_RAW.encode('ascii', 'replace').decode('ascii')
if MSG != _MSG_RAW:
    print('note: commit message folded to ASCII')
MSG = MSG.replace('"', "'")          # the message sits inside "..." in the .bat

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
# (v14.3) session-state ships the LIVING documents plus ONLY the newest dated snapshot — 39 stale
# v10/v11-era snapshots were re-shipped in every payload (~1.5MB) purely because nobody had ever
# deleted them, and they tripped the size guard. They stay in git history; they stop riding along.
_SNAP=re.compile(r'^\d{4}-\d{2}-\d{2}_resume-')
_snaps=sorted(f for f in os.listdir('session-state') if _SNAP.match(f))
_keep_snap=_snaps[-1] if _snaps else None
for f in sorted(os.listdir('session-state')):
    p=os.path.join('session-state', f)
    if not os.path.isfile(p) or f.endswith('.log'): continue
    if _SNAP.match(f) and f!=_keep_snap: continue
    FILES.append(p)
for f in sorted(os.listdir('tools')):
    p=os.path.join('tools', f)
    # (v13.9) NO .bat, NO .log — an installer must never contain installers.
    if os.path.isfile(p) and not f.endswith('.log') and not f.lower().endswith('.bat'):
        FILES.append(p)
# (v14.3) mockups are DESIGN documents: html/md only. A 0.64MB day-data .json and old .patch files
# had drifted in and were shipping with every build.
for f in sorted(os.listdir('mockups')):
    p=os.path.join('mockups', f)
    if os.path.isfile(p) and (f.endswith('.html') or f.endswith('.md')):
        FILES.append(p)
FILES += sorted(f for f in os.listdir('.') if f.startswith('test_') and f.endswith('.js'))

# --- size guard: fail LOUDLY before shipping a payload cmd.exe cannot digest --------------------
# `more +n` walks the whole file line by line; past a few MB of base64 it is minutes, not seconds,
# and the user reads that as a hang. 6MB of payload ≈ 80K lines ≈ the v13.x sizes that worked.
_PAYLOAD_CAP = 6 * 1024 * 1024
_total = sum(os.path.getsize(p) for p in FILES)
if _total > _PAYLOAD_CAP:
    _big = sorted(((os.path.getsize(p), p) for p in FILES), reverse=True)[:10]
    print('PAYLOAD TOO BIG: %.1f MB raw against a %.0f MB cap. Largest members:' % (_total/1e6, _PAYLOAD_CAP/1e6))
    for _s, _p in _big:
        print('  %8.2f MB  %s' % (_s/1e6, _p))
    raise SystemExit('refusing to build an installer that will hang on extraction — trim the manifest')

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
rem (v13.9) purge the old downloaded installers that the v13.8 push swept into mockups/ -
rem ~28MB of dead weight that ballooned this installer to 30MB and hung its own extraction.
rem They remain recoverable from git history; this removes them from the tree and the push
rem records the deletion so the repo shrinks back.
del /q "%REPO%\\mockups\\install*.bat" >nul 2>&1
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

# ---- ALSO EMIT A VERSIONED COPY, and deliver THAT ------------------------------------------------
# User instruction 2026-08-24: every installer handed over must say its version IN THE FILENAME.
# Reason it exists: eight installers were delivered in one session all named `install.bat`, and the
# user ran an older one and reported bugs that had already been fixed three builds earlier. A file
# whose name cannot be told apart from seven others is a file that gets run out of order.
# `install.bat` stays as the canonical repo path (the scheduled task and the docs reference it);
# ⚠⚠ THE DELIVERY IS **ONE FILE**. OPERATOR-MANDATED 2026-08-15, RESTATED 2026-08-27:
#   "you are supposed to just give me an install file."
# He downloads ONE thing and double-clicks it. Not a zip plus an applier, not a pair, not "primary
# and fallback" - ONE. On 2026-08-27 this banner named the zip+applier pair as primary, and a
# context followed a two-file banner over his standing rule and sent him three attachments. It was
# wrong, not the rule.
# The self-extracting `.bat` IS the deliverable. The zip+applier pair below still gets BUILT, because
# it is the proven fallback for the day the self-extractor fails on his machine again — but it is
# never what gets sent unless he is told the .bat failed and asks for it.
# ⚠ THE NAME CARRIES NO DASHES AND NO DOTS except the extension: downloads strip both, and eight
# identically-named installers in one session got run out of order.
# `install-v<VER>.bat` is the copy that gets sent. It is gitignored so it never enters the payload.
import shutil
_versioned = 'install-v%s.bat' % V
shutil.copyfile('install.bat', _versioned)
# THE FILE THAT ACTUALLY GETS SENT — dash-free, dot-free, unmistakable in a Downloads folder.
_DELIVER = 'installv%s.bat' % V.replace('.', '')
shutil.copyfile('install.bat', _DELIVER)

# ==== (v14.3) THE ZIP + APPLIER PAIR — THE PRIMARY DELIVERY ======================================
# 2026-08-25: the self-extracting installer failed on the user's machine THREE ways in one day —
# a 30MB payload hung `more`, then extraction failed for an unreported reason (certutil is a known
# Avast target), and the download STRIPPED DASHES from filenames so an exact-name check missed.
# What worked, first try, was: a plain zip + a tiny CRLF .bat that extracts it with Windows' own
# tar (bsdtar reads zip natively) — no certutil, no base64, no self-parsing. So the builder now
# emits that pair and IT is what gets delivered. Rules learned the hard way, encoded here:
#   - filenames carry NO dashes and NO dots except the extension (downloads strip dashes)
#   - the .bat is CRLF (a hand-made LF applier closed instantly on double-click)
#   - the applier finds the zip by WILDCARD in three places (beside itself, mockups, Downloads)
#   - it deletes the zip and unstages helper files before committing, so they cannot enter history
VD = V.replace('.', '')                    # 14.3 -> 143: version digits for dash/dot-free names
ZIPNAME = 'gexdrop%s.zip' % VD
BATNAME = 'applygex%s.bat' % VD
import zipfile as _zf
with _zf.ZipFile(ZIPNAME, 'w', _zf.ZIP_DEFLATED) as _z:
    for _p in FILES:
        _z.write(_p, _p)
# round-trip the zip too
with _zf.ZipFile(ZIPNAME) as _z:
    _zbad = [nm for nm in _z.namelist() if _z.read(nm) != open(nm, 'rb').read()]
    assert not _zbad, 'zip differs from working tree: ' + ', '.join(_zbad)
    assert len(_z.namelist()) == len(FILES), 'zip file count mismatch'
_MSGBAT = MSG.replace('%', '%%')
_APPLY = """@echo off
setlocal EnableDelayedExpansion
REM ============================================================
REM   GEX v{V} APPLIER - put {ZIP} anywhere; this finds it.
REM   Plain tools only - Windows tar reads zip natively. No certutil.
REM ============================================================
set REPO=C:\\Dev\\gex-signal-tapereader
if not exist "%REPO%\\.git" (
  echo ERROR: repo not found at %REPO%
  pause
  exit /b 1
)
set ZIP=
for %%Z in ("%~dp0gexdrop*.zip") do if not defined ZIP set ZIP=%%~fZ
if not defined ZIP for %%Z in ("%REPO%\\mockups\\gexdrop*.zip") do if not defined ZIP set ZIP=%%~fZ
if not defined ZIP for %%Z in ("%USERPROFILE%\\Downloads\\gexdrop*.zip") do if not defined ZIP set ZIP=%%~fZ
if not defined ZIP (
  echo ERROR: no gexdrop*.zip found beside this script, in mockups, or in Downloads.
  pause
  exit /b 1
)
echo Using %ZIP%
echo Extracting...
tar -xf "%ZIP%" -C "%REPO%"
if errorlevel 1 (
  echo ERROR: tar extract failed. Report exactly this line.
  pause
  exit /b 1
)
del /q "%ZIP%" >nul 2>&1
del /q "%REPO%\\mockups\\install*.bat" "%REPO%\\mockups\\apply*.bat" "%REPO%\\mockups\\gex*drop*.zip" >nul 2>&1
set GIT=
where git >nul 2>&1 && set GIT=git
if not defined GIT if exist "C:\\Program Files\\Git\\cmd\\git.exe" set GIT=C:\\Program Files\\Git\\cmd\\git.exe
if not defined GIT if exist "C:\\Program Files (x86)\\Git\\cmd\\git.exe" set GIT=C:\\Program Files (x86)\\Git\\cmd\\git.exe
if not defined GIT if exist "%LOCALAPPDATA%\\Programs\\Git\\cmd\\git.exe" set GIT=%LOCALAPPDATA%\\Programs\\Git\\cmd\\git.exe
if not defined GIT (
  for /d %%D in ("%LOCALAPPDATA%\\GitHubDesktop\\app-*") do if exist "%%D\\resources\\app\\git\\cmd\\git.exe" set GIT=%%D\\resources\\app\\git\\cmd\\git.exe
)
if not defined GIT (
  echo Files are installed at %REPO% but git was not found - push manually.
  pause
  exit /b 0
)
pushd "%REPO%"
"!GIT!" add -A
"!GIT!" reset -q -- "mockups/*.zip" "mockups/apply*" "gexdrop*.zip" "applygex*.bat" "pushgex*.bat" >nul 2>&1
"!GIT!" diff --cached --quiet
if errorlevel 1 (
  "!GIT!" commit -m "{MSG}"
  "!GIT!" push
  if errorlevel 1 (
    echo Committed locally but git push failed - push manually from %REPO%.
    popd
    pause
    exit /b 1
  )
  echo.
  echo ============================================================
  echo   PUSHED v{V}. Wait 5 minutes, then update Tampermonkey:
  echo.
  echo   https://raw.githubusercontent.com/rassulshah/gex-signal-tapereader/main/current/gex-signal-tapereader.user.js
  echo.
  echo   (companion UNCHANGED unless this says otherwise: {COMPAN_NOTE})
  echo   Then RELOAD the Atlas tab. Footer must say v{V}.
  echo ============================================================
) else (
  echo Nothing to commit - repo already at v{V}.
)
popd
echo.
pause
""".replace('{V}', V).replace('{ZIP}', ZIPNAME).replace('{MSG}', _MSGBAT).replace('{COMPAN_NOTE}', COMPAN_NOTE)
with open(BATNAME, 'wb') as _fh:
    _fh.write(_APPLY.replace('\r\n', '\n').replace('\n', '\r\n').encode('ascii'))
_atxt = open(BATNAME, 'r', encoding='ascii').read()
assert 'powershell' not in _atxt.lower(), 'PowerShell in applier'
assert '\r\n' in open(BATNAME, 'rb').read().decode('ascii'), 'applier is not CRLF'
assert '-' not in ZIPNAME.replace('.zip','') and '-' not in BATNAME.replace('.bat',''), 'dashes in delivery names'

print('install.bat  %d bytes  HDRLINES=%d  %d files  script v%s  companion v%s'
      % (os.path.getsize('install.bat'), n, len(names), V, VC))
print('')
print('==== DELIVER EXACTLY ONE FILE ====')
print('   %s      <- send THIS, and nothing else' % _DELIVER)
print('   (fallback only, if he reports the .bat failed: %s + %s)' % (ZIPNAME, BATNAME))
print('  (install-v%s.bat still exists as the self-extracting fallback)' % V)
print('round-trip: tar payload AND zip both byte-identical to the working tree')
# (v14.3, user-directed: "you must give me the tampermonkey link every time you give me an install
# file") — the block prints HERE, last, so the delivery message is written with it in view, and the
# applier itself now echoes the URL on success. Two places; forgetting requires ignoring both.
print('')
print('==== PASTE THIS WITH THE INSTALL FILE — EVERY TIME ====')
print('**Tampermonkey — update ONLY what changed:**')
print('- **Tapereader v%s** (changed) — https://raw.githubusercontent.com/rassulshah/gex-signal-tapereader/main/current/gex-signal-tapereader.user.js' % V)
print('- Companion v%s%s' % (VC, ' — UNCHANGED, do not reinstall' if 'UNCHANGED' in COMPAN_NOTE else ' — CHANGED, update it too'))
print('Then wait ~5 min (raw CDN cache) and RELOAD the Atlas tab — footer must say v%s.' % V)
