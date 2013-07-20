fn copyName(CXString src, char* dst, int maxLen)->void 
{
	const char* cstr=clang_getCString(src);
	size_t len =strlen(cstr);
	len=len>(maxLen-1)?(maxLen-1):len;
	memcpy((void*)dst,(const void*) cstr,len);
	dst[len]=0;
}


fn CXType_to_str(const CXType& t)->const char* {
	// TODO: Rust translations
	#define IMPL_TYPE(TYPE) case CXType_ ## TYPE : return #TYPE ;
	switch (t.kind) {
	IMPL_TYPE(Invalid)
	IMPL_TYPE(Unexposed)
	IMPL_TYPE(Void)
	IMPL_TYPE(Bool)
	IMPL_TYPE(Char_U)
	IMPL_TYPE(UChar)
	IMPL_TYPE(Char16)
	IMPL_TYPE(Char32)
	IMPL_TYPE(UShort)
	IMPL_TYPE(UInt)
	IMPL_TYPE(ULong)
	IMPL_TYPE(ULongLong)
	IMPL_TYPE(UInt128)
	IMPL_TYPE(Char_S)
	IMPL_TYPE(SChar)
	IMPL_TYPE(WChar)
	IMPL_TYPE(Short)
	IMPL_TYPE(Int)
	IMPL_TYPE(Long)
	IMPL_TYPE(LongLong)
	IMPL_TYPE(Int128)
	IMPL_TYPE(Float)
	IMPL_TYPE(Double)
	IMPL_TYPE(LongDouble)
	IMPL_TYPE(NullPtr)
	IMPL_TYPE(Overload)
	IMPL_TYPE(Dependent)
	IMPL_TYPE(ObjCId)
	IMPL_TYPE(ObjCClass)
	IMPL_TYPE(ObjCSel)
	IMPL_TYPE(Complex)
	IMPL_TYPE(Pointer)
	IMPL_TYPE(BlockPointer)
	IMPL_TYPE(LValueReference)
	IMPL_TYPE(RValueReference)
	IMPL_TYPE(Record)
	IMPL_TYPE(Enum)
	IMPL_TYPE(Typedef)
	IMPL_TYPE(ObjCInterface)
	IMPL_TYPE(ObjCObjectPointer)
	IMPL_TYPE(FunctionNoProto)
	IMPL_TYPE(FunctionProto)
	IMPL_TYPE(ConstantArray)
	IMPL_TYPE(Vector)

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



