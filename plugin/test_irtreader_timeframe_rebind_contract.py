"""IRTReader regression contract: a timeframe-only rebind preserves DBO arrival ordering."""
from pathlib import Path

root = Path(__file__).resolve().parent
source = (root / "IRTReader.cpp").read_text(encoding="latin-1")
logic = (root / "IRTReaderLogic.h").read_text(encoding="latin-1")
start = source.index("void IRTReader::depthIdentify(")
end = source.index("// one writer per market", start)
body = source[start:end]

checks = {
    "same-stream helper requires a populated matching market and contract": (
        "inline bool sameMboStream" in logic
        and "!oldMarket.empty() && !oldSymbol.empty()" in logic
        and "oldMarket == newMarket && oldSymbol == newSymbol" in logic
    ),
    "depth rebind distinguishes MBO stream identity before mutation": (
        "const bool sameDboStream = irl::sameMboStream(D.mkt, D.sym, mkt, sym);" in body
    ),
    "arrival clock resets only for a changed market or contract": (
        "if (!sameDboStream) D.arrival.reset();" in body
    ),
    "period remains part of lifecycle identity": (
        'const std::string id = mkt + "|" + sym + "|" + std::to_string(spb);' in body
    ),
}

failed = []
for name, ok in checks.items():
    print(("ok   " if ok else "FAIL ") + name)
    if not ok:
        failed.append(name)
print(f"\n{len(checks) - len(failed)} passed, {len(failed)} failed")
raise SystemExit(bool(failed))
