#!/usr/bin/env python3
"""Generate install.bat from the working tree.

WHY THIS EXISTS (2026-08-23). install.bat was hand-edited every build and drifted: the v11.86 installer
announced "GEX Tapereader installer - v11.49", named its temp files gex-v1149-payload, committed with the
message "v11.79 ...", and told the user the companion was "@version 1.8, unchanged" when it was 1.13.
Four stale strings from three different builds, in the one artefact the user actually runs. The PAYLOAD
was correct every time, which is exactly why nobody noticed.

Every version string here is READ FROM THE FILES. Nothing is typed twice.

  python3 tools/build-installer.py "v11.87: one-line commit message"

Payload rules (tools/install-template.md): base64 after `exit /b 0`, extracted with a `for /f "skip=<HDRLINES>"` copy (v15.64; `more +N` before that)
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
#
# ⚠⚠ THE BASELINE MUST BE FETCHED FIRST, AND FOR EIGHT BUILDS IT WAS NOT (fixed 2026-08-28).
# Operator: "the companion has a reinstall instead of update." He was reading a true signal off
# Tampermonkey and my note contradicted it. The cause: this clone's `origin/main` ref was pinned at
# v14.71 — the cloud cannot PUSH (403), so nothing here ever advanced it — while HIS machine had
# pushed every installer since. The companion last moved at v14.72 (1.15 -> 1.16) and has been
# byte-identical for eight releases, but every diff against a nine-release-old ref said "CHANGED",
# so I told him to reinstall it eight times.
#
# ⚠ THE CLOUD CAN FETCH. Only push is blocked. `git fetch` was never tried here because "the cloud
# has no GitHub access" was carried as one fact when it is two — the same shape as the `file://`
# polling error: one true observation generalised past its evidence.
FETCH_OK = True
try:
    _f = subprocess.run(['git', 'fetch', 'origin', 'main'], capture_output=True, timeout=120)
    FETCH_OK = (_f.returncode == 0)
    if not FETCH_OK:
        print('WARNING: git fetch failed - the CHANGED/UNCHANGED verdict below is not trustworthy')
except Exception as e:
    FETCH_OK = False
    print('WARNING: git fetch threw (%s) - CHANGED/UNCHANGED not trustworthy' % e)

def changed(path):
    try:
        r = subprocess.run(['git', 'diff', '--quiet', 'origin/main', '--', path])
        return r.returncode != 0
    except Exception:
        return True

COMPAN_NOTE = 'companion, @version %s' % VC
if not FETCH_OK:
    # ⚠ NEVER SAY "CHANGED" ON A STALE BASELINE. An unverifiable answer is stated as unverifiable;
    # guessing "changed" is what produced eight false reinstall instructions.
    COMPAN_NOTE += ', CHANGE UNVERIFIED (no fetch)'
elif not changed(COMPAN):
    COMPAN_NOTE += ', UNCHANGED this release'

# ⚠ THE PANEL LINE WAS HARDCODED "(changed)" — the same defect, unnoticed because it is USUALLY
# true. On a build that only touches tools/ or tests it is false, and a false "(changed)" sends him
# to reinstall a script Tampermonkey will offer as "Reinstall". Both lines are now measured.
def _panel_verdict():
    if not FETCH_OK:
        return '(change unverified - no fetch)'
    return '(changed)' if changed(SCRIPT) else '(UNCHANGED, do not reinstall)'

def _compan_verdict(dash):
    if 'UNVERIFIED' in COMPAN_NOTE:
        return ' %s COULD NOT VERIFY (no fetch) - check Tampermonkey: it says Update or Reinstall' % dash
    if 'UNCHANGED' in COMPAN_NOTE:
        return ' %s UNCHANGED, do not reinstall' % dash
    return ' %s CHANGED, update it too' % dash

# --- the payload ---------------------------------------------------------------------------------
FILES = []
for pat in ['current/gex-signal-tapereader.user.js', 'current/gex-if-levels.user.js',
            'changelog/CHANGELOG.md', '.gitignore', '.gitattributes', '.gex-config.json',   # (v15.72) .gitattributes rides: it keeps the task scripts CRLF on his git
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
# (v14.68) ⚠⚠ session-state/pending/ — WORK THAT IS BUILT BUT DELIBERATELY NOT SHIPPED YET.
# `os.listdir` returns the directory name and `os.path.isfile` rejects it, so anything nested here
# was silently dropped — the FIFTH directory this manifest has lost (design/, skylit-docs/,
# tools/fixtures/, docs/, now this). The first thing parked here was the bounded-write fix for
# FINDINGS F-10, held back so it could not risk a live trading session. **A patch that exists only
# in a sandbox does not exist** — that is how ITEM 18 and the ES corpus were lost.
if os.path.isdir('session-state/pending'):
    for f in sorted(os.listdir('session-state/pending')):
        p=os.path.join('session-state/pending', f)
        if os.path.isfile(p):
            FILES.append(p)
# (v14.63) ⚠⚠ skylit-docs/ WAS NEVER IN THE PAYLOAD EITHER, AND THAT NEARLY LOST FINDINGS.md ON
# THE VERY BUILD THAT CREATED IT. Same shape as the design/ omission fixed at v14.59: a directory a
# `load gex` is REQUIRED to read, travelling by luck rather than by manifest. FINDINGS.md is named by
# three live hovers and by SOURCE-OF-TRUTH; the Academy mirror is the project's doctrine.
# Caught by decoding the .bat before sending - the only check that finds this class of bug.
# Markdown only: no captured HTML, no images.
for _root, _dirs, _fs in os.walk('skylit-docs'):
    for f in sorted(_fs):
        if f.endswith('.md'):
            FILES.append(os.path.join(_root, f))
FILES.append('SOURCE-OF-TRUTH.md')
# (v15.64) the lock beside package.json — jsdom's version pinned, so `npm install` on his machine matches the cloud's
for _pk in ('package.json', 'package-lock.json'):
    if os.path.isfile(_pk): FILES.append(_pk)
# (v15.64) ⚠⚠ roadmap/ AND archive/ WERE NEVER IN THE MANIFEST — THE SEVENTH DIRECTORY LOST THIS WAY.
# Found 2026-09-04 on a fresh clone: roadmap/ROADMAP.md (v15.59, pinned by test_process / test_v1559 / test_lessons),
# roadmap/DEFLECTION-ROADMAP.md (v15.50, test_roadmap), PREREGISTER.md, the FINDINGS-* notes, SIMPLIFICATION-PLAN.md,
# the v15.38 PARKED block in PRODUCT-ROADMAP.md, and the whole archive/v15.53/ (4,233 lines + 26 retired tests +
# INDEX.md) existed only in sandbox commits; GitHub had none of them. The suite a fresh clone ran was 130/37, not
# the 137/5 that suite.json advertised. The archive was rebuilt from git (tools/recover-archive.py); the roadmap
# is regenerated from the plan (tools/plan-seed.py); the rest is recorded as lost in LESSONS. Text only.
for _root, _dirs, _fs in os.walk('roadmap'):
    for f in sorted(_fs):
        if f.endswith('.md'):
            FILES.append(os.path.join(_root, f))
for _root, _dirs, _fs in os.walk('archive'):
    for f in sorted(_fs):
        if f.endswith('.md') or f.endswith('.js'):
            FILES.append(os.path.join(_root, f))
# the retired tests the installer must REMOVE from the operator's tree: an installer can add files but never
# delete them, so the 26 tests archived at v15.53 stayed at his repo root — red — until this build's .bat
# deletes the root copies (their archive copies ride along above). The list is the archive's own tests/ dir.
RETIRED_TESTS = sorted(f for f in os.listdir('archive/v15.53/tests') if f.endswith('.js')) if os.path.isdir('archive/v15.53/tests') else []
RETIRED_DEL = ''.join('del /q "%%REPO%%\\%s" >nul 2>&1\n' % f for f in RETIRED_TESTS)
for f in sorted(os.listdir('tools')):
    p=os.path.join('tools', f)
    # (v13.9) NO .bat, NO .log — an installer must never contain installers.
    if os.path.isfile(p) and not f.endswith('.log') and not f.lower().endswith('.bat'):
        FILES.append(p)
# ⚠⚠ (v14.92) SUBDIRECTORIES OF tools/ WERE SILENTLY EXCLUDED. `os.listdir` + `isfile` skips every
# directory, so tools/nightly/ — the harness, the protocol, the pre-registered hypothesis bank and
# the verdict ledger — was committed locally and would NEVER have reached GitHub. This is the exact
# shape of the loss recorded in data/es-1min/README.md: "anything that only exists in a sandbox
# commit does not exist." The hypothesis bank is the one artefact whose whole value is that it was
# written down BEFORE the data existed; losing it would make it unprovable.
for _sub in ('nightly',):
    _d=os.path.join('tools', _sub)
    if os.path.isdir(_d):
        for f in sorted(os.listdir(_d)):
            _p=os.path.join(_d, f)
            if os.path.isfile(_p) and not f.endswith('.log') and not f.lower().endswith('.bat'):
                FILES.append(_p)
# (v14.3) mockups are DESIGN documents: html/md only. A 0.64MB day-data .json and old .patch files
# had drifted in and were shipping with every build.
# (v14.57) PNGs ride too, and they are small. The render + overlap audit is MANDATORY before a
# mockup is sent (PROJECT-CONSTANTS L-D) and it has caught four real collisions in two days — but the
# output was landing nowhere durable, so the evidence for "this was audited" evaporated with each
# sandbox. ~120KB total against a 6MB cap; the argument for excluding them was never a size one.
# (v14.63) ⚠ mockups/ HAD GROWN TO 1.84MB AND TIPPED THE 6MB CAP on the build that created
# FINDINGS.md — it was shipping every mockup ever made. Same disease the session-state snapshots had
# at v14.3, same cure: SPECS always (.md, kilobytes, and they are the approved designs), but only the
# TWELVE most recent renders. Older ones stay in git history, which is where a superseded render
# belongs; they stop riding in every payload forever.
_mk_md = [f for f in sorted(os.listdir('mockups')) if f.endswith('.md')]
_mk_bin = [f for f in os.listdir('mockups') if f.endswith('.html') or f.endswith('.png')]
_mk_bin.sort(key=lambda f: os.path.getmtime(os.path.join('mockups', f)), reverse=True)
for f in _mk_md + _mk_bin[:12]:
    p = os.path.join('mockups', f)
    if os.path.isfile(p):
        FILES.append(p)
# (v14.59) ⚠⚠ design/ WAS NEVER IN THE PAYLOAD, AND THAT IS HOW DATA-ARCHITECTURE.md WAS LOST.
# The 2026-08-27 note recorded `design/DATA-ARCHITECTURE.md` as a file that "never landed" and blamed
# an unpushed sandbox commit. Both halves were wrong: it had never been WRITTEN, and even once it was,
# this builder would have dropped it silently, because `design/` was not in the manifest at all.
# A doc that a `load gex` is required to read cannot travel by luck. Text only - the mockup .html
# files in design/ are big and already have a home in mockups/.
# (v15.64) …and the RENDERS: design/render-vNNNN-face.png is the shipped script drawn in Chromium on a recorded day,
# the reference the CHANGELOG and the resume note point at. v15.63's never reached GitHub — same landmine, a
# file type the walk did not take. The mockup .png files are still excluded (mockups/ carries them).
# (v15.70) ⚠ THE RENDERS TIPPED THE 8 MB CAP (five 2×-DPI PNGs, 3 MB): only the THREE newest renders ride; the older
# ones are pushed over the desktop bridge when the session is linked (device_commit_files → the sync task) and stay in
# git history. Same rule as mockups', same reason.
_rn = [f for f in os.listdir('design') if f.startswith('render-') and f.endswith('.png')]
_rn.sort(key=lambda f: os.path.getmtime(os.path.join('design', f)), reverse=True)
for f in sorted(os.listdir('design')):
    p = os.path.join('design', f)
    if os.path.isfile(p) and (f.endswith('.md') or f.endswith('.txt') or f in _rn[:3]):
        FILES.append(p)
# (v14.59) the fixtures the tests read. test_futbars.js and append-futures.py both use
# tools/fixtures/futbars-day.json; shipping the test without its input turns his suite red for a
# missing file rather than a real defect - the exact trap BASERATES.json fell into at v14.57.
if os.path.isdir('tools/fixtures'):
    for f in sorted(os.listdir('tools/fixtures')):
        p = os.path.join('tools/fixtures', f)
        if os.path.isfile(p):
            FILES.append(p)
# (v14.67) ⚠⚠ docs/ WAS NEVER IN THE MANIFEST EITHER — THE FOURTH DIRECTORY IT HAS SILENTLY
# DROPPED (after design/ at v14.59, skylit-docs/ at v14.63 and tools/fixtures/). This one is the
# worst of the four: docs/LLM-NIGHTLY-BRIEF.md is the CONTRACT the scheduled review clones and
# follows. Ship the fix without it and the nightly keeps reading the old instructions from GitHub —
# the build would look successful and change nothing.
# ⚠ THE PATTERN IS THE POINT: a manifest built from an explicit list silently omits every directory
# nobody thought to add. Each was caught only by DECODING THE .bat before sending. Text only.
for f in sorted(os.listdir('docs')):
    p = os.path.join('docs', f)
    if os.path.isfile(p) and (f.endswith('.md') or f.endswith('.txt')):
        FILES.append(p)
FILES += sorted(f for f in os.listdir('.') if f.startswith('test_') and f.endswith('.js'))
# (v14.57) THE EVIDENCE THE HOD/LOD SECTION RESTS ON — but NOT the corpus itself.
# ⚠ test_hodlod.js READS data/es-1min/BASERATES.json to assert the panel's baked ladder still equals
# the study's output. Ship the test without the file and the suite goes red on his machine for a
# missing input rather than a real defect. Caught by decoding the payload before sending, which is
# the only check that would have found it.
# ⚠ EPM26-1min.csv.gz is DELIBERATELY EXCLUDED: 5.1MB against the 6MB payload cap below. It reaches
# GitHub by living in his working tree, not by riding the installer. See data/es-1min/README.md.
# ⚠⚠ (v14.72) FARSIDE.json JOINS THEM, AND IT WAS MISSING FROM THE FIRST BUILD OF THE FEATURE THAT
# NEEDS IT — caught by decoding the .bat before sending, which is the ONLY check that finds this
# class of bug (landmine L-Q, now four occurrences). The far-side courier fetches this file from
# GitHub; if it never reaches GitHub, the courier 404s forever and the panel silently serves its
# baked-in table with nothing saying so.
# ⚠ (v14.73) tools/irt/ RIDES TOO. The FlexLevels server, its launchers and the autostart setup
# existed ONLY as chat attachments until 2026-08-28, which is exactly why a later context searched
# the repo, found nothing, and told the operator they had never been built. Anything he has to RUN
# belongs in the payload.
for _p in ['data/es-1min/BASERATES.json', 'data/es-1min/FARSIDE.json', 'data/es-1min/README.md',
           'tools/irt/irtserve.py', 'tools/irt/irtserve.bat', 'tools/irt/irtstartup.bat',
           'tools/irt/setupautostart.bat', 'tools/irt/README.md',
           'tools/irt/FlexLevelsExport.sample.csv',
           # (v15.68) the two scheduled tasks — he RUNS these: the sync's setup and script (shipped over the bridge
           # until now) and the nightly's (setup once → tools/gex-nightly.bat every 10 min → tick.py)
           'setup-gex-sync.bat', 'tools/gex-sync.bat', 'setup-gex-nightly.bat', 'tools/gex-nightly.bat']:
    if os.path.exists(_p):
        FILES.append(_p)
# the approved HOD/LOD design lives at the repo ROOT, not in mockups/ — which is exactly why two
# earlier sessions reported it lost. Ship it so a fresh clone has the spec beside its transcription.
FILES += sorted(f for f in os.listdir('.') if f.startswith('mockuphodlod') and f.endswith('.html'))
# (v15.57b) ⚠⚠ EVERYTHING THE PANEL FETCHES FROM GITHUB MUST RIDE THE INSTALLER, BECAUSE THE CLOUD CANNOT PUSH.
# Found 2026-09-03 by probing his live panel after "reloaded double check": the raw repo had v15.57 but
# learning/studies.json, learning/register.json, learning/requests.json, data/es-1min/SWEEPS*.json and
# learning/log/*.json returned 404 — only learning/rules.json was ever in this manifest. So the Analysis tab
# showed the seed ("registry not fetched"), the sweep table never arrived, the READ quoted no rates, the
# nightly's verdicts never came back, and the register the panel ran on was the built-in seed. The SIXTH
# directory this manifest has lost. Rule from here: every file a `pipeFetch(PIPE_RAW_BASE+...)` names is
# listed HERE, by glob, and test_installer_manifest.js pins the list against the panel's fetch calls.
import glob as _glob
for _p in sorted(set(_glob.glob('learning/*.json') + _glob.glob('learning/*.md') + _glob.glob('learning/log/*.json') +
                     _glob.glob('learning/nightly/*.md') + _glob.glob('data/es-1min/*.json') + _glob.glob('review/*.json') +
                     # (v15.62) the deflection learning doc: the json the 📚 Learn tab fetches, the .md a context reads, the images
                     _glob.glob('learning/deflections/*.json') + _glob.glob('learning/deflections/*.md') + _glob.glob('learning/deflections/img/*.png'))):
    if os.path.isfile(_p) and _p not in FILES:
        FILES.append(_p)
# --list: print the manifest and stop — what the test reads
if '--list' in sys.argv:
    for _p in FILES:
        print(_p)
    raise SystemExit(0)

# --- size ADVISORY on the raw tree ------------------------------------------------------------
# ⚠⚠ (v15.22) THIS USED TO BE THE HARD GATE AND IT MEASURED THE WRONG THING. What `more +n` walks is
# the FINISHED .bat — lines of base64 — and the raw tree size is only a proxy for it across gzip
# (~4x on text) and base64 (+33%). On 2026-09-01 it refused a 6.3 MB tree whose real artefact was
# ~2.9 MB, the same size as the installer that had shipped an hour earlier and worked.
# **A proxy that blocks a good build costs as much as one that passes a bad one.** The hard gate is
# now on the artefact itself, where it is measured; this stays as a heads-up that the tree is growing.
_PAYLOAD_ADVISORY = 6 * 1024 * 1024
_total = sum(os.path.getsize(p) for p in FILES)
if _total > _PAYLOAD_ADVISORY:
    _big = sorted(((os.path.getsize(p), p) for p in FILES), reverse=True)[:10]
    print('PAYLOAD ADVISORY: %.1f MB raw, past the %.0f MB advisory. Largest members:' % (_total/1e6, _PAYLOAD_ADVISORY/1e6))
    for _s, _p in _big:
        print('  %8.2f MB  %s' % (_s/1e6, _p))
    print('  ADVISORY only — the hard gate is the finished .bat, measured after it is written.')

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

echo Extracting payload (about ten seconds)...
set HDRLINES={HDRLINES}
rem (v15.64) NOT `more +N` ANY MORE. `more` copies the payload out one line at a time and sat on this
rem line for minutes at 66,000 lines (5.1 MB) - and "hung" on a 30 MB file at v14.4x. A for /f skip
rem loads the file once and copies the payload lines in seconds. Safe with delayed expansion because
rem base64 carries no `!`; eol is `;` and no base64 line starts with one. Plain cmd only (Avast).
(for /f "usebackq skip=%HDRLINES% delims=" %%L in ("%SELF%") do echo(%%L)>"%TEMP%\\gex-{TAG}-payload.b64"
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
rem (v15.64) the tests archived at v15.53 - their copies now live in archive\\v15.53\\tests; the root copies
rem stayed on this machine (an installer adds, never deletes) and kept the suite red. Removed here so the
rem commit records the move.
{RETIRED_DEL}rem (v15.64) v10.js is GENERATED by tools/run-tests.sh and .gitignore'd, yet GitHub still tracked a v11.48 copy -
rem the stale-but-green trap BUILD-CHECKLIST warns about. Drop it from the index here; the file stays on disk, ignored.
"!GIT!" rm --cached -q v10.js >nul 2>&1
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
                      COMPAN_NOTE=COMPAN_NOTE, NFILES=len(FILES), RETIRED_DEL=RETIRED_DEL)

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
# (v15.64) NO BLANK LINES IN THE HEADER. The for /f skip counts lines, and whether an EMPTY line counts
# toward `skip=` is exactly the kind of cmd detail a build must not depend on: a blank line becomes
# `rem` (same line count, nothing printed), so the skip lands on the first base64 line either way.
_hl = header.split('\n')
header = '\n'.join((l if l.strip() else 'rem') if i < len(_hl) - 1 else l for i, l in enumerate(_hl))
assert header.count('\n') == n and not any(l.strip() == '' for l in header.split('\n')[:-1]), 'blank header line'
# …and no header line may look like a base64 line (a second guard for the payload boundary)
import re as _re
assert not any(_re.fullmatch(r'[A-Za-z0-9+/=]{76}', l) for l in header.split('\n')), 'a header line looks like payload'
assert not any(_re.match(r'\s*more\s+\+', l) for l in header.split('\n')), '`more +N` extraction is back — it stops at 65,535 lines (L-S)'

out = header.replace('\n', '\r\n') + '\r\n'.join(B64_LINES) + '\r\n'

# --- THE HARD SIZE GATE, ON THE THING `more` ACTUALLY WALKS ------------------------------------
# ⚠⚠ (v15.22) MEASURED, NOT MODELLED. `more +n` reads the FINISHED .bat line by line, so the .bat's
# own size and line count are the quantity that decides whether extraction feels instant or hangs.
# The raw-tree cap above was a proxy for this across gzip and base64, and on 2026-09-01 it refused a
# build whose real artefact was ~2.9 MB — the same size as the one that shipped an hour earlier.
# ⚠ The reference is not a guess: v13.x through v15.21 all extracted correctly, and v15.21 measured
# 2.89 MB across 37,106 lines. The cap is set at roughly double that, which is the largest artefact
# this delivery path has evidence for surviving. Raise it only with a build that actually ran.
# ⚠⚠ (v15.64, the install) THE REAL CEILING WAS `more` ITSELF: it stops at 65,535 lines (a 16-bit line count) and
# waits for a keypress on a prompt written into the redirected file — v15.64's 66,123 lines sat on "Extracting
# payload..." for good while v15.63's 59,759 had worked. The caps below were "double the largest artefact with
# evidence", a guess; the tool's limit was the fact. `more` is gone (a for /f skip copy has no such limit), and the
# assertion after the header is rendered refuses it coming back. The caps stay as a sanity limit on the file itself.
_BAT_BYTES_CAP = 8 * 1024 * 1024
_BAT_LINES_CAP = 110000
_bat_bytes = len(out.encode('ascii'))
_bat_lines = out.count('\r\n')
if _bat_bytes > _BAT_BYTES_CAP or _bat_lines > _BAT_LINES_CAP:
    print('INSTALLER TOO BIG: %.2f MB / %d lines (caps %.0f MB / %d lines)'
          % (_bat_bytes/1e6, _bat_lines, _BAT_BYTES_CAP/1e6, _BAT_LINES_CAP))
    _big = sorted(((os.path.getsize(p), p) for p in FILES), reverse=True)[:10]
    for _s, _p in _big:
        print('  %8.2f MB  %s' % (_s/1e6, _p))
    raise SystemExit('refusing to build an installer that will hang on extraction — trim the manifest')

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
print('==== DELIVER EXACTLY ONE FILE, WITH THE LINKS BELOW IT ====')
print('   %s      <- send THIS, and nothing else' % _DELIVER)
print('   (fallback only, if he reports the .bat failed: %s + %s)' % (ZIPNAME, BATNAME))
# ⚠⚠ THE LINKS ARE PART OF THE DELIVERY, NOT A SEPARATE STEP. Operator-mandated 2026-08-24 and
# again 2026-08-27: "make sure you give me tampermonkey links when you give me install files."
# On 2026-08-27 installv1458.bat went out with NO links and he sat on a build with a known unit bug
# while Tampermonkey's once-a-day update check had not fired. The links used to print sixty lines
# further down, in a block easy to scroll past; they print HERE now, welded to the filename.
print('')
print('   PASTE THESE WITH IT, EVERY TIME:')
print('   - Tapereader v%s %s - https://raw.githubusercontent.com/rassulshah/gex-signal-tapereader/main/current/gex-signal-tapereader.user.js' % (V, _panel_verdict()))
# ⚠⚠ (v15.22) THE COMPANION LINE CARRIES ITS URL TOO. Operator, 2026-09-01: "why is there not
# companion link.. you should give me that because a change is required." The panel's link has been
# printed here since v14.3 and the companion's never was — so on every build that changed the
# companion he was told to update it and handed no way to. The rule the panel link exists for
# ("give me the tampermonkey link every time") was never about the panel; it was about being able
# to install what changed.
print('   - Companion v%s%s - https://raw.githubusercontent.com/rassulshah/gex-signal-tapereader/main/current/gex-if-levels.user.js' % (VC, _compan_verdict('-')))
print('   - Then: wait ~5 min (CDN) -> CLICK THE LINK -> reload Atlas. TM auto-update is')
print('     ONCE A DAY by default, so the click is the reliable step. "Reinstall" means he')
print('     already has it, which is fine, not a failure.')
print('')
print('  (install-v%s.bat still exists as the self-extracting fallback)' % V)
print('round-trip: tar payload AND zip both byte-identical to the working tree')
# (v14.3, user-directed: "you must give me the tampermonkey link every time you give me an install
# file") — the block prints HERE, last, so the delivery message is written with it in view, and the
# applier itself now echoes the URL on success. Two places; forgetting requires ignoring both.
print('')
print('==== PASTE THIS WITH THE INSTALL FILE — EVERY TIME ====')
print('**Tampermonkey — update ONLY what changed:**')
print('- **Tapereader v%s** %s — https://raw.githubusercontent.com/rassulshah/gex-signal-tapereader/main/current/gex-signal-tapereader.user.js' % (V, _panel_verdict()))
print('- **Companion v%s**%s — https://raw.githubusercontent.com/rassulshah/gex-signal-tapereader/main/current/gex-if-levels.user.js' % (VC, _compan_verdict('—')))
print('Then wait ~5 min (raw CDN cache) and RELOAD the Atlas tab — footer must say v%s.' % V)

# ---- LEAVE THE TREE CLEAN --------------------------------------------------------------------
# ⚠ `install.bat` is an INTERMEDIATE, not a deliverable: everything shipped is one of the versioned
# copies above, and nothing below the copy step reads it. It is also listed in .gitignore — but it
# has been TRACKED since v10.43, so the ignore never applied and every single build rewrote a
# tracked file with 36,000 lines of fresh base64. That showed up as a dirty tree at the end of each
# build, inviting a context to either commit the blob or "clean up" by deleting a file the build
# scripts and docs still name. Restoring it from HEAD costs nothing and removes the choice.
# ⚠ Deliberately NOT `git rm --cached`: that would delete the file out of the operator's own repo on
# his next pull, and the installer can only add files, never restore one.
import subprocess as _sp
try:
    _tracked = _sp.run(['git', 'ls-files', '--error-unmatch', 'install.bat'],
                       capture_output=True).returncode == 0
    if _tracked and _sp.run(['git', 'diff', '--quiet', '--', 'install.bat']).returncode != 0:
        _sp.run(['git', 'checkout', '--', 'install.bat'], check=True)
        print('')
        print('(install.bat restored from HEAD — it is an intermediate; the delivery is installv%s.bat)'
              % V.replace('.', ''))
except Exception as _e:
    print('WARN: could not restore install.bat (%s)' % _e)

# ---- THE SAVE CONFIRMATION, DERIVED FROM THE COMMIT ------------------------------------------
# ⚠⚠ OPERATOR-MANDATED 2026-08-30, RESTATED 2026-09-01: "i dont see the tamper monkey links or save
# confirmations which you are supposed to give me everytime there is a build telling me the files
# that were saved (eg chat history, lessons learned etc.)"
#
# ⚠ THE FAILURE THIS CLOSES IS MINE, NOT THE BUILDER'S. The Tampermonkey block above has printed on
# every build since v14.3 — and for several builds running I did not paste it into the message, and
# I never listed the record files at all. A rule that lives only in my head is a rule with no
# mechanism, which is the same lesson `test_lessons` and `test_chat_history` already encode: the
# only rules this project keeps are the ones something prints or something fails on.
#
# ⚠ IT IS READ FROM `git show --stat HEAD`, NOT FROM MEMORY. A hand-written list drops whatever the
# writer forgets — exactly how ITEM 18 was lost — so the confirmation states what was ACTUALLY
# committed, and the mandated files are checked off against it rather than asserted.
print('')
print('==== SAVE CONFIRMATION — PASTE THIS TOO ====')
try:
    _stat = _sp.check_output(['git', 'show', '--stat', '--format=', 'HEAD'],
                             stderr=_sp.DEVNULL).decode()
    _files = [l.split('|')[0].strip() for l in _stat.split('\n') if '|' in l]
    _MANDATED = [
        ('session-state/CHAT-HISTORY.md',      'chat history (regenerated from the transcript)'),
        ('session-state/LESSONS.md',           'lessons learned'),
        ('changelog/CHANGELOG.md',             'changelog'),
        ('session-state/latest-resume-note.md','resume note'),
    ]
    for _p, _label in _MANDATED:
        print('  %s  %-34s %s' % ('saved  ' if _p in _files else 'MISSING', _label, _p))
    _other = [f for f in _files if f not in [m[0] for m in _MANDATED]]
    if _other:
        print('  also: %s' % ', '.join(_other[:8]) + (' + %d more' % (len(_other) - 8) if len(_other) > 8 else ''))
    print('  commit: %s' % _sp.check_output(['git', 'log', '-1', '--format=%h %s'],
                                            stderr=_sp.DEVNULL).decode().strip()[:96])
except Exception as _e:
    print('  COULD NOT READ THE COMMIT (%s) — say so rather than claiming a save.' % _e)
