#include "cpp2rustcommon.h"
#include "emitrust.h"
#include <stdio.h>

FILE* gOut; // TODO - yuk, instead of global pass a context object.


// TODO - yuk, passing a global. make an emitRust object

fn buildMyAstNodes(CXCursor cu, CXCursor parent,  CXClientData data)->CXChildVisitResult {
	auto parentNode=(AstNode*) data;

	char elemName[256];
	auto vd=(VisitorData*)data;
	clang_getCursorName(cu,elemName,256);

	// create a mirror of the node in C++..
	auto ct=clang_getCursorType(cu);
	auto returnType=clang_getCursorResultType(cu);
	
	auto typeName=clang_getTypeKindSpelling(ct.kind);
	auto newNode = parentNode->createSubNode(clang_getCursorKind(cu), elemName,clang_getCString(typeName),ct,returnType);
	clang_disposeString(typeName);
	//newNode->cxType = clang_getCursorType(cu);
	if ((!strcmp(elemName,"std") || (elemName[0]=='_' && elemName[1]=='_'))&& clang_getCursorKind(cu)==CXCursor_Namespace) 
	{	
		// omit std namespace!
		return CXChildVisit_Continue;
	}
	switch (clang_getCursorKind(cu)) {
	// dont handle these in out AST mirror
//	case CXCursor_StmtExpr:
//	case CXCursor_CompoundStmt:
//	case CXCursor_FirstStmt:
//	default:
		break;
	// definitely handle these
	case CXCursor_StructDecl:
	case CXCursor_ClassDecl:
	case CXCursor_UnionDecl:
	case CXCursor_Namespace:
	case CXCursor_FieldDecl:
	case CXCursor_FunctionDecl:
	case CXCursor_CXXMethod:
	case CXCursor_ParmDecl:
	case CXCursor_TypedefDecl:
	case CXCursor_Constructor:
	case CXCursor_Destructor:
	case CXCursor_TemplateTypeParameter:
	case CXCursor_TemplateRef:
	case CXCursor_ClassTemplate:
	case CXCursor_UsingDirective:
	case CXCursor_UsingDeclaration:
	case CXCursor_TypeRef:
	case CXCursor_CallExpr:
	case CXCursor_FunctionTemplate:
	case CXCursor_EnumDecl:
	case CXCursor_EnumConstantDecl:
	case CXCursor_TypeAliasDecl:
	case CXCursor_CXXBaseSpecifier:
	case CXCursor_VarDecl:
//	case CXCursor_VarDecl:
	case CXCursor_FirstExpr:
	case CXCursor_IntegerLiteral:
	case CXCursor_StringLiteral:
	case CXCursor_FloatingLiteral:
	case CXCursor_CharacterLiteral:

	case CXCursor_DeclRefExpr:
default:
	// todo - extract return type? how? it doesn't seem to be there
		clang_visitChildren(cu, buildMyAstNodes, (CXClientData*) newNode);
	break;
		break;
	}
//	printf("num args=%d\n", clang_getCursorNumArgs(cu));
	//printf("cursor data %x\n",data);
	return CXChildVisit_Continue;
}

fn testVisit(CXTranslationUnit tu)->void {
	
}

struct SomeBase {
	int count;
};

struct Options : SomeBase{
	struct Option_s {
		int x,y;
	};
	enum OptionStuff {
		OPT_FOO,
		OPT_BAR,
		OPT_BAZ
	};
	bool dumpAst=false, emitRust=false,emitCpp=false;
	Options() {};
	~Options() {};
};

Options gOptions;

template<typename T>
struct Vector {
	T* first,*last,*capacity;
};
string gOutputFilename("cpp2rust_output");
fn parseArgs(int argc, const char** argv)->int 
{
	// TODO- Filter which namespaces to emit.
	// dont emit std:: by default?
	int	myargs=0;
	for (int i=0; i<argc; i++) {
		// some hardcoded convinient default combinations, TODO handle properly.
		if (!strcmp(argv[i],"-w")) {
			gOutputFilename=string(argv[i+1]);
			i+=2;
		}
		if (!strcmp(argv[i],"-d"))
			{ gOptions.dumpAst=true;myargs=i;gOptions.emitRust=false;}
		if (!strcmp(argv[i],"-dr"))
			{ gOptions.dumpAst=true;myargs=i;gOptions.emitRust=true;}
		if (!strcmp(argv[i],"-c"))
			{ gOptions.emitCpp=true; gOptions.emitRust=false;gOptions.dumpAst=false;myargs=i;}
		if (!strcmp(argv[i],"-r"))
			{ gOptions.emitCpp=false; gOptions.emitRust=true;gOptions.dumpAst=false;myargs=i;}
		if (!strcmp(argv[i],"-cr"))
			{ gOptions.emitCpp=true;gOptions.emitRust=true; myargs=i;}
		if (!strcmp(argv[i],"-dcr"))
			{ gOptions.dumpAst=true;myargs=i;gOptions.emitCpp=true;gOptions.emitRust=true;}
	}
	return myargs;
}
fn main(int argc, const char** argv)->int
{
	int myArgs=parseArgs(argc,argv); argc-=myArgs;argv+=myArgs;
	auto ix= clang_createIndex(0,0);
	auto tu =clang_parseTranslationUnit(ix, 0, argv, argc, 0,0, CXTranslationUnit_None);

	for (unsigned i=0,n=clang_getNumDiagnostics(tu); i!=n; ++i) {
		auto diag = clang_getDiagnostic(tu,i);
		auto str = clang_formatDiagnostic(diag, clang_defaultDiagnosticDisplayOptions());
		fprintf(stdout, "%sn", clang_getCString(str));
		clang_disposeString(str);
	}
	AstNode	root;
	clang_visitChildren(clang_getTranslationUnitCursor(tu), buildMyAstNodes, (CXClientData) &root);

	if (gOptions.dumpAst) {
		dump(root,0);
	}
	if (gOptions.emitCpp) {
		auto fname=gOutputFilename+std::string(".cpp");
		auto fp = fopen(fname.c_str(),"wb"); if (fp) gOut=fp;
		emitRustRecursive(EmitRustMode_CppShim, root,0);
		if (fp) {fclose(fp);gOut=stdout;}
	}
	if (gOptions.emitRust) {
		auto fname=gOutputFilename+std::string(".rs");
		auto fp = fopen(fname.c_str(),"wb"); if (fp) gOut=fp;
		emitRustRecursive(EmitRustMode_Rust, root,0);
		if (fp) {fclose(fp);gOut=stdout;}
	}
	// no options given , write files..

	clang_disposeTranslationUnit(tu);
	clang_disposeIndex(ix);

	return 0;
}
