#!/usr/bin/env python3
"""gen_hodlod_expected.py - bake the per-market HOD/LOD model candles into plugin/HodLodExpected.h (native, no runtime file).

Source: research-artifacts/multi-market-hodlod-model-candles-v1/multi_market_hodlod_model_candles.json (the LRA research run that
also writes MULTI_MARKET_HODLOD_MODEL_CANDLES.md). The values are copied, never re-estimated: the arithmetic means of the retained
model paths ("visible_center" in config/multi-market-hodlod-model-candles.v1.json), window "all" (every completed session) by default.

    python gen_hodlod_expected.py <model_candles.json> <out HodLodExpected.h> [--window 0|30|60|90] [--md <the .md, cross-checked>]

Exit 1 (and nothing written) when the input is missing a market or a field, or when --md disagrees with the JSON. The output is
deterministic: the same input gives byte-identical output, so the nightly job can run it every night and the gex auto-build only
rebuilds lsHodLod / lsSessionInfo when the research actually changed.
"""
import hashlib, json, math, os, re, sys

MARKETS = ("ES", "NQ", "CL", "GC", "HG", "NG", "EU")
FIELDS_T = ("openToFirstExtremeMinutes", "firstExtremeToOpenReclaimMinutes", "totalWickFormationMinutes",
            "firstToSecondExtremeMinutes", "openToSecondExtremeMinutes")
FIELDS_R = ("wickPoints", "wickDollars", "fullRangePoints", "fullRangeDollars")


def hm(s):
    m = re.fullmatch(r"(\d\d):(\d\d)", s or "")
    if not m or int(m.group(1)) > 23 or int(m.group(2)) > 59:
        raise ValueError("bad HH:MM %r" % s)
    return int(m.group(1)) * 60 + int(m.group(2))


def num(x, what):
    if not isinstance(x, (int, float)) or isinstance(x, bool) or not math.isfinite(x) or x < 0:
        raise ValueError("%s is not a finite non-negative number: %r" % (what, x))
    return float(x)


def dur(m):          # the .md's duration text ("3h 51m", "37m")
    m = int(round(m)); h, mm = divmod(m, 60)
    return "%dh %dm" % (h, mm) if h else "%dm" % mm


def check_md(md_text, rows):
    """every market's two model paragraphs in the .md must carry the same rounded numbers as the JSON"""
    bad = []
    for mk, (up, dn) in rows.items():
        heads = [(m.start(), m.group(1)) for m in re.finditer(r"^## (ES|NQ|CL|GC|HG|NG|EU) \u2014 ", md_text, re.M)]
        at = [i for i, (pos, k) in enumerate(heads) if k == mk]
        if not at:
            bad.append("%s: section missing in md" % mk); continue
        a = heads[at[0]][0]; b = heads[at[0] + 1][0] if at[0] + 1 < len(heads) else len(md_text)
        body = md_text[a:b]
        for name, r in (("Model up candle", up), ("Model down candle", dn)):
            part = body.split("### " + name, 1)
            if len(part) < 2:
                bad.append("%s %s missing" % (mk, name)); continue
            p = part[1]
            want = ["**%d** complete paths" % r["modeled"], "$%s**" % format(int(round(r["rangeUsd"])), ",") ,
                    "over an average **%s**" % dur(r["firstToSecond"]), "**%s** from open to the first final extreme" % dur(r["openToFirst"]),
                    "**%s** directly from open to that reclaim" % dur(r["openToReclaim"])]
            for w in want:
                if w not in p:
                    bad.append("%s %s: md lacks %r" % (mk, name, w))
    return bad


def main(argv):
    args = [a for a in argv[1:]]
    window, md = "0", None
    if "--window" in args:
        i = args.index("--window"); window = args[i + 1]; del args[i:i + 2]
    if "--md" in args:
        i = args.index("--md"); md = args[i + 1]; del args[i:i + 2]
    if len(args) != 2 or window not in ("0", "30", "60", "90"):
        print(__doc__); return 2
    src, out = args
    raw = open(src, "rb").read()
    d = json.loads(raw)
    sha = hashlib.sha256(raw).hexdigest()[:16]
    anchor = 8 * 60 + 30            # the research's common session window (config: common_session_window.rth_start)
    cw = d.get("commonSessionWindow") or {}
    if cw.get("rthStart"):
        anchor = hm(cw["rthStart"])
    rows = {}
    for mk in MARKETS:
        m = d["markets"][mk]["modelCandles"][window]
        if str(m.get("lookbackSessions")) != window:
            raise ValueError("%s window mismatch" % mk)
        pair = []
        for side in ("bullish", "bearish"):
            s = m[side]
            r = {"raw": int(s["rawPaths"]), "modeled": int(s["modeledPaths"])}
            if r["modeled"] <= 0 or r["raw"] < r["modeled"]:
                raise ValueError("%s %s: bad path counts" % (mk, side))
            for f in FIELDS_T:
                r[{"openToFirstExtremeMinutes": "openToFirst", "firstExtremeToOpenReclaimMinutes": "firstToReclaim",
                   "totalWickFormationMinutes": "openToReclaim", "firstToSecondExtremeMinutes": "firstToSecond",
                   "openToSecondExtremeMinutes": "openToSecond"}[f]] = num(s["timing"][f]["mean"], mk + side + f)
            r["medFirst"] = num(s["timing"]["openToFirstExtremeMinutes"]["median"], mk + side + "median first")      # the typical time (E 1st Time)
            r["medSecond"] = num(s["timing"]["openToSecondExtremeMinutes"]["median"], mk + side + "median second")    # the static 2nd window
            for f in FIELDS_R:
                r[{"wickPoints": "sizePts", "wickDollars": "sizeUsd", "fullRangePoints": "rangePts",
                   "fullRangeDollars": "rangeUsd"}[f]] = num(s["range"][f]["mean"], mk + side + f)
            pair.append(r)
        rows[mk] = (pair[0], pair[1], int(m["selectedCompletedSessions"]), m.get("firstSessionDate", ""), m.get("lastSessionDate", ""))
    if md:
        bad = check_md(open(md, encoding="utf-8").read(), {k: (v[0], v[1]) for k, v in rows.items()})
        if bad:
            print("MD CROSS-CHECK FAILED:\n  " + "\n  ".join(bad)); return 1
    L = []
    L.append("// HodLodExpected.h - GENERATED by tools/gen_hodlod_expected.py - do not edit by hand (the nightly job regenerates it).")
    L.append("// Source: %s (sha256 %s), definition %s %s, window %s (0 = all completed sessions)." % (
        os.path.basename(src), sha, d.get("definitionId", "?"), d.get("definitionVersion", "?"), window))
    L.append("// Values: arithmetic means of the retained model paths (Tukey-filtered), medFirst / medSecond = medians; minutes are measured from the research's RTH")
    L.append("// anchor (HLX_ANCHOR_MIN, the common 08:30 CT window - not each market's own open). Descriptive research, not a forecast.")
    L.append("#ifndef HODLOD_EXPECTED_H")
    L.append("#define HODLOD_EXPECTED_H")
    L.append("namespace hlx {")
    L.append("struct Model { int rawPaths, modeledPaths; double openToFirst, firstToReclaim, openToReclaim, firstToSecond, openToSecond, sizePts, sizeUsd, rangePts, rangeUsd, medFirst, medSecond; };")
    L.append("struct MarketModels { const char* m; int sessions; const char* from; const char* to; Model up, down; };")
    L.append("static const char* const HLX_SOURCE_SHA = \"%s\";" % sha)
    L.append("static const int HLX_WINDOW = %s;" % window)
    L.append("static const int HLX_ANCHOR_MIN = %d;" % anchor)
    L.append("static const MarketModels HLX_MODELS[%d] = {" % len(MARKETS))

    def mod(r):
        return "{ %d, %d, %.6f, %.6f, %.6f, %.6f, %.6f, %.10g, %.6f, %.10g, %.6f, %.6f, %.6f }" % (
            r["raw"], r["modeled"], r["openToFirst"], r["firstToReclaim"], r["openToReclaim"], r["firstToSecond"], r["openToSecond"],
            r["sizePts"], r["sizeUsd"], r["rangePts"], r["rangeUsd"], r["medFirst"], r["medSecond"])
    for mk in MARKETS:
        up, dn, n, a, b = rows[mk]
        L.append("    { \"%s\", %d, \"%s\", \"%s\", %s,\n             %s }," % (mk, n, a, b, mod(up), mod(dn)))
    L.append("};")
    L.append("inline const MarketModels* modelsFor(const char* m) { for (int i = 0; i < %d; i++) { const char* a = HLX_MODELS[i].m; const char* b = m; while (*a && *a == *b) { a++; b++; } if (!*a && !*b) return &HLX_MODELS[i]; } return 0; }" % len(MARKETS))
    L.append("}  // namespace hlx")
    L.append("#endif")
    text = "\n".join(L) + "\n"
    tmp = out + ".tmp"
    with open(tmp, "w", newline="\n") as f:
        f.write(text)
    os.replace(tmp, out)
    print("wrote %s (%d markets, window %s, sha %s)" % (out, len(MARKETS), window, sha))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
