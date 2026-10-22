// HostSlot: per-chart state when the host keeps the slot; ONE shared state per DLL when it does not. ASan/leak checked.
#include "HostSlot.h"
#include <cstdio>
#include <cstdlib>
struct Host { void* data = nullptr; bool keeps = true; };
static Host* cur = nullptr;
struct Ext { void* getUserData() { return cur->data; } void setUserData(void* p) { if (cur->keeps) cur->data = p; } };
struct St { int n = 0; };
static int fails = 0, passes = 0;
static void check(bool ok, const char* m) { if (ok) ++passes; else { ++fails; std::printf("FAIL %s\n", m); } }
int main() {
    { Ext x; HostSlot<St> s; Host a, b; cur = &a; St* pa = s.get(&x, true); pa->n = 1; cur = &b; St* pb = s.get(&x, true); pb->n = 2;
      check(pa != pb, "two charts get two states"); cur = &a; check(s.get(&x, false)->n == 1, "chart A keeps its state");
      s.release(&x); check(a.data == nullptr && s.get(&x, false) == nullptr, "release clears A"); cur = &b; check(s.get(&x, false)->n == 2, "B untouched"); s.release(&x); }
    // (HostSlot.h 2026-10-12 audit) a host that does not keep the pointer FAILS CLOSED: no state at all (a shared fallback would mix two
    // charts' subscriptions, counters and recorder files) - the pre-audit "one shared state" expectation is retired
    { Ext x; HostSlot<St> s; Host a; a.keeps = false; cur = &a; St* p1 = s.get(&x, true); St* p2 = s.get(&x, true);
      check(p1 == nullptr && p2 == nullptr && s.noSlot, "no slot: fail closed (no state, noSlot set)"); check(s.get(&x, false) == nullptr, "get without create: still none");
      Host b; b.keeps = false; cur = &b; check(s.get(&x, true) == nullptr, "no chart gets a shared state"); s.release(&x); check(s.get(&x, false) == nullptr && a.data == nullptr && b.data == nullptr, "release is a no-op, nothing leaked into the host"); }
    { Ext x; HostSlot<St> s; Host a; cur = &a; check(s.get(&x, false) == nullptr, "no create = no allocation"); }
    std::printf("RESULT %d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
