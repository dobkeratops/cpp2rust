
struct AstNode {
//	AstNodeId	nodeType;
	AstNode*	parent;
	CXCursorKind	nodeKind;
	string name;
	string typeName;
	///TODO mutable string	rustName;	// translated, overload resolution
	CXType	cxType;
	CXType	resultType;	// why need 2, todo: union/access one or the other. cxType is just 'functoinProto'
	vector<AstNode>	subNodes;	
	AstNode() {};
	AstNode(AstNode* parentNode,const CXCursorKind k, const char* n,const char* tn, CXType cxt,CXType returnType) {
		this->parent=parentNode;
		this->nodeKind=k; this->name=n; this->typeName=tn; this->cxType=cxt; this->resultType=returnType;
	}

	AstNode*	createSubNode(CXCursorKind k, const char* name, const char* typeName, CXType cxt, CXType result) {
		this->subNodes.emplace_back(this,k,name,typeName,cxt,result);
		return (AstNode*)&(this->subNodes.back());
	}
template<typename T> T* append(T* parent,vector<T>& vec,//AstNodeId id, 
							CXCursorKind k,
							const char* name) { 
	int len=vec.size();
	vec.resize(len+1);
	auto newItem=&vec[len];
//	newItem->nodeType = id;
	newItem->parent=parent;
	newItem->nodeKind = k;
	newItem->name = name;
	return	newItem;
}

	template<typename F>
	void visit(F& f) const { for (auto &sn:subNodes) f(sn);}
	bool is(CXCursorKind k)const {return nodeKind==k;}
	bool is(CXCursorKind k0,CXCursorKind k1)const {return nodeKind==k0||nodeKind==k1;}
	bool is(CXCursorKind k0,CXCursorKind k1,CXCursorKind k2)const {return nodeKind==k0||nodeKind==k1||nodeKind==k2;}

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
	void filter( CXCursorKind k, vector<const AstNode*>& results,bool recurse=false) const{
		for (auto& s:subNodes)
			if (s.nodeKind==k)
				results.push_back(&s);
		if(recurse) {
			for (auto& s:subNodes) {
				s.filter(k,results,recurse);
			}
		}
	}
	int count( CXCursorKind k) const{
		int num=0;
		for (auto& s:subNodes)
			if (s.nodeKind==k)
				num++;
		return num;
	}
	const AstNode* findFirst(CXCursorKind k,bool recurse=false) const {
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
	const AstNode* findFirstRec(CXCursorKind k) const { return findFirst(k,true);}
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

