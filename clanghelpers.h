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
typedef const char *pconstchar_t;
typedef const CXType& CrCXType;
fn CXType_to_str(CrCXType t, EmitLang lang)->const char* ;
fn copyName(CXString src, char* dst, int maxLen)->void;
fn clang_getCursorName(CXCursor cu, char* dst, int maxLen)->void;
fn CXCursorKind_to_str(CXCursorKind k)->pconstchar_t;

//#include "clanghelpers_fn.hxx"


#endif /* CLANGHELPERS_H_ */
