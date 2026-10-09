"""Independent reference for lsSessionVWAP 1.2.0 (pandas, plain sums - not the plugin's Welford code), and the checks.

    python reference.py <bars dir: data/irt/bars> <work dir> <plugin runner>

For every market and bar size (1m, 3m) of September-October 2026 (the bars the bridge stored from IRT, stamped at the bar's END,
Central time) it runs the REAL plugin (one full calc AND bar-by-bar growth like IRT) and compares each of the 11 outputs with
the reference built from the session rules written out independently:
  start = end - bar size;  start >= 17:00 -> the next calendar day's session;  Sat / Sun sessions do not exist
  RTH = start >= open and end <= close (ES/NQ 08:30-15:00, CL/NG 08:00-13:30, GC/HG 07:20-12:30, EU 07:20-14:00)
  O/N = start < open (incl. the evening);  after the close: RTH frozen until 17:00
  VWAP = sum(v tp) / sum(v), SD = sqrt(sum(v tp^2) / sum(v) - VWAP^2) (float64 sums; tp = (h + l + c) / 3)
  pRTH = the last RTH VWAP of the latest earlier session that had RTH, on O/N bars
"""
import gzip, os, subprocess, sys
import numpy as np
import pandas as pd

RTH = {"ES": ("08:30", "15:00"), "NQ": ("08:30", "15:00"), "CL": ("08:00", "13:30"), "NG": ("08:00", "13:30"),
       "GC": ("07:20", "12:30"), "HG": ("07:20", "12:30"), "EU": ("07:20", "14:00")}
ROOT = {"ES": "EPZ26", "NQ": "NQ", "CL": "CLEX26", "NG": "NGEX26", "GC": "QGC", "HG": "CPEZ26", "EU": "EU6Z26"}


def minutes(s):
    h, m = s.split(":"); return int(h) * 60 + int(m)


def reference(df, per_s, m):
    o, c = (minutes(x) * 60 for x in RTH[m])
    end = df["t"]
    start = end - pd.to_timedelta(per_s, unit="s")
    ssec = start.dt.hour * 3600 + start.dt.minute * 60 + start.dt.second
    sday = start.dt.normalize()
    sess = sday + pd.to_timedelta((ssec >= 17 * 3600).astype(int), unit="D")
    kind = np.where(ssec >= 17 * 3600, "ON", np.where(ssec < o, "ON", np.where(ssec + per_s <= c, "RTH", "POST")))
    kind = np.where(sess.dt.weekday >= 5, "NONE", kind)
    tp = (df["h"].astype(float) + df["l"].astype(float) + df["c"].astype(float)) / 3.0
    v = df["v"].astype(float)
    out = np.zeros((len(df), 11))
    last_rth = {}                                   # session -> final RTH vwap
    for key, idx in pd.Series(range(len(df))).groupby(sess.values):
        idx = idx.values
        for k_name, col0 in (("RTH", 0), ("ON", 5)):
            sel = idx[kind[idx] == k_name]
            if len(sel) == 0:
                continue
            sv = np.cumsum(v.values[sel]); svp = np.cumsum(v.values[sel] * tp.values[sel]); svp2 = np.cumsum(v.values[sel] * tp.values[sel] ** 2)
            with np.errstate(invalid="ignore", divide="ignore"):
                vw = svp / sv
                sd = np.sqrt(np.maximum(svp2 / sv - vw ** 2, 0))
            ok = sv > 0
            for j, val in enumerate((vw, vw + sd, vw - sd, vw + 2 * sd, vw - 2 * sd)):
                out[sel[ok], col0 + j] = val[ok]
            if k_name == "RTH" and ok.any():
                last_rth[key] = out[sel[ok][-1], 0:5].copy()
        if key in last_rth:                         # POST: frozen
            post = idx[kind[idx] == "POST"]
            out[post, 0:5] = last_rth[key]
    # pRTH
    keys = sorted(last_rth)
    for key, idx in pd.Series(range(len(df))).groupby(sess.values):
        prev = [k for k in keys if k < key]
        if prev:
            on = idx.values[kind[idx.values] == "ON"]
            on = on[out[on, 5] > 0]
            out[on, 10] = last_rth[prev[-1]][0]
    return out, kind


def main():
    bars_dir, work, runner = sys.argv[1:4]
    os.makedirs(work, exist_ok=True)
    fails, report = 0, []
    for m in ["ES", "NQ", "CL", "NG", "GC", "HG", "EU"]:
        for size, per in (("3m", 180), ("1m", 60)):
            parts = []
            for mon in ("2026-09", "2026-10"):
                p = os.path.join(bars_dir, m, f"{size}_{mon}.csv.gz")
                if os.path.exists(p):
                    parts.append(pd.read_csv(p))
            if not parts:
                continue
            df = pd.concat(parts).drop_duplicates("t_ct").sort_values("t_ct")
            df = df.rename(columns={"t_ct": "t"}); df["t"] = pd.to_datetime(df["t"])
            if size == "1m":
                df = df[df["t"] >= pd.Timestamp("2026-09-21")]     # two-plus weeks of 1-min bars keep the run short
            df = df.reset_index(drop=True)
            epoch = (df["t"] - pd.Timestamp("1970-01-01")).dt.total_seconds().astype("int64")
            src = os.path.join(work, f"{m}_{size}.csv")
            with open(src, "w") as f:
                for e, h, l, c, v in zip(epoch, df["h"], df["l"], df["c"], df["v"]):
                    f.write(f"{e},{h:.6f},{l:.6f},{c:.6f},{int(v)}\n")
            ref, kind = reference(df, per, m)
            tick = {"ES": .25, "NQ": .25, "CL": .01, "NG": .001, "GC": .1, "HG": .0005, "EU": .00005}[m]
            for mode, step in (("full", None), ("grow", 1 if size == "3m" else 5)):
                dst = os.path.join(work, f"{m}_{size}_{mode}.out")
                cmd = [runner, src, ROOT[m], dst] + ([str(step)] if step else [])
                subprocess.run(cmd, check=True)
                got = pd.read_csv(dst, header=None).values[:, 1:]
                # float32 outputs: allow a tenth of a tick or 2e-7 relative
                tol = np.maximum(tick * 0.1, np.abs(ref) * 2e-7)
                bad = np.abs(got - ref) > tol
                nb = int(bad.sum())
                drawn = int((got[:, 0] > 0).sum()), int((got[:, 5] > 0).sum()), int((got[:, 10] > 0).sum())
                if nb:
                    fails += 1
                    i, k = np.argwhere(bad)[0]
                    report.append(f"FAIL {m} {size} {mode}: {nb} values differ; first bar {df['t'][i]} out {k}: plugin {got[i, k]} ref {ref[i, k]} ({kind[i]})")
                else:
                    report.append(f"ok   {m} {size} {mode}: {len(df)} bars, RTH {drawn[0]} / O/N {drawn[1]} / pRTH {drawn[2]} bars drawn, all 11 outputs match")
    print("\n".join(report))
    print(f"\n{fails} failing runs")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
