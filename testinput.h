#ifndef TESTINPUT_H
#define TESTINPUT_H

template<typename T>
struct Vector {
    T* first,*last,*capacity;
};

struct Banana{
    int x,y,z;
};

struct SomeBase {
    int count;
    void Foo(int x,float f);
    void Bar(const char* x);
    int Baz2(SomeBase* a,Banana& b);
    SomeBase Baz(vector<Banana>& b);
};

struct Options : SomeBase{
    struct Option_s {
        int x,y;
    };
    enum OptionStuff {
        OPT_FOO,
        OPT_BAR,
        OPT_BAZ
    };
    bool dumpAst=false, emitRust=false,emitCpp=false;
    Options() {};
    ~Options() {};
};

extern Options gOptions;


#endif // TESTINPUT_H
