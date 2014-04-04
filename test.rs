#[feature(globs)];
#[feature(struct_variant)];
#[feature(default_type_params)];

pub use std::libc::{c_char,c_void,c_int,c_uint,c_ulonglong};
mod testoutput;

pub struct allocator<A>;
pub struct char_traits<A>;
pub struct less<T>;
pub struct pair<A,B>{ first:A, second:B }
pub struct vector<T,A=allocator<T> > {  begin:*T, end:*T, capacity:*T}
pub type string=basic_string<c_char,char_traits<c_char>,allocator<c_char>>;
pub struct basic_string<C,CTraits=char_traits<C>,Alloc=allocator<C>> { data:*C }
pub type pconstchar_t=*c_char;
pub struct CXCursorKind;
pub type FILE = *c_void;
pub struct set<T,cmp,A=allocator<T> >;
pub type EmitContext=int;
pub type CpAstNode=*AstNode;
pub type int32_t=i32;
pub type uint32_t=u32;
pub struct time_t;
pub struct CrCXType;
pub struct IndexerCallbacks;
pub struct CXClientData;
pub struct CXCodeCompleteResults;
pub struct CXComment;
pub struct CXCompletionResult;
pub struct CXCompletionString;
pub struct CXCursor;
pub struct CXCursorAndRangeVisitor;
pub struct CXCursorSet;
pub struct CXCursorVisitor;
pub struct CXDiagnostic;
pub struct CXDiagnosticSet;
pub struct CXFile;
pub struct CXFileUniqueID;
pub struct CXIdxAttrInfo;
pub struct CXIdxCXXClassDeclInfo;
pub struct CXIdxClientContainer;
pub struct CXIdxClientEntity;
pub struct CXIdxClientFile;
pub struct CXIdxContainerInfo;
pub struct CXIdxDeclInfo;
pub struct CXIdxEntityInfo;
pub struct CXIdxEntityKind;
pub struct CXIdxIBOutletCollectionAttrInfo;
pub struct CXIdxLoc;
pub struct CXIdxObjCCategoryDeclInfo;
pub struct CXIdxObjCContainerDeclInfo;
pub struct CXIdxObjCInterfaceDeclInfo;
pub struct CXIdxObjCPropertyDeclInfo;
pub struct CXIdxObjCProtocolRefListInfo;
pub struct CXInclusionVisitor;
pub struct CXIndex;
pub struct CXIndexAction;
pub struct CXModule;
pub struct CXModuleMapDescriptor;
pub struct CXRemapping;
pub struct CXResult;
pub struct CXSourceLocation;
pub struct CXSourceRange;
pub struct CXSourceRangeList;
pub struct CXString;
pub struct CXToken;
pub struct CXTranslationUnit;
pub struct CXType;
pub struct CXVirtualFileOverlay;
pub type size_t = uint;
pub struct AstNode;

// do something using the created binding...
fn main() {

	std::io::println("rust hello world");
	let opts=testoutput::Options::Option_s{x:1,y:2};
	let mut b=testoutput::Banana{x:1,y:2,z:3};
	let mut x = testoutput::SomeBase{count:0};
	unsafe { x.Foo(1,10.0);}
	let baz_result=x.Baz2(&b);
	println!("rust gets value from C++ {}",baz_result);
}

