// lsDealerProfile 2.28.0 / lsLiquidityProfile 0.58.0 (2026-10-27): the queue meter and the Q badge are not drawn, their rows are
// hidden, the queue record is still written. Build (Linux, from plugin/): g++ -std=c++17 -I. test_dealerprofile_2270.cpp -o t2270 && ./t2270
#include "SettingsSpec.h"
#include "DealerProfileSettings.h"
#include "LiquidityProfileSettings.h"
#include <fstream>
#include <sstream>
#include <iostream>
static std::string slurp(const char* p) { std::ifstream f(p); std::stringstream s; s << f.rdbuf(); return s.str(); }
int main() {
    int fail = 0; auto ck = [&](bool ok, const std::string& w) { std::cout << (ok ? "PASS " : "FAIL ") << w << "\n"; if (!ok) ++fail; };
    const ss::Spec& sp = dprset::spec();
    for (int k : { dprset::K_QUEUE_METER, dprset::K_QM_REACH, dprset::K_QM_BID, dprset::K_QM_ASK, dprset::K_QM_WARN })
        ck(sp.rows[k].hidden, std::string("DP row hidden: ") + sp.rows[k].key);
    ck(sp.paramVersion == 10, "DP parameter version 10 (rows hidden)");
    const std::string cpp = slurp("DealerProfile.cpp");
    ck(cpp.find("c.qm = false;") != std::string::npos, "DP: the meter is never on");
    ck(cpp.find("DP_VERSION = \"2.29.0\"") != std::string::npos && cpp.find("p->setVersion(\"2.29.0\")") != std::string::npos
       && std::string(dprset::PLUGIN_VERSION) == "2.29.0", "DP versions agree 2.29.0");
    const ss::Spec& lq = lpset::profileSpec(); bool hid = false;
    for (int k = 0; k < lq.n; ++k) if (std::string(lq.rows[k].key) == "lmz_far") hid = lq.rows[k].hidden;
    ck(hid, "LP row 'Queue Q at levels' hidden");
    const std::string lp = slurp("LiquidityProfile.cpp");
    ck(lp.find("if (false && S.lzSet.farOn)") != std::string::npos, "LP: the Q badge is never drawn");
    ck(lp.find("\\\\Strips\\\\QUEUE-") != std::string::npos, "LP: the queue Strip record is still written");
    ck(lp.find("#define LQ_VERSION \"0.60.1\"") != std::string::npos && lp.find("p->setVersion(\"0.60.1\")") != std::string::npos, "LP versions agree 0.60.1");
    std::cout << (fail ? "FAILED " : "ALL PASS ") << fail << "\n"; return fail != 0;
}
