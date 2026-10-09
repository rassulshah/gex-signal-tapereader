#include <cassert>
#include <cmath>
#include <ctime>
#include <limits>

// Linux-only test adapter for the Windows SDK spelling used by the production source.
#define RT_LONG unsigned long long
static int tm_localtime_s(struct tm* out, const time_t* in)
{
    return localtime_r(in, out) ? 0 : 1;
}
#define localtime_s tm_localtime_s
#include "TradeManager.cpp"
#undef localtime_s

int main()
{
    double d = 0.0;
    assert(parseNum(" 1,234.50 ", d) && d == 1234.5);
    assert(!parseNum("12x", d));
    assert(!parseNum("nan", d));
    assert(!parseNum("inf", d));

    int i = 0;
    assert(parseInt(" 42 ", i) && i == 42);
    assert(!parseInt("4.2", i));
    assert(!parseInt("2147483648", i));

    assert(hmMin("00:00") == 0);
    assert(hmMin("23:59") == 1439);
    assert(hmMin("24:00") == -1);
    assert(hmMin("09:6x") == -1);
    assert(inWindow(23 * 60 + 45, 23 * 60 + 30, 30));
    assert(inWindow(15, 23 * 60 + 30, 30));
    assert(!inWindow(12 * 60, 23 * 60 + 30, 30));
    assert(minuteDelta(23 * 60 + 50, 10) == 20);
    assert(minuteDelta(10, 23 * 60 + 50) == -20);

    assert(std::fabs(tickOf("HG") - 0.0005) < 1e-12);
    assert(std::fabs(tickOf("EU") - 0.0001) < 1e-12);
    assert(std::fabs(microPV("HG") * tickOf("HG") - 1.25) < 1e-12);
    assert(std::fabs(microPV("EU") * tickOf("EU") - 1.25) < 1e-12);

    assert(sizeForRisk(49.9, 10.0, 1.0, 5) == 4);
    assert(sizeForRisk(50.0, 10.0, 1.0, 5) == 5);
    assert(sizeForRisk(1.0, 10.0, 1.0, 5) == 0);
    assert(sizeForRisk(std::numeric_limits<double>::max(), 1.0, 1.0, 5) == 5);
    assert(sizeForRisk(100.0, std::numeric_limits<double>::infinity(), 1.0, 5) == 0);

    TMState first, second;
    first.mkt = "ES";
    second.mkt = "NQ";
    first.stamps["guard"] = 1;
    assert(second.mkt == "NQ" && second.stamps.empty());
    return 0;
}
