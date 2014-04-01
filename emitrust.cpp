#include "emitrust.h"

#define EMIT(...) fprintf(gOut, __VA_ARGS__)
#define EMIT_INDENT(D,...) {indent(D);fprintf(gOut,__VA_ARGS__);}
fn LangOf(EmitRustMode m)->EmitLang {
	switch (m) {
		case EmitRustMode_Rust: return EL_RUST;
		case EmitRustMode_CppShim: return EL_CPP;
		default: printf("%s:%d: unhandled case\n",__FILE__,__LINE__); exit(0);
	}
}

fn emitRustRecursive(EmitRustMode m,const AstNode& n,EmitContext depth)->void;
fn emitRustItem(EmitRustMode m,const AstNode* item,EmitContext depth)->bool;
void indent(int depth) { int i; for (i=0; i<depth; i++) fprintf(gOut, "\t");}

template<typename C, typename F,typename S>
auto apply_separated( C& items, const F& main_item_function, const S& separating_function)->void {

	for (auto&item: items) {
		main_item_function(item);
		if (&item!= &items.back()) separating_function(item);
	}
}
/*
fn emit_Typename(EmitLang lang, CpAstNode n,bool returnType, const vector<AstNode>* parent, int index)->string;
fn emitRust_Typename(const AstNode* n,bool returnType , const vector<AstNode>* parent, int index)->string {
	return emit_Typename(EL_RUST,n,returnType, parent,index);
}
*/
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

fn combineSigilInTypename(EmitLang l, const string& s, const char* sigil)->string {
    if (l==EL_RUST || EL_CPP ) {
        switch (l) {
            case EL_RUST: return string(sigil)+s;
            case EL_CPP: return s+string(sigil);
        }
    } else {
        return string(sigil)+s;
    }
}

fn combinePtrInTypename(EmitLang l, const string& s)->string 
{
    return combineSigilInTypename(l,s,"*");
}
fn combineRefInTypename(EmitLang l, const string& s)->string 
{
    return combineSigilInTypename(l,s,"&");
}

bool shouldEmitFunction(CpAstNode m) {
	 // Dont do operators. new/delete. 
	 // details of rust/c++ creating eachothers objects would be scary.
	 // we just want to pass useable references across.
	bool b= !(strcmp(m->cname(),"operator")>=8);
	return b;
}

string emit_CXTypeDecl(enum EmitLang lang, CXCursor c) {
	CXString name=clang_getCursorSpelling(c);
	string str(clang_getCString(name));
	clang_disposeString(name);

	return str;
}

// todo - how to handle the mutability swizzle ?
// absence of const modifier requires mut. const modifier is to be ignored.
auto emit_CXType(enum EmitLang lang, CXType cxType)->string {
    string str;

    bool immutable = clang_isConstQualifiedType(cxType);

    // todo: wrap immutability as a call, it might be post/prefix in other syntax?
    if ( lang==EL_RUST && !immutable) {
        str+="mut ";
    } else if (immutable) {
        str+="const " ;
    }

	CXCursor c=clang_getTypeDeclaration(cxType);

    switch (cxType.kind) {
/*        case CXType_Void:   str.append("void"); break
        case CXType_Bool:   str.append("Void"); break
        case CXType_Char_U = 4,
        case CXType_UChar = 5,
        case CXType_Char16 = 6,
        case CXType_Char32 = 7,
        case CXType_UShort = 8,
        case CXType_UInt = 9,
        case CXType_ULong = 10,
        case CXType_ULongLong = 11,
        case CXType_UInt128 = 12,
        case CXType_Char_S = 13,
        case CXType_SChar = 14,
        case CXType_WChar = 15,
        case CXType_Short = 16,
        case CXType_Int = 17,
        case CXType_Long = 18,
        case CXType_LongLong = 19,
        case CXType_Int128 = 20,
        case CXType_Float = 21,
        case CXType_Double = 22,
        case CXType_LongDouble = 23,
        case CXType_NullPtr = 24,
        case CXType_Overload = 25,
        case CXType_Dependent = 26,
        case CXType_ObjCId = 27,
*/

        case CXType_Invalid: //get from the node, but we dont understand why :(//string("CXTY_INVALID");
        case CXType_Unexposed: //get from the node, but we dont understand why we ever get this :(//string("CXTY_UNEXPOSED");
        case CXType_Enum:
        case CXType_Record:
            //str.append("RECORD=");
            str+=emit_CXTypeDecl(lang,c);//get from the node, but we dont understand why :(
            break;
        case CXType_Pointer:{
            auto pointee=clang_getPointeeType(cxType);
            str+= combinePtrInTypename(lang, emit_CXType(lang, pointee));
            break;
        }
        case CXType_RValueReference:
        case CXType_LValueReference:
            str+= combineRefInTypename(lang, emit_CXType(lang, clang_getPointeeType(cxType)));
        break;

        case CXType_Typedef:
            //str.append("TYPDEF=");
            str+=emit_CXTypeDecl(lang,c); // hmm, but we *can* have template params on typedefs :(
            //str.append(CXType_to_str(cxType,lang));//emit_CXTypeDecl(lang, c);
            return str; // no template params.
        break;

        default:
            str.append(CXType_to_str(cxType,lang));//emit_CXTypeDecl(lang, c);
    }

    // todo: with this propper handling of template args and so on... we can eliminate all the first|SubNodes junk
    signed int numtyargs=clang_Type_getNumTemplateArguments(cxType);    //clang says  -1 = no template args.
    if (numtyargs>=0) {
        str+="<";
        for (int i=0; i<numtyargs; i++) {
            if (i>0) str+=",";
            auto ty=clang_Type_getTemplateArgumentAsType(cxType, i);
            str +=emit_CXType(lang, ty);
        }
        str+=">";
    }
    return str;
}
/*
auto emit_Typename(EmitLang lang, const AstNode* n,bool retnType, const vector<AstNode>* parent, int thisIndex)->string {

    auto cxType=&n->cxType;
    if (retnType) {
        cxType=&n->resultType;
    }
    return emit_CXType(lang, *cxType);
}
*/

fn emitRust_GenericTypeParams(vector<CpAstNode>& typeParams)->void {
	if (typeParams.size()) {
		EMIT("<");
		apply_separated(typeParams,
			[](CpAstNode& p){
				EMIT("%s",p->cname());
			},
			[](CpAstNode& p){
				EMIT(",");
			}
		);
		EMIT(">");
	}
}

fn emit_FunctionArguments(EmitLang lang,const AstNode&n, EmitContext depth,const char* selfType)->void
{
	vector<CpAstNode> args; vector<int> arg_indices; 
	n.filterIndexed([](CpAstNode p){return p->nodeKind==CXCursor_ParmDecl;}, args,arg_indices);
	EMIT("(");
	if (selfType) {
		if (lang==EL_RUST) {
			EMIT("&self");
		} else
			EMIT("%s *self",selfType); 
		if (args.size()) EMIT(",");
	}
	apply_separated(args,
		[&](CpAstNode& s) {
            auto tn=emit_CXType(lang, s->cxType);
			if (lang==EL_RUST) {
				EMIT("%s:%s",s->cname(),tn.c_str());	
			} else {
				EMIT("%s %s",tn.c_str(), s->cname());
			}
		},
		[](CpAstNode& s){EMIT(",");}
	);
	EMIT(")");
}

fn emitRust_FunctionArguments(const AstNode&n, EmitContext depth,const char* selfType)->void {
	return emit_FunctionArguments(EL_RUST,n, depth,selfType);
}

fn emit_FunctionReturnType_asStr(EmitLang lang, const AstNode& n, vector<AstNode>* parent, int index)->string{
    return emit_CXType(lang,n.resultType);
}
fn emitRust_FunctionReturnType_asStr(const AstNode& n, vector<AstNode>* parent, int index)->string{
    return emit_CXType(EL_RUST,n.resultType);
}

fn emit_CShimArgs(EmitLang lang,const AstNode& n, EmitContext depth, const char* selfType)->void {
	vector<CpAstNode> args; n.filterByKind(CXCursor_ParmDecl, args);
	EMIT("(");
	EMIT("self%s",args.size()?",":"");
	apply_separated(args,
		[](CpAstNode& a) {
			EMIT("%s",a->cname());
		},
		[](CpAstNode& a) {
			EMIT(",");
		}
	);

	EMIT(")");
}
fn emitRust_CShimArgs(const AstNode& n, EmitContext depth, const char* selfType)->void {
	emit_CShimArgs(EL_RUST,n,depth,selfType);
}

fn emitRust_FunctionDecl(const AstNode&n, EmitContext depth,const char* selfType, bool emitRustToCShimCall)->void 	{
	if (!shouldEmitFunction(&n)) return; 
	EMIT_INDENT(depth,"pub fn %s",n.cname());
	vector<CpAstNode> typeParams; n.filterByKind(CXCursor_TemplateTypeParameter, typeParams);
	vector<CpAstNode> args; n.filterByKind(CXCursor_ParmDecl, args);
	emitRust_GenericTypeParams(typeParams);
	emitRust_FunctionArguments(n,depth,selfType);
	EMIT("->%s",emitRust_FunctionReturnType_asStr(n,0,0).c_str());

	if (emitRustToCShimCall && selfType) {
		EMIT("{\n");
		EMIT_INDENT(depth+1,"unsafe { %s_%s",selfType,n.cname());
		emitRust_CShimArgs(n,depth,selfType);
		EMIT(" }\n");
		EMIT_INDENT(depth,"}");

	}
	EMIT(";\n");
}

fn emitRust_GlobalFunctionDecl(const AstNode&n, EmitContext depth)->void 	{
	// TODO - filter and dont emit global functions that are already declared "extern "C"
	EMIT_INDENT(depth,"extern { pub fn %s",n.cname());
	if (!shouldEmitFunction(&n)) return;
	// TODO - some instantiation of some requested types... 
	// with a rule for emiting C shims with a naming scheme
	//vector<CpAstNode> typeParams; n.filter(CXCursor_TemplateTypeParameter, typeParams);
	vector<CpAstNode> args; n.filterByKind(CXCursor_ParmDecl, args);
	//emitRust_GenericTypeParams(typeParams);
	emitRust_FunctionArguments(n,depth,nullptr);
	EMIT("->%s",emitRust_FunctionReturnType_asStr(n,0,0).c_str());


	EMIT(";}\n");
}

fn emitCpp2CShim_GlobalFunctionDecl(const AstNode&n, EmitContext depth)->void 	{
	if (!shouldEmitFunction(&n)) return;

	auto rtnType=emit_FunctionReturnType_asStr(EL_CPP,n,0,0);
	EMIT_INDENT(depth,"extern \"C\" %s %s",rtnType.c_str(), n.cname());
	vector<CpAstNode> args; n.filterByKind(CXCursor_ParmDecl, args);
	emit_FunctionArguments(EL_CPP, n,depth,nullptr);
	EMIT("{");
	EMIT("return %s(",n.cname());
	apply_separated(args,
		[](CpAstNode n) { EMIT(n->cname());},
		[](CpAstNode n) { EMIT(",");}
	);
	EMIT(");");
//	emitCpp_FunctionArguments(n,depth,nullptr);

	EMIT("};\n");
}


fn emitRust_Constructor(const AstNode& n, EmitContext depth, const char* selfType,bool emitCShim)->void {
	/*
		// todo - wrappers for constructors with overload underscore qualifiers
	*/
	EMIT_INDENT(depth,"pub fn new",n.cname());
	emitRust_FunctionArguments(n, depth, nullptr);
	EMIT("->");
	EMIT(selfType);

	if (selfType && emitCShim) {
		EMIT("{\n");
		EMIT_INDENT(depth+1,"unsafe{ new_%s",n.cname());
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

fn emitRust_Destructor(const AstNode& n, EmitContext depth, const char* selfType, bool emitCShim)->void {
	/*
		// todo - wrappers for constructors with overload underscore qualifiers
	*/
	EMIT_INDENT(depth,"pub fn drop(&self)",n.cname());
	if (emitCShim && selfType) {
		EMIT("{ unsafe { delete_%s(self)} }", selfType);
	}
	EMIT(";\n");
}


fn emitRust_InnerDecls(const AstNode& n, const vector<CpAstNode>& decls, EmitContext depth)->void {
	EMIT_INDENT(depth,"mod %s { // inner declarations of %s\n", n.cname(),n.cname());
	for (auto&sn: decls) {
		emitRustItem(EmitRustMode_Rust, sn, depth+1);
	}
	EMIT_INDENT(depth,"} //mod %s\n",n.cname());
}


fn emitRust_Enum(const AstNode& n, EmitContext depth)->void {
	vector<CpAstNode>	decls;
	n.filterByKind(CXCursor_EnumConstantDecl,decls);
	EMIT_INDENT(depth,"enum %s {\n",n.cname());
	int	i;
	int	enumValue=0;
	for (i=0; i<decls.size(); i++) {
		auto d=decls[i];
		EMIT_INDENT(depth+1,"%s", d->cname());
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
fn emitCpp2CShim_ClassTemplate(const AstNode& n, EmitContext depth)->void {
	vector<CpAstNode> methods;
	n.filterByKind(CXCursor_CXXMethod,methods);
	const char* selfType = n.name.c_str();
	EMIT("//struct %s numMethods=%d\n",n.name.c_str(),methods.size());
	
	for (auto &m:methods) {
		if (!shouldEmitFunction(m)) continue;
		
		EMIT_INDENT(depth,"extern \"C\" %s\t%s_%s",emit_FunctionReturnType_asStr(EL_CPP,*m,0,0).c_str(), selfType, m->cname());

// todo , what to do with templates...
//		vector<CpAstNode> typeParams; n.filter(CXCursor_TemplateTypeParameter, typeParams);
		vector<CpAstNode> args; m->filterByKind(CXCursor_ParmDecl, args);
//		emitRust_GenericTypeParams(typeParams);
//.		emitRust_FunctionArguments(*m,depth,selfType);
		emit_FunctionArguments(EL_CPP, *m,depth,selfType);

			

		EMIT(" {\n");
		EMIT_INDENT(depth+1,"return self->%s(",m->cname());
		int	i;
		for (i=0; i<args.size(); i++) {
			EMIT((i>0)?",%s":"%s",args[i]->cname());
		}
		EMIT(");\n");
		EMIT_INDENT(depth,"};\n");
	}
}
fn emitRust_ClassTemplate(const AstNode& n, EmitContext depth)->void {

	// filter template params...
//	n.filter([](AstNode& n){ if (n==XCursor_TemplateT)}
	vector<CpAstNode> typeParams;
	n.filterByKind(CXCursor_TemplateTypeParameter, typeParams);
	vector<CpAstNode> fields;
	n.filterByKind(CXCursor_FieldDecl, fields);
	vector<CpAstNode> methods;
	n.filterByKind(CXCursor_CXXMethod, methods);
	vector<CpAstNode> innerDecls;

	n.filterByKind(CXCursor_StructDecl, innerDecls);
	n.filterByKind(CXCursor_ClassDecl, innerDecls);
	n.filterByKind(CXCursor_ClassTemplate, innerDecls);
	n.filterByKind(CXCursor_EnumDecl, innerDecls);
	n.filterByKind(CXCursor_TypedefDecl, innerDecls);

	auto base = n.findFirst(CXCursor_CXXBaseSpecifier);

	auto dtr=n.findFirst(CXCursor_Destructor);
	if (dtr)  {
		EMIT_INDENT(depth,"impl Drop for %s {\n", n.cname());
		emitRust_Destructor(*dtr,depth+1, n.cname(),true);
		EMIT_INDENT(depth,"}\n");
	}
	// todo - filter what this is really.
	// only make it a rust struct if it has data elements?
	// ..otherwise if its a collection of functions/types it's really trait?

	// other metadata for binding ?
	EMIT_INDENT(depth,"struct\t%s", n.cname());
	emitRust_GenericTypeParams(typeParams);

	if (!fields.size()) { EMIT("\t{}\n");}
	else {
		EMIT("\t{\n",n.cname());
		if (base) {
			EMIT_INDENT(depth+1,"tbase:%s,\n",base->cname());
		}
		apply_separated(fields,
			[&](CpAstNode& s) {
				EMIT_INDENT(depth+1,"%s:", s->cname());
                EMIT("%s",emit_CXType(EL_RUST, s->cxType).c_str());
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

		EMIT("%s",n.cname());
		emitRust_GenericTypeParams(typeParams);

		EMIT(" {\n" );
//		auto f=n.findFirst(CXCursor_Constructor);
		auto ctr=emitRust_FindDefaultConstructor(n);
		if (ctr) 
			emitRust_Constructor(*ctr,depth+1, n.cname(),true);
		for (auto &m:methods) {
			emitRust_FunctionDecl(*m,depth+1, n.cname(), true);
		}
		EMIT_INDENT(depth,"}\n");
		if (ctr) {
			EMIT_INDENT(depth,"extern{ fn new_%s",n.cname());
			emitRust_FunctionArguments(*ctr,depth,nullptr);
			EMIT("->*%s;}\n",n.cname(),n.cname(),n.cname());
		}
		// emit C shim prototypes..
		for (auto &m:methods) {	
			EMIT_INDENT(depth,"extern{ fn %s_%s",n.cname(),m->cname());
			emitRust_FunctionArguments(*m,depth,n.cname());
			EMIT("->%s;",emitRust_FunctionReturnType_asStr(*m,0,0).c_str());
			EMIT("}\n");
		}
	}
	if (dtr) {
		EMIT_INDENT(depth,"extern{ fn delete_%s(&self_ptr:*%s);}\n",n.cname(),n.cname())
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

fn emitRust_GatherFunctionsAsMethodsAndTraits(const AstNode& n,EmitContext depth)->void{
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
        auto type=emit_CXType(EL_RUST,firstParam->cxType);
		const char* typeName=type.c_str();
		if (typeName[0]=='*'||typeName[0]=='&')typeName++;

		const char* s1,*s2;
		char lastMatch=0;
		for (s1=sn.cname(),s2=typeName; *s1&&*s1 && *s1==*s2; s1++,s2++) {
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

fn emitRustItem(EmitRustMode m,CpAstNode n,int depth)->bool
{
	if (m==EmitRustMode_Rust) {
		switch (n->nodeKind) {
			case CXCursor_EnumDecl:
				emitRust_Enum(*n,depth);
				return	 true;
			case CXCursor_StructDecl:
			case CXCursor_ClassDecl:
			case CXCursor_ClassTemplate:
				emitRust_ClassTemplate(*n,depth);
			break;
			case CXCursor_FunctionDecl:
			case CXCursor_FunctionTemplate:
				emitRust_GlobalFunctionDecl(*n,depth);
				return 	true;
			break;
		}
	} else if (m==EmitRustMode_CppShim) {
		switch (n->nodeKind) {
			case CXCursor_StructDecl:
			case CXCursor_ClassDecl:
			case CXCursor_ClassTemplate:
				emitCpp2CShim_ClassTemplate(*n,depth);
				return 	true;
			break;
			case CXCursor_FunctionDecl:
			case CXCursor_FunctionTemplate:
				// no need for rust shims for global functions.
				// .. because we manually make these extern "C"?
//				emitCpp2CShim_GlobalFunctionDecl(*n,depth);
				return 	true;
            break;
		}
	}
	return false;
}

// overloaded methods
// could a tuple struct dispatch it?
//
// Yada(a,b,c).yada();
//
//

fn emitRustRecursive(EmitRustMode m,const AstNode& n,EmitContext depth)->void {
	#define EMIT_TYPE(T) \
		case CXCursor_ ## T: emitRust_ ## T(n,depth); break;
	bool didEmit=emitRustItem(m,&n,depth);
	if(!didEmit) {
		for (auto& sn: n.subNodes) {
			emitRustRecursive(m,sn,depth);
		}
	}
	emitRust_GatherFunctionsAsMethodsAndTraits(n,depth);
}
#undef EMIT
