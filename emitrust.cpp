template<typename C, typename F,typename S>
void apply_separated(C& items, const F& main_item_function, const S& separating_function) {

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
	//todo: template
	switch (n->cxType.kind) {
	case CXType_Record:
	case CXType_LValueReference:
		if (n->subNodes.size())
			return string("&")+emitRust_TypenameSub(&n->subNodes[0]);
		else return string("&UNKNOWN");
	break;
	case CXType_Typedef:
		if (n->subNodes.size())
			return emitRust_TypenameSub(&n->subNodes[0]);
		else return string("&UNKNOWN");

	break;
	default:
		return string(CXType_to_str(n->cxType));
	}
}

void
emitRust_ClassTemplate(const AstNode& n, int depth) 
{

	// filter template params...
//	n.filter([](AstNode& n){ if (n==XCursor_TemplateT)}
	vector<CpAstNode> params;
	n.filter(CXCursor_TemplateTypeParameter, params);
	vector<CpAstNode> fields;
	n.filter(CXCursor_FieldDecl, fields);

	// todo - filter what this is really.
	// only make it a rust struct if it has data elements?
	// ..otherwise if its a collection of functions/types it's really trait?

	// other metadata for binding ?
	printf("struct\t%s", n.name.c_str());
	if (params.size()) {
		printf("<");
		apply_separated(params,
			[](CpAstNode& p){
				printf("%s",p->name.c_str());
			},
			[](CpAstNode& p){
				printf(",");
			}
		);
		printf(">");
	}

	if (!fields.size()) { printf("\t{}\n");}
	else {
		printf("\t{\n",n.name.c_str());
		apply_separated(fields,
			[](CpAstNode& s) {
				printf("\t%s:", s->name.c_str());
				printf("%s",emitRust_Typename(s).c_str());
			},
			[](CpAstNode& s) {
				printf(",\n");
			}
		);
		printf("\n}\n");
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
	}

	for (auto& sn: n.subNodes) {emitRust(sn,depth+1);}
	#undef EMIT_TYPE
}

