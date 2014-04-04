#include "cpp2rustcommon.h"
#include "emitrust.h"
#include "testinput.h"
#include <stdio.h>

FILE* gOut; // TODO - yuk, instead of global pass a context object.


// TODO - yuk, passing a global. make an emitRust object

bool filterByFilename(CXCursor cu) {
	// hardcoded to exclude anything from '/usr/include' - todo: take a list of paths to accept/ reject
	bool ret=true;
	CXSourceLocation srcLoc = clang_getCursorLocation(cu);
	CXFile file;
	clang_getSpellingLocation(srcLoc,&file,0,0,0);
	CXString filename = clang_getFileName(file);
	const char* szfilename=clang_getCString(filename);
	const char* exclude_path="/usr/include";
	if (szfilename) {
		const char* s1=szfilename,*s2=exclude_path;
		while (*s1 && *s2) { if (*s1!=*s2) break; s1++,s2++; }
		if (!*s2) ret=false;
		if (!ret) {printf("filtered out %s\n", szfilename);}
	}
	clang_disposeString(filename);

	return	ret;
}

static int g_dbg_watch=0;
fn buildMyAstNodes(CXCursor cu, CXCursor parent,  CXClientData data)->CXChildVisitResult {

	bool useThisNode = filterByFilename(cu);
	if (!useThisNode)
		return CXChildVisit_Continue;

	auto parentNode=(AstNode*) data;

	char elemName[256];
	auto vd=(VisitorData*)data;
	clang_getCursorName(cu,elemName,256);

	// create a mirror of the node in C++..
	auto ct=clang_getCursorType(cu);
	auto returnType=clang_getCursorResultType(cu);

	auto typeName=clang_getTypeKindSpelling(ct.kind);
	if ((!strcmp(elemName,"std") || (elemName[0]=='_' && elemName[1]=='_'))&& clang_getCursorKind(cu)==CXCursor_Namespace) 
	{	
		// omit std namespace!
		return CXChildVisit_Continue;
	}
	
	auto newNode = parentNode->createSubNode(clang_getCursorKind(cu), elemName,clang_getCString(typeName),ct,returnType);
	clang_disposeString(typeName);
	//newNode->cxType = clang_getCursorType(cu);
	
	


	auto ck=clang_getCursorKind(cu);
	/*
	if (CXCursor_TemplateRef) {
		g_dbg_watch=4;	
	}
	if (g_dbg_watch>=0) {
		printf("following template %d \n", ck);
	}
	*/

	switch (ck) {
	// dont handle these in out AST mirror
//	case CXCursor_StmtExpr:
//	case CXCursor_CompoundStmt:
//	case CXCursor_FirstStmt:
//	default:
		break;
	// definitely handle these
	case CXCursor_CXXTypeidExpr:
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
	case CXCursor_NonTypeTemplateParameter:
	case CXCursor_TemplateTemplateParameter:
	case CXCursor_TemplateRef:
	case CXCursor_FunctionTemplate:
	case CXCursor_ClassTemplate:
	case CXCursor_UsingDirective:
	case CXCursor_UsingDeclaration:
	case CXCursor_TypeRef:
	case CXCursor_CallExpr:
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
		clang_visitChildren(cu, buildMyAstNodes, (CXClientData*) newNode);
	break;
default:
		//printf("Unhandled Cursor Kind: %d %s  ",ck, elemName);

	// todo - extract return type? how? it doesn't seem to be there
		clang_visitChildren(cu, buildMyAstNodes, (CXClientData*) newNode);
	break;
		
	}
//	printf("num args=%d\n", clang_getCursorNumArgs(cu));
	//printf("cursor data %x\n",data);
//	return CXChildVisit_Recurse;
	return CXChildVisit_Continue;
}

fn testVisit(CXTranslationUnit tu)->void {
	
}

Options gOptions;
FILE* assert_fopen(const char* filename, const char* mode) {
	FILE* fp=fopen(filename,mode);
	if (!fp){
		printf("Could not open %s\n",filename);
		exit(1);
		return nullptr;
	}
	return fp;
}


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
		// todo parse properly. lazy here.
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
	if(argc<=1) {
		printf("Cpp2Rust bindings generatoe\n");
		printf("useage:\n");
		printf("cpp2rs -w <outputname> [opts] <input cpp source> <compiler options>\n");
		printf("eg options: default is to emit c++ shims & rust bindings as\n");
		printf("'<outputname>.cpp' '<outputname>.rs'\n");
		printf("\n");
		printf("Example:\n");
		printf("\tcpp2rs -w bindings mylibrary.h -std=c++11 -x c++\n");
		printf("\n");
		printf("options -d  dump ast\n");
		printf("options -c  output c++ shim only\n");
		printf("options -r  output rust binding only\n");
		exit(0);
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
		auto fp = assert_fopen(fname.c_str(),"wb"); 
		gOut=fp;
		emitRust(EmitRustMode_CppShim, root);
		if (fp) {fclose(fp);gOut=stdout;}
		printf("\nWrote %s C++ externC shims\n", gOutputFilename.c_str());
	}
	if (gOptions.emitRust) {
		auto fname=gOutputFilename+std::string(".rs");
		auto fp = assert_fopen(fname.c_str(),"wb");
		gOut=fp;

		emitRust_transformNestedClassesToMods(root);	// TODO: we might yet have to do this transform internally,dynamic.
		emitRust(EmitRustMode_Rust, root);
		if (fp) {fclose(fp);gOut=stdout;}
		printf("\nWrote %s rust bindings\n", gOutputFilename.c_str());
	}
	// no options given , write files..

	clang_disposeTranslationUnit(tu);
	clang_disposeIndex(ix);

	return 0;
}

