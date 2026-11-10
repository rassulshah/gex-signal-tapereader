# lsBreakout / lsBreakout1 1.15.0 + lsWavePOC 1.11.0 (2026-11-09 evening)

## 1. Wave PB POC: POC only, >= 38.2% deep (Rassul 18:46 CT)
"limit only to poc which should only be shown if the poc is at least a .38 retracement"
- The pullback profile now draws ONLY its POC row (white, lots label above it). No orange HVN rows and no grey rows
  ("limit only to poc" = the POC row only, no profile rows).
- Shown only when the POC PRICE (row centre) is at / beyond the 38.2% retracement of the wave: up wave
  POC <= high - 0.382 x (high - origin); down wave mirrored (wpc::pbPocDeep). Re-checked every bar while the pullback
  develops (repaint allowed, as before): a shallow POC draws nothing, and a later deeper POC can appear.
- Unchanged: the 38.2% retrace gate, the width rule (to the 2nd candle after the top pivot, min 20 px), freeze
  rules (Rev, separate BO / Rly, trade beyond the wave extreme, 16:00), invalidation on a close beyond the origin.
  An invalid pullback keeps state INVALID; a shallow POC there draws nothing either.
- Status: new VAP_SRC line (IRT VAP only; see 3).

## 2. Pdoc / ENT consistency
- bko::Engine::pbPocTk returns "none" when the frozen PB POC is shallower than 38.2% (same wpc::pbPocDeep), so the
  pivot is a plain Pb (or Pboc if the wave POC is tested). No ENT fact without a deep PB POC. Decided labels never change.
- Real ES cases that changed: 11-09 00:45 Pdoc -> Pb (PB POC 7429.75 shallow); 10-21 01:06 Pdoc -> Pb; 10-21 Rly
  pullbacks 22:12 (10-20), 00:12, 12:03 Pdoc -> no label. Kept Pdoc: 10-21 02:12, 14:30; 11-04 01:21, 08:30.
  Wave POC test: ES 11-09 PB POC 7462.75 (deep, shown); NQ 11-09 PB POC 27317.50 shallow vs 27312.41 -> not shown.

## 3. VAP never arrives (HG chart, urgent)
- Regression: 1.12.0's "no VAP -> draw nothing until it arrives" left HG with 0 Pb drawn (343 waiting).
- Fix: a Pb waits VAP_WAIT_BARS = 3 bars after its decision bar for IRT's VAP; still missing -> decided ONCE as a plain
  "Pb" (no POC test possible), record line boc "na", never switched later. A no-BO Rly pullback waiting the same
  3 bars -> decided "no label" (its rule needs the POC). Status PDOC line: "decided without VAP after 3 bars: Pb n / Rly n".
- NOT done tonight: Wave POC building VAP from live MBO prints (needs the DEPTH_ORDER trade/fill action mapping and
  bar mapping; too big to do safely tonight). Wave profiles on a chart whose IRT VAP is missing stay pending; the
  VAP_SRC status line says so. The real fix for HG is IRT-side (the HG chart has no VAP even for today's bars).
