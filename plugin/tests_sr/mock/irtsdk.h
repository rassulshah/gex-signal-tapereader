#ifndef OF_TEST_SDK_H
#define OF_TEST_SDK_H
// TEST DOUBLE ONLY. Not a replacement for the supplied Linnsoft SDK. (lsStopRun tests: + getBackgroundColor)
#include <algorithm>
#include <ctime>
#include <cstring>
#include <string>
#include <vector>
#include <climits>
#include <cfloat>
#include <map>
#ifdef OFF_REAL_WIDTHS
#include "tr_widths.h"
#endif
#define DIR_SEPARATOR_CHR '/'
typedef unsigned long RTDATE;
typedef unsigned long COLOR;
typedef unsigned short RTBOOL;
enum BRUSH_STYLE { PAT_NONE, PAT_HOLLOW, PAT_SOLID };   // global, as in the real irtsdk.h
enum PEN_STYLE { P_SOLID, P_DOT, P_DASH };              // global, as in the real irtsdk.h (1.0.1: the pen is tracked)
typedef int RTX_RESULT;
typedef struct DEPTH_ORDER { RTDATE timestamp; float price; float previousPrice; float bid, ask; unsigned long bidsize, asksize; unsigned long size; int orderID; int aggressorOrderID; unsigned char action; unsigned char buySell; } DEPTH_ORDER;
enum { RTX_OK=0, RTX_FAIL=-1, POST_DRAWING=1, OVERLAY=2, NO_UI=4, INSTRUMENT_SCALE=8, VAP_REQUIRED=16, FRONT_DRAWING=32, TRACK_MOUSE=(1<<27), kParmAppendSameLine=1, SYM_TICKINCR=171, CALL_CONTINUOUSLY=1 };
typedef struct DEPTH_LEVEL { float bid, ask; unsigned short bidsize, asksize; } DEPTH_LEVEL;
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
struct Draw { std::string type,text; int x,y; COLOR color; int r=0,b=0; COLOR border=0; int width=0; int pat=0; int fid=0,fsize=0,fstyle=0; COLOR pen=0; int penW=0; Draw(const char* t,int a,int b_,const std::string& s="",COLOR c=0):type(t),text(s),x(a),y(b_),color(c){} };
struct Feed { bool subscribe = true; bool throwOnNext = false; std::vector<DEPTH_ORDER> ev; size_t at = 0; long long getNext = 0, opens = 0, live = 0, nullOpens = 0; std::string lastTicker;
    void push(unsigned char a, unsigned char side, float px, unsigned long size, RTDATE t, int id) { DEPTH_ORDER o; std::memset(&o, 0, sizeof(o)); o.timestamp = t; o.price = px; o.size = size; o.orderID = id; o.action = a; o.buySell = side; ev.push_back(o); } };
extern Feed feed;
struct Host {
    std::vector<DEPTH_LEVEL> depth; bool depthOn=true; unsigned long now=0; int depthObjects=0;
    std::vector<Bar> bars; void* data; bool noSlot; float tick; int seconds;
    std::string symbol,root,period,chart,directory;
    int parms[16]; int listIndex=0; int lists[16]; int boxes[16]; float floats[16]; bool floatSet[16]; int listSets=0, valueSets=0, boxSets=0; bool ignoreSets=false; int box6=1; float lineParm=1.5f, spikeAvg=2.0f, spikeJump=1.5f; int invalidates=0; unsigned long flags=0, xflags=0; int visFrom=-1, visTo=-1; int ppb=10; int faultBar=-1; int rowCalls,statsCalls; std::vector<Draw> draws;
    float priceBase = 80; int paneTop,paneBottom,paneLeft,paneRight,scaleLeft;
    COLOR ink; COLOR pen = 0x00FFFFFF; int penW = 1; int barW = 10;   // pen: starts WHITE (the outline Rassul saw if a box is drawn with the pen left in effect)
    int fid = 0, fsize = 0, fstyle = 0;   // bar spacing (px) and the current font, for the render check
    Host():data(NULL),noSlot(false),tick(1),seconds(60),symbol("ESZ26"),root("ES"),period("1 min"),chart("test"),rowCalls(0),statsCalls(0),paneTop(0),paneBottom(1000),paneLeft(0),paneRight(2000),scaleLeft(1980),ink(0){parms[0]=20;parms[1]=1;parms[2]=9;parms[3]=0;parms[4]=10;parms[5]=0;parms[6]=1000;parms[7]=0; for(int k=8;k<16;++k)parms[k]=0; for(int k=0;k<16;++k){lists[k]=-99;boxes[k]=0;floats[k]=0;floatSet[k]=false;}}
};
extern Host* current;
}
enum { E_MOUSE_UP = 7, E_MOUSE_MOVE, E_MOUSE_DBL, E_MOUSE_SCROLL, E_MOUSE_HOVER = 113 };
typedef struct RTX_EVENT { int type; union { struct { short v, h, shift, control; long bar; float price; } mouse; struct { int id; void* userdata; void* futureUse; } timer; } v; } RTX_EVENT;
namespace mock { inline std::map<int,int>& timers() { static std::map<int,int> t; return t; } inline bool& refuseTimers() { static bool r = false; return r; } }
class cppExtension {
public:
    enum { COURIER=2,HELVETICA=1,BOLD=1,PLAIN=0,P_SOLID=0,P_DASH=2,DRAW_OPAQUE=0,kBarCenter=0,kBarRight=1 };
    struct MARKET_DEPTH { int maxLevels; MARKET_DEPTH(const char*):maxLevels(static_cast<int>(mock::current->depth.size())){++mock::current->depthObjects;} ~MARKET_DEPTH(){--mock::current->depthObjects;} int available(){maxLevels=static_cast<int>(mock::current->depth.size());return mock::current->depthOn?1:0;} DEPTH_LEVEL& operator[](unsigned n){return mock::current->depth.at(n);} };
    struct FONT { int id,size,style; };
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
    struct PNT {
        short h,v;
        int set(int bar,float price,int=0) { h=static_cast<short>(50+bar*mock::current->barW); v=static_cast<short>(mock::current->paneBottom-(price-mock::current->priceBase)*20); return 0; }
        void setDrawPosition(){} void drawLineTo(){ mock::current->draws.push_back(mock::Draw("line",h,v)); mock::current->draws.back().pen=mock::current->pen; }
        int showHelpTip(const char* s){ mock::current->draws.push_back(mock::Draw("tip",h,v,s)); return 0; } int hideHelpTip(){ mock::current->draws.push_back(mock::Draw("tipoff",h,v)); return 0; }   // (1.22.0) the info icon
    };
    struct RCT {
        short left,top,right,bottom;
        void set(short l,short t,short r,short b){left=l;top=t;right=r;bottom=b;}
        void getPaneRect(bool){set(mock::current->paneLeft,mock::current->paneTop,mock::current->paneRight,mock::current->paneBottom);}
        void getScaleRect(){set(mock::current->scaleLeft,0,mock::current->paneRight,mock::current->paneBottom);}
        void draw(int w,COLOR border,COLOR fill,int,int pat){ mock::Draw d("box",left,top,"",fill); d.r=right; d.b=bottom; d.border=border; d.width=w; d.pat=pat; d.pen=mock::current->pen; d.penW=mock::current->penW; mock::current->draws.push_back(d); }
        void drawText(const char* t,bool centred,bool rightAligned){ mock::Draw d("text",rightAligned?right:left,(top+bottom)/2,t,mock::current->ink); d.r=right; d.b=bottom; d.width=centred?1:0; d.fid=mock::current->fid; d.fsize=mock::current->fsize; d.fstyle=mock::current->fstyle; mock::current->draws.push_back(d); }
        void drawOval(int){ mock::current->draws.push_back(mock::Draw("ring",(left+right)/2,(top+bottom)/2)); }
    };
    struct CBRUSH { CBRUSH(COLOR,int){} void set(){} };
    struct VOLPROFILE { float price; long buyVolume,sellVolume,totalVolume,tickCount,maxDelta,minDelta; };
    struct BARSTATISTICS { long buyVolume,sellVolume,totalVolume; short prices; };
    enum { DRAW_INVISIBLE=0, OUTPUT_NO_UI=2 };
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
    virtual int draw(){return RTX_FAIL;} virtual int timer(RTX_EVENT*){return RTX_FAIL;} virtual int mouse(RTX_EVENT*){return RTX_FAIL;} virtual int scale(int,int,double*,double*){return RTX_FAIL;} virtual int parmsLoad(){return RTX_FAIL;} virtual int parmsApply(){return RTX_FAIL;} virtual int parmsUpdt(unsigned int){return RTX_FAIL;}
    RTX_RESULT createTimer(int id,int ms=1000,void* =NULL){ if(mock::refuseTimers()||ms<=0) return RTX_FAIL; mock::timers()[id]=ms; return RTX_OK; } RTX_RESULT destroyTimer(int id){ return mock::timers().erase(id)?RTX_OK:RTX_FAIL; }
    void* getUserData(){return mock::current->data;} void setUserData(void* p){if(!mock::current->noSlot)mock::current->data=p;}
    long getBarCount(){return static_cast<long>(mock::current->bars.size());}
    const char* getRootSymbol(char* b=NULL,const char* =NULL){if(b){std::strcpy(b,mock::current->root.c_str());return b;}return mock::current->root.c_str();} const char* getSymbol(){return mock::current->symbol.c_str();}
    const char* getPeriodicityLabel(){return mock::current->period.c_str();} const char* getChartLabel(){return mock::current->chart.c_str();}
    const char* getWorkingDirectoryPath(){return mock::current->directory.c_str();}
    float getProperty(int){return mock::current->tick;} RTDATE currentDate(){return mock::current->now;} void setExtendedFlags(unsigned long f){mock::current->xflags=f;} int getSecondsPerBar(){return mock::current->seconds;}
    tm* getLocaltime(RTDATE stamp,tm* t){time_t v=static_cast<time_t>(stamp);return gmtime_r(&v,t);}
    // (settings layer 1.1.0) TEXT rows
    void setStringParameter(const char*, const char*, size_t, int = 0, int = 0) { ++nparm(); }
    int getParameterText(int, char* t, size_t n) { if (t && n) t[0] = 0; return RTX_OK; } int setParameterText(int, const char*) { return RTX_OK; }
    // (2026-10-18 settings layer) SettingsHost.h: rows counted per DLL (ids = registration order, as IRT), colour / label rows, enable
    static int& nparm(){ static int n=0; return n; } int getParameterCount(){ return nparm(); }
    void setColorParameter(const char*,COLOR,int=0,int=0){ ++nparm(); } void setLabelParameter(const char*,int=0,int=0){ ++nparm(); }
    COLOR getParameterColor(int){ return 0; } int setParameterColor(int,COLOR){ return 0; } int enableParameter(int,RTBOOL){ return 0; }
    void setParameterVersion(int){} void setParameterDialogHeight(int){} void setIntegerParameter(const char*,int,int=0,int=0){ ++nparm(); }
    int getIntegerValue(int i){return mock::current->parms[i];} int isBoxChecked(int i){return i==6?mock::current->box6:(i>=0&&i<16?mock::current->boxes[i]:0);} int checkBox(int i,int b){++mock::current->boxSets; if(mock::current->ignoreSets)return 0; if(i==6)mock::current->box6=b?1:0; else if(i>=0&&i<16)mock::current->boxes[i]=b?1:0; return 0;} void setBoolParameter(const char*,int,int=0){ ++nparm(); } int getListIndex(int i){return i>=0&&i<16&&mock::current->lists[i]!=-99?mock::current->lists[i]:mock::current->listIndex;} int setListIndex(int i,int v=-1){++mock::current->listSets; if(mock::current->ignoreSets)return 0; if(i>=0&&i<16)mock::current->lists[i]=v; return 0;} void setListParameter(const char*,short,const char*,int=0,int=0){ ++nparm(); }
    int getVisibleBars(int* a,int* b){ if(mock::current->visFrom<0)return RTX_FAIL; *a=mock::current->visFrom; *b=mock::current->visTo; return RTX_OK; } short getPixelsPerBar(){return static_cast<short>(mock::current->ppb);} void setIntegerValue(int i,int v){++mock::current->valueSets; if(mock::current->ignoreSets)return; if(i>=0&&i<16)mock::current->parms[i]=v;}
    void setFont(FONT f){mock::current->fid=f.id;mock::current->fsize=f.size;mock::current->fstyle=f.style;} void setTextColor(COLOR c){mock::current->ink=c;}
#ifdef OFF_REAL_WIDTHS
    int getTextWidth(const char* t,int){ int pt=mock::current->fsize; if(pt<6)pt=6; if(pt>14)pt=14; int w=0; for(const unsigned char* q=(const unsigned char*)t;*q;++q) w+= *q>=32 ? TR_W[mock::current->fstyle==1?1:0][pt-6][*q-32] : 0; return w; }   // (1.2.1) Liberation Sans at the font size
#else
    int getTextWidth(const char* t,int){return static_cast<int>(std::strlen(t))*6;}
#endif
    COLOR getBackgroundColor(){return 0x00101010;} void setPen(COLOR c,int w,int){mock::current->pen=c;mock::current->penW=w;} int invalidateChart(bool=false){++mock::current->invalidates;return 0;} void setFloatParameter(const char*,float,int=0,int=0){ ++nparm(); } void setFloatValue(int i,float v){++mock::current->valueSets; if(mock::current->ignoreSets)return; if(i>=0&&i<16){mock::current->floats[i]=v;mock::current->floatSet[i]=true;}} float getFloatValue(int i){if(i>=0&&i<16&&mock::current->floatSet[i])return mock::current->floats[i]; return i==2?mock::current->spikeAvg:i==3?mock::current->spikeJump:mock::current->lineParm;} void setExtendedFlags2(unsigned long){} void setParameterDialogHeight2(int){} void setArrayCount(int){} void setFlags(unsigned long f){mock::current->flags=f;} void setDescription(const char*){} void setVersion(const char*){}
};
#endif
