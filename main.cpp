#include <iostream>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <limits>
#include <stdint.h>
#include <clang-c/Index.h>

//unity build
using namespace std;
#ifdef DEBUG
#define ASSERT(X) if (!X) {printf("error %s:%s:\n%s\n",__FILE__,__FUNCTION__,#X)}

#else
#define ASSERT(X)
#endif
#define dbprintf printf

struct VisitorData {
	int depth;
};
/*
enum AstNodeId {
	AST_UNKNOWN, AST_STRUCT,AST_FUNCTION,AST_METHOD,AST_PARAM,AST_FIELD,AST_NAMESPACE,AST_NUM
};
const char* g_AstNodeNames[]= {
	"?", "struct","function","method","param","field","namespace"
};
*/



#include "clanghelpers.cpp"
#include "ast.cpp"
#include "emitrust.cpp"

template<typename T> T* append(vector<T>& vec,//AstNodeId id, 
							CXCursorKind k,
							const char* name) { 
	int len=vec.size();
	vec.resize(len+1);
	auto newItem=&vec[len];
//	newItem->nodeType = id;
	newItem->nodeKind = k;
	newItem->name = name;
	return	newItem;
}

CXChildVisitResult buildMyAstNodes(CXCursor cu, CXCursor parent,  CXClientData data) {
	auto parentNode=(AstNode*) data;

	char elemName[256];
	auto vd=(VisitorData*)data;
	clang_getCursorName(cu,elemName,256);

	// create a mirror of the node in C++..

	auto newNode = append(parentNode->subNodes,clang_getCursorKind(cu), elemName);
	newNode->cxType = clang_getCursorType(cu);
	if ((!strcmp(elemName,"std") || (elemName[0]=='_' && elemName[1]=='_'))&& clang_getCursorKind(cu)==CXCursor_Namespace) 
	{	
		// omit std namespace!
		return CXChildVisit_Continue;
	}
	switch (clang_getCursorKind(cu)) {
	// dont handle these in out AST mirror
	case CXCursor_StmtExpr:
	case CXCursor_CompoundStmt:
	case CXCursor_FirstStmt:
	default:
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
	// todo - extract return type? how? it doesn't seem to be there
		clang_visitChildren(cu, buildMyAstNodes, (CXClientData*) newNode);
	break;
		break;
	}
//	printf("num args=%d\n", clang_getCursorNumArgs(cu));
	//printf("cursor data %x\n",data);
	return CXChildVisit_Continue;
}

void testVisit(CXTranslationUnit tu) {
	
}

struct SomeBase {
	int count;
};

struct Options : SomeBase{
	struct Option_s {
		int x,y;
	};
	enum OptionStuff {
		OPT_EMIT_AST=0x000,
		OPT_EMIT_RUST,
		OPT_EMIT_C_WRAPPER=0x004
	};
	bool dumpAst;
} gOptions;

template<typename T>
struct Vector {
	T* first,*last,*capacity;
};

int parseArgs(int argc, const char** argv) 
{
	// TODO- Filter which namespaces to emit.
	// dont emit std:: by default?

	int	myargs=0;
	for (int i=0; i<argc; i++) if (!strcmp(argv[i],"-d")){ gOptions.dumpAst=true;myargs=i;}
	return myargs;
}
int main(int argc, const char** argv) 
{
	int myArgs=parseArgs(argc,argv); argc-=myArgs;argv+=myArgs;
	CXIndex ix= clang_createIndex(0,0);
	CXTranslationUnit tu =clang_parseTranslationUnit(ix, 0, argv, argc, 0,0, CXTranslationUnit_None);

	for (unsigned i=0,n=clang_getNumDiagnostics(tu); i!=n; ++i) {
		CXDiagnostic diag = clang_getDiagnostic(tu,i);
		CXString str = clang_formatDiagnostic(diag, clang_defaultDiagnosticDisplayOptions());
		fprintf(stdout, "%sn", clang_getCString(str));
		clang_disposeString(str);
	}
	AstNode	root;
	clang_visitChildren(clang_getTranslationUnitCursor(tu), buildMyAstNodes, (CXClientData) &root);
	if (gOptions.dumpAst)
		dump(root);
	emitRust(root,0);

	clang_disposeTranslationUnit(tu);
	clang_disposeIndex(ix);

	return 0;
}
