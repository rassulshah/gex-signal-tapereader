"""IRTReader 0.8.3 (0.8.2) source contract (2026-10-22, MBO recorder fix P1): the order-feed writer stamps rows by arrival time."""
import os, re, sys
src = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "IRTReader.cpp"), encoding="latin-1").read()
body = src[src.index("void IRTReader::dboRead("):src.index("void IRTReader::depthCoverage(")]
checks = {
    "version 0.8.3": 'IR_VERSION = "0.8.3"' in src,
    "ArrivalStamp.h included": '#include "ArrivalStamp.h"' in src,
    "one arrival clock per depth state": "arr::Clock arrival;" in src,
    "clock applied to every written row": body.count("D.arrival.apply(") == 1,
    "the hour file comes from the arrival time": '"\\\\dbo_" + aHH + ".csv"' in body and "aSess" in body and "D.tsHH + \".csv\"" not in body,
    "the written time is the arrival time": "aY, aMo, aD, aH, aMi, aS," in body and "t.tm_year + 1900, t.tm_mon + 1" not in body,
    "clock applied after the skip / validity checks": body.index("D.dboSkip) { D.dboNotWritten++") < body.index("D.arrival.apply(") and body.index("std::isfinite(o.price)") < body.index("D.arrival.apply("),
    "reset only on a changed market / symbol stream": "const bool sameDboStream = irl::sameMboStream(D.mkt, D.sym, mkt, sym);" in src and "if (!sameDboStream) D.arrival.reset();" in src,
    "status shows restamped and max_back_s": '",restamped " << D.arrival.restamped << ",max_back_s " << D.arrival.maxBackS' in src,
    "fill coverage still uses IRT's own stamp": "D.cov.addFill((long long)o.timestamp" in body,
}
fails = 0
for k, ok in checks.items():
    print(("ok   " if ok else "FAIL ") + k); fails += (not ok)
print(f"\n{len(checks) - fails} passed, {fails} failed"); sys.exit(1 if fails else 0)
