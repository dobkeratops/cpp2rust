#include "clanghelpers.h"

fn copyName(CXString src, char* dst, int maxLen)->void 
{
	const char* cstr=clang_getCString(src);
	size_t len =strlen(cstr);
	len=len>(maxLen-1)?(maxLen-1):len;
	memcpy((void*)dst,(const void*) cstr,len);
	dst[len]=0;
}


fn CXType_to_str(const CXType& t, EmitLang lang)->pconstchar_t {
	#define IMPL_TYPE(CLANG_TYPE,RUST_TYPE,CPP_TYPE) case CXType_ ## CLANG_TYPE : return lang==EL_RUST?#RUST_TYPE:#CPP_TYPE ;
#define c_invalid c_void
	switch (t.kind) {
	IMPL_TYPE(Invalid,c_invalid,INVALID_TYPE)
	IMPL_TYPE(Unexposed,c_invalid,UNEXPOSED_TYPE)
	IMPL_TYPE(Void,c_void,void)
	IMPL_TYPE(Bool,c_bool,bool)
	IMPL_TYPE(Char_U,c_char,char) // todo: not sure of the translation..
	IMPL_TYPE(UChar,c_uchar,unsigned char)
	IMPL_TYPE(Char16,c_short,short)
	IMPL_TYPE(Char32,i32,int32_t)
	IMPL_TYPE(UShort,c_ushort,unsigned short)
	IMPL_TYPE(UInt,c_uint,unsigned int)
	IMPL_TYPE(ULong,c_ulong,unsigned long)
	IMPL_TYPE(ULongLong,c_ulonglong, unsigned long long)
	IMPL_TYPE(UInt128,c_void, uint128)
	IMPL_TYPE(Char_S,c_char,char) /// TODO .. ?
	IMPL_TYPE(SChar, c_schar,signed char)
	IMPL_TYPE(WChar, c_short,WCHAR) // TODO ?
	IMPL_TYPE(Short, c_short,short)
	IMPL_TYPE(Int, c_int,int)
	IMPL_TYPE(Long, c_long,long)
	IMPL_TYPE(LongLong,c_longlong,long long)
	IMPL_TYPE(Int128, c_invalid,int128)
	IMPL_TYPE(Float,c_float,float)
	IMPL_TYPE(Double, c_double,double)
	IMPL_TYPE(LongDouble, c_void,LONGDOUBLE)
	IMPL_TYPE(NullPtr,c_voidptr,NULLPTR)
	IMPL_TYPE(Overload,c_invalid,OVERLOAD)
	IMPL_TYPE(Dependent,c_invalid,DEPENDANT)
	IMPL_TYPE(ObjCId,c_invalid,OBJCID)
	IMPL_TYPE(ObjCClass,c_invalid,OBJCCLASS)
	IMPL_TYPE(ObjCSel,c_invalid,OBJCSEL)
	IMPL_TYPE(Complex,c_invalid,COMPLEX)
	IMPL_TYPE(Pointer,c_invalid,CPP_POINTER)
	IMPL_TYPE(BlockPointer,c_invalid,INVALID_TYPE)
	IMPL_TYPE(LValueReference,c_invalid,CPP_L_VALUE_REFERENCE)
	IMPL_TYPE(RValueReference,c_invalid,CPP_R_VALUE_REFERENCE)
	IMPL_TYPE(Record,c_invalid,CPP_RECORD)
	IMPL_TYPE(Enum,c_invalid,CPP_ENUM)
	IMPL_TYPE(Typedef,c_invalid,CPP_TYPEDEF)
	IMPL_TYPE(ObjCInterface,c_invalid,OBJCINTERFACE)
	IMPL_TYPE(ObjCObjectPointer,c_invalid,OBJCOBJECTPOINTER)
	IMPL_TYPE(FunctionNoProto,c_void,void)
	IMPL_TYPE(FunctionProto,c_void,void)
	IMPL_TYPE(ConstantArray,c_invalid,CONSTANT_ARRAY)
	IMPL_TYPE(Vector,c_invalid,VECTOR)

//	IMPL_TYPE(FirstBuiltin)=Void
	default: printf("%x %d",t.data,t.kind);return "?";
	}
	#undef IMPL_TYPE
}

fn clang_getCursorName(CXCursor cu, char* dst, int maxLen)->void {
	auto name=clang_getCursorSpelling(cu);
	copyName(name, dst,maxLen);
	clang_disposeString(name);
}
char g_hack[16];
// sed "s/\s*CXCursor_\([a-zA-Z0-9]*\)\s*=.*/IMPL(\1)/" cxcursor.txt > cxcursor.h + prune duplicates
fn CXCursorKind_to_str(CXCursorKind k)->pconstchar_t {
#define IMPL(k) case CXCursor_ ## k : return # k; 
	switch(k) {
	#include "cxcursor.h"
	default: sprintf(g_hack,"%d",k);return g_hack;
	}
#undef IMPL
}



