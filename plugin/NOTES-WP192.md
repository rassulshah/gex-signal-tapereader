# lsWavePOC 1.9.2 (2026-11-06 13:11-13:13 CT)

Rassul 13:11: "If the wave poc is above the breakdown low, dont display the hvns", "the wave poc should only be on Cob, Rly and BO waves", "fix the issues not fixed yet".
Rassul 13:13 (ES screenshot): "why is there a profile being drawn on this little wave ?" That was the 1.7.0 live wave, which is drawn from its first bars.

## 1. Only CoB, Rly and BO / BD waves (rule 192, the new default; 150 = the 1.5.0 rule and 133 stay for the older suites)
- **A confirmed leg is judged GRACE = 3 bars after its confirmation** (Engine::legQ), so the BO / Rly / CoB decisions of its bars are in. It qualifies if it is one of these:
  - **BO / BD wave:** a fired BO / BD of its direction whose candle is inside the leg and that fired by then (bko::Engine on the same bars; BoCand.tf).
  - **Rly wave:** a fired Rly of its direction ending within the leg's last bars (bko::Engine rly; RlyCand).
  - **CoB wave:** a drawn CoB of its direction on a bar of the leg, i.e. lsCobSignal's rule with ZZO >= 10, decided by then (cobs::Engine run natively; CobCand). This replaces the RTL read of cob_bull_3m_1b / cob_bear_3m_1b; COB_SOURCE now says "native lsCobSignal ...".
- **Records keep their nodes, but no longer qualify a wave:** a wave recorded under the older rules is not drawn unless it passes rule 192.
- **The live (provisional) wave is drawn only once a BO / BD or a CoB has fired inside it.** A Rly is known only at the wave's end.
- **The new wave stops older nodes at the bar it is decided at.**
- **Status:** QUALIFIED_BY gives the counts by BO / BD, Rly and CoB, plus the inputs.

## 2. No HVN when the POC is in the zone
- Down wave: if the POC row is at or above the break level (the BD low), no HVNs are drawn. Up wave mirror.
- This only applies with a real break level: the BO / BD candle, or the bar that broke the prior pivot. With no break level the zone is the whole wave, which always holds the POC.

## 3. VAP retry
- Waves waiting for IRT's volume at price are retried every 60 s (was 10 min).
- HG, Nov 6: after the IRT restart, every wave of the day sat at "VAP pending".

## Tests
| Test | Result |
|---|---|
| test_wavepoc_190 (+ [6] rule 192 on the ES fixture: every profiled wave is BO / Rly / CoB, every leg with a fired BO / BD is profiled, the live wave only with a BO / CoB; + POC-in-zone unit checks) | 43 passed |
| test_wavepoc_150 (pinned to rule 150; the default is now 192) | 49 passed |
| test_wavepoc_logic | 187 passed |
| test_wavepoc_host (the 14 RTL CoB checks are retired; COB_SOURCE = native) | 108 passed |
| MinGW against the real irtsdk.h | 0 errors, 0 warnings |
