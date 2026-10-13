#ifndef LS_HOST_SLOT_H
#define LS_HOST_SLOT_H
// HostSlot.h (2026-10-12) -- the SDK's per-chart user-data slot, used safely.
// The audited indicators keep each chart's state in getUserData()/setUserData() so two charts on one DLL do not share it.
// If the host does NOT keep the pointer we set (getUserData() reads back something else), fail closed.  A shared fallback
// would mix two charts' subscriptions, counters and recorder files, which is worse than showing no diagnostic on that host.
#include <new>
template <class T> struct HostSlot {
    bool noSlot = false;
    template <class X> T* get(X* x, bool create)
    {
        if (noSlot) return nullptr;
        T* p = static_cast<T*>(x->getUserData());
        if (!p && create) {
            p = new (std::nothrow) T();
            if (!p) return nullptr;
            x->setUserData(p);
            if (x->getUserData() != p) {                          // host has no usable per-chart slot
                delete p; noSlot = true;
                return nullptr;
            }
        }
        return p;
    }
    template <class X> void release(X* x)
    {
        if (noSlot) return;
        T* p = static_cast<T*>(x->getUserData());
        if (p) { delete p; x->setUserData(nullptr); }
    }
};
#endif
