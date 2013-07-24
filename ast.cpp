#include "ast.h"

fn AstNode::filter( CXCursorKind k, vector<const AstNode*>& results,bool recurse=false) const->void {
	for (auto& s:subNodes)
		if (s.nodeKind==k)
			results.push_back(&s);
	if(recurse) {
		for (auto& s:subNodes) {
			s.filter(k,results,recurse);
		}
	}
}
fn AstNode::count( CXCursorKind k) const->int{
	int num=0;
	for (auto& s:subNodes)
		if (s.nodeKind==k)
			num++;
	return num;
}
fn AstNode::findFirst(CXCursorKind k,bool recurse=false) const->CpAstNode {
	for (auto& s:subNodes) {
		if (s.nodeKind==k)
			return &s;
		if (recurse){
			auto sf=s.findFirst(k,recurse);
			if (sf)
				return sf;
		}
	}
	return nullptr;
}
fn AstNode::findFirstRec(CXCursorKind k) const->CpAstNode {
	return findFirst(k,true);
}

fn dump( AstNode& node,int depth=0)->void {
	int	i;
	auto indent=[](int d) {
		for (int i=0; i<d; i++) 
			dbprintf("\t");
	};
	indent(depth);
//	int nt=node.nodeType;
//	dbprintf("%s:%s",node.name.c_str(), /*CXType_to_str(node.cxType),g_AstNodeNames[node.nodeType]*/
//			(nt>=0 && nt<AST_NUM)?g_AstNodeNames[nt]:"?");
	auto returnTypeString = clang_getTypeKindSpelling(node.resultType.kind);
	dbprintf("(%s:%s:%s\t%s", node.name.c_str(), /*CXType_to_str(node.cxType)*/node.typeName.c_str(), clang_getCString(returnTypeString), CXCursorKind_to_str(node.nodeKind));
	clang_disposeString(returnTypeString);

	if (node.subNodes.size()) {
		dbprintf("\n");
		for (auto &n : node.subNodes) {	
			dump(n,depth+1);
		}
		indent(depth); 
	}
	dbprintf(")\n");
}

