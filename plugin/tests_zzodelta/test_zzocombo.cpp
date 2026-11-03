// test_zzocombo.cpp - lsZzoCombo 2.0.0 (ZzoDeltaLogic.h 1.3.2 + ZzoComboView.h) on real IRT data (ES Oct 26 - Nov 2, GC Oct 1 - 7, full
// sessions, footprint extremes): every wave's count, cap, Ab rule, pivot extreme share, effort / result and heavy-Ab flag must equal the
// independent Python model (lra.zzo_model 1.1.0, ref_zzc_*.txt); then the drawing: inside the pane, no tag on top of another, the ratio /
// count only on 2.0x+ waves, the count right-justified inside its wave, no zero counts, Ab only where showAb is on, no tags on the
// forming wave, labels dropped in short panes, the "no footprint" header.
#include "../ZzoComboView.h"
#include <cstdio>
#include <fstream>
#include <sstream>
static int pass = 0, fail = 0;
#define CHECK(c, m) do { if (c) ++pass; else { ++fail; std::printf("FAIL %s (line %d)\n", m, __LINE__); } } while (0)

static zzd::Engine feed(const char* csv, const char* mkt, double tick, bool foot = true) {
    zzd::Engine E; E.setMarket(mkt, 180);
    std::ifstream f(csv); std::string ln;
    while (std::getline(f, ln)) {
        if (ln.empty() || ln[0] == '#') continue;
        std::stringstream s(ln); std::string a; std::vector<std::string> x; while (std::getline(s, a, ',')) x.push_back(a);
        zzd::Bar b; b.t = std::stoll(x[0]); zzd::toTicks(std::stod(x[1]), tick, b.h); zzd::toTicks(std::stod(x[2]), tick, b.l);
        b.vol = std::stoll(x[3]); b.dlt = std::stoll(x[4]); b.vap = foot;
        if (foot) { b.xh = std::stoll(x[5]); b.xl = std::stoll(x[6]); b.xok = x[7] == "1"; }
        E.addBar(b);
    }
    return E;
}
static std::string line(const zzd::Wave& w) {
    char xs[32], es[32]; double er = 0;
    if (w.xsh < 0) std::snprintf(xs, sizeof xs, "na"); else std::snprintf(xs, sizeof xs, "%.4f", w.xsh);
    if (zzd::effortResult(w, er)) std::snprintf(es, sizeof es, "%.3f", er); else std::snprintf(es, sizeof es, "na");
    char b[160]; std::snprintf(b, sizeof b, "%d,%d,%d,%d,%d,%s,%s,%d", w.e, w.dir, w.cnt, (int)w.cntCap, w.ab, xs, es, (int)zzd::abHeavy(w)); return b;
}
static zzd::View view(const zzd::Engine& E, int height, int from) {
    zzd::View v; v.l = 0; v.t = 0; v.r = 9 * ((int)E.B.size() - from) + 20; v.b = height; v.first = from; v.last = (int)E.B.size() - 1; v.bw = 9;
    v.X = [from](int j) { return (j - from) * 9.0 + 5; }; v.textW = [](const std::string& t) { return (int)t.size() * 6; }; v.textH = 11;
    return v;
}
static void model(const char* csv, const char* ref, const char* mkt, double tick) {
    const zzd::Engine E = feed(csv, mkt, tick);
    std::ifstream r(ref); std::string ln; std::vector<std::string> want; while (std::getline(r, ln)) if (!ln.empty()) want.push_back(ln);
    int same = 0, shown = 0;
    for (size_t k = 0; k < E.W.size() && k < want.size(); ++k) { const std::string g = line(E.W[k]); if (g == want[k]) ++same; else if (shown++ < 5) std::printf("  %s wave %zu\n    got  %s\n    want %s\n", csv, k, g.c_str(), want[k].c_str()); }
    std::printf("  %s: %zu waves, %d match the Python model\n", csv, E.W.size(), same);
    CHECK(E.W.size() == want.size() && same == (int)want.size(), "count / extreme / E/R / Ab = the Python model");
}
static void drawing(const char* csv, const char* mkt, double tick) {
    zzd::Engine E = feed(csv, mkt, tick);
    const int from = std::max(0, (int)E.B.size() - 400);
    zzd::Bar form = E.B.back(); form.t += 180; form.vol = E.B.back().vol;   // a forming bar after the last one
    const std::vector<zzd::Wave> waves = E.view(&form);
    for (int H : { 260, 150, 60, 40 }) {
        const zzd::View v = view(E, H, from);
        const zzd::ComboScene S = zzd::layoutCombo(E, waves, form.vol, v, "ZZO Combo 2.0.0");
        bool in = true, noClash = true, noZero = true; int tagBoxes = 0;
        std::vector<const zzd::Prim*> boxes;
        for (const zzd::Prim& p : S.p) {
            if (p.y0 < v.t || p.y1 > v.b || p.x0 < v.l - 1 || p.x1 > v.r + 1) in = false;
            if (p.k == zzd::Prim::TEXT && p.text == "0") noZero = false;
            if (p.k == zzd::Prim::FILL && p.y1 - p.y0 >= v.textH - 1 && p.col != 0x22C55E && p.col != 0xEF4444 && p.col != 0x15803D && p.col != 0xB91C1C) boxes.push_back(&p);   // tag boxes
        }
        // tag boxes are the fills right before a white TEXT that is a tag
        boxes.clear();
        for (size_t i = 0; i + 1 < S.p.size(); ++i) { const zzd::Prim& a = S.p[i]; const zzd::Prim& t = S.p[i + 1];
            if (a.k == zzd::Prim::FILL && t.k == zzd::Prim::TEXT && t.col == 0xFFFFFF && (t.text == "Cmx" || t.text == "AR" || t.text == "ST" || t.text == "CoB" || t.text == "weak" || t.text.rfind("Ab ", 0) == 0)) boxes.push_back(&a); }
        tagBoxes = (int)boxes.size();
        for (size_t i = 0; i < boxes.size(); ++i) for (size_t j = i + 1; j < boxes.size(); ++j) {
            const zzd::Prim& a = *boxes[i]; const zzd::Prim& b = *boxes[j];
            if (a.x0 <= b.x1 && b.x0 <= a.x1 && a.y0 <= b.y1 && b.y0 <= a.y1) noClash = false; }
        char m[96];
        std::snprintf(m, sizeof m, "%s %dpx: everything inside the pane", mkt, H); CHECK(in, m);
        std::snprintf(m, sizeof m, "%s %dpx: no tag on top of another (%d tags)", mkt, H, tagBoxes); CHECK(noClash, m);
        std::snprintf(m, sizeof m, "%s %dpx: no zero count", mkt, H); CHECK(noZero, m);
        {   // (2.0.1) no column is drawn after any label / tag (a later wave never covers them)
            size_t lastCol = 0, firstText = S.p.size();
            for (size_t i = 0; i < S.p.size(); ++i) { const zzd::Prim& p = S.p[i];
                if (p.k == zzd::Prim::FILL && (p.col == 0x22C55E || p.col == 0xEF4444 || p.col == 0x15803D || p.col == 0xB91C1C) && p.y1 == v.b - 3) lastCol = i;   // columns end at bot - 1 = v.b - 3 (tag boxes end higher)
                if (p.k == zzd::Prim::TEXT && firstText == S.p.size() && p.text.rfind("ZZO", 0) != 0) firstText = i; }
            std::snprintf(m, sizeof m, "%s %dpx: labels and tags drawn after every column", mkt, H); CHECK(firstText == S.p.size() || lastCol < firstText, m); }
        if (H < 70) { std::snprintf(m, sizeof m, "%s %dpx: ratio labels dropped", mkt, H); CHECK(S.ratios == 0, m); }
        if (H < 50) { std::snprintf(m, sizeof m, "%s %dpx: counts dropped", mkt, H); CHECK(S.counts == 0, m); }
        if (H == 260) {
            std::snprintf(m, sizeof m, "%s: ratios %d counts %d Ab %d Cmx %d CoB %d weak %d", mkt, S.ratios, S.counts, S.ab, S.cmx, S.cob, S.weak); std::printf("  %s\n", m);
            CHECK(S.ratios > 0 && S.counts > 0, "ratios and counts drawn in a tall pane");
            CHECK(E.showAb ? S.ab > 0 : S.ab == 0, "Ab drawn only where showAb is on");
            // every ratio label belongs to a 2.0x+ wave; every count sits inside its wave's right edge
            int bad = 0;
            for (const zzd::Wave& w : waves) {
                if (!w.hasV || w.e < v.first) continue;
                const double Vr = (double)w.vol / w.medV;
                char rb[24]; std::snprintf(rb, sizeof rb, "%.1fx", Vr);
                for (const zzd::Prim& p : S.p) if (p.k == zzd::Prim::TEXT && p.text == rb && Vr < zzd::CNT_MIN_VOL && std::fabs((p.x0 + p.x1) / 2.0 - v.X(w.e)) < 3) ++bad;
            }
            CHECK(bad == 0, "ratio only on 2.0x+ waves");
            // the forming wave (last in view) carries no tag
            const zzd::Wave& lw = waves.back();
            bool liveTag = false;
            for (size_t i = 0; i + 1 < S.p.size(); ++i) { const zzd::Prim& a = S.p[i]; const zzd::Prim& t = S.p[i + 1];
                if (a.k == zzd::Prim::FILL && t.k == zzd::Prim::TEXT && t.col == 0xFFFFFF && t.text != "" && !std::isdigit((unsigned char)t.text[0])) {
                    const double xm = (v.X(std::max(lw.s, v.first)) + v.X(lw.e)) / 2.0; if (lw.live && std::fabs((a.x0 + a.x1) / 2.0 - xm) < 1 && lw.e - lw.s >= 3) liveTag = true; } }
            CHECK(lw.live && !liveTag, "no tags on the forming wave");
        }
    }
    // no footprint: the volume still draws, Ab never does, the header says so (host builds the header; the engine has no xsh)
    zzd::Engine N = feed(csv, mkt, tick, false);
    const zzd::View v = view(N, 260, std::max(0, (int)N.B.size() - 400));
    const zzd::ComboScene S = zzd::layoutCombo(N, N.view(nullptr), 0, v, "ZZO Combo 2.0.0  no footprint - Ab off");
    CHECK(S.cols > 0 && S.ab == 0, "no footprint: volume drawn, no Ab");
    CHECK(N.noVap == (long long)N.B.size(), "no footprint counted on every bar");
}
int main() {
    model("zzc_es_1026_1102.csv", "ref_zzc_es_1026_1102.txt", "ES", 0.25);
    model("zzc_gc_1001_1007.csv", "ref_zzc_gc_1001_1007.txt", "GC", 0.1);
    { zzd::Engine E; E.setMarket("ES", 180); CHECK(E.showAb, "ES: Ab on"); E.setMarket("GC", 180); CHECK(!E.showAb, "GC: Ab off (logged)"); E.setMarket("6E", 180); CHECK(!E.showAb, "6E: Ab off"); }
    drawing("zzc_es_1026_1102.csv", "ES", 0.25);
    drawing("zzc_gc_1001_1007.csv", "GC", 0.1);
    std::printf("%d passed, %d failed\n", pass, fail); return fail ? 1 : 0;
}
