fn copyName(CXString src, char* dst, int maxLen)->void 
{
	const char* cstr=clang_getCString(src);
	size_t len =strlen(cstr);
	len=len>(maxLen-1)?(maxLen-1):len;
	memcpy((void*)dst,(const void*) cstr,len);
	dst[len]=0;
}


fn CXType_to_str(const CXType& t, bool rust=false)->const char* {
	#define IMPL_TYPE(TYPE,RUST_TYPE) case CXType_ ## TYPE : return rust?#RUST_TYPE:#TYPE ;
#define c_invalid c_void
	switch (t.kind) {
	IMPL_TYPE(Invalid,c_invalid)
	IMPL_TYPE(Unexposed,c_invalid)
	IMPL_TYPE(Void,c_void)
	IMPL_TYPE(Bool,c_bool)
	IMPL_TYPE(Char_U,c_char) // todo: not sure of the translation..
	IMPL_TYPE(UChar,c_uchar)
	IMPL_TYPE(Char16,c_short)
	IMPL_TYPE(Char32,c_int)
	IMPL_TYPE(UShort,c_ushort)
	IMPL_TYPE(UInt,c_uint)
	IMPL_TYPE(ULong,c_ulong)
	IMPL_TYPE(ULongLong,c_ulonglong)
	IMPL_TYPE(UInt128,c_void)
	IMPL_TYPE(Char_S,c_char) /// TODO .. ?
	IMPL_TYPE(SChar, c_schar)
	IMPL_TYPE(WChar, c_short) // TODO ?
	IMPL_TYPE(Short, c_short)
	IMPL_TYPE(Int, c_int)
	IMPL_TYPE(Long, c_long)
	IMPL_TYPE(LongLong,c_longlong)
	IMPL_TYPE(Int128, c_invalid)
	IMPL_TYPE(Float,c_float)
	IMPL_TYPE(Double, c_double)
	IMPL_TYPE(LongDouble, c_void)
	IMPL_TYPE(NullPtr,c_voidptr)
	IMPL_TYPE(Overload,c_invalid)
	IMPL_TYPE(Dependent,c_invalid)
	IMPL_TYPE(ObjCId,c_invalid)
	IMPL_TYPE(ObjCClass,c_invalid)
	IMPL_TYPE(ObjCSel,c_invalid)
	IMPL_TYPE(Complex,c_invalid)
	IMPL_TYPE(Pointer,c_invalid)
	IMPL_TYPE(BlockPointer,c_invalid)
	IMPL_TYPE(LValueReference,c_invalid)
	IMPL_TYPE(RValueReference,c_invalid)
	IMPL_TYPE(Record,c_invalid)
	IMPL_TYPE(Enum,c_invalid)
	IMPL_TYPE(Typedef,c_invalid)
	IMPL_TYPE(ObjCInterface,c_invalid)
	IMPL_TYPE(ObjCObjectPointer,c_invalid)
	IMPL_TYPE(FunctionNoProto,c_invalid)
	IMPL_TYPE(FunctionProto,c_invalid)
	IMPL_TYPE(ConstantArray,c_invalid)
	IMPL_TYPE(Vector,c_invalid)

//	IMPL_TYPE(FirstBuiltin)=Void
	default: printf("%x %d",t.data,t.kind);return "?";
	}
	#undef IMPL_TYPE
}

fn clang_getCursorName(CXCursor cu, char* dst, int maxLen)->void 
{
	auto name=clang_getCursorSpelling(cu);
	copyName(name, dst,maxLen);
	clang_disposeString(name);
}
char g_hack[16];
// sed "s/\s*CXCursor_\([a-zA-Z0-9]*\)\s*=.*/IMPL(\1)/" cxcursor.txt > cxcursor.h + prune duplicates
fn CXCursorKind_to_str(CXCursorKind k)->const char* {
#define IMPL(k) case CXCursor_ ## k : return # k; 
	switch(k) {
	#include "cxcursor.h"
	default: sprintf(g_hack,"%d",k);return g_hack;
	}
#undef IMPL
}



