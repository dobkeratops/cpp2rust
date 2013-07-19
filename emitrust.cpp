#define EMIT printf
template<typename C, typename F,typename S>
void apply_separated( C& items, const F& main_item_function, const S& separating_function) {

	for (auto&item: items) {
		main_item_function(item);
		if (&item!= &items.back()) separating_function(item);
	}
}
string emitRust_Typename(const AstNode* n);
string emitRust_TypenameSub(const AstNode* n) {
	switch (n->nodeKind) {
		case CXCursor_TypeRef:
			return string(n->name)+" ";
		break;
	}
	return emitRust_Typename(n);
}

string emitRust_Typename(const AstNode* n) {
	static AstNode unknown(CXCursor_UnexposedDecl,"UNKNOWN","void",CXType{CXType_Void});
	const AstNode* firstSubNode=n->subNodes.size()?&n->subNodes[0]:&unknown;
	//todo: template
	switch (n->cxType.kind) {
	case CXType_Record:
	case CXType_Pointer:
		return string("*")+emitRust_TypenameSub(firstSubNode);

	case CXType_LValueReference:
		return string("&")+emitRust_TypenameSub(firstSubNode);
	break;
	case CXType_Typedef:
		return emitRust_TypenameSub(firstSubNode);
	break;
	default:
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


void emitRust_FunctionDecl(const AstNode&n, int depth,const char* selfType) 	{
	EMIT("pub fn %s",n.name.c_str());
	vector<CpAstNode> args; n.filter(CXCursor_ParmDecl, args);
	vector<CpAstNode> typeParams; n.filter(CXCursor_TemplateTypeParameter, typeParams);

	emitRust_GenericTypeParams(typeParams);
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
	// todo: return type;
	EMIT(";\n");
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

	// todo - filter what this is really.
	// only make it a rust struct if it has data elements?
	// ..otherwise if its a collection of functions/types it's really trait?

	// other metadata for binding ?
	EMIT("struct\t%s", n.name.c_str());
	emitRust_GenericTypeParams(typeParams);

	if (!fields.size()) { EMIT("\t{}\n");}
	else {
		EMIT("\t{\n",n.name.c_str());
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
		EMIT("impl %s {\n", n.name.c_str());
		for (auto &m:methods) {
			emitRust_FunctionDecl(*m,depth+1, n.name.c_str());
		}
		EMIT("}\n");
	}
}
void emitRust(const AstNode& n,int depth=0) 
{	
	#define EMIT_TYPE(T) \
		case CXCursor_ ## T: emitRust_ ## T(n,depth); break;
	switch (n.nodeKind) {
		case CXCursor_StructDecl:
		case CXCursor_ClassTemplate:
			emitRust_ClassTemplate(n,depth);
		break;
		case CXCursor_FunctionDecl:
		case CXCursor_FunctionTemplate:
			emitRust_FunctionDecl(n,depth,nullptr);
		break;
	}

	for (auto& sn: n.subNodes) {emitRust(sn,depth+1);}
	#undef EMIT_TYPE
}
#undef EMIT
