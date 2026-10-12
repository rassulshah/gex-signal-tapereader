// test_dealerprofile_labels.cpp -- 2.7.3: key-level labels centred, stepping beside a label on the same row, never past maxR
//   g++ -std=c++17 -I. test_dealerprofile_labels.cpp -o t && ./t   (the function is pasted from DealerProfile.cpp by the build check)
#include <vector>
#include <cstdio>
#include <cstddef>
struct LblBox { short l, t, r, b; };
static short placeLabel(short want, short w, short t, short b, const std::vector<LblBox>& taken, short minL, short maxR)
{
    auto clash = [&](short l, size_t* who) {
        for (size_t i = 0; i < taken.size(); ++i) {
            const LblBox& x = taken[i];
            if (b <= x.t || t >= x.b) continue;                    // another row
            if (l + w + 3 <= x.l || l >= x.r + 3) continue;        // clear of it (3 px gap)
            if (who) *who = i; return true;
        }
        return false;
    };
    short l = want; size_t who = 0;
    for (int k = 0; k < 8 && clash(l, &who); ++k) l = (short)(taken[who].r + 6);   // step right past each box on the row
    if (l + w <= maxR && !clash(l, nullptr)) return l;
    l = want;
    for (int k = 0; k < 8 && clash(l, &who); ++k) l = (short)(taken[who].l - 6 - w); // else step left
    if (l < minL) l = minL;
    return l;
}

static int f=0,p=0;
#define CHECK(c,m) do{ if(c) ++p; else { ++f; std::printf("FAIL %s\n", m);} }while(0)
int main(){
    std::vector<LblBox> T;
    CHECK(placeLabel(400,100,90,104,T,2,765)==400,"empty row: centred");
    T.push_back(LblBox{440,92,600,106});                       // the MenthorQ "90.00 G1" box on the same row
    short l=placeLabel(400,100,90,104,T,2,765);
    CHECK(l>=606,"same row: steps right of the MenthorQ label");
    CHECK(l+100<=765,"and stays left of the profiles");
    T.push_back(LblBox{l,90,(short)(l+100),104});
    short l2=placeLabel(400,200,90,104,T,2,765);             // no room on the right: goes left
    CHECK(l2+200<=440-3 && l2>=2,"no room right: placed left of the boxes");
    CHECK(placeLabel(400,100,200,214,T,2,765)==400,"another row: centred, untouched");
    std::printf("%d passed, %d failed\n",p,f); return f!=0;
}
