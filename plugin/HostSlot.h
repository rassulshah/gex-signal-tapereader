#ifndef LS_HOST_SLOT_H
#define LS_HOST_SLOT_H
// HostSlot.h (2026-10-08) -- the SDK's per-chart user-data slot, used safely.
// The audited indicators keep each chart's state in getUserData()/setUserData() so two charts on one DLL no longer share it.
// If the host does NOT keep the pointer we set (getUserData() reads back something else), the indicator falls back to ONE
// state per DLL - exactly the behaviour before the audit - instead of allocating a fresh state on every callback.
#include <new>
template <class T> struct HostSlot {
    T* shared = nullptr;
    bool noSlot = false;
    template <class X> T* get(X* x, bool create)
    {
        if (noSlot) { if (!shared && create) shared = new (std::nothrow) T(); return shared; }
        T* p = static_cast<T*>(x->getUserData());
        if (!p && create) {
            p = new (std::nothrow) T();
            if (!p) return nullptr;
            x->setUserData(p);
            if (x->getUserData() != p) {                          // the host keeps no slot: one shared state
                delete p; noSlot = true;
                if (!shared) shared = new (std::nothrow) T();
                return shared;
            }
        }
        return p;
    }
    template <class X> void release(X* x)
    {
        if (noSlot) { delete shared; shared = nullptr; return; }
        T* p = static_cast<T*>(x->getUserData());
        if (p) { delete p; x->setUserData(nullptr); }
    }
    ~HostSlot() { delete shared; }
};
#endif
