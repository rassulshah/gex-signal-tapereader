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
    { Ext x; HostSlot<St> s; Host a; a.keeps = false; cur = &a; St* p1 = s.get(&x, true); St* p2 = s.get(&x, true);
      check(p1 && p1 == p2 && s.noSlot, "no slot: one shared state, not one per call"); check(s.get(&x, false) == p1, "get without create returns it");
      Host b; b.keeps = false; cur = &b; check(s.get(&x, true) == p1, "every chart shares it (pre-audit behaviour)"); s.release(&x); check(s.get(&x, false) == nullptr, "release frees it"); }
    { Ext x; HostSlot<St> s; Host a; cur = &a; check(s.get(&x, false) == nullptr, "no create = no allocation"); }
    std::printf("RESULT %d passed, %d failed\n", passes, fails); return fails ? 1 : 0;
}
