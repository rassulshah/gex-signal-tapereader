// scen.h - per-market box content for the 2.17.9 layout tests (his 11-04 status files: Exp / Act text, sweep rows with today's price formats)
#include <string>
#include <vector>
struct Mk { const char* code; const char* hi[6]; const char* lo[6]; const char* e1; const char* a2; const char* a2s; };
static const Mk MKS[6] = {
 { "ES", {"7,692.50","7,713.00 - 7,720.50","7,751.25","7,757.50","7,767.25","7,773.75"}, {"7,664.25","7,664.00 - 7,665.25","below open","below open","7,725.25","7,630.50"}, "8:33 - 9:12", "HOD so far 10:57", "ONH \xB7 CR" },
 { "NQ", {"28,449.50","28,512.25 - 28,560.00","28,301.25","28,355.75","28,610.00","28,449.50"}, {"28,402.25","28,180.50 - 28,240.75","below open","below open","28,120.25","27,990.00"}, "8:33 - 8:54", "HOD so far 9:09", "PFH \xB7 CR" },
 { "CL", {"61.42","61.55 - 61.80","60.98","61.10","62.05","62.40"}, {"60.22","59.95 - 60.10","above open","60.40","59.70","59.10"}, "8:03 - 8:24", "HOD so far 9:15", "PDH" },
 { "NG", {"3.456","3.471 - 3.502","3.440","3.452","3.520","3.588"}, {"3.391","3.360 - 3.377","above open","3.402","3.330","3.290"}, "8:03 - 8:21", "LOD so far 10:12", "ONL \xB7 G1" },
 { "HG", {"5.1235","5.1410 - 5.1625","5.1150","5.1205","5.1880","5.2100"}, {"5.0810","5.0605 - 5.0740","above open","5.0905","5.0410","5.0005"}, "7:13 - 7:42", "HOD so far 9:51", "LonHI" },
 { "GC", {"4,012.30","4,021.10 - 4,035.80","4,001.50","4,008.20","4,044.90","4,061.00"}, {"3,980.40","3,962.10 - 3,975.60","above open","3,990.10","3,951.30","3,930.00"}, "7:23 - 7:45", "HOD so far 8:57", "PDH \xB7 CR" } };
static hl::View mkView(int m, bool sw = true) {
    const Mk& K = MKS[m]; hl::View v; v.has = true; v.prior = false;
    v.headLeftSegs.push_back(hl::Seg{ std::string(K.code) + " RTH 8:30-3:00", hl::C_SLATE }); v.trendReady = true; v.trendLvl = 0;
    v.sessSegs.push_back(hl::Seg{ "Regime Positive", hl::C_GREEN }); v.sessSegs.push_back(hl::Seg{ "Expiry 3:00 PM", hl::C_SLATE });
    v.E[hl::K_1ST] = hl::mkCell(K.e1, hl::C_BLUE); v.E[hl::K_1ST].s2 = "60%"; v.headTail = "HOD after 11:30 56%";
    v.A[hl::K_1ST] = hl::mkCell("8:36", hl::C_SLATE); v.A[hl::K_1ST].s2 = "LOD"; v.A[hl::K_1ST].col2 = hl::C_GREEN; v.headMain = "LOD IN 89%";
    v.A[hl::K_2ND] = hl::mkCell(K.a2, hl::C_SLATE); v.A[hl::K_2ND].s2 = K.a2s;
    v.hdrLeft.push_back(hl::Seg{ "RTH 44% done", hl::C_SLATE }); v.hdrLeft.push_back(hl::Seg{ "Bar 2:42 left", hl::C_SLATE });
    v.htf.push_back(hl::Seg{ "Dly:", hl::C_GREY }); v.htf.push_back(hl::Seg{ "Dn, Wk", hl::C_RED });
    v.htf.push_back(hl::Seg{ "Hrly:", hl::C_GREY }); v.htf.push_back(hl::Seg{ "Up, Strong", hl::C_GREEN });
    v.htf.push_back(hl::Seg{ "3m:", hl::C_GREY }); v.htf.push_back(hl::Seg{ "Side, Avg", hl::C_SLATE });
    v.tmOk = true; v.tmOpenMin = 510; v.tmCum[0] = 50; v.tmCum[1] = 67; v.tmCum[2] = 84; v.tmCum[3] = 100;
    v.swOn = sw;
    const char* nH[6] = { "PHH", "ON3", "DPoc", "VAH", "PDH", "PFH" }; const char* nL[6] = { "PHL", "ON3", "DPoc", "VAL", "PDL", "PFL" };
    if (sw) for (int i = 0; i < 6; i++) { v.swHi.push_back(hl::View::SwRow{ nH[i], K.hi[i], std::to_string(46 - i * 5) + "%", true });
                                          v.swLo.push_back(hl::View::SwRow{ nL[i], K.lo[i], std::to_string(45 - i * 6) + "%", std::string(K.lo[i]).find("open") == std::string::npos }); }
    return v;
}
// one scenario's table items as text (the version string normalised) and its FNV-1a 64 hash - the golden_2178.txt file holds 2.17.8's
static std::string itemsText(const hl::Layout& L) {
    std::string o; char b[160];
    std::snprintf(b, sizeof b, "ok %d sw %d box %d %d %d %d\n", L.tableOk, L.swOk, L.table.l, L.table.t, L.table.r, L.table.b); o += b;
    for (const hl::Item& it : L.items) if (it.role == hl::R_TABLE) { std::string s = it.s; if (s.size() > 2 && s[0] == 'v' && s.compare(1, 5, "2.17.") == 0) s = "vVER";
        std::snprintf(b, sizeof b, "%d %d %d %d %d %u %u %d %d ", it.k, it.l, it.t, it.r, it.b, it.c1, it.c2, it.fs, (int)it.bold); o += b; o += s; o += "\n"; }
    return o;
}
static unsigned long long fnv(const std::string& s) { unsigned long long h = 1469598103934665603ULL; for (unsigned char c : s) { h ^= c; h *= 1099511628211ULL; } return h; }
