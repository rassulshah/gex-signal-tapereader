"""Test data for tests_turn/test_turn_host.cpp (run on the research machine; the event files are not kept in the repo: 200 MB).
  ev_ES.txt / ev_GC.txt   mk_events.py <MKT> 2026-11-20 08:20 09:12 <out>   (raw Databento MBO -> IRT-style order events)
  LRA-Turn-ES.csv / -GC   the nightly's params as of the morning of 2026-11-20 (lra.turn_pane run with today = 2026-11-20)
  LRA-Turn-ES-nightly.csv the next nightly's params (SW rows of the 11-20 sweeps), stamps moved to the mock's clock
  wy-ES.txt / wy-GC.txt   lsRevRead's real level lines of 11-20 + the Sw lines, stamps on the mock's clock
usage: prep_host.py <out dir> <ev dir> <flex of the 11-20 morning run> <flex of the next nightly> <RevRead dir>"""
import sys, os, shutil
out, evd, flex20, flex23, rev = sys.argv[1:6]
os.makedirs(out, exist_ok=True)
OFF = 2082844800                                   # RTDATE (1904) -> the mock's clock (1970), both CT wall clock
for m in ("ES", "GC"):
    dst = os.path.join(out, f"ev_{m}.txt")
    if not os.path.exists(dst):
        os.symlink(os.path.join(evd, f"ev_{m}.txt"), dst)
    shutil.copy(os.path.join(flex20, f"LRA-TurnPane-{m}.csv"), os.path.join(out, f"LRA-Turn-{m}.csv"))
    L = [ln for ln in open(os.path.join(rev, f"wy-{m}-180.txt")).read().splitlines() if ln.startswith("WY|") or ln.startswith("L|")]
    sw = {"ES": ["3878009460|Sw|-1|LHI|28733|R99+", "3878010180|Sw|-1|ONH|28797|R99+"], "GC": ["3878010180|Sw|-1|PhrH|35824|V99+"]}[m]
    for ln in sw:
        c = ln.split("|"); c[0] = str(int(c[0]) - OFF); L.append("|".join(c))
    open(os.path.join(out, f"wy-{m}.txt"), "w").write("\n".join(L) + "\n")
t = []
for ln in open(os.path.join(flex23, "LRA-TurnPane-ES.csv")).read().splitlines():
    if ln.startswith("SW,"):
        c = ln.split(","); c[1] = str(int(c[1]) - OFF); ln = ",".join(c)
    t.append(ln)
open(os.path.join(out, "LRA-Turn-ES-nightly.csv"), "w").write("\n".join(t) + "\n")
print("ok", sorted(os.listdir(out)))
