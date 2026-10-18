// mock_irtsdk.h - the parameter API of irtsdk.h 15.1.6 as IRT behaves on his PC (for SettingsHost.h tests on Linux).
// Modelled behaviour (BUILD-LESSONS 42 / 44 / KingTracker v0.15 / BigBidAsk 1.16): outside the settings window every box reads
// unchecked, every list -1, every colour 0, every number 0; setIntegerValue / getIntegerValue on a COLOUR row do nothing / read 0
// (the black swatch); a window can be told to IGNORE fills (the suspected IRT behaviour lsBigBidAsk 1.16 logs).
#pragma once
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

typedef unsigned long COLOR;
typedef unsigned short RTBOOL;
typedef int RTX_RESULT;
static const RTX_RESULT RTX_OK = 0;
static const RTX_RESULT RTX_FAIL = -1;
static const short kParmAppendSameLine = (1 << 0);
#define MAKE_COLOR(r,g,b) ((COLOR)((((unsigned long)(r) & 0xFF) << 16) | (((unsigned long) (g) & 0xFF) << 8)  | (((unsigned long) (b) & 0xFF))))

namespace mock {
enum PT { P_BOOL = 1, P_INT, P_FLOAT, P_COLOR, P_LIST, P_LABEL };
struct Param { PT type; std::string name; short width = 0, flags = 0; int i = 0; float f = 0; COLOR c = 0; bool b = false; int list = 0, nList = 0; bool enabled = true; };
struct Ext {
    std::vector<Param> p;
    bool dialog = true;        // false = the chart copy
    bool ignoreFill = false;   // the window ignores values written by the plugin
    unsigned paramVersion = 0; int dialogLines = 0;
};
}

class cppExtension {
public:
    mock::Ext m;
    virtual ~cppExtension() {}
    const char* getPeriodicityLabel() { return m.dialog ? "" : "3 Min"; }
    int getParameterCount() { return (int)m.p.size(); }
    RTX_RESULT setParameterVersion(unsigned v) { m.paramVersion = v; return RTX_OK; }
    void setParameterDialogHeight(int n, int = 22) { m.dialogLines = n; }
    RTX_RESULT add(mock::PT t, const char* n, short w, short fl) { mock::Param q; q.type = t; q.name = n ? n : ""; q.width = w; q.flags = fl; m.p.push_back(q); return RTX_OK; }
    RTX_RESULT setBoolParameter(const char* n, RTBOOL d, short fl = 0) { add(mock::P_BOOL, n, 0, fl); m.p.back().b = d != 0; return RTX_OK; }
    RTX_RESULT setIntegerParameter(const char* n, int d, short w = 0, short fl = 0) { add(mock::P_INT, n, w, fl); m.p.back().i = d; return RTX_OK; }
    RTX_RESULT setFloatParameter(const char* n, float d, short w = 0, short fl = 0) { add(mock::P_FLOAT, n, w, fl); m.p.back().f = d; return RTX_OK; }
    RTX_RESULT setColorParameter(const char* n, COLOR d, short w = 0, short fl = 0) { add(mock::P_COLOR, n, w, fl); m.p.back().c = d; return RTX_OK; }
    RTX_RESULT setListParameter(const char* n, short d, const char* ch, short w = 0, short fl = 0) { add(mock::P_LIST, n, w, fl); m.p.back().list = d; int c = 1; for (const char* x = ch; x && *x; ++x) if (*x == ';') ++c; m.p.back().nList = c; return RTX_OK; }
    RTX_RESULT setLabelParameter(const char* t, short w = 0, short fl = 0) { return add(mock::P_LABEL, t, w, fl); }
    mock::Param* at(int i) { return (i >= 0 && i < (int)m.p.size()) ? &m.p[(size_t)i] : nullptr; }
    // reads: real only inside the window
    RTBOOL isBoxChecked(int i) { mock::Param* q = at(i); return (q && m.dialog && q->type == mock::P_BOOL && q->b) ? 1 : 0; }
    int getIntegerValue(int i) { mock::Param* q = at(i); return (q && m.dialog && q->type == mock::P_INT) ? q->i : 0; }   // a colour row reads 0
    float getFloatValue(int i) { mock::Param* q = at(i); return (q && m.dialog && q->type == mock::P_FLOAT) ? q->f : 0.0f; }
    COLOR getParameterColor(int i) { mock::Param* q = at(i); return (q && m.dialog && q->type == mock::P_COLOR) ? q->c : 0; }
    int getListIndex(int i) { mock::Param* q = at(i); return (q && m.dialog && q->type == mock::P_LIST) ? q->list : -1; }
    RTBOOL isEnabled(int i) { mock::Param* q = at(i); return q && q->enabled ? 1 : 0; }
    // writes (the plugin filling the window)
    RTX_RESULT checkBox(int i, RTBOOL b) { mock::Param* q = at(i); if (!q || q->type != mock::P_BOOL) return RTX_FAIL; if (!m.ignoreFill) q->b = b != 0; return RTX_OK; }
    void setIntegerValue(int i, int v) { mock::Param* q = at(i); if (q && q->type == mock::P_INT && !m.ignoreFill) q->i = v; }   // a colour row ignores it
    void setFloatValue(int i, float v) { mock::Param* q = at(i); if (q && q->type == mock::P_FLOAT && !m.ignoreFill) q->f = v; }
    RTX_RESULT setParameterColor(int i, COLOR c) { mock::Param* q = at(i); if (!q || q->type != mock::P_COLOR) return RTX_FAIL; if (!m.ignoreFill) q->c = c; return RTX_OK; }
    RTX_RESULT setListIndex(int i, int v = -1) { mock::Param* q = at(i); if (!q || q->type != mock::P_LIST || v < -1 || v >= q->nList) return RTX_FAIL; if (!m.ignoreFill) q->list = v; return RTX_OK; }
    RTX_RESULT enableParameter(int i, RTBOOL e) { mock::Param* q = at(i); if (!q) return RTX_FAIL; q->enabled = e != 0; return RTX_OK; }
    // the user acting in the window
    void userCheck(int i, bool b) { at(i)->b = b; }
    void userColor(int i, COLOR c) { at(i)->c = c; }
    void userInt(int i, int v) { at(i)->i = v; }
    void userList(int i, int v) { at(i)->list = v; }
};
