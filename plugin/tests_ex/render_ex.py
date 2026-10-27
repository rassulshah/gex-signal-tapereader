"""Render the mock host's draw ops (test_ex_host drawdump) to a PNG: the candles as the mock places them + every Ex text op exactly
at its rectangle (outline ops in dark, the letter in its colour). usage: python3 render_ex.py draws.txt out.png"""
import sys
import matplotlib; matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle
P = None; bars = []; texts = []; bo = None
for ln in open(sys.argv[1]):
    f = ln.strip().split(",")
    if f[0] == "PANE": P = dict(bottom=float(f[1]), base=float(f[2]), per=float(f[3]), x0=float(f[4]), w=float(f[5]))
    elif f[0] == "BAR": bars.append((int(f[1]),) + tuple(map(float, f[2:6])))
    elif f[0] == "BO": bo = int(f[1])
    elif f[0] == "TEXT": texts.append((f[1], int(f[2]), int(f[3]), int(f[4]), int(f[5]), int(f[6], 16)))
y = lambda px: P["bottom"] - (px - P["base"]) * P["per"]
fig, ax = plt.subplots(figsize=(9, 6), dpi=130); fig.patch.set_facecolor("#0b0f17"); ax.set_facecolor("#0b0f17")
for i, o, h, l, c in bars:
    x = P["x0"] + i * P["w"]; col = "#22C55E" if c >= o else "#EF4444"
    ax.plot([x, x], [y(h), y(l)], color=col, lw=1)
    ax.add_patch(Rectangle((x - P["w"] * 0.35, min(y(o), y(c))), P["w"] * 0.7, max(1.5, abs(y(o) - y(c))), color=col))
ax.axhline(y(7712.0), color="#6B7280", lw=0.8, ls="--"); ax.text(P["x0"] + bars[0][0] * P["w"], y(7712.0) - 4, "PDH 7,712.00 (key level, zone +/- 5 ticks)", color="#9CA3AF", fontsize=8)
ax.axhspan(y(7713.25), y(7710.75), color="#6B7280", alpha=0.12)
if bo is not None:   # lsBigBidAsk's BO letter (drawn by that plugin, shown here where it sits: green just above its candle)
    hb = [b for b in bars if b[0] == bo][0]; ax.text(P["x0"] + bo * P["w"], y(hb[2]) - 9, "BO", color="#22C55E", alpha=0.55, fontsize=9, fontweight="bold", ha="center", va="center")
for t, l, tp, r, b, col in texts:
    c = "#%06X" % col if col else "#000000"
    ax.text((l + r) / 2, (tp + b) / 2 - 2, t, color=c, fontsize=9, fontweight="bold", ha="center", va="center", zorder=5 if col else 4)
xs = [P["x0"] + b[0] * P["w"] for b in bars]
ax.set_xlim(min(xs) - 15, max(xs) + 60); ax.set_ylim(y(7697), y(7720)); ax.invert_yaxis() if y(7697) < y(7720) else None
ax.set_xticks([]); ax.set_yticks([y(v) for v in range(7698, 7721, 2)]); ax.set_yticklabels([str(v) for v in range(7698, 7721, 2)], color="#9CA3AF", fontsize=7)
for s in ax.spines.values(): s.set_color("#1f2937")
ax.set_title("lsLiquidityProfile 0.61.0 - Ex mark, mock-host draw ops (ES 3-min, 2026-10-22 synthetic)", color="#E5E7EB", fontsize=9)
ax.text(0.01, 0.02, "red Ex? = buyers exhausted at PDH (decision 10:03:30, candle 10:03-10:06; moved clear of lsBigBidAsk's BO letter (faint))\n"
        "green Ex = sellers exhausted at PDH from above (params file: 31 key-level cases)", transform=ax.transAxes, color="#9CA3AF", fontsize=7.5)
fig.tight_layout(); fig.savefig(sys.argv[2], facecolor=fig.get_facecolor())
