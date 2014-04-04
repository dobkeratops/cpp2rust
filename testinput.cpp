#include "testinput.h"


#define TRACE(X) printf("%s:%s:%d:%s=%s",__FUNCTION__,__FILE__,__LINE__, #X, (X));
void SomeBase::Foo(int x,float f) {
	TRACE(0)
}
void SomeBase::Bar(const char* x) {
	TRACE(0)
}

SomeBase  SomeBase::Baz(const std::vector<Banana>& astd) const {
	TRACE(0)
}

int SomeBase::Baz2(const SomeBase*, const Banana& b) const {
	TRACE(0)
}

