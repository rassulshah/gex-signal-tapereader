# GEX Signal Tapereader — Live Market Plan (2026-08-10)

## Purpose
Tomorrow's live session is for data capture and operational improvement, not just UI observation. The app should collect enough truthful structural data to support the next layers: better Read, better ACM, setup-resolution tracking, and later calibration / base-rate work.

## Current understanding
- Canonical time is now stored in UTC.
- Display time can be rendered in the user's local timezone.
- Session logic must still respect market-session boundaries even if display is local.
- Time-unit reconciliation is no longer the primary blocker; tomorrow it should be validated live rather than treated as still unresolved.

## Tomorrow's goals
1. Validate live data capture under open-market conditions.
2. Confirm the recorder preserves node, setup, and accumulation truth in the right sequence.
3. Capture enough outcome data to support the next logic layers and future calibration work.
4. Identify any remaining logic issues in targets, setup progression, accumulation interpretation, and resolution tracking.

## What the app must capture
### 1) Structure snapshots
- current price
- full live node map
- current node
- future meaningful nodes above and below
- node role labels (Gatekeeper, King, Cluster, Floor, Ceiling, etc.)
- reshuffles / map changes

### 2) Accumulation state
- node percentage of King
- build / steady / fade state
- node-in-play changes
- lower support strengthening beneath the current pullback node
- overhead resistance strengthening against an upward move
- cluster behavior when multiple nodes matter

### 3) Setup lifecycle
- BO
- FT
- PB/TST
- CONF
- GO
- VOID
- expiry / timeout if applicable

### 4) Direction and target context
- whether price is acting up, down, flat, or chop
- whether targets are aligned with the actual active direction
- whether a nearer weak node should be de-emphasized in favor of a stronger higher/lower node
- whether lower support should replace the current candidate buy node as the better structural level to wait for

### 5) Resolution / outcome truth
The recorder should move toward storing:
- hit T1
- hit T2
- failed
- expired
- still unresolved

## Live validation checklist
### A. UTC / session validation
- confirm feed events, candle events, setup events, and recorder entries line up correctly under the UTC model
- confirm display timezone conversion does not distort event order
- confirm session grouping and bucket assignment still match the market session

### B. Node / accumulation validation
- confirm the node shown in Read is the same structural context reflected by ACM
- confirm ACM shows the real node in play, not just the nearest arbitrary node
- confirm building / steady / fading labels are supported by live sequences
- confirm strengthening lower support or stronger second overhead resistance is recognized

### C. Setup validation
- confirm setups progress in the right order
- confirm voids and expiries are logged honestly
- confirm setup timestamps and recorded bars are consistent

### D. Target validation
- confirm T1/T2 are directional and path-aware
- do not show high upside targets when price is clearly moving down
- if multiple future nodes exist, ensure the stronger relevant node is surfaced

## Current bugs / risks to watch
### No longer primary blocker
- time-unit reconciliation, assuming UTC canonical storage is now implemented correctly

### Still worth validating live
- session/bucket alignment under the UTC model
- event ordering across feed, candle, and setup transitions
- target selection when multiple meaningful nodes exist on both sides
- synchronization between Read, ACM, and Signals

### Still unfinished / pending
- resolution recorder for hit T1 / hit T2 / failed / expired
- richer feature capture (RSI, divergence, multi-symbol confirmations)
- Phase 2 base-rate / calibration engine

## Claude's role tomorrow
Claude should act as the live-market collection coordinator. That means:
- observe and preserve structural evidence
- organize examples by node / accumulation / setup / target / resolution
- note any mismatches between Read, ACM, and Signals
- prepare precise follow-up coding notes after the live session

Claude should not invent market truth, confidence, or target logic. The source of truth must remain recorded app data and verified live observations.

## Success criteria for tomorrow
Tomorrow is successful if by the end of the session we have:
- validated live UTC-based sequencing
- captured real examples of node interaction and accumulation changes
- captured setup progressions and failures honestly
- identified what recorder fields still need to be added for resolution tracking
- collected enough evidence to improve the next logic layers from recorded truth rather than intuition
