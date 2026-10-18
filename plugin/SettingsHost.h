// SettingsHost.h  1.0.0  (2026-10-18)  -  the SDK side of SettingsSpec.h. Include AFTER irtsdk.h.
// Every indicator's settings window goes through these five calls - no set*Parameter / getIntegerValue by hand:
//   setup():        ss::registerAll(this, SPEC, IDS)            rows registered from the table, ids kept per table row
//   parmsLoad():    ss::fillDialog(this, SPEC, IDS, V, SNAP)    the window filled from the chart's file (colours with setParameterColor)
//   parmsApply():   V2 = ss::readDialog(this, SPEC, IDS, V, SNAP); store.save(V2)   (only inside the window - inDialog)
//   parmsUpdt(i):   ss::onUpdate(this, SPEC, IDS, i)            group master: items checked / unchecked + enabled / disabled
//   draw / calc:    store.load(mkt, nowMs) -> Values            the chart reads ONLY the file (IRT hands the chart copy blank controls)
#pragma once
#include "SettingsSpec.h"

namespace ss {

struct Ids { std::vector<int> id; bool ok = false; int count = 0; };

// The settings window's copy has no bar size (getPeriodicityLabel() == ""): only there are the controls' values real.
inline bool inDialog(cppExtension* x) { const char* pl = x->getPeriodicityLabel(); return !(pl && pl[0]); }

inline void registerAll(cppExtension* x, const Spec& S, Ids& ids) {
    ids.id.assign((size_t)S.n, -1); ids.ok = false;
    x->setParameterVersion(S.paramVersion);
    const int nl = visibleLines(S); x->setParameterDialogHeight(nl < 1 ? 1 : nl);
    for (int k = 0; k < S.n; ++k) {
        const Row& r = S.rows[k]; if (r.hidden) continue;
        const short fl = r.same ? (short)kParmAppendSameLine : (short)0;
        const int at = x->getParameterCount();
        switch (r.type) {
            case T_BOOL:  x->setBoolParameter(r.label, (RTBOOL)(r.def != 0), fl); break;
            case T_INT:   x->setIntegerParameter(r.label, (int)r.def, r.width, fl); break;
            case T_FLOAT: x->setFloatParameter(r.label, (float)r.def, r.width, fl); break;
            case T_COLOR: x->setColorParameter(r.label, (COLOR)((unsigned long)r.def & 0xFFFFFFul), r.width, fl); break;
            case T_LIST:  x->setListParameter(r.label, (short)r.def, r.choices, r.width, fl); break;
            case T_LABEL: x->setLabelParameter(r.label, r.width, fl); break;
            case T_TEXT:  x->setStringParameter(r.label, r.text ? r.text : "", (size_t)r.hi + 1, r.width, fl); break;
        }
        if (x->getParameterCount() == at + 1) ids.id[(size_t)k] = at;   // the id IRT gave THIS row (a refused row keeps -1: never read / filled)
    }
    ids.count = x->getParameterCount();
    ids.ok = true;
}

inline int rowOf(const Ids& ids, int parm) { for (size_t k = 0; k < ids.id.size(); ++k) if (ids.id[k] == parm) return (int)k; return -1; }

// What the window shows now, as Values (raw: invalid values stay as read so the caller can see them).
inline Values dialogNow(cppExtension* x, const Spec& S, const Ids& ids, const Values& file) {
    Values V = file;
    for (int k = 0; k < S.n; ++k) {
        const int id = k < (int)ids.id.size() ? ids.id[(size_t)k] : -1; if (id < 0) continue;
        double& o = V.v[(size_t)k];
        switch (S.rows[k].type) {
            case T_BOOL:  o = x->isBoxChecked(id) ? 1 : 0; break;
            case T_INT:   o = (double)x->getIntegerValue(id); break;
            case T_FLOAT: o = (double)x->getFloatValue(id); break;
            case T_COLOR: o = (double)((unsigned long)x->getParameterColor(id) & 0xFFFFFFul); break;   // NEVER getIntegerValue
            case T_LIST:  o = (double)x->getListIndex(id); break;
            case T_LABEL: break;
            case T_TEXT: { char b[1024]; std::memset(b, 0, sizeof b); const size_t cap = (size_t)S.rows[k].hi + 1 < sizeof b ? (size_t)S.rows[k].hi + 1 : sizeof b;
                           if (x->getParameterText(id, b, cap) == RTX_OK) { b[sizeof b - 1] = 0; if (V.t.size() != (size_t)S.n) V.t.resize((size_t)S.n); V.t[(size_t)k] = b; } break; }
        }
    }
    return V;
}

inline void applyEnable(cppExtension* x, const Spec& S, const Ids& ids, const Values& V) {
    if ((int)ids.id.size() != S.n) return;
    for (int k = 0; k < S.n; ++k) {
        const Row& r = S.rows[k]; if (!r.group) continue;
        const int m = find(S, r.group), id = ids.id[(size_t)k]; if (m < 0 || id < 0) continue;
        x->enableParameter(id, (RTBOOL)(V.b(m) ? 1 : 0));
    }
}

inline void fillDialog(cppExtension* x, const Spec& S, const Ids& ids, const Values& V, Snap& snap) {
    if (!ids.ok || (int)ids.id.size() != S.n) return;
    for (int k = 0; k < S.n; ++k) {
        const int id = ids.id[(size_t)k]; if (id < 0) continue;
        const double v = V.v[(size_t)k];
        switch (S.rows[k].type) {
            case T_BOOL:  x->checkBox(id, (RTBOOL)(v != 0)); break;
            case T_INT:   x->setIntegerValue(id, (int)std::llround(v)); break;
            case T_FLOAT: x->setFloatValue(id, (float)v); break;
            case T_COLOR: x->setParameterColor(id, (COLOR)((unsigned long)std::llround(v) & 0xFFFFFFul)); break;   // NEVER setIntegerValue
            case T_LIST:  x->setListIndex(id, (int)std::llround(v)); break;
            case T_LABEL: break;
            case T_TEXT:  x->setParameterText(id, V.s(k).c_str()); break;
        }
    }
    applyEnable(x, S, ids, V);
    snap.take(dialogNow(x, S, ids, V));
}

// The window -> values to save: an untouched control keeps the file's value; an invalid one keeps the file's (garbage never saved);
// group items of a master that is off are saved unchecked.
inline Values readDialog(cppExtension* x, const Spec& S, const Ids& ids, const Values& file, const Snap& snap) {
    if (!ids.ok || (int)ids.id.size() != S.n) return file;          // nothing registered: the file stays as it is
    const Values now = dialogNow(x, S, ids, file);
    Values V = file;
    for (int k = 0; k < S.n; ++k) {
        if (ids.id[(size_t)k] < 0) continue;                                  // hidden rows keep the file's value
        if (S.rows[k].type == T_TEXT) { if (V.t.size() != (size_t)S.n) V.t.resize((size_t)S.n);
            std::string tv = snap.pickText((size_t)k, now.s(k), file.s(k)); while (!tv.empty() && tv.back() == ' ') tv.pop_back();
            if (validText(S.rows[k], tv)) V.t[(size_t)k] = tv;
            continue; }
        const double pick = snap.pick((size_t)k, now.v[(size_t)k], file.v[(size_t)k]);
        if (valid(S.rows[k], pick)) V.v[(size_t)k] = pick;
    }
    for (int k = 0; k < S.n; ++k) if (S.rows[k].master && !V.b(k)) for (int j : itemsOf(S, k)) if (S.rows[j].type == T_BOOL) V.v[(size_t)j] = 0;
    return V;
}

// parmsUpdt: a master click checks / unchecks its BOOL items and enables / disables every item of its group.
inline bool onUpdate(cppExtension* x, const Spec& S, const Ids& ids, int parm) {
    const int k = rowOf(ids, parm); if (k < 0 || !S.rows[k].master) return false;
    const bool on = x->isBoxChecked(parm) != 0;
    for (int j : itemsOf(S, k)) { const int id = ids.id[(size_t)j]; if (id < 0) continue;
        if (S.rows[j].type == T_BOOL) x->checkBox(id, (RTBOOL)(on ? 1 : 0));
        x->enableParameter(id, (RTBOOL)(on ? 1 : 0)); }
    return true;
}

} // namespace ss
