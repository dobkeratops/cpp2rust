/*
 * clanghelpers.h
 *
 *  Created on: 25 Jul 2013
 *      Author: walter
 */

#ifndef CLANGHELPERS_H_
#define CLANGHELPERS_H_

#include "cpp2rustcommon.h"

enum EmitLang {
	EL_RUST,
	EL_CPP
};

// requires 'fn macro' and 1 line "fn name..->{" to auto-generate
#include "clanghelpers_fn.hxx"


#endif /* CLANGHELPERS_H_ */
