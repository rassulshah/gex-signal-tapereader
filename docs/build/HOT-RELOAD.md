# Hot reload for the IRT indicators

2026-10-18. Rassul asked at 15:17 CT: *"is there a way for you to do the build without me having to restart every time, maybe i can just refresh the chart instead?"*

**Short answer:** yes, with one extra small DLL per indicator (a "shim"). Investor/RT has no reload feature of its own, so the shim does the job. You restart IRT **one time** to load each shim. After that, a new build turns up on the chart when you **refresh the chart**. You only need to restart again when a build changes the indicator's settings list.

The prototype is a tiny test indicator, **lsHotReloadDemo**. It is built and tested. It has not been tried on your IRT yet. The steps for that are in section 5.

---

## 1. What the SDK says (I read the SDK first, as asked)

Files read: `C:\Program Files\LinnSoft\InvestorRT\sdk\c++\include\irtsdk.h` (15.1.6), `sdk\c\include\irtsdk.h`, the Doxygen pages (`docs\html\index.html`, `classcpp_extension.html`, `irtsdk__private_8h_source.html`, `irtsdk_8cpp.html`), and the machine code of `sdk\c++\lib\irtsdkV143-x64.lib` (the `irtsdk.obj` inside it, disassembled with llvm-objdump).

| Question | Finding | Evidence |
|---|---|---|
| Is there a reload / unload / re-register call? | **No.** The header and the docs never mention reload, unload, LoadLibrary or FreeLibrary. | `irtsdk.h`: 0 matches. All 6 doc pages read: 0 matches. |
| When are init / setup called? | Once, *"when the RTX DLL is loaded by the core platform at startup"*. | `irtsdk.h` lines 1418 and 1421 |
| done / destroy | done: *"prior to destroy() and whenever the host chart changes periodicity or symbol"*. destroy: *"when the host chart is closed or when the RTX instance is removed from a chart"*. | `irtsdk.h` lines 1427 and 1430 |
| What does IRT call in a DLL? | **One exported function**, `cpp_main(void *pAPI)`. It lives inside irtsdk.lib. That lib exports nothing else. | `irtsdk.h` line 2738. `.drectve` of irtsdk.obj: `/EXPORT:cpp_main` only |
| How does cpp_main work? | It saves pAPI in a static global. On the first call it runs `CreateExtension()`. Then it switches on `*(int*)pAPI`, the reason (28 cases), and calls the virtual function. REASON_CALC always runs `calc(0)`, a full recalculation. REASON_UPDT runs `updt()` = `calc(bars-2)`. | disassembly of `cpp_main` (jump table `$LN69`) |
| Where is the per-chart state? | In the host's struct at offset **0xA0** (`void *userdata`). getUserData and setUserData only read or write that field. The host never frees it. | struct `irtapi`, `irtsdk_private.h` lines 196-217 (Doxygen source page). Disassembly of get/setUserData: `mov 0xa0(pAPI)` |
| Version handshake | REASON_INIT writes `checkversion` (offset **0xB0**) = `0x24A5A` (150106, SDK 15.1.6). | disassembly of cpp_main `$LN6` |
| Settings | `set*Parameter` only works **during REASON_SETUP** (allocParameter checks the reason). The settings table is built once, by the DLL IRT loaded at startup. `getParameterCount()` returns -1 outside SETUP. | disassembly of `allocParameter` and `getParameterCount` |
| Reading settings | `getIntegerValue` and `getParameters` ask IRT through `callIRT`. They do not use the DLL's own memory. A later build can read the settings IRT already has. | disassembly of `getIntegerValue` and `getParameters` |
| How IRT finds plugins | Every `ls*.dll` in `%USERPROFILE%\InvestorRT\dllx64`, loaded at startup. gex-build.bat copies there, and the copy fails while IRT has the DLL loaded. That failure is the "PENDING" state. | `tools\gex-build.bat` (DLLDIR, the copy step). Listing of dllx64 |
| A possible built-in route | The docs say the IRT **Object Manager** (File > Open > Object Manager) can build an RTX into DLLx64 and then add it to a chart. They do not say whether IRT unloads and reloads an RTX that is already in use. | `irtsdk.h` lines 56-61. Not tested. It would mean building through IRT's own wizard projects instead of our pipeline. |

**Verdict:** there is no documented hot reload in IRT. But the SDK's whole contract with IRT is that one export, plus one struct whose layout I checked against the lib's own code. A forwarding DLL can sit between IRT and the real indicator without touching the SDK.

## 2. The design (prototype)

```
IRT ──cpp_main(pAPI)──► lsHotReloadDemo.dll  (SHIM: loaded once, in dllx64, never changes)
                              │  reads InvestorRT\hotreload\lsHotReloadDemo.manifest (at most once a second)
                              ├─► lsHotReloadDemo_impl_1.dll  (build 1, the normal RTX build, renamed)
                              └─► lsHotReloadDemo_impl_2.dll  (build 2, loaded while IRT runs)
```

* **The implementation is a normal RTX build.** Same .cpp, links irtsdk.lib, exports cpp_main. Its only addition is one line, `HR_IMPL_EXPORTS(...)` (HotImpl.h). That line exports `hr_info` (name, build, version, settings signature, output count, flags) and `hr_attach` (lets the build show the shim's status line).
* **The shim forwards the host's own call.** It exports `cpp_main` and passes each call to the implementation's `cpp_main`. Because the interface between them is IRT's own, there is no second ABI to keep in step. The shim does not include irtsdk.h.
* **Each build keeps its own per-chart state.** The shim puts a small record of its own in the 0xA0 slot. Around each call it swaps in the pointer that belongs to that build. A build only ever sees its own state, never an older build's.
* **The swap.** The build script copies the new implementation under a **new** file name, so the file IRT has loaded is never overwritten. It then writes the manifest (a .tmp file moved into place in one step). The shim notices the new build on the next calc or update and prints "build N ready - refresh the chart". On the chart's next **full recalculation** (REASON_CALC = refresh), and only when no call for that chart is running, the shim does three things:
  1. sends REASON_DONE and REASON_DESTR to the old build, exactly what IRT sends when an indicator is removed, so the old build frees its state;
  2. clears the slot;
  3. sends the CALC to the new build, which rebuilds from the bars plus its record files. Signals are kept in record files, so nothing repaints.

  Each chart switches on its own refresh. An optional `SWAP tick` line in the manifest also switches on the next live update; the update is then turned into a full calc for that one call.
* **Handshake.** The shim refuses a build and keeps the running one when any of these is true:
  * the ABI is wrong;
  * the name is wrong;
  * the build number does not match the manifest;
  * the **settings list**, the output count or the flags differ from the build IRT started with.

  In that case the chart's status line reads, for example, *"build 3: the settings list changed - restart Investor/RT once"*. The shim never guesses.
* **Layout check.** If REASON_INIT does not write `0x24A5A` at 0xB0 (a different SDK layout), the shim switches to plain forwarding with no swaps. It says so in its status file.
* **Crash safety.**
  * Old builds are **never unloaded**. IRT may still hold pointers into the first one, for example its version text. This also means a call can never be running inside unloaded code.
  * Every call into a build is wrapped in SEH (__try/__except). If a new build crashes, the shim marks it bad, the chart goes blank, and the next refresh puts it back on the last good build. A later fixed build heals it without a restart.
  * There is a limit of 16 builds per IRT session.
* **Threads and nesting.** The shim's tables sit behind one lock, and **no lock is held while a build runs** (the tests prove this). A call that a build triggers for its own chart, from inside its own call, goes to the same build with the build's own pointer.
* **Status.** `InvestorRT\hotreload\lsHotReloadDemo.status.txt` is rewritten only when something changes. It shows the active build, the builds loaded, how many charts are on each, the swap count, and any refused build or crash. The build side writes `plugin\out\hot\HotReloadDemo.status.txt` and log lines in `tools\gex-build.log`.

### What still needs an IRT restart

* the first install of each shim, or a change to the shim itself (rare: it is generic);
* any change to the **settings list** (names, order, types, parameter version), to the output count, or to the flags or extended flags. IRT reads these once, at startup;
* a brand-new indicator;
* the description and version text in IRT's own indicator list. These stay as build 1's. The version drawn on the chart does update, because the panels draw it themselves.

## 3. The prototype: files

| File | What |
|---|---|
| `plugin\HotReloadDemo\HotReloadLogic.h` | All decisions, with no SDK and no Win32: manifest parser, handshake, router / swap state machine, status text |
| `plugin\HotReloadDemo\HotShim.cpp` | The Win32 plumbing: LoadLibraryExW, GetProcAddress, file stamps, CRITICAL_SECTION, SEH. Exports `cpp_main` |
| `plugin\HotReloadDemo\HotImpl.h` | `HR_IMPL_EXPORTS(...)` for an implementation build, plus `hotreload::shimStatus()` |
| `plugin\HotReloadDemo\HotReloadDemoImpl.cpp` (+ `HotReloadDemoLogic.h`) | The test indicator. Box: "Hot Reload Demo v1.0.0 build N calcs K" and the shim's status line. One blue line at the last close |
| `plugin\HotReloadDemo\test_hotreload_logic.cpp` | 180 checks with fake builds |
| `plugin\HotReloadDemo\test_hotreload_e2e.cpp` + `test_hotreload_e2e_impl.cpp` | End to end on Linux: real files, the manifest published with tmp + rename, real dlopen of three versioned builds. 15 checks |
| `plugin\HotReloadDemo\run_hotreload_tests.sh` | Runs both |
| `tools\gex-hot-build.bat` | Builds the implementation, publishes it while IRT is open, builds the shim and installs it while IRT is closed (CRLF, no PowerShell) |
| `tools\gex-build.bat` | One added line: `call "%REPO%\tools\gex-hot-build.bat" HotReloadDemo` |

### Tests (all passing)

* **Logic, 180 checks** (g++ and clang++, `-fsanitize=address,undefined -Werror -Wall -Wextra`):
  * manifest parsing: good input, plus 18 kinds of bad input including path traversal, other indicators' files, garbage numbers and oversize files;
  * every handshake result;
  * startup with no manifest, and with a bad first build;
  * plain-forwarding mode;
  * two charts with separate state, where the host slot holds the shim's record and each build sees its own pointer;
  * the swap: DONE then DESTR to the old build with the old pointer, then a fresh CALC on the new one, with the host's reason restored afterwards;
  * the other chart staying on the old build until its own refresh;
  * "ready - refresh the chart" appearing on a live tick;
  * SWAP tick, where the update is promoted to a full calc;
  * refusing a build whose settings, outputs or flags changed, never calling into it and not retrying it;
  * transient load failures (retried);
  * a garbled manifest (the running build is kept);
  * older builds being ignored;
  * a crash in a new build (fallback, a blank chart, recovery on refresh, the crashed build never called again);
  * a crash in the first build (healed by a fixed build);
  * nested calls;
  * the status call from inside draw (no deadlock);
  * the 16-build limit;
  * foreign pointers left alone;
  * the status file written only when something changes;
  * the demo's text.
* **End to end, 15 checks:** start, two charts, build 2 published (shown as ready on a tick, switched on refresh, the old build's state destroyed), build 3 with changed settings refused, charts removed.
* **Windows checks:**
  * `HotShim.cpp` compiles and **links as a real Windows DLL** with MinGW (`-Wall -Wextra -Werror`). Its export table holds exactly `cpp_main`.
  * `HotReloadDemoImpl.cpp` passes a MinGW syntax check against the **real** `irtsdk.h`: 0 errors, 0 warnings outside the SDK.
  * The MSVC build happens in your GEX build task.

**Not verified yet (needs your IRT):** that a chart refresh sends REASON_CALC; that IRT is happy with the extra DONE/DESTR the shim sends mid-session; the MSVC compile of both DLLs.

## 4. Risks

1. **Whether "refresh" sends a full calc.** The SDK says REASON_CALC is the full calculation, but which IRT action sends it is not documented. If a refresh does not switch the build, opening the indicator's settings and pressing OK will (a settings change recalculates). The fallback is `SWAP tick`: then no action is needed at all.
2. **The extra DONE/DESTR sent mid-session.** These are the same calls IRT makes when you remove an indicator. If an indicator's destroy calls something IRT only allows during real teardown, it shows up in the test of that indicator.
3. **Code that relies on setup() or init().** These run only in the first build (an SDK rule). An indicator that keeps parameter numbers from `getParameterCount()` in members (SessionVWAP does) must switch to constant numbers first.
4. **Leftover resources.** Threads, timers, DEPTH subscriptions or static handles that destroy() does not release keep running in the old build, which stays loaded. This needs checking per indicator.
5. **Memory.** Old builds stay in memory until IRT exits, roughly 0.2-1 MB each, at most 16 per indicator per session.
6. **Two builds side by side.** Two charts can run different builds until both are refreshed. They write the same record files, which is already the case with two charts today.
7. **Antivirus.** A scan of a new DLL may block it briefly. The shim retries every second.
8. **Thread safety.** I assume IRT never calls the same chart's indicator from two threads at once. The SDK's own static pAPI makes the same assumption.

## 5. Your one-time test (step by step)

The demo is a test only. It has no settings and goes in **one** chart.

* **Step 1** – Wait about 2 minutes after this was committed. The build task publishes build 1 of the demo and builds the shim.
* **Step 2** – Close Investor/RT with **File > Exit** (switching layouts does not install). Wait 2 minutes so the shim installs itself, then open IRT again. This is the only restart.
* **Step 3** – Add the indicator **lsHotReloadDemo** to one chart, for example ES. At the top left it shows `Hot Reload Demo v1.0.0  build 1  calcs …` and `hot reload OK`.
* **Step 4** – Tell me "demo is on". I change the version to 1.0.1 and the build task publishes build 2, about 2 minutes. You do **not** close IRT. The box's second line changes to `build 2 ready - refresh the chart`.
* **Step 5** – Refresh that chart. The box reads `Hot Reload Demo v1.0.1  build 2  calcs 1`: the new build, started fresh, without a restart. If the box still says "ready - refresh the chart", open the indicator's settings and press **OK**.
* **Step 6** – Tell me what the box says. I read `InvestorRT\hotreload\lsHotReloadDemo.status.txt` to confirm the swap. Then remove the demo from the chart.

## 6. Plan for converting the real indicators (after the settings migration is finished)

**What changes in the build**

* Move `HotReloadLogic.h`, `HotShim.cpp` and `HotImpl.h` into `plugin\HotReload\`.
* Extend `gex-hot-build.bat` to take the same arguments as `:one`, that is `<Name> <lsName> <source .cpp> <headers...>`.
* Each converted indicator's `call :one X lsX ...` line becomes a `call :hot X lsX X.cpp ...` line. The implementation builds from the **same .cpp** as today.
* `lsX.dll` in dllx64 becomes the shim under the same name, so charts and saved settings stay the same. The BUILD-STATUS `:status` list gains a hot section.

**Per indicator**

* Add `HR_IMPL_EXPORTS(...)` with a settings string, and a Linux test that checks the string against the set*Parameter calls in setup(). This stops the string from drifting.
* Audit init(), setup(), destroy(), threads, timers and DEPTH objects (risks 3 and 4).
* Run the host tests through the shim with a swap in the middle: the chart state is rebuilt, there is no repaint, and the record files match.

**First wave** (the indicators changed most often this week):

* DeltaProfile
* LiquidityProfile and LiquidityFlow (same source)
* BigBidAsk
* BidAskStats
* BookFlow
* TapeReader
* DealerProfile
* HodLod
* CobImpulse
* SessionVWAP (needs its parameter numbers made constant)

That is 11 DLLs.

**Second wave:**

* the 7 FootprintReader variants and the 7 TapeFlow variants (one source each);
* DealerRead, DealerSig, DealerSummary;
* the rest.

**Switch-over:** one IRT restart for the whole first wave, outside trading hours.

**Estimate**

| Work | Time | Cost |
|---|---|---|
| Shared folder + generalised batch | about 20-30 min | |
| First wave (11 DLLs), at 10-20 min each for the audit, edit and swap test | about 2.5-3.5 h | about $25-40 |
| Second wave | about 1 h | about $10 |

I will give you this estimate again for your go before starting, once the settings migration is done.
