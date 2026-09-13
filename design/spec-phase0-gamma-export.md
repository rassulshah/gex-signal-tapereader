# PHASE 0 — GAMMA PROFILE EXPORT (panel → RTX plugin)

**Status:** build, 2026-09-12. The RTX gamma-profile plugin reads this file. Additive to the panel; rides the
**same FlexLevels transport** (the `lsFlexLevels` FileSystemDirectoryHandle + in-place FSA write) that
`irtExportNow()` already uses. Written next to `FlexLevelsExport.csv` as **`GammaProfile.json`**.

## Why a second file, not more CSV rows
The FlexLevels CSV is a *lines* format (the operator reduced it to Kings+Walls — too many lines). The histogram
needs the **whole per-5pt-strike book**, which is data, not lines. A JSON file the plugin parses is the clean
carrier and keeps the CSV lean.

## Data sources in the panel (all already present)
- `T = tapeMap('SPXW')` → `T.pct` = **the full per-strike %King map** (strike→signed %King), `T.king` = King strike.
- `ifLevels('SPY')` → `{ cw, pw, zg, spot }` = Call Wall / Put Wall / Zero-Gamma(Flip) / spot.
- `emBand('SPY')` → `{ em, k, open }` = the 0DTE straddle expected move (points).
- SPX→ES price: reuse the export's `esOf('SPXW', strike)` (Skylit's own ES price) with `k * R.r` fallback, exactly
  as `irtBuildCsv()` does. Carry both `spx` and `es` per strike so the plugin can sit on either axis.

## Schema (`GammaProfile.json`, v1)
```json
{
  "v": 1,
  "asof": 1789450000000,
  "sym": "EPU26",
  "spot": 6xx.xx,
  "ratio": 11.57,
  "king": 7660,
  "strikes": [
    { "spx": 7585, "es": 7586.25, "pct": 24,  "rank": 13, "king": false },
    { "spx": 7660, "es": 7661.5,  "pct": 100, "rank": 1,  "king": true  }
  ],
  "levels": {
    "spxKing": 7660, "spxKingEs": 7661.5,
    "callWall": 7680, "putWall": 7620, "flip": 7645,
    "emPts": 21.0, "emHigh": 7681, "emLow": 7639
  }
}
```
- `strikes`: **every strike in the book** (5-pt), sorted by strike ascending. `pct` signed (%King, + call / − put),
  `rank` by |pct| (King=1), `king` flag. The plugin filters (Top-N / ≥N% / All) and colors (yellow +/purple −)
  client-side.
- `levels`: the horizontal-line rail. `flip` = zero-gamma. `emHigh/emLow` = spot ± emPts (0DTE straddle).
- Prices in `es`/`emHigh`/`emLow`/`*Es` are in the futures price space the plugin's chart uses; `spx`/`spxKing`/
  `callWall`/`putWall`/`flip` are SPX strikes (carry both so the plugin can convert if it prefers).

## Export code (additive block for `current/gex-signal-tapereader.user.js`)
Place near `irtExportNow`. `esProf()` mirrors the export's `esOf`; if factoring is easier, lift `esOf` to a shared
helper. Gate on `CFG.irt.on` (same as the CSV) plus a `CFG.irt.profileOn` toggle (default true).

```js
function gammaProfileBuild(){
  try{
    var cfgI=CFG.irt||{};
    var T=null; try{ T=tapeMap('SPXW'); }catch(e){}
    if(!T || !T.pct || T.king==null) return null;
    var iflv=null; try{ iflv=ifLevels('SPY'); }catch(e){}
    var band=null; try{ band=emBand('SPY'); }catch(e){}
    var R=null; try{ R=ifLadder('SPY'); }catch(e){}
    var ratio=(R&&R.r>1)?R.r:null;
    var esProf=function(k){ // Skylit ES price, else k*ratio
      try{ var sp=skylitFutPx((SKY_FUT&&SKY_FUT.ES)||'ES1','SPXW',k);
           if(sp&&typeof sp.px==='number'&&isFinite(sp.px)) return irtRound(sp.px,0.25); }catch(e){}
      return (ratio)? irtRound(k*ratio,0.25) : null;
    };
    var king=T.king, strikes=[];
    Object.keys(T.pct).forEach(function(kk){
      var k=parseFloat(kk), p=T.pct[kk];
      if(!isFinite(k)||typeof p!=='number'||!isFinite(p)) return;
      strikes.push({ spx:k, es:esProf(k), pct:+p.toFixed(1), king:(Math.abs(k-king)<0.001) });
    });
    if(!strikes.length) return null;
    var byMag=strikes.slice().sort(function(a,b){ return Math.abs(b.pct)-Math.abs(a.pct) || a.spx-b.spx; });
    byMag.forEach(function(s,i){ s.rank=i+1; });
    strikes.sort(function(a,b){ return a.spx-b.spx; });
    var spot=(iflv&&iflv.spot)||null, em=(band&&band.em)||null;
    return JSON.stringify({
      v:1, asof:Date.now(), sym:(cfgI.futSym||'ES'), spot:spot, ratio:ratio, king:king,
      strikes:strikes,
      levels:{
        spxKing:king, spxKingEs:esProf(king),
        callWall:(iflv&&iflv.cw)||null, putWall:(iflv&&iflv.pw)||null, flip:(iflv&&iflv.zg)||null,
        emPts:em, emHigh:(em&&spot)?+(spot+em).toFixed(2):null, emLow:(em&&spot)?+(spot-em).toFixed(2):null
      }
    });
  }catch(e){ return null; }
}

// write it next to the CSV, in-place (same gotchas as irtExportNow: keepExistingData + truncate by BYTES)
function gammaProfileExportNow(force){
  try{
    var cfgI=CFG.irt||{}; if(!force && (!cfgI.on || cfgI.profileOn===false)) return;
    var json=gammaProfileBuild(); if(!json) return;
    repoKvGet('irtDir', function(h){
      if(!(h && h.getFileHandle)) return;
      var name='GammaProfile.json', bytes; try{ bytes=new Blob([json]).size; }catch(e){ bytes=json.length; }
      h.getFileHandle(name,{create:true})
        .then(function(fh){ return fh.createWritable({keepExistingData:true}); })
        .then(function(w){ return w.write({type:'write',position:0,data:json}).then(function(){ return w.truncate(bytes); }).then(function(){ return w.close(); }); })
        .catch(function(){ /* mirror irtExportNow's replace-fallback if desired */ });
    });
  }catch(e){}
}
```

## Integration points
1. **Timer:** in the same tick that calls `irtExportNow()` (the 180s `irtTick`), also call `gammaProfileExportNow()`.
2. **Manual:** wherever `__gptsDebug.irtExport` / the 💾 export button fires, also fire `gammaProfileExportNow(true)`.
3. **Settings:** add `profileOn:true` to `CFG.irt`; one checkbox "Export gamma profile (RTX)".
4. **Version + ceremony:** bump `@version` in all three spots, snapshot, resume note + changelog, `tools/run-tests.sh`,
   build the installer, probe live (confirm `GammaProfile.json` appears in `lsFlexLevels` and parses).

## Test hooks
- `__gptsDebug.gammaProfile = function(){ return JSON.parse(gammaProfileBuild()||'null'); };` for a live dump.
- Sanity on the dump: `strikes.length` ≥ ~20, exactly one `king:true`, `levels.callWall > spot > levels.putWall`,
  `emHigh > emLow`, every `pct` in [-100,100] with the King at ±100.

The RTX plugin (Phase 1/2) is written against this schema; a sample `GammaProfile.sample.json` accompanies this spec
so the plugin can be built and run before the panel side is wired.
