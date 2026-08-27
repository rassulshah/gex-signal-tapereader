// ==UserScript==
// @name         Gex Signal Tapereader — LOCAL (stub)
// @namespace    gpts
// @version      1.0
// @description  Loads the REAL tapereader straight off disk, so a build lands on a page reload with no Tampermonkey click, no GitHub push and no CDN wait. Install this ONCE; it never needs updating again.
// @match        https://app.skylit.ai/atlas*
// @grant        none
// @run-at       document-idle
// @require      file:///C:/Dev/gex-signal-tapereader/current/gex-signal-tapereader.user.js
// ==/UserScript==

// ⚠⚠ THIS FILE HAS NO BODY ON PURPOSE. Everything is in the @require above.
//
// WHY IT EXISTS
// -------------
// The normal loop is: run the installer -> it pushes to GitHub -> click the Tampermonkey link ->
// wait ~5 minutes for the raw CDN (max-age=300) -> reload Atlas. Four steps and a wait, every build.
// Tampermonkey reads a @require itself, from the local filesystem, so with this stub the loop is:
// run the installer -> reload Atlas. The installer already writes to exactly the path above.
//
// ⚠ `@grant none` IS LOAD-BEARING AND IS COPIED HERE DELIBERATELY.
// The panel's Layer-0 feed hooks patch window.fetch and XMLHttpRequest IN PAGE CONTEXT. Any @grant
// switches Tampermonkey to a sandboxed context, the hooks land on a wrapper instead of the page's
// real objects, and the tape dies silently. A stub that required the script under a different grant
// would look installed and produce a dead panel.
//
// ⚠⚠ DISABLE THE ORIGINAL "Gex Signal Tapereader" SCRIPT BEFORE ENABLING THIS ONE.
// Both match https://app.skylit.ai/atlas* . Running both means TWO panels writing the same
// localStorage keys and the same IndexedDB store on every tick — the double-writer condition the
// project already forbids for a second /atlas tab. One or the other, never both.
//
// ⚠ KNOWN RISK, UNVERIFIED AS OF 2026-08-27: Tampermonkey CACHES @require externals. If a build
// does not show up after a reload, the cache is the reason, not the path. The externals update
// interval lives in Tampermonkey Settings (Config mode: Advanced). If it cannot be made to re-read
// on every load, fall back to Tampermonkey's own auto-update via @updateURL.
//
// HOW TO TELL WHICH ONE IS RUNNING: the console line below fires only from this stub.
console.log('[GPTS] LOCAL STUB active — script loaded from C:/Dev/gex-signal-tapereader/current/');
