/*
 * emitrust.h
 *
 *  Created on: 25 Jul 2013
 *      Author: walter
 */

#ifndef EMITRUST_H_
#define EMITRUST_H_

#include "cpp2rustcommon.h"
#include "clanghelpers.h"
#include "ast.h"

enum EmitRustMode {
	EmitRustMode_Rust=1,
	EmitRustMode_CppShim
};

// TODO replace for 'EmitContext', which is currently just depth
struct Emitter{
	FILE* fp;
	int depth;
};
typedef int EmitContext;


pconstchar_t FOOBARFUNCTION1(int32_t ,pair<int32_t,string>& FOOBARFUNCTION_ARG);
class Foo {
	void FOOBARFUNCTION2(vector<int32_t>& src);
};


#include "emitrust_fn.hxx"

#endif /* EMITRUST_H_ */
