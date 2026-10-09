#pragma once
#include <sys/stat.h>
inline int _mkdir(const char* p){return mkdir(p,0777);}
