#[feature(globs)];
#[feature(struct_variant)];
mod testoutput;

struct CrCXType;
struct CXString;
struct CXCursorKind;
struct char_traits;
struct allocator;
struct basic_string;
struct EmitContext;
struct CXClientData;
struct CXChildVisitResult;
struct CXTranslationUnit;

// do something using the created binding...
fn main() {

	std::io::println("hello world");
	let opts=testoutput::Options{x:1,y:2};
	
}

