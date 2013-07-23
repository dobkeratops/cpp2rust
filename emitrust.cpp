#define EMIT(...) printf(__VA_ARGS__)
#define EMIT_INDENT(D,...) {indent(D);printf(__VA_ARGS__);}
fn emitRustRecursive(const AstNode& n,int depth=0)->void;
fn emitRustItem(const AstNode* item,int depth=0)->bool;
void indent(int depth) { int i; for (i=0; i<depth; i++) printf("\t");}

template<typename C, typename F,typename S>
fn apply_separated( C& items, const F& main_item_function, const S& separating_function)->void {

	for (auto&item: items) {
		main_item_function(item);
		if (&item!= &items.back()) separating_function(item);
	}
}
fn emitRust_Typename(const AstNode* n,bool returnType=false)->string;
/*
string emitRust_TypenameSub(const AstNode* n) {
	switch (n->nodeKind) {
		case CXCursor_TypeRef:
			return string(n->name)+" ";
		break;
	}
	return emitRust_Typename(n);
}
*/
fn emitRust_Typename(const AstNode* n,bool retnType)->string {

	if(!n) {
//		printf("%x =0 first sub node passed in\n");
//		exit(0);
		return string("UNKNOWN_TYPE");
	}
	static AstNode unknown(nullptr,CXCursor_UnexposedDecl,"UNKNOWN","void",CXType{CXType_Void},CXType{CXType_Void});
	const AstNode* firstSubNode=n->subNodes.size()?&n->subNodes[0]:&unknown;
	//todo: template
	if (n->nodeKind==CXCursor_TypeRef)
		return string(n->name);
	auto cxType=&n->cxType;
	if (retnType) {
		cxType=&n->resultType;
		//firstSubNode=n->findFirst(CXCursor_TypeRef);
		//if (!firstSubNode) firstSubNode==&unknown;	
	}

	switch (cxType->kind) {
	case CXType_Invalid:
	case CXType_Unexposed:
		if(firstSubNode) return emitRust_Typename(firstSubNode);
		return string("INVALID");
		break;
	case CXType_Record:
		return emitRust_Typename(firstSubNode);
	case CXType_Pointer:
		return string("*")+emitRust_Typename(firstSubNode);
	case CXType_LValueReference:
		return string("&")+emitRust_Typename(firstSubNode);
	break;
	case CXType_Typedef:
		return emitRust_Typename(firstSubNode);
	break;
	default:
		// todo: seperate this case out
		// template instance
		if(firstSubNode && (firstSubNode->nodeKind==CXCursor_TemplateRef)) {
			//printf("emiting template type ref %s s=%d %s %s\n",firstSubNode->name.c_str(),n->subNodes.size(),n->subNodes[0].name.c_str(),n->subNodes[1].name.c_str());
			string ret=string(firstSubNode->name)+"<";
			for (size_t i=1; i<n->subNodes.size(); i++) {
				ret+=emitRust_Typename(&n->subNodes[i]);
				if (i!=n->subNodes.size()-1) ret+=",";
			}
			ret+=">";
			return ret;
		}
		else
			return string(CXType_to_str(*cxType,true));
	}
}

fn emitRust_GenericTypeParams(vector<CpAstNode>& typeParams)->void {
	if (typeParams.size()) {
		EMIT("<");
		apply_separated(typeParams,
			[](CpAstNode& p){
				EMIT("%s",p->name.c_str());
			},
			[](CpAstNode& p){
				EMIT(",");
			}
		);
		EMIT(">");
	}
}

fn emitRust_FunctionArguments(const AstNode&n, int depth,const char* selfType)->void
{
	vector<CpAstNode> args; n.filter(CXCursor_ParmDecl, args);
	EMIT("(");
	if (selfType) {
		EMIT("&self"); if (args.size()) EMIT(",");
	}
	apply_separated(args,
		[](CpAstNode& s) {
			EMIT("%s:",s->name.c_str());
			EMIT("%s",emitRust_Typename(s).c_str());	
		},
		[](CpAstNode& s){EMIT(",");}
	);
	EMIT(")");
}
fn emitRust_FunctionReturnType_asStr(const AstNode& n)->string{
	return emitRust_Typename(&n,true);
}

fn emitRust_CShimArgs(const AstNode& n, int depth, const char* selfType)->void {
	vector<CpAstNode> args; n.filter(CXCursor_ParmDecl, args);
	EMIT("(");
	EMIT("self%s",args.size()?",":"");
	apply_separated(args,
		[](CpAstNode& a) {
			EMIT("%s",a->name.c_str());
		},
		[](CpAstNode& a) {
			EMIT(",");
		}
	);

	EMIT(")");
}

fn emitRust_FunctionDecl(const AstNode&n, int depth,const char* selfType, bool emitRustToCShimCall)->void 	{
	EMIT_INDENT(depth,"pub fn %s",n.name.c_str());
	vector<CpAstNode> typeParams; n.filter(CXCursor_TemplateTypeParameter, typeParams);
	vector<CpAstNode> args; n.filter(CXCursor_ParmDecl, args);
	emitRust_GenericTypeParams(typeParams);
	emitRust_FunctionArguments(n,depth,selfType);
	EMIT("->%s",emitRust_FunctionReturnType_asStr(n).c_str());

	if (emitRustToCShimCall && selfType) {
		EMIT("{\n");
		EMIT_INDENT(depth+1,"unsafe { %s_%s",selfType,n.name.c_str());
		emitRust_CShimArgs(n,depth,selfType);
		EMIT(" }\n");
		EMIT_INDENT(depth,"}");

	}

	EMIT(";\n");
}
fn emitRust_GlobalFunctionDecl(const AstNode&n, int depth)->void 	{
	EMIT_INDENT(depth,"extern { pub fn %s",n.name.c_str());
	// TODO - some instantiation of some requested types... 
	// with a rule for emiting C shims with a naming scheme
	//vector<CpAstNode> typeParams; n.filter(CXCursor_TemplateTypeParameter, typeParams);
	vector<CpAstNode> args; n.filter(CXCursor_ParmDecl, args);
	//emitRust_GenericTypeParams(typeParams);
	emitRust_FunctionArguments(n,depth,nullptr);
	EMIT("->%s",emitRust_FunctionReturnType_asStr(n).c_str());


	EMIT(";}\n");
}

fn emitRust_Constructor(const AstNode& n, int depth, const char* selfType,bool emitCShim)->void {
	/*
		// todo - wrappers for constructors with overload underscore qualifiers
	*/
	EMIT_INDENT(depth,"pub fn new",n.name.c_str());
	emitRust_FunctionArguments(n, depth, nullptr);
	EMIT("->");
	EMIT(selfType);

	if (selfType && emitCShim) {
		EMIT("{\n");
		EMIT_INDENT(depth+1,"unsafe{ new_%s",n.name.c_str());
		emitRust_CShimArgs(n,depth,selfType);
		EMIT("}\n");
		EMIT_INDENT(depth,"}");
	}

	EMIT(";\n");
}

fn emitRust_FindDefaultConstructor(const AstNode& n)->CpAstNode {
	for (auto &sn:n.subNodes) {
		if (sn.nodeKind==CXCursor_Constructor)
			if (sn.count(CXCursor_ParmDecl))
				return &sn;
	}
	return n.findFirst(CXCursor_Constructor);
}

fn emitRust_Destructor(const AstNode& n, int depth, const char* selfType, bool emitCShim)->void {
	/*
		// todo - wrappers for constructors with overload underscore qualifiers
	*/
	EMIT_INDENT(depth,"pub fn drop(&self)",n.name.c_str());
	if (emitCShim && selfType) {
		EMIT("{ unsafe { delete_%s(self)} }", selfType);
	}
	EMIT(";\n");
}


fn emitRust_InnerDecls(const AstNode& n, const vector<CpAstNode>& decls, int depth)->void {
	EMIT_INDENT(depth,"mod %s { // inner declarations of %s\n", n.name.c_str(),n.name.c_str());
	for (auto&sn: decls) {
		emitRustItem(sn, depth+1);
	}
	EMIT_INDENT(depth,"} //mod %s\n",n.name.c_str());
}


fn emitRust_Enum(const AstNode& n, int depth)->void {
	vector<CpAstNode>	decls;
	n.filter(CXCursor_EnumConstantDecl,decls);
	EMIT_INDENT(depth,"enum %s {\n",n.name.c_str());
	int	i;
	int	enumValue=0;
	for (i=0; i<decls.size(); i++) {
		auto d=decls[i];
		EMIT_INDENT(depth+1,"%s", d->name.c_str());
		if (d->subNodes.size()) {
			// we have a value..
			auto valueNode=
				d->findFirstRec(CXCursor_IntegerLiteral);
			if (valueNode) {
				//TODO-where is the literal value?!
//				printf("using literal value %s\n",valueNode->name.c_str());
//				exit(0);
			}
		}
		EMIT("=%d", enumValue);
		if (i<decls.size()-1)
			EMIT(",");
		EMIT("\n");
		enumValue++;
	}
	EMIT_INDENT(depth,"}\n");
}

fn emitRust_ClassTemplate(const AstNode& n, int depth)->void
{

	// filter template params...
//	n.filter([](AstNode& n){ if (n==XCursor_TemplateT)}
	vector<CpAstNode> typeParams;
	n.filter(CXCursor_TemplateTypeParameter, typeParams);
	vector<CpAstNode> fields;
	n.filter(CXCursor_FieldDecl, fields);
	vector<CpAstNode> methods;
	n.filter(CXCursor_CXXMethod, methods);
	vector<CpAstNode> innerDecls;

	n.filter(CXCursor_StructDecl, innerDecls);
	n.filter(CXCursor_ClassDecl, innerDecls);
	n.filter(CXCursor_ClassTemplate, innerDecls);
	n.filter(CXCursor_EnumDecl, innerDecls);
	n.filter(CXCursor_TypedefDecl, innerDecls);

	auto base = n.findFirst(CXCursor_CXXBaseSpecifier);

	auto dtr=n.findFirst(CXCursor_Destructor);
	if (dtr)  {
		EMIT_INDENT(depth,"impl Drop for %s {\n", n.name.c_str());
		emitRust_Destructor(*dtr,depth+1, n.name.c_str(),true);
		EMIT_INDENT(depth,"}\n");
	}
	// todo - filter what this is really.
	// only make it a rust struct if it has data elements?
	// ..otherwise if its a collection of functions/types it's really trait?

	// other metadata for binding ?
	EMIT_INDENT(depth,"struct\t%s", n.name.c_str());
	emitRust_GenericTypeParams(typeParams);

	if (!fields.size()) { EMIT("\t{}\n");}
	else {
		EMIT("\t{\n",n.name.c_str());
		if (base) {
			EMIT_INDENT(depth+1,"tbase:%s,\n",base->name.c_str());
		}
		apply_separated(fields,
			[&](CpAstNode& s) {
				EMIT_INDENT(depth+1,"%s:", s->name.c_str());
				EMIT("%s",emitRust_Typename(s).c_str());
			},
			[](CpAstNode& s) {
				EMIT(",\n");
			}
		);
		EMIT("\n");
		EMIT_INDENT(depth,"}\n");
	}
	if  (methods.size()) {
		// todo: gather overloaded methods 
		// and emit postfixed types
		EMIT_INDENT(depth,"impl ");
		emitRust_GenericTypeParams(typeParams);

		EMIT("%s",n.name.c_str());
		emitRust_GenericTypeParams(typeParams);

		EMIT(" {\n" );
//		auto f=n.findFirst(CXCursor_Constructor);
		auto ctr=emitRust_FindDefaultConstructor(n);
		if (ctr) 
			emitRust_Constructor(*ctr,depth+1, n.name.c_str(),true);
		for (auto &m:methods) {
			emitRust_FunctionDecl(*m,depth+1, n.name.c_str(), true);
		}
		EMIT_INDENT(depth,"}\n");
		if (ctr) {
			EMIT_INDENT(depth,"extern{ fn new_%s",n.name.c_str());
			emitRust_FunctionArguments(*ctr,depth,nullptr);
			EMIT("->*%s;}\n",n.name.c_str(),n.name.c_str(),n.name.c_str());
		}
		// emit C shim prototypes..
		for (auto &m:methods) {	
			EMIT_INDENT(depth,"extern{ fn %s_%s",n.name.c_str(),m->name.c_str());
			emitRust_FunctionArguments(*m,depth,n.name.c_str());
			EMIT("->%s;",emitRust_FunctionReturnType_asStr(*m).c_str());
			EMIT("}\n");
		}
	}
	if (dtr) {
		EMIT_INDENT(depth,"extern{ fn delete_%s(&self_ptr:*%s);}\n",n.name.c_str(),n.name.c_str())
	}

	


	if (innerDecls.size()>0)
		emitRust_InnerDecls(n,innerDecls,depth);

}
using namespace std;
template<typename KEY,typename VALUE> struct MultiMap {
	set<KEY>	keys;
	multimap<KEY,VALUE>	keyValues;
	void insert(const KEY& k,const VALUE& v) {
		keys.insert(k);
		keyValues.insert(make_pair(k,v));
	}
	typedef  typename multimap<KEY,VALUE>::iterator iterator;
	struct Range  {
		pair<iterator,iterator> sub;
		iterator begin(){return sub.first;}
		iterator end(){return sub.second;}
		Range(pair<iterator,iterator>&i) :sub(i){};
	};
	Range getValues(const KEY& k)  {
//		auto first=keyValues.find(k);
		pair<iterator,iterator> r=keyValues.equal_range(k);
		return Range(r);
	}
	size_t size()const {return keyValues.size();}
};

fn emitRust_GatherFunctionsAsMethodsAndTraits(const AstNode& n,int depth)->void{
	// Find all the functions which look like methods.
	// i.e. first argument type is prefixed
	// stuff them into an impl.

	MultiMap<string,CpAstNode > functionsByName;
//	multimap<string,CpAstNode > functionsByFirstTypeName;
//	set<string> firstTypeName;
	MultiMap<string,CpAstNode> functionsByFirstTypeName;

	for (auto& sn: n.subNodes) {
		if (!(sn.nodeKind==CXCursor_FunctionDecl ||
			sn.nodeKind==CXCursor_FunctionTemplate)) {
			continue;	
		}
		auto firstParam=sn.findFirst(CXCursor_ParmDecl);
		if (!firstParam)
			continue;
		auto functionNode=&sn;

		functionsByName.insert(functionNode->name,functionNode);
		auto type=emitRust_Typename(firstParam);
		const char* typeName=type.c_str();
		if (typeName[0]=='*'||typeName[0]=='&')typeName++;

		const char* s1,*s2;
		char lastMatch=0;
		for (s1=sn.name.c_str(),s2=typeName; *s1&&*s1 && *s1==*s2; s1++,s2++) {
			//scan until first break;
		}
		// did we get the whole typename as a prefix?
		if (*s2)
			continue;
		functionsByFirstTypeName.insert(string(typeName), functionNode);
	}

	/*
	// report overloaded functions 
	// todo - strip prototypes if function bodies are founds
	//TODO.. some algo for gathering as traits?
	for (auto &ms:functionsByName.keys) {
		auto values=functionsByName.getValues(ms);
		if (values.end()!=(++values.begin())) {
			cout<<"//"<<ms<<" Overloads:-\n";
			for (auto &v:values) {
				cout<<"//";
				emitRust_FunctionDecl(*v.second,depth,nullptr);
			}
		}
	}
	*/

	// Possible type_method(type,...) impls..
	/*
	if (functionsByFirstTypeName.size()) {
		cout<<"//Create impls for:-\n";
		for (auto &ms:functionsByFirstTypeName.keys) {
			cout<<"//impl\t"<<ms<<"\t{\n";
			auto range=functionsByFirstTypeName.getValues(ms);
			for (auto &v:range) {
				cout<<"//\t"<<v.second->name<<"\n";
			}
			cout<<"//}\n";
		}
	}
	*/
	#undef EMIT_TYPE
}

fn emitRustItem(const AstNode* n,int depth)->bool
{
	switch (n->nodeKind) {
		case CXCursor_EnumDecl:
			emitRust_Enum(*n,depth);
			return	 true;
		case CXCursor_StructDecl:
		case CXCursor_ClassTemplate:
			emitRust_ClassTemplate(*n,depth);
			return true;
		break;
		case CXCursor_FunctionDecl:
		case CXCursor_FunctionTemplate:
			emitRust_GlobalFunctionDecl(*n,depth);
			return 	true;
		break;
	}
	return false;
}

// overloaded methods
// create multiple dispatch for it?
//
// (a,b,c).yada(non-overloaded-params);
//
//

fn emitRustRecursive(const AstNode& n,int depth)->void
{	
	#define EMIT_TYPE(T) \
		case CXCursor_ ## T: emitRust_ ## T(n,depth); break;
	bool didEmit=emitRustItem(&n,depth);
	if(!didEmit) {
		for (auto& sn: n.subNodes) {
			emitRustRecursive(sn,depth);
		}
	}
	emitRust_GatherFunctionsAsMethodsAndTraits(n,depth);
}
#undef EMIT
