"""gen_2400.py - lsHodLod 2.40.0 golden for t_hl2400.cpp: the MQ Reader's own TODAY PLAN rows for each test case.
For each case: the CobSignal status HTFNOW line + a Reader state (from the real ES state of 2026-11-20 09:43 CT, tp_state_es_1120.json,
mutated per case). The bridge's htf dict is built with lra.hodlod_box.htf_status / px_txt (analytics path given as argv[1]), then the
Reader's todayPlanHtf() (tp_reader_2400.js, copied verbatim) renders the plan; its rows are parsed from the HTML into golden_2400.txt.
Run (from plugin/tests_hodlod): python -I gen_2400.py <level-reversal-analytics/analytics>   (needs node)"""
import copy, html, json, os, re, subprocess, sys

sys.path.insert(0, sys.argv[1])
from lra import hodlod_box as hb   # noqa: E402

BASE = json.load(open("tp_state_es_1120.json", encoding="utf-8"))
REAL_HTF = [ln for ln in open("tp_cob_status_es_1120.txt", encoding="utf-8").read().splitlines() if ln.startswith("HTFNOW,")][0]


def L(code, px, fmt, zone=(), mq_only=False):
    return {"code": code, "price": px, "price_fmt": fmt, "mq_zone": list(zone), "mq_only": mq_only}


def es(mut=None, mkt="ES"):
    s = copy.deepcopy(BASE)
    if mut:
        mut(s["markets"][mkt] if mkt in s["markets"] else s["markets"].setdefault(mkt, {}))
    return s


cases = []   # (name, market, htfnow line, state)
cases.append(("real_0943", "ES", REAL_HTF, es()))
cases.append(("away_7200", "ES", REAL_HTF, es(lambda x: x.update(price_live=7200.0))))
cases.append(("near_edge", "ES", REAL_HTF, es(lambda x: x.update(price_live=7105 + 0.25 * 159.87))))
cases.append(("past_edge", "ES", REAL_HTF, es(lambda x: x.update(price_live=7105 + 0.2501 * 159.87))))
cases.append(("no_live_price", "ES", REAL_HTF, es(lambda x: x.update(price_live=None, price=7110.0))))
cases.append(("spring", "ES", "HTFNOW,7440.5,1,7440.5,1,7105,1,7090,1,-1,7098.25", es(lambda x: x.update(price_live=7140.0))))
cases.append(("s2_both", "ES", "HTFNOW,7440.5,1,7440.5,1,7105,1,7100,1,1,7174.5", es()))
cases.append(("s1_broken", "ES", "HTFNOW,7440.5,1,7440.5,1,7105,0,7105,0,1,7174.5", es()))


def upthrust(x):
    x.update(price_live=7176.0)
    x["ladder"] += [L("ONL · LL", 7181.0, "7,181.00", ["GEX 2 7180"]), L("LH", 7185.5, "7,185.50"), L("AH", 7178.25, "7,178.25", []),
                    L("MQ", 7175.0, "7,175.00", ["1D Max 7,175.00", "0DTE HVL 7,175.00", "Foo 7,175"], True)]


cases.append(("upthrust_tags", "ES", "HTFNOW,7180,1,7210,1,7105,1,7105,1,1,7186.75", es(upthrust)))
cases.append(("r_near_no_target", "ES", "HTFNOW,7130,1,7130,1,7000,1,6990,1,1,7131", es(lambda x: x.update(price_live=7128.0))))
cases.append(("no_em", "ES", REAL_HTF, es(lambda x: x.update(em=0))))
cases.append(("nq_real", "NQ", REAL_HTF.replace("7440.5", "26100").replace("7105", "25900"), es(mkt="NQ")))
cases.append(("gc_fmt", "GC", "HTFNOW,4150.3,1,4160,1,4101.1,1,4100,1,-1,4099.9",
              es(lambda x: x.update(price=4102.0, price_live=4102.0, price_fmt="4,102.0", em=40.0,
                                    ladder=[L("PDL", 4101.0, "4,101.0", ["Put Support 4,100"]), L("PDH", 4120.0, "4,120.0"), L("PWH", 4140.4, "4,140.4")]), mkt="GC")))
cases.append(("eu_fmt", "EU", "HTFNOW,1.1052,1,1.1060,1,1.10215,1,1.1020,1,1,1.1050",
              es(lambda x: x.update(price=1.1049, price_live=1.1049, em=0.004,
                                    ladder=[L("PFH", 1.10522, "1.10522", ["Call Resistance 1.1050"]), L("AL", 1.1040, "1.10400"), L("PDL", 1.1000, "1.10000")]), mkt="EU")))

js = open("tp_reader_2400.js", encoding="utf-8").read()
out = ["# golden_2400.txt - written by gen_2400.py from the Reader's todayPlanHtf() (tp_reader_2400.js). CASE|name|market / HTF|line / STATE|json / BUY|level|tags|chip|target / SELL|... / END"]
for name, mkt, htf, st in cases:
    hs = hb.htf_status(htf, mkt)
    now = hs["now"]
    H = dict(now, age_s=60, stale=False, fmt={k: hb.px_txt(now[k], mkt) for k in ("r1", "r2", "s1", "s2") if now.get(k) is not None})
    S = {"markets": {mkt: st["markets"][mkt]}, "now_ct": st["now_ct"]}
    prog = js + "\nconst S = " + json.dumps(S) + ";\nconst HB_ = {markets: {" + json.dumps(mkt) + ": {htf: " + json.dumps(H) + "}}};\nconst QA = () => '';\nprocess.stdout.write(todayPlanHtf());\n"
    h = subprocess.run(["node", "-e", prog], capture_output=True, text=True, encoding="utf-8", check=True).stdout
    buy, sell = [], []
    m = re.search(r'<tr class="rw"><td class="lmk tpc"><b class="tpk">[^<]*</b></td><td class="lkl tpc">(.*?)</td><td class="lkl tpc">(.*?)</td></tr>', h)
    if m:
        for cell, dst in ((m.group(1), buy), (m.group(2), sell)):
            for r in re.finditer(r'<b class="tpl"[^>]*>(.*?)</b> <span class="tpm">(.*?)</span> <span class="chip[^"]*"[^>]*>(.*?)</span><span class="ptg"[^>]*>(?:T1 <b>(.*?)</b>)?</span>', cell):
                dst.append([html.unescape(r.group(i) or "") for i in (1, 2, 3, 4)])
    out.append(f"CASE|{name}|{mkt}")
    out.append("HTF|" + htf)
    out.append("STATE|" + json.dumps(st, ensure_ascii=True))
    for side, rows in (("BUY", buy), ("SELL", sell)):
        for r in rows:
            out.append(side + "|" + "|".join(r))
    out.append("TIME|" + re.search(r"Today plan · updates live · (.*?) CT", h).group(1))
    out.append("END")
    print(name, "buy", buy, "sell", sell)
open("golden_2400.txt", "w", encoding="utf-8", newline="\n").write("\n".join(out) + "\n")
