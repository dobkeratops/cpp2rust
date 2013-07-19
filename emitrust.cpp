#define EMIT printf
void emitRust(const AstNode& n,int depth=0) ;

template<typename C, typename F,typename S>
void apply_separated( C& items, const F& main_item_function, const S& separating_function) {

	for (auto&item: items) {
		main_item_function(item);
		if (&item!= &items.back()) separating_function(item);
	}
}
string emitRust_Typename(const AstNode* n);
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
string emitRust_Typename(const AstNode* n) {
	static AstNode unknown(CXCursor_UnexposedDecl,"UNKNOWN","void",CXType{CXType_Void});
	const AstNode* firstSubNode=n->subNodes.size()?&n->subNodes[0]:&unknown;
	//todo: template
	if (n->nodeKind==CXCursor_TypeRef)
		return string(n->name);

	switch (n->cxType.kind) {
	case CXType_Record:
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
		if(firstSubNode->nodeKind==CXCursor_TemplateRef) {
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
			return string(CXType_to_str(n->cxType));
	}
}

void emitRust_GenericTypeParams(vector<CpAstNode>& typeParams) {
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

void emitRust_FunctionArguments(const AstNode&n, int depth,const char* selfType)
{
	vector<CpAstNode> args; n.filter(CXCursor_ParmDecl, args);
	EMIT("(");
	if (selfType) {
		EMIT("self"); if (args.size()) EMIT(",");
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

void emitRust_FunctionDecl(const AstNode&n, int depth,const char* selfType) 	{
	EMIT("pub fn %s",n.name.c_str());
	vector<CpAstNode> typeParams; n.filter(CXCursor_TemplateTypeParameter, typeParams);

	emitRust_GenericTypeParams(typeParams);
	emitRust_FunctionArguments(n,depth,selfType);
	// todo: return type;
	EMIT(";\n");
}

void emitRust_Constructor(const AstNode& n, int depth, const char* selfType) {
	/*
		// todo - wrappers for constructors with overload underscore qualifiers
	*/
	EMIT("\tpub fn new",n.name.c_str());
	emitRust_FunctionArguments(n, depth, nullptr);
	EMIT("->");
	EMIT(selfType);
	EMIT(";\n");
}

const AstNode* emitRust_FindDefaultConstructor(const AstNode& n) {
	for (auto &sn:n.subNodes) {
		if (sn.nodeKind==CXCursor_Constructor)
			if (sn.count(CXCursor_ParmDecl))
				return &sn;
	}
	return n.findFirst(CXCursor_Constructor);
}

void emitRust_Destructor(const AstNode& n, int depth, const char* selfType) {
	/*
		// todo - wrappers for constructors with overload underscore qualifiers
	*/
	EMIT("\tpub fn drop",n.name.c_str());
	emitRust_FunctionArguments(n, depth, selfType);
	EMIT(";\n");
}

void
emitRust_InnerDecls(const AstNode& n, const vector<CpAstNode>& decls, int depth) {
	EMIT("mod %s {\n", n.name.c_str());
	for (auto&sn: decls)
		emitRust(*sn, depth+1);
	EMIT("}\n");
}

void
emitRust_Enum(const AstNode& n, int depth) {
	vector<CpAstNode>	decls;
	n.filter(CXCursor_EnumConstantDecl,decls);
	EMIT("enum %s {\n",n.name.c_str());
	int	i;
	int	enumValue=0;
	for (i=0; i<decls.size(); i++) {
		auto d=decls[i];
		EMIT("\t%s", d->name.c_str());
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
	EMIT("}\n");
}

void
emitRust_ClassTemplate(const AstNode& n, int depth) 
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
	n.filter(CXCursor_EnumConstantDecl, innerDecls);
	n.filter(CXCursor_TypedefDecl, innerDecls);

	if (innerDecls.size())
		emitRust_InnerDecls(n,innerDecls,depth);

	auto base = n.findFirst(CXCursor_CXXBaseSpecifier);

	auto f=n.findFirst(CXCursor_Constructor);
	if (f)  {
		EMIT("impl Drop for %s {\n", n.name.c_str());
		emitRust_Destructor(*f,depth+1, n.name.c_str());
		EMIT("}\n");
	}
	// todo - filter what this is really.
	// only make it a rust struct if it has data elements?
	// ..otherwise if its a collection of functions/types it's really trait?

	// other metadata for binding ?
	EMIT("struct\t%s", n.name.c_str());
	emitRust_GenericTypeParams(typeParams);

	if (!fields.size()) { EMIT("\t{}\n");}
	else {
		EMIT("\t{\n",n.name.c_str());
		if (base) {
			EMIT("\tbase:%s,\n",base->name.c_str());
		}
		apply_separated(fields,
			[](CpAstNode& s) {
				EMIT("\t%s:", s->name.c_str());
				EMIT("%s",emitRust_Typename(s).c_str());
			},
			[](CpAstNode& s) {
				EMIT(",\n");
			}
		);
		EMIT("\n}\n");
	}
	if  (methods.size()) {
		EMIT("impl ");
		emitRust_GenericTypeParams(typeParams);

		EMIT("%s",n.name.c_str());
		emitRust_GenericTypeParams(typeParams);

		EMIT(" {\n" );
//		auto f=n.findFirst(CXCursor_Constructor);
		auto f=emitRust_FindDefaultConstructor(n);
		if (f) 
			emitRust_Constructor(*f,depth+1, n.name.c_str());
		for (auto &m:methods) {
			EMIT("\t");
			emitRust_FunctionDecl(*m,depth+1, n.name.c_str());
		}
		EMIT("}\n");
	}
}

void emitRust(const AstNode& n,int depth) 
{	
	#define EMIT_TYPE(T) \
		case CXCursor_ ## T: emitRust_ ## T(n,depth); break;
	switch (n.nodeKind) {
		case CXCursor_EnumDecl:
			emitRust_Enum(n,depth);
		case CXCursor_StructDecl:
		case CXCursor_ClassTemplate:
			emitRust_ClassTemplate(n,depth);
		break;
		case CXCursor_FunctionDecl:
		case CXCursor_FunctionTemplate:
			emitRust_FunctionDecl(n,depth,nullptr);
		break;
	}

	for (auto& sn: n.subNodes) {
		emitRust(sn,depth+1);
	}

	// Find all the functions which look like methods.
	// stuff them into an impl.
	
	for (auto& sn: n.subNodes) {
		if (!(sn.nodeKind==CXCursor_FunctionDecl ||
			sn.nodeKind==CXCursor_FunctionTemplate)) {
			continue;
		}
		auto firstParam=sn.findFirst(CXCursor_ParmDecl);
		if (!firstParam)
			continue;
		auto type=emitRust_Typename(firstParam);
	}
	#undef EMIT_TYPE
}
#undef EMIT
