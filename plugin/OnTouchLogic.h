// OnTouchLogic.h - the overnight (outside RTH) touch odds shared by lsSessionVWAP 1.3 and lsDealerProfile 2.6, computed natively.
// (Rassul 2026-10-09 07:21-07:22 "you are still not showing the percentages on the vwap indicator" / "i also dont see the
// percentages on the levels like CR") The RTH odds come from the RTH-fitted law; outside RTH this is the method lra.on_vwap_study
// tests walk-forward every night. Shown with "?" until proven.
#pragma once
#include <cmath>

namespace otl {

// (1.3.0, Rassul 2026-10-09 07:21 "you are still not showing the percentages on the vwap indicator") OVERNIGHT touch odds,
// natively - the method lra.on_vwap_study tests walk-forward every night: sigma per minute from the last 20 closes of the
// chart (realised), the next 60 minutes at that pace (flat clock: the time-of-day curve added only 0.01 of skill), the same
// fat-tailed first-passage law as the RTH badges (15-node Gauss-Hermite, nu 0.35). Shown with "?" until the study calls the
// market "ready" (every well-filled 10% bin within 5 points on 20+ unseen sessions; ES was within 3 points on 2026-10-09).
inline double onSigmaMin(const double* closes, int n, int perSec)
{
    if (n < 16 || perSec <= 0) return 0.0;
    int a = n - 20 < 1 ? 1 : n - 20, k = 0; double s2 = 0;
    for (int i = a; i < n; i++) {
        double d = closes[i] - closes[i - 1];
        if (!std::isfinite(d)) return 0.0;
        s2 += d * d; k++;
    }
    if (k < 15) return 0.0;
    return std::sqrt(s2 / k / (perSec / 60.0));
}
inline double onTouch(double dist, double sigMin, int horizonMin = 60, double nu = 0.35)
{
    static const double Z[15] = { -6.363947888829839, -5.190093591304781, -4.1962077112690155, -3.2890824243987664, -2.432436827009758,
        -1.6067100690287297, -0.799129068324548, 0.0, 0.799129068324548, 1.6067100690287297, 2.432436827009758, 3.2890824243987664,
        4.1962077112690155, 5.190093591304781, 6.363947888829839 };
    static const double W[15] = { 8.589649899633252e-10, 5.975419597920599e-07, 5.642146405189029e-05, 0.001567357503549956,
        0.017365774492137616, 0.08941779539984437, 0.23246229360973225, 0.31825951825951815, 0.23246229360973225, 0.08941779539984437,
        0.017365774492137616, 0.001567357503549956, 5.642146405189029e-05, 5.975419597920599e-07, 8.589649899633252e-10 };
    if (!std::isfinite(dist) || !std::isfinite(sigMin)) return -1.0;
    double V = sigMin * sigMin * horizonMin;
    if (!(V > 0)) return std::fabs(dist) < 1e-12 ? 1.0 : 0.0;
    double r = std::sqrt(V), s = 0;
    for (int i = 0; i < 15; i++) s += W[i] * std::erfc(std::fabs(dist) / (std::sqrt(2.0) * r * std::exp(nu * Z[i] - 0.5 * nu * nu)));
    return s < 0 ? 0 : s > 1 ? 1 : s;
}

} // namespace otl
