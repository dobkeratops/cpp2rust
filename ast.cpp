#include "ast.h"

fn AstNode::filterByKindRec( CXCursorKind k, vector<CpAstNode>& results) const->void {
	filterByKindRec(k,results);
	for (auto& s:subNodes)
		s.filterByKindRec(k,results);
}
//grr. lost default args with header-generator.

fn AstNode::count( CXCursorKind k) const->int{
	int num=0;
	for (auto& s:subNodes)
		if (s.nodeKind==k)
			num++;
	return num;
}
// TODO - depth/vs breadth
fn AstNode::findFirstSub(CXCursorKind k,bool recurse) const->CpAstNode {
	for (auto& s:subNodes) {
		if (s.nodeKind==k)
			return &s;
		if (recurse){
			auto sf=s.findFirstSub(k,recurse);
			if (sf)
				return sf;
		}
	}
	return nullptr;
}
fn AstNode::findFirstRec(CXCursorKind k) const->CpAstNode {
	return findFirstSub(k,true);
}
fn AstNode::findFirst(CXCursorKind k) const->CpAstNode {
	return findFirstSub(k,false);
}

fn AstNode::numTemplateParams() const ->int32_t {
	return this->count(CXCursor_TemplateTypeParameter)+this->count(CXCursor_TemplateTypeParameter);
}

fn dump( const AstNode& node,int depth)->void {
	int	i;
	auto indent=[](int d) {
		for (int i=0; i<d; i++) 
			dbprintf("\t");
	};
	indent(depth);
//	int nt=node.nodeType;
//	dbprintf("%s %s",node.name.c_str(), /*CXType_to_str(node.cxType),g_AstNodeNames[node.nodeType]*/
//			(nt>=0 && nt<AST_NUM)?g_AstNodeNames[nt]:"?");
	auto returnTypeString = clang_getTypeKindSpelling(node.resultType.kind);
	dbprintf("{");
	const char* sep="";
	if (node.name.length()>0) {
		dbprintf("name:\"%s\"",node.name.c_str());sep=",";
	}
	if (node.typeName!="Invalid") {
		dbprintf("%s type:\"%s\"", sep,node.typeName.c_str());sep=",";
	}
	auto rtcstr=clang_getCString(returnTypeString);
	if (rtcstr && strlen(rtcstr)>0 && strcmp(rtcstr,"Invalid")) {
		dbprintf("%s ret:\"%s\"",sep,rtcstr);sep=",";
	}
	dbprintf("%s kind:\"%s\"", sep, CXCursorKind_to_str(node.nodeKind));
	clang_disposeString(returnTypeString);

	if (node.subNodes.size()) {
		dbprintf(", sub:[\n");
		for (auto &n : node.subNodes) {	
			dump(n,depth+1);
		}
		indent(depth+1);
		dbprintf("]");
		//indent(depth);
	}
	dbprintf("}\n");
}

