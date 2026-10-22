#ifndef LQP_TEST_SDK_H
#define LQP_TEST_SDK_H
// TEST DOUBLE ONLY. (lsDealerHedge 0.1.0 tests: + DRAW_TRANSLUCENT, kBarLeft, FRONT_DRAWING, getBackgroundColor, timers that work, timer())
// Not a replacement for the supplied Linnsoft SDK.
// (lsLiquidityProfile 0.44.0 tests) = tests_lqp/irtsdk.h + the newer calls the plugin uses (bool / colour rows, check boxes, float values,
// timers) so the WHOLE plugin compiles and runs on the mock.
// (0.15.0 tests) the 0.14 LiquidityProfile mock + what OrderFlowFeed.h PART 2 needs (DEPTH_ORDER / DEPTH_BY_ORDER / RTBOOL /
// invalidateChart, the of140 mock's feed) + full draw records for the render check (rect, fill, border, pen, font) + a price
// scale (pxPer px per price unit) and an optional font-sized text width (textPx > 0: px per character per point).
#include <algorithm>
#include <ctime>
#include <cstring>
#include <string>
#include <vector>
#include <climits>
#include <cfloat>
#include <map>
#define DIR_SEPARATOR_CHR '/'
typedef unsigned long RTDATE;
typedef unsigned long COLOR;
typedef unsigned short RTBOOL;
typedef int RTX_RESULT;
enum BRUSH_STYLE { PAT_NONE, PAT_HOLLOW, PAT_SOLID };   // global, as in the real irtsdk.h
enum PEN_STYLE { P_SOLID, P_DOT, P_DASH };              // global, as in the real irtsdk.h
typedef struct DEPTH_ORDER { RTDATE timestamp; float price; float previousPrice; float bid, ask; unsigned long bidsize, asksize; unsigned long size; int orderID; int aggressorOrderID; unsigned char action; unsigned char buySell; } DEPTH_ORDER;
enum { FRONT_DRAWING=(1<<20), RTX_OK=0, RTX_FAIL=-1, POST_DRAWING=1, OVERLAY=2, NO_UI=4, INSTRUMENT_SCALE=8, VAP_REQUIRED=16, TRACK_MOUSE=(1<<27), kParmAppendSameLine=1, SYM_TICKINCR=171, CALL_CONTINUOUSLY=1 };
typedef struct DEPTH_LEVEL { float bid, ask; unsigned short bidsize, asksize; } DEPTH_LEVEL;
enum iARRAY { barDateTime,barVolume };
enum fARRAY { barOpen,barHigh,barLow,barClose,fEmptyArray };
inline std::map<std::string,float>& mockCustom(){ static std::map<std::string,float> m; return m; }   // (0.13.0) custom indicators' last values
enum pARRAY { barVolumeProfile };
namespace mock {
struct Price { float price; long buy,sell,total; Price(float p=100,long b=1,long s=0,long v=1):price(p),buy(b),sell(s),total(v){} };
struct Bar {
    unsigned long time,volume; float open,high,low,close;
    std::vector<Price> rows;
    int failStats,failPrice,reportedPrices;
    Bar():time(0),volume(0),open(100),high(101),low(99),close(100),failStats(0),failPrice(-1),reportedPrices(-1){}
};
struct Draw { std::string type,text; int x,y; COLOR color; int r=0,b=0; COLOR border=0; int width=0; int pat=0; int fid=0,fsize=0,fstyle=0; COLOR pen=0; int penW=0; int l=0,t=0; bool centred=false;
    Draw(const char* tp,int a,int b_,const std::string& s="",COLOR c=0):type(tp),text(s),x(a),y(b_),color(c){} };
struct Feed { bool subscribe = true; bool throwOnNext = false; std::vector<DEPTH_ORDER> ev; size_t at = 0; long long getNext = 0, opens = 0, live = 0, nullOpens = 0; std::string lastTicker;
    void push(unsigned char a, unsigned char side, float px, unsigned long size, RTDATE t, int id, int aggr = 0) { DEPTH_ORDER o; std::memset(&o, 0, sizeof(o)); o.timestamp = t; o.price = px; o.size = size; o.orderID = id; o.aggressorOrderID = aggr; o.action = a; o.buySell = side; ev.push_back(o); } };
extern Feed feed;
struct Host {
    std::vector<DEPTH_LEVEL> depth; bool depthOn=true; unsigned long now=0; int depthObjects=0;
    std::vector<Bar> bars; void* data; bool noSlot; float tick; int seconds;
    std::string symbol,root,period,chart,directory;
    int parms[3]; int rowCalls,statsCalls; std::vector<Draw> draws;
    float priceBase = 80; float pxPer = 20; int barW = 10; int barX0 = 50; double textPx = 0; int invalidates = 0;
    int paneTop,paneBottom,paneLeft,paneRight,scaleLeft; std::map<int,int> timers; bool timerRefuse = false; COLOR bg = 0;
    COLOR ink; COLOR pen = 0x00FFFFFF; int penW = 1; int fid = 0, fsize = 0, fstyle = 0;
    Host():data(NULL),noSlot(false),tick(1),seconds(60),symbol("ESZ26"),root("ES"),period("1 min"),chart("test"),rowCalls(0),statsCalls(0),paneTop(0),paneBottom(1000),paneLeft(0),paneRight(2000),scaleLeft(1980),ink(0){parms[0]=50;parms[1]=0;parms[2]=9;}
};
extern Host* current;
}
enum { E_MOUSE_UP = 7, E_MOUSE_MOVE, E_MOUSE_DBL, E_MOUSE_SCROLL, E_MOUSE_HOVER = 113 };
typedef struct RTX_EVENT { int type; union { struct { short v, h, shift, control; long bar; float price; } mouse; struct { int id; } timer; } v; } RTX_EVENT;
class cppExtension {
public:
    enum { COURIER=2,HELVETICA=1,BOLD=1,PLAIN=0,DRAW_OPAQUE=0,DRAW_TRANSLUCENT=1,kBarCenter=0,kBarRight=1,kBarLeft=2 };
    enum { DRAW_INVISIBLE=0, OUTPUT_NO_UI=2 };
    struct MARKET_DEPTH { int maxLevels; MARKET_DEPTH(const char*):maxLevels(static_cast<int>(mock::current->depth.size())){++mock::current->depthObjects;} ~MARKET_DEPTH(){--mock::current->depthObjects;} int available(){maxLevels=static_cast<int>(mock::current->depth.size());return mock::current->depthOn?1:0;} DEPTH_LEVEL& operator[](unsigned n){return mock::current->depth.at(n);} };
    class DEPTH_BY_ORDER {
    public:
        DEPTH_BY_ORDER(const char* ticker = NULL) { index = 0; std::memset(symbol, 0, sizeof(symbol)); std::memset(&dbo, 0, sizeof(dbo)); ++mock::feed.opens; ++mock::feed.live; if (!ticker) ++mock::feed.nullOpens; else { mock::feed.lastTicker = ticker; std::strncpy(symbol, ticker, 23); } bDboSubscribed = mock::feed.subscribe ? 1 : 0; }
        ~DEPTH_BY_ORDER() { --mock::feed.live; }
    private:
        unsigned int index;
        RTBOOL bDboSubscribed;
    public:
        char symbol[24];
        DEPTH_ORDER dbo;
        RTX_RESULT getNext(void) {
            ++mock::feed.getNext;
            if (mock::feed.throwOnNext) throw 1;
            if (!bDboSubscribed) { if (mock::feed.subscribe) bDboSubscribed = 1; return RTX_FAIL; }
            if (mock::feed.at >= mock::feed.ev.size()) { std::memset(&dbo, 0, sizeof(dbo)); return RTX_FAIL; }
            dbo = mock::feed.ev[mock::feed.at++]; return RTX_OK;
        }
    };
    struct FONT { int id,size,style; };
    struct PNT {
        short h,v;
        int set(int bar,float price,int pos=0) { h=static_cast<short>(mock::current->barX0+bar*mock::current->barW+(pos==1?mock::current->barW/2:pos==2?-mock::current->barW/2:0)); v=static_cast<short>(mock::current->paneBottom-(price-mock::current->priceBase)*mock::current->pxPer); return 0; }
        void setDrawPosition(){} void drawLineTo(){ mock::Draw d("line",h,v); d.pen=mock::current->pen; d.penW=mock::current->penW; mock::current->draws.push_back(d); }   // (LP 0.48.0 tests) the pen of a line
    };
    struct RCT {
        short left,top,right,bottom;
        void set(short l,short t,short r,short b){left=l;top=t;right=r;bottom=b;}
        void getPaneRect(bool){set(mock::current->paneLeft,mock::current->paneTop,mock::current->paneRight,mock::current->paneBottom);}
        void getScaleRect(){set(mock::current->scaleLeft,0,mock::current->paneRight,mock::current->paneBottom);}
        void draw(int w,COLOR border,COLOR fill,int mode,int pat){ mock::Draw d("box",left,top,"",fill); d.fid=mode; d.l=left; d.t=top; d.r=right; d.b=bottom; d.border=border; d.width=w; d.pat=pat; d.pen=mock::current->pen; d.penW=mock::current->penW; mock::current->draws.push_back(d); }
        void drawText(const char* tx,bool centred,bool rightAligned){ mock::Draw d("text",rightAligned?right:left,(top+bottom)/2,tx,mock::current->ink); d.l=left; d.t=top; d.r=right; d.b=bottom; d.centred=centred; d.fid=mock::current->fid; d.fsize=mock::current->fsize; d.fstyle=mock::current->fstyle; mock::current->draws.push_back(d); }
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
        bool custom=false; float cv=0;
        RTARRAY(fARRAY t):type(t),count(t==fEmptyArray?0:static_cast<long>(mock::current->bars.size())){}
        int getCustomIndicator(char* n,int=1,bool=false,const char* =nullptr,int=0){ auto it=mockCustom().find(n); if(it==mockCustom().end()) return RTX_FAIL; custom=true; cv=it->second; count=1; return RTX_OK; }
        float& operator[](int i){ if(custom) return cv; mock::Bar& b=mock::current->bars.at(i); return type==barOpen?b.open:type==barHigh?b.high:type==barLow?b.low:b.close;}
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
    virtual int draw(){return RTX_FAIL;} virtual int mouse(RTX_EVENT*){return RTX_FAIL;} virtual int parmsLoad(){return RTX_FAIL;} virtual int parmsApply(){return RTX_FAIL;} virtual int parmsUpdt(unsigned int){return RTX_FAIL;}
    void* getUserData(){return mock::current->data;} void setUserData(void* p){if(!mock::current->noSlot)mock::current->data=p;}
    long getBarCount(){return static_cast<long>(mock::current->bars.size());}
    const char* getRootSymbol(char* b=NULL,const char* =NULL){if(b){std::strcpy(b,mock::current->root.c_str());return b;}return mock::current->root.c_str();} const char* getSymbol(){return mock::current->symbol.c_str();}
    const char* getPeriodicityLabel(){return mock::current->period.c_str();} const char* getChartLabel(){return mock::current->chart.c_str();}
    const char* getWorkingDirectoryPath(){return mock::current->directory.c_str();}
    float getProperty(int){return mock::current->tick;} RTDATE currentDate(){return mock::current->now;} void setExtendedFlags(unsigned long){} int getSecondsPerBar(){return mock::current->seconds;}
    tm* getLocaltime(RTDATE stamp,tm* t){time_t v=static_cast<time_t>(stamp);return gmtime_r(&v,t);}
    // (settings layer 1.1.0) TEXT rows
    void setStringParameter(const char*, const char*, size_t, int = 0, int = 0) { ++nparm(); }
    int getParameterText(int, char* t, size_t n) { if (t && n) t[0] = 0; return RTX_OK; } int setParameterText(int, const char*) { return RTX_OK; }
    // (LP 0.45.0 settings layer) SettingsHost.h: rows counted per DLL; label / enable calls
    static int& nparm(){ static int n=0; return n; } int getParameterCount(){ return nparm(); }
    void setLabelParameter(const char*,int=0,int=0){ ++nparm(); } int enableParameter(int,RTBOOL){ return 0; } int setListIndex(int,int=-1){ return 0; }
    void setParameterVersion(int){} void setParameterDialogHeight(int){} void setIntegerParameter(const char*,int,int,int=0){ ++nparm(); }
    int getIntegerValue(int i){return mock::current->parms[i];} void setIntegerValue(int i,int v){mock::current->parms[i]=v;}
    void setFont(FONT f){mock::current->fid=f.id;mock::current->fsize=f.size;mock::current->fstyle=f.style;} void setTextColor(COLOR c){mock::current->ink=c;}
    int getTextWidth(const char* t,int){ const int n=static_cast<int>(std::strlen(t)); return mock::current->textPx>0 ? static_cast<int>(n*mock::current->textPx*mock::current->fsize+0.5) : n*6; }
    void setPen(COLOR c,int w,int){mock::current->pen=c;mock::current->penW=w;} int invalidateChart(bool=false){++mock::current->invalidates;return 0;}
    int getListIndex(int){return 0;} void setListParameter(const char*,short,const char*,int=0,int=0){ ++nparm(); } void setFloatParameter(const char*,float,int=0,int=0){ ++nparm(); } float getFloatValue(int){return 1.5f;}
    int getVisibleBars(int*,int*){return RTX_FAIL;} short getPixelsPerBar(){return static_cast<short>(mock::current->barW);} void setExtendedFlags2(unsigned long){}
    virtual int scale(int,int,double*,double*){return RTX_FAIL;}
    void setBoolParameter(const char*,bool,int=0){ ++nparm(); } void setColorParameter(const char*,COLOR,int=0,int=0){ ++nparm(); } COLOR getParameterColor(int){return 0;} int setParameterColor(int,COLOR){return 0;}   // (0.44.1) the banner colour row
    void checkBox(int i,bool on){ mockBoxes()[i]=on; } bool isBoxChecked(int i){ auto it=mockBoxes().find(i); return it!=mockBoxes().end() && it->second; }
    void setFloatValue(int,float){} int createTimer(int id,int ms){ if(mock::current->timerRefuse) return RTX_FAIL; mock::current->timers[id]=ms; return RTX_OK; } int destroyTimer(int id){ mock::current->timers.erase(id); return RTX_OK; } int killTimer(int){return RTX_OK;} COLOR getBackgroundColor(){ return mock::current->bg; } virtual int timer(RTX_EVENT*){ return RTX_FAIL; }
    static std::map<int,bool>& mockBoxes(){ static std::map<int,bool> m; return m; }
    void setParameterDialogHeight2(int){} void setArrayCount(int){} void setFlags(unsigned long){} void setDescription(const char*){} void setVersion(const char*){}
};
#endif
