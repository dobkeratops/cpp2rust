template<typename COLLECTION, typename F,typename SEPERATOR>
void apply_separated(COLLECTION& items, const F& f, const SEPERATOR& s) {

	for (auto&item: items) {
		f(item);
		if (&item!= &items.back()) s(item);
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
				printf("\t%s:%s", s->name.c_str(), s->typeName.c_str() );
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
	EMIT_TYPE(ClassTemplate)
	}

	for (auto& sn: n.subNodes) {emitRust(sn,depth+1);}
	#undef EMIT_TYPE
}

