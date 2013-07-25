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

#include "emitrust_fn.hxx"

#endif /* EMITRUST_H_ */
