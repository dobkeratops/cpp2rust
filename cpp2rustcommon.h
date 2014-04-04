
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
#define ASSERT(X) if (!(X)) {printf("error %s:%s:\n%s\n",__FILE__,__FUNCTION__,#X);}
#define TRACE(X) printf("%s:%d: %s() %s=",__FILE__,__LINE__,__FUNCTION__, #X);cout<<(X)<<"\n";
#else
#define ASSERT(X)
#define TRACE(X)
#endif
#define dbprintf printf
#define fn auto
#define let auto

struct Options {
    bool dumpAst=false, emitRust=true,emitCpp=true;
	bool defaults=true;
};

extern FILE* gOut; // TODO remove, pass context with depth/file
extern Options gOptions;


#endif /* CPP2RUSTCOMMON_H_ */
