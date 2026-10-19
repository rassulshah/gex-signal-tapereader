// (DP 2.18.0) the volume-at-price SDK calls DealerProfile's key levels now use (RTARRAYP(barVolumeProfile)): one row per bar at
// its close with the bar's volume. Include AFTER the test's Chart / CUR definitions (needs CUR->c, CUR->v).
cppExtension::RTARRAYP::RTARRAYP(pARRAY a) : tempRTLong(0), bVolumeProfile(true), pArray(&tempArrayP), pArrayNumber(a), count(0)
{ tempArrayP.data = nullptr; tempArrayP.size = 0; tempArrayP.width = 0; tempArrayP.element = 0; if (CUR) count = (long)CUR->c.size(); }
cppExtension::RTARRAYP::~RTARRAYP() {}
RTX_RESULT cppExtension::RTARRAYP::getBarStatistics(int bar, BARSTATISTICS& st)
{
    if (!CUR || bar < 0 || bar >= (int)CUR->c.size()) return (RTX_RESULT)-1;
    st.prices = 1; st.totalVolume = bar < (int)CUR->v.size() ? (long)CUR->v[(size_t)bar] : 0; return RTX_OK;
}
RTX_RESULT cppExtension::RTARRAYP::getVolumeProfile(int bar, int i, VOLPROFILE& r)
{
    if (!CUR || bar < 0 || bar >= (int)CUR->c.size() || i != 0) return (RTX_RESULT)-1;
    r.price = CUR->c[(size_t)bar]; r.totalVolume = bar < (int)CUR->v.size() ? (long)CUR->v[(size_t)bar] : 0; return RTX_OK;
}
