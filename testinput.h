#ifndef TESTINPUT_H
#define TESTINPUT_H
#include <stdio.h>
#include <stdint.h>
#include <vector>
using namespace std;

template<typename T>
struct Vector {
    T* first,*last,*capacity;
};

struct Banana{
    int x,y,z;
};
struct Apple {
	vector<int> mem;
};

struct SomeBase {
    int count;
    void Foo(int x,float f);
    void Bar(const char* x);
    int Baz2(const Banana& b) const;
    SomeBase Baz(const vector<Banana>& b) const;
};
struct Vec3 { float x; float y; float z;};
struct Scene {
	struct Triangle {
		int32_t i0,i1,i2;
	};
	struct Mesh {
		vector<Vec3>		vertices;
		vector<Triangle>	triangles;
	};
	struct Instance {
		Mesh* mesh;
		Vec3 pos,ax,ay,az;
	};
	vector<Mesh>		meshes;
	vector<Instance>	instances;
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
