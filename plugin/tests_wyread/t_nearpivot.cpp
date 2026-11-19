#include "../WyRead.h"
#include <cstdio>
int main(){ int f=0; wyr::Bar piv; piv.h=110; piv.l=100; wyr::Bar b; b.l=104; b.h=120;
 if(!wyr::nearPivot(b,piv,1,4.0)) {f++;puts("FAIL at 4");} if(wyr::nearPivot(b,piv,1,3.9)) {f++;puts("FAIL over");}
 wyr::Bar c; c.h=106; c.l=90; if(!wyr::nearPivot(c,piv,-1,4.0)) {f++;puts("FAIL bear at 4");} if(wyr::nearPivot(c,piv,-1,3.0)) {f++;puts("FAIL bear over");}
 if(!wyr::nearPivot(b,piv,1,0)) {f++;puts("FAIL no atr");}
 std::vector<wyr::Bar> B(20); for(int i=0;i<20;i++){B[i].h=110;B[i].l=100;B[i].c=105;} auto A=wyr::atr13(B); if(A[19]<9.99||A[19]>10.01){f++;printf("FAIL atr %f\n",A[19]);}
 printf("%s %d fail\n", f?"FAILED":"ALL PASS", f); return f; }
