#!/usr/bin/env python3
"""Static contract for DealerRead's SDK host-state lifecycle.

This deliberately inspects source rather than simulating Investor/RT: the real SDK
owns current-host selection for getUserData/setUserData, which is unavailable to the
Linux unit-test linker. It prevents a future edit from moving chart state back onto
the single shared extension object or from omitting either cleanup path.
"""
from pathlib import Path
import re
import sys

SOURCE = Path(__file__).with_name("DealerRead.cpp")
text = SOURCE.read_text(encoding="utf-8")
passed = 0
failed = 0


def check(condition, label):
    global passed, failed
    if condition:
        passed += 1
        print("  ok   ", label)
    else:
        failed += 1
        print("  FAIL ", label)


print("test_dealerread_lifecycle_contract -- source-level SDK lifecycle contract")
check("struct DealerReadState {" in text, "dedicated host-state record exists")
for field in ("Settings cfg;", "dl::Data D;", "loadedStamp", "profReach", "dltStamp", "lastStatusWrite"):
    state_match = re.search(r"struct DealerReadState \{(.*?)\n\};", text, re.S)
    check(state_match is not None and field in state_match.group(1), "state record contains " + field)

state_body = re.search(r"DealerReadState& DealerRead::state\(\)\n\{(.*?)\n\}", text, re.S)
check(state_body is not None and "slot_.get(this, true)" in state_body.group(1), "state reads the current host slot (HostSlot)")
check("HostSlot<DealerReadState> slot_;" in text, "state allocates a record only when absent (HostSlot)")
check('#include "HostSlot.h"' in text, "new record is registered with current host (HostSlot)")

for field in (
    "cfg", "D", "mkt", "root", "u", "loadedStamp", "loadedPath", "inV14",
    "profReach", "reachStamp", "dragging", "gripL", "gripT", "gripR", "gripB",
    "gridL", "gridT", "gridH", "dragDX", "dragDY", "posX", "posB", "posMkt",
    "dltW", "dltLeft", "dltStamp", "lastClearR", "lastLeft", "lastW",
    "lastStatusPath", "lastStatusText", "lastStatusWrite",
):
    check("#define %s (state().%s)" % (field, field) in text,
          "host-state alias routes " + field)

release_body = re.search(r"void DealerRead::releaseState\(\)\n\{(.*?)\n\}", text, re.S)
check(release_body is not None and "slot_.release(this);" in release_body.group(1), "cleanup deletes the record and clears the host slot (HostSlot)")
check("int DealerRead::done(void)    { releaseState(); return RTX_OK; }" in text, "done releases changed-symbol/period state")
check("int DealerRead::destroy(void) { releaseState(); return RTX_OK; }" in text, "destroy releases removed-chart state")

class_match = re.search(r"class DealerRead : public cppExtension \{(.*?)\n\};\nint cppExtension::init", text, re.S)
class_body = class_match.group(1) if class_match else ""
for residual in ("Settings cfg;", "dl::Data D;", "loadedStamp", "lastStatusWrite", "profReach"):
    check(residual not in class_body, "shared extension object has no " + residual + " member")

print("\n%d passed, %d failed" % (passed, failed))
sys.exit(failed)
