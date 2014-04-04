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


struct EmitCtx {
	EmitCtx* parent;
	set<string>	defined_symbols;
	set<string>	referenced_symbols;	
	set<string>	emit_this_pass;
	map<string,EmitCtx>	sub_scopes;
	
	void define(const string& s) {
		this->defined_symbols.insert(s);
		this->emit_this_pass.insert(s);
	}
	EmitCtx() { parent=0;}
};

void emitRust_transformNestedClassesToMods(AstNode& root);



#include "emitrust_fn.hxx"

#endif /* EMITRUST_H_ */
