
struct AstNode {
//	AstNodeId	nodeType;
	CXCursorKind	nodeKind;
	string name;
	string typeName;
	CXType	cxType;
	vector<AstNode>	subNodes;	
	AstNode() {};
	AstNode(CXCursorKind& k, const char* n,const char* tn, CXType cxt) {
		nodeKind=k; name=n; typeName=tn; cxType=cxt;
	}
	AstNode* getSubOfType(CXCursorKind k) {
		for(auto& s:subNodes)
			if (s.nodeKind==k)
				return &s;
	}
	template<typename F>
	void filter(F& f,vector<const AstNode*>& results) const {
		for (auto& s:subNodes)
			if (f(s)) results.push_back(&s);
	}
	void filter( CXCursorKind k, vector<const AstNode*>& results) const{
		for (auto& s:subNodes)
			if (s.nodeKind==k)
				results.push_back(&s);
	}
};
typedef const AstNode* CpAstNode;



void dump( AstNode& node,int depth=0) {
	int	i;
	auto indent=[](int d) {
		for (int i=0; i<d; i++) 
			dbprintf("\t");
	};
	indent(depth);
//	int nt=node.nodeType;
//	dbprintf("%s:%s",node.name.c_str(), /*CXType_to_str(node.cxType),g_AstNodeNames[node.nodeType]*/
//			(nt>=0 && nt<AST_NUM)?g_AstNodeNames[nt]:"?");
	dbprintf("(%s:%s\t%s", node.name.c_str(), CXType_to_str(node.cxType), CXCursorKind_to_str(node.nodeKind));
	if (node.subNodes.size()) {
		dbprintf("\n");
		for (auto &n : node.subNodes) {	
			dump(n,depth+1);
		}
		indent(depth); 
	}
	dbprintf(")\n");

}

