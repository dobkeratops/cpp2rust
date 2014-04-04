#include "testinput.h"
#include <iostream>

#define TRACE(X) printf("%s:%d: %s() %s=",__FILE__,__LINE__,__FUNCTION__, #X);cout<<(X)<<"\n";
void SomeBase::Foo(int x,float f) {
	TRACE(0)
}
void SomeBase::Bar(const char* x) {
	TRACE(0)
}

SomeBase  SomeBase::Baz(const std::vector<Banana>& astd) const {
	TRACE(0)
}

int SomeBase::Baz2(const Banana& b) const {
	TRACE(0)
	printf("C++ gets Banana={x: %d, y:%d, z:%d}\n",b.x,b.y,b.z);
	return b.x+b.y+b.z;
}


