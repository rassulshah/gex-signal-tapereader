#pragma once
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cstdint>
#define _TRUNCATE ((size_t)-1)
#define strncpy_s(d,n,s,t) strncpy(d,s,(n)-1)
#define sscanf_s sscanf
#define _stat64 stat
static inline int localtime_s(struct tm* r,const time_t* t){localtime_r(t,r);return 0;}
typedef long long RT_LONG;
