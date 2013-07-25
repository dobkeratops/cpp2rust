
#ifndef CPP2RUSTCOMMON_H_
#define CPP2RUSTCOMMON_H_

#include <iostream>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <limits>
#include <stdint.h>
#include <clang-c/Index.h>
#include <map>
#include <set>
#include <string>

//unity build
using namespace std;
#ifdef DEBUG
#define ASSERT(X) if (!X) {printf("error %s:%s:\n%s\n",__FILE__,__FUNCTION__,#X)}

#else
#define ASSERT(X)
#endif
#define dbprintf printf
#define fn auto
#define let auto

extern FILE* gOut; // TODO remove, pass context with depth/file
#endif /* CPP2RUSTCOMMON_H_ */
