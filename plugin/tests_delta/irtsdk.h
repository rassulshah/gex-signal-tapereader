#ifndef DELTA_TEST_SDK_H
#define DELTA_TEST_SDK_H
// TEST DOUBLE ONLY. Not a replacement for the supplied Linnsoft SDK.
#include <algorithm>
#include <ctime>
#include <cstring>
#include <string>
#include <vector>
#include <climits>
#include <cfloat>
#define DIR_SEPARATOR_CHR '/'
typedef unsigned long RTDATE;
typedef unsigned long COLOR;
enum { RTX_OK=0, RTX_FAIL=-1, POST_DRAWING=1, OVERLAY=2, NO_UI=4, INSTRUMENT_SCALE=8, VAP_REQUIRED=16, kParmAppendSameLine=1, SYM_TICKINCR=171 };
enum iARRAY { barDateTime,barVolume };
enum fARRAY { barOpen,barHigh,barLow,barClose };
enum pARRAY { barVolumeProfile };
namespace mock {
struct Price { float price; long buy,sell,total; Price(float p=100,long b=1,long s=0,long v=1):price(p),buy(b),sell(s),total(v){} };
struct Bar {
    unsigned long time,volume; float open,high,low,close;
    std::vector<Price> rows;
    int failStats,failPrice,reportedPrices;
    Bar():time(0),volume(0),open(100),high(101),low(99),close(100),failStats(0),failPrice(-1),reportedPrices(-1){}
};
struct Draw { std::string type,text; int x,y; COLOR color; Draw(const char* t,int a,int b,const std::string& s="",COLOR c=0):type(t),text(s),x(a),y(b),color(c){} };
struct Host {
    std::vector<Bar> bars; void* data; bool noSlot; float tick; int seconds;
    std::string symbol,root,period,chart,directory;
    int parms[3]; int rowCalls,statsCalls; std::vector<Draw> draws;
    int paneTop,paneBottom,paneLeft,paneRight,scaleLeft;
    COLOR ink;
    Host():data(NULL),noSlot(false),tick(1),seconds(60),symbol("ESZ26"),root("ES"),period("1 min"),chart("test"),rowCalls(0),statsCalls(0),paneTop(0),paneBottom(1000),paneLeft(0),paneRight(2000),scaleLeft(1980),ink(0){parms[0]=50;parms[1]=0;parms[2]=9;}
};
extern Host* current;
}
class cppExtension {
public:
    enum { HELVETICA=1,BOLD=1,PLAIN=0,P_SOLID=0,PAT_SOLID=0,PAT_HOLLOW=1,DRAW_OPAQUE=0,kBarCenter=0 };
    struct FONT { int id,size,style; };
    struct PNT {
        short h,v;
        void set(int bar,float price,int=0) { h=static_cast<short>(50+bar*10); v=static_cast<short>(mock::current->paneBottom-(price-80)*20); }
        void setDrawPosition(){} void drawLineTo(){ mock::current->draws.push_back(mock::Draw("line",h,v)); }
    };
    struct RCT {
        short left,top,right,bottom;
        void set(short l,short t,short r,short b){left=l;top=t;right=r;bottom=b;}
        void getPaneRect(bool){set(mock::current->paneLeft,mock::current->paneTop,mock::current->paneRight,mock::current->paneBottom);}
        void getScaleRect(){set(mock::current->scaleLeft,0,mock::current->paneRight,mock::current->paneBottom);}
        void draw(int,COLOR,COLOR,int,int){ mock::current->draws.push_back(mock::Draw("box",left,top)); }
        void drawText(const char* t,bool,bool rightAligned){ mock::current->draws.push_back(mock::Draw("text",rightAligned?right:left,(top+bottom)/2,t,mock::current->ink)); }
        void drawOval(int){ mock::current->draws.push_back(mock::Draw("ring",(left+right)/2,(top+bottom)/2)); }
    };
    struct CBRUSH { CBRUSH(COLOR,int){} void set(){} };
    struct VOLPROFILE { float price; long buyVolume,sellVolume,totalVolume,tickCount,maxDelta,minDelta; };
    struct BARSTATISTICS { long buyVolume,sellVolume,totalVolume; short prices; };
    struct RTARRAYI {
        iARRAY type; long count;
        RTARRAYI(iARRAY t):type(t),count(static_cast<long>(mock::current->bars.size())){}
        unsigned long& operator[](int i){return type==barDateTime?mock::current->bars.at(i).time:mock::current->bars.at(i).volume;}
    };
    struct RTARRAY {
        fARRAY type; long count;
        RTARRAY(fARRAY t):type(t),count(static_cast<long>(mock::current->bars.size())){}
        float& operator[](int i){mock::Bar& b=mock::current->bars.at(i); return type==barOpen?b.open:type==barHigh?b.high:type==barLow?b.low:b.close;}
    };
    struct RTARRAYP {
        long count;
        RTARRAYP(pARRAY):count(static_cast<long>(mock::current->bars.size())){}
        int getBarStatistics(int i,BARSTATISTICS& s){
            ++mock::current->statsCalls; mock::Bar& b=mock::current->bars.at(i);
            if(b.failStats)return RTX_FAIL;
            s.buyVolume=0;s.sellVolume=0;s.totalVolume=0;
            for(const mock::Price& p:b.rows){s.buyVolume+=p.buy;s.sellVolume+=p.sell;s.totalVolume+=p.total;}
            s.prices=static_cast<short>(b.reportedPrices<0?b.rows.size():b.reportedPrices);return RTX_OK;
        }
        int getVolumeProfile(int i,int k,VOLPROFILE& v){
            ++mock::current->rowCalls;mock::Bar& b=mock::current->bars.at(i);
            if(b.failPrice==k)return RTX_FAIL;
            const mock::Price& p=b.rows.at(k);v.price=p.price;v.buyVolume=p.buy;v.sellVolume=p.sell;v.totalVolume=p.total;return RTX_OK;
        }
    };
    virtual int init(); virtual int setup(); virtual int calc(int); virtual int done(); virtual int destroy();
    virtual int draw(){return RTX_FAIL;} virtual int parmsLoad(){return RTX_FAIL;} virtual int parmsApply(){return RTX_FAIL;} virtual int parmsUpdt(unsigned int){return RTX_FAIL;}
    void* getUserData(){return mock::current->data;} void setUserData(void* p){if(!mock::current->noSlot)mock::current->data=p;}
    long getBarCount(){return static_cast<long>(mock::current->bars.size());}
    const char* getRootSymbol(char* b=NULL,const char* =NULL){if(b){std::strcpy(b,mock::current->root.c_str());return b;}return mock::current->root.c_str();} const char* getSymbol(){return mock::current->symbol.c_str();}
    const char* getPeriodicityLabel(){return mock::current->period.c_str();} const char* getChartLabel(){return mock::current->chart.c_str();}
    const char* getWorkingDirectoryPath(){return mock::current->directory.c_str();}
    float getProperty(int){return mock::current->tick;} int getSecondsPerBar(){return mock::current->seconds;}
    tm* getLocaltime(RTDATE stamp,tm* t){time_t v=static_cast<time_t>(stamp);return gmtime_r(&v,t);}
    void setParameterVersion(int){} void setParameterDialogHeight(int){} void setIntegerParameter(const char*,int,int,int=0){}
    int getIntegerValue(int i){return mock::current->parms[i];} void setIntegerValue(int i,int v){mock::current->parms[i]=v;}
    void setFont(FONT){} void setTextColor(COLOR c){mock::current->ink=c;}
    int getTextWidth(const char* t,int){return static_cast<int>(std::strlen(t))*6;}
    void setPen(COLOR,int,int){} void setArrayCount(int){} void setFlags(unsigned long){} void setDescription(const char*){} void setVersion(const char*){}
};
#endif
