// test_dealerprofile_labels.cpp -- level badge placement (2.7.3 centred / stepping beside a label; 2.7.4 off the price bars,
// inside minL..maxR, the wider "<price> <NAME>  <odds>" badges)
//   g++ -std=c++17 -Wall -Wextra -I. test_dealerprofile_labels.cpp -o t && ./t   (the function lives in DealerProfileAuditLogic.h)
#include "DealerProfileAuditLogic.h"
#include <vector>
#include <cstdio>
using dpaudit::LblBox;
using dpaudit::placeLabel;

static int f=0,p=0;
#define CHECK(c,m) do{ if(c) ++p; else { ++f; std::printf("FAIL %s (line %d)\n", m, __LINE__);} }while(0)
static bool clear(short l, short w, short t, short b, const std::vector<LblBox>& T)
{
    for (const LblBox& x : T) if (!(b <= x.t || t >= x.b) && !(l + w + 3 <= x.l || l >= x.r + 3)) return false;
    return true;
}
int main(){
    std::vector<LblBox> T;
    // ---- 2.7.3 behaviour kept
    CHECK(placeLabel(400,100,90,104,T,2,765)==400,"empty row: centred");
    T.push_back(LblBox{440,92,600,106});                       // the MenthorQ "90.00 G1" box on the same row
    short l=placeLabel(400,100,90,104,T,2,765);
    CHECK(l>=606,"same row: steps right of the MenthorQ label");
    CHECK(l+100<=765,"and stays left of the profiles");
    T.push_back(LblBox{l,90,(short)(l+100),104});
    short l2=placeLabel(400,200,90,104,T,2,765);             // no room on the right: goes left
    CHECK(l2+200<=440-3 && l2>=2,"no room right: placed left of the boxes");
    CHECK(clear(l2,200,90,104,T),"and the left spot is really free (2.7.3 could return a clashing spot)");
    CHECK(placeLabel(400,100,200,214,T,2,765)==400,"another row: centred, untouched");

    // ---- 2.7.4: price bars are boxes too - a badge never covers a candle
    {
        std::vector<LblBox> B;
        for (int x = 100; x <= 700; x += 6) B.push_back(LblBox{(short)(x-2), 80, (short)(x+2), 120});   // bars trading across the row 90..104
        short a = placeLabel(380, 160, 90, 104, B, 110, 900);
        CHECK(a > 700 && a + 160 <= 900, "busy row: the badge goes past the last bar, left of the profiles");
        CHECK(clear(a,160,90,104,B), "and touches no bar");
        std::vector<LblBox> C;                                   // bars only where price traded: a gap 300..460 at the centre
        for (int x = 100; x <= 700; x += 6) if (x < 300 || x > 460) C.push_back(LblBox{(short)(x-2), 80, (short)(x+2), 120});
        short c = placeLabel(320, 120, 90, 104, C, 110, 900);
        CHECK(c >= 300 && c + 120 <= 460 && clear(c,120,90,104,C), "a gap among the bars near the centre is used");
        std::vector<LblBox> D;                                   // bars far above / below the row are no obstacle
        for (int x = 100; x <= 700; x += 6) D.push_back(LblBox{(short)(x-2), 200, (short)(x+2), 260});
        CHECK(placeLabel(380, 160, 90, 104, D, 110, 900) == 380, "bars not at the level's price: centred");
    }
    // ---- 2.7.4: wider badges ("6850.25 PDH  33%  /  50%") still step correctly and respect both edges
    {
        std::vector<LblBox> E{ LblBox{300, 92, 520, 106} };      // a MenthorQ badge already centred on the row
        short w = 230;                                            // the key-level badge, wider now that it carries its price
        short a = placeLabel(295, w, 90, 104, E, 110, 760);       // right side 526..756 fits
        CHECK(a == 526 && a + w <= 760 && clear(a,w,90,104,E), "wide badge: right of the MenthorQ badge");
        short b = placeLabel(295, w, 90, 104, E, 110, 700);       // right does not fit: left 64 < minL 110 -> no free spot
        CHECK(b >= 110 && b + w <= 700, "no free spot: stays inside minL..maxR (never into the Liquidity / Delta / Dealer columns)");
        short c = placeLabel(800, 100, 300, 314, {}, 110, 760);
        CHECK(c + 100 <= 760, "a centre right of maxR is pulled back left of the profiles");
        short d = placeLabel(20, 100, 300, 314, {}, 110, 760);
        CHECK(d >= 110, "a centre left of minL is pushed right of the Liquidity Profile column");
    }
    std::printf("%d passed, %d failed\n",p,f); return f!=0;
}
