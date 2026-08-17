# v10.48 spec — GEX/VEX dual-capture + mode-independent King & ladder

Goal (user, 2026-08-17): capture BOTH GEX and VEX continuously for analysis, and
guarantee the King (and the whole structural read) is correct whether the Skylit
heatmap is displaying GEX, VEX, or GEX+VEX. The display toggle must stop being able
to corrupt the read.

## Root cause (verified in v10.47 source)
- Skylit only sends the `data_type` (`gamma`/`vanna`/`combined`) for what is DISPLAYED.
- `onFeed` (~L263-264): `vanna`→`LASTVEX`; `gamma` OR `combined`→`LASTFEED`. So combined
  display contaminates the gamma cache (the A.4 wrong-King-780).
- `tapeMap()` reads the DOM tape, which shows whatever is displayed. The King reconciler
  `tapeSync` mixes tape votes (tag, tapemax) with the feed vote — so VEX/combined display
  makes 2 of 3 votes wrong (false out-of-sync, or unanimous-but-wrong King).

## Changes (data layer)
1. New `var LASTDISP = { SPY:null, QQQ:null };` (near LASTFEED/LASTVEX, ~L170) — the mode
   the user is DISPLAYING (distinct from what LASTFEED holds).
2. New `var LASTFEEDURL = null;` — template of the real gex/levels request URL.
3. Hooks (fetch + XHR): when a `gex/levels` URL is seen, set `LASTFEEDURL = url` (before parse).
4. `onFeed(sym, feed, j, viaSelf)`:
   - if `!viaSelf` set `LASTDISP[sym] = feed;`  (reflects the display)
   - `vanna` → `LASTVEX[sym] = {j, ts:Date.now()};` return
   - `gamma` → `LASTFEED[sym] = {j, feed:'gamma', ts:Date.now()};` observeFeedCadence; return
   - `combined` (or anything else): **do NOT write LASTFEED**. return. (self-fetch supplies gamma)
5. `selfFetch(sym, type)`:
   - require `LASTFEEDURL`; build URL = LASTFEEDURL with `symbol=<sym>`, `data_type=<type>`,
     `v=<Date.now()>` swapped (regex replace each param; if a param is absent, skip).
   - `fetch(url, {credentials:'include'})` → if `res.ok` → `res.json()` → `onFeed(sym, type, j, true)`.
   - swallow all errors (esp. 503). Per-(sym,type) throttle: skip if fetched < `SELF_MIN_MS` ago
     (SELF_MIN_MS = 4000). Track `SELF_LAST[sym+type]`.
6. `ensureFeeds()` on `setInterval(…, 5000)` started once in installFeedObserver, guarded by
   `document.visibilityState==='visible'` and `LASTFEEDURL`:
   - syms = ['SPY','QQQ']; for each: if `!LASTFEED[sym] || now-LASTFEED[sym].ts > FEED_STALE_MS`
     → `selfFetch(sym,'gamma')`; if `!LASTVEX[sym] || now-LASTVEX[sym].ts > FEED_STALE_MS`
     → `selfFetch(sym,'vanna')`. FEED_STALE_MS = 12000.
   - Net effect: the displayed mode stays fresh via the hook (never self-fetched); only the
     other mode(s) get self-fetched → ≤1 extra request per cycle in GEX/VEX display.

## Changes (structural source — makes the WHOLE panel GEX-correct in any mode)
7. `feedStructMap(sym)` — returns a `kingResolve`-shaped object built from `extractWalls(LASTFEED[sym].j)`:
   - `var ew = extractWalls(lf.j);` (needs `lf.j.levels`); if no `ew.king` return null.
   - `pct = {}`; for each `w` in `ew.walls`: `pct[w.k.toFixed(2)] = (w.k===ew.king) ? 100 : (w.pos ? w.pct : -w.pct);`
   - return `{ pct:pct, king:ew.king, count:ew.walls.length, kingSrc:'feed', kingTagged:ew.king,
     kingConflict:false, kingKd:null, fromFeed:true };`
8. `tapeMap(sym)` routing (keep the 1s cache):
   - `var disp = LASTDISP[sym];`
   - if `disp && disp!=='gamma'`: `var fm=feedStructMap(sym); if(fm) return fm;` (fall through if null)
   - else: existing DOM path; and where it currently returns `c?c.data:null` on unreadable tape,
     first try `var fm2=feedStructMap(sym); if(fm2) return fm2;` then the existing fallback.
   - Because tag/tapemax now come from the SAME feed-derived map, `tapeSync` reconciles cleanly
     (no false out-of-sync) in VEX/combined display.

## Changes (honesty UI — minimal)
9. Footer status (~L7123, currently `txt='SPY:'+f.feed`): when `LASTDISP.SPY && LASTDISP.SPY!=='gamma'`
   show `SPY:gamma·feed (disp <VEX|GEX+VEX>)` so it is explicit the structure is coming from the
   pure-gamma feed while the user views another mode. Colour = longAccent (healthy).

## Invariants to preserve
- exactly one `render()`, final line `})();`, no storage-key renames.
- `current/gex-signal-tapereader.user.js` and root `v10.js` MUST stay byte-identical (tests read v10.js).
- Version bump 10.47 → **10.48** in THREE spots: header `@version` (~L4), the part1 console.log
  (~L267 — currently WRONG at 10.43, set to 10.48), footer `feed v10.48` (~L7166).

## Tests
- Add `test_mode_king.js` (mirror existing harness: read ./v10.js, extract fns): assert
  feedStructMap builds correct King+pct from a synthetic gamma payload; assert that with
  LASTDISP='vanna'/'combined', the King resolves from the gamma feed (not the tape); assert
  onFeed ignores `combined` for LASTFEED and routes `vanna`→LASTVEX; assert ensureFeeds picks the
  stale mode. Run the FULL `test_*.js` suite — must be green except the 5 known-stale (layout_2col,
  node_identity, node_role_badge, nodemap, tapeking/jsdom).
