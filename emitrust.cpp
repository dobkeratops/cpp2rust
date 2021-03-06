#include "emitrust.h"
#include <algorithm>
// temporary until we've perfected how const -> mut

// KNOWN BUGS: Nested Class with Type Parameters:
// it doesn't propogate the type-params to interior nodes that need them.
// it doesn't know how to re-word/store interior nodes' type-params.
// Better to spit out an error message about it.

#define NO_MUTABLE

#define ER_TRACE(X)

#define EMIT(...) fprintf(gOut, __VA_ARGS__)
#define EMIT_INDENT(D,...) {indent(D);fprintf(gOut,__VA_ARGS__);}
fn LangOf(EmitRustMode m)->EmitLang {
	switch (m) {
		case EmitRustMode_Rust: return EL_RUST;
		case EmitRustMode_CppShim: return EL_CPP;
		default: printf("%s:%d: unhandled case\n",__FILE__,__LINE__); exit(0);
	}
}

fn emitRustRecursive(EmitCtx& ec,EmitRustMode m,const AstNode& n,EmitContext depth)->void;
fn emitRustItem(EmitCtx& ec,EmitRustMode m,const AstNode* item,EmitContext depth)->bool;
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

fn combineSigilInTypename(EmitLang l, const string& qualifier, const string& s, const char* sigil)->string {
    if (l==EL_RUST || EL_CPP ) {
        switch (l) {
            case EL_RUST: return string(sigil)+qualifier+s;
            case EL_CPP: return qualifier+s+string(sigil);  // not so sure :(
        }
    } else {
        return qualifier+ string(sigil)+s;
    }
}

fn combinePtrInTypename(EmitLang l, const string& qualifier,const string& typestr)->string
{
    return combineSigilInTypename(l,qualifier,typestr,"*");
}
fn combineRefInTypename(EmitLang l, const string& qualifier, const string& typestr)->string
{
    return combineSigilInTypename(l,qualifier,typestr,"&");
}

bool alphanumeric(const char c) {
	return (c>='a' && c<='z') || (c>='A' && c<='Z') || c=='_' || (c>='0' && c<='9');
}

bool shouldEmitFunction(EmitCtx& ec, CpAstNode m) {
	// todo: rename operators for binding
	 // Dont do operators. new/delete. 
	 // details of rust/c++ creating eachothers objects would be scary.
	 // we just want to pass useable references across.
	const char* s0=m->cname(), *s1="operator";
	for (; *s0 && *s1; s0++,s1++) {
		if (*s0!=*s1) return true;
	}
	char c1=*s1;
	char c0=*s0;
	if (!c1) { // all characters matched.. and we break a csymbol here -its an operator. return false;
		if (!alphanumeric(c0)) return false;
	}
	return true;
}

string emit_CXTypeDecl(EmitCtx& ec, enum EmitLang lang, CXCursor c) {
	CXString name=clang_getCursorSpelling(c);
	string str(clang_getCString(name));
	if (!str.size()) {
		str="c_void /*error,was nameless*/"; printf("error got a nameless type, writing as c_void");
	} else {
		if (!ec.referenced_symbols.count(str)) printf("REFERENCING: %s\n",str.c_str());
		ec.referenced_symbols.insert(str);
	}
//	if (strcmp(str.c_str(), "struct")>=strlen("struct")) {
	if (str.length()>=strlen("struct") && str[0]=='s' && str[1]=='t' && str[2]=='r' && str[3]=='u' && str[4]=='c'  && str[5]=='t') {
		printf("got struct in name, %s\n", str.c_str());
//		exit(0);
	}
	clang_disposeString(name);

	return str;
}
enum MutabilityInfo {
    IM_Mutable, IM_Immutable, IM_DontCare
};

// todo - how to handle the mutability swizzle ?
// absence of const modifier requires mut. const modifier is to be ignored.
auto emit_CXTypeRec(EmitCtx& ec, enum EmitLang lang, CXType cxType, MutabilityInfo mut_in)->string {
    string str;

    MutabilityInfo is_mutable = (mut_in==IM_DontCare)?IM_DontCare:(clang_isConstQualifiedType(cxType)?IM_Immutable:IM_Mutable);


    string qualifier="";
    // todo: wrap immutability as a call, it might be post/prefix in other syntax?

    if ( lang==EL_RUST) {
        if ((is_mutable==IM_Mutable) && mut_in==IM_Immutable && mut_in!=IM_DontCare) {
            qualifier+="mut ";
        }
    } else if (lang!=EL_RUST) {
        if ((is_mutable==IM_Immutable) && mut_in!=IM_DontCare) {
            //qualifier+=mut_in==IM_DontCare?"IM_DontCare":(mut_in==IM_Immutable)?"IM_DontCare":"IM_Mutable";
            qualifier+="const " ;
        }
    }
#ifdef NO_MUTABLE
	if (lang==EL_RUST) {
		qualifier="";
	}
#endif
    // TODO: verify these basic cases work! where is the qualifier?
    //  const Foo& f    => f:&Foo
    //  Foo& f    => f:&mut Foo
    //  Foo& f    => f:&mut Foo


	CXCursor c=clang_getTypeDeclaration(cxType);

    switch (cxType.kind) {
        case CXType_Invalid: //get from the node, but we dont understand why :(//string("CXTY_INVALID");
        case CXType_Unexposed: //get from the node, but we dont understand why we ever get this :(//string("CXTY_UNEXPOSED");
        case CXType_Enum:
        case CXType_Record:
            //str.append("RECORD=");
            if (lang!=EL_RUST) str+=qualifier;
            str+=emit_CXTypeDecl(ec,lang,c);//get from the node, but we dont understand why :(
            break;
        case CXType_Pointer:{
            auto pointee=clang_getPointeeType(cxType);
            str+= combinePtrInTypename(lang, qualifier,emit_CXTypeRec(ec,lang, pointee, is_mutable));
            break;
        }
        case CXType_RValueReference:
        case CXType_LValueReference:
            str+= combineRefInTypename(lang, qualifier,emit_CXTypeRec(ec,lang, clang_getPointeeType(cxType), is_mutable));
        break;

        case CXType_Typedef:
            //str.append("TYPDEF=");
            if (lang!=EL_RUST) str+=qualifier;
            str+=emit_CXTypeDecl(ec, lang,c); // hmm, but we *can* have template params on typedefs :(
            //str.append(CXType_to_str(cxType,lang));//emit_CXTypeDecl(lang, c);
            return str; // no template params.
        break;

        default:
            if (lang!=EL_RUST) str+=qualifier;
            str.append(CXType_to_str(cxType,lang));//emit_CXTypeDecl(lang, c);
    }

    // todo: with this propper handling of template args and so on... we can eliminate all the first|SubNodes junk
    signed int numtyargs=clang_Type_getNumTemplateArguments(cxType);    //clang says  -1 = no template args.
    if (numtyargs>=0) {
        str+="<";
        for (int i=0; i<numtyargs; i++) {
            if (i>0) str+=",";
            auto ty=clang_Type_getTemplateArgumentAsType(cxType, i);
            str +=emit_CXTypeRec(ec,lang, ty, is_mutable);
        }
        str+=">";
    }
    return str;
}
auto emit_CXType(EmitCtx& ec, enum EmitLang lang, CXType cxType)->string {
    let ret=emit_CXTypeRec(ec,lang,cxType,(lang==EL_RUST)?IM_Immutable:IM_Mutable /* have we said its mutable yet */);
	return ret;
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

fn emitRust_GenericTypeParams(EmitCtx& ec, vector<CpAstNode>& typeParams)->void {
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

fn emit_FunctionArguments(EmitCtx& ec, EmitLang lang,const AstNode&n, EmitContext depth,bool asMethod, const char* selfType, bool isSelfMutable)->void
{
	vector<CpAstNode> args; vector<int> arg_indices; 
	n.filterIndexed([](CpAstNode p){return p->nodeKind==CXCursor_ParmDecl;}, args,arg_indices);
	EMIT("(");
	if (selfType) {
        // todo - mutability of self seems completely wrong:(
		if (asMethod) {
			if (lang==EL_RUST) {
				EMIT(isSelfMutable?"&mut self":"&self");
		     } else {
		         EMIT(isSelfMutable?"%s *self":"const %s *self",selfType);
		     }
		} else {
			if (lang==EL_RUST) {
				EMIT(isSelfMutable?"mut this:&%s":"this:&%s", selfType);
		     } else {
		         EMIT(isSelfMutable?"%s *self":"const %s *self",selfType);
		     }
		}
		if (args.size()) EMIT(",");
	}
	apply_separated(args,
					[&](CpAstNode& s) {
			auto tn=emit_CXType(ec,lang, s->cxType);
			auto argname=s->cname();
			if (!strlen(argname)) {
				argname="_";
			}
			if (!strcmp(argname,"fn")) argname="_fn"; // rust reserved words..
			if (!strcmp(argname,"in")) argname="_in"; // rust reserved words..
			if (!strcmp(argname,"of")) argname="_of";
			if (!strcmp(argname,"Trait")) argname="_trait";
			if (!strcmp(argname,"crate")) argname="_crate";
			if (!strcmp(argname,"self")) argname="_self";
			if (!strcmp(argname,"Self")) argname="_Self";
			if (!strcmp(argname,"mut")) argname="_mut";
			if (!strcmp(argname,"let")) argname="_let";
			if (lang==EL_RUST) {
				EMIT("%s:%s",argname,tn.c_str());
			} else {
				EMIT("%s %s",tn.c_str(), s->cname() );
			}
		},
		[](CpAstNode& s){EMIT(",");}
	);
	EMIT(")");
}

fn emitRust_FunctionArguments(EmitCtx& ec, const AstNode&n, EmitContext depth,bool asMethod, const char* selfType, bool isSelfMutable)->void {
    return emit_FunctionArguments(ec,EL_RUST,n, depth,asMethod, selfType,isSelfMutable);
}

fn emit_FunctionReturnType_asStr(EmitCtx& ec, EmitLang lang, const AstNode& n, vector<AstNode>* parent, int index)->string{
    return emit_CXType(ec,lang,n.resultType);
}
fn emitRust_FunctionReturnType_asStr(EmitCtx& ec, const AstNode& n, vector<AstNode>* parent, int index)->string{
    return emit_CXType(ec, EL_RUST,n.resultType);
}

fn emit_CShimArgs(EmitCtx& ec, EmitLang lang,const AstNode& n, EmitContext depth, const char* selfType, bool isSelfMutable)->void {
	vector<CpAstNode> args; n.filterByKind(CXCursor_ParmDecl, args);
	EMIT("(");
    if (isSelfMutable) {
        EMIT(lang==EL_RUST?"mut self%s":"self%s",args.size()?",":"");
    } else {
        EMIT(lang==EL_RUST?"self%s":"const self%s",args.size()?",":"");
    }
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
fn emitRust_CShimArgs(EmitCtx& ec, const AstNode& n, EmitContext depth, const char* selfType,bool isSelfMutable)->void {
    emit_CShimArgs(ec,EL_RUST,n,depth,selfType,isSelfMutable);
}

fn emitRust_TypeParamsOfNode(EmitCtx& ec, const AstNode& n)->void {
	vector<CpAstNode> typeParams; n.filterByKind(CXCursor_TemplateTypeParameter, typeParams);
	emitRust_GenericTypeParams(ec,typeParams);
}

fn emitRust_FunctionDecl(EmitCtx& ec, const AstNode&n, EmitContext depth,bool asMethod, const char* selfType,bool isSelfMutable, bool emitRustToCShimCall)->void 	{
	if (!shouldEmitFunction(ec,&n)) return; 
	EMIT_INDENT(depth,"pub fn %s",n.cname());
    //isSelfMutable = ??? perhaps the 'this pointer' is actually one down?
    // perhaps its a modifier of the method node.. one we missed in building our ast copy.
	vector<CpAstNode> args; n.filterByKind(CXCursor_ParmDecl, args);
	emitRust_TypeParamsOfNode(ec,n);
    emitRust_FunctionArguments(ec,n,depth,asMethod, selfType,isSelfMutable);
	EMIT("->%s",emitRust_FunctionReturnType_asStr(ec,n,0,0).c_str());

	if (emitRustToCShimCall && selfType) {
		EMIT("{\n");
		EMIT_INDENT(depth+1,"unsafe { %s_%s",selfType,n.cname());
        emitRust_CShimArgs(ec,n,depth,selfType,isSelfMutable);
		EMIT(" }\n");
		EMIT_INDENT(depth,"}");

	}
	EMIT("\n");
}

fn emitRust_GlobalFunctionDecl(EmitCtx& ec, const AstNode&n, EmitContext depth)->void 	{
	// TODO - filter and dont emit global functions that are already declared "extern "C"
	if (!shouldEmitFunction(ec,&n)) return;
	EMIT_INDENT(depth,"extern { pub fn %s",n.cname());
	// TODO - some instantiation of some requested types... 
	// with a rule for emiting C shims with a naming scheme
	//vector<CpAstNode> typeParams; n.filter(CXCursor_TemplateTypeParameter, typeParams);
	vector<CpAstNode> args; n.filterByKind(CXCursor_ParmDecl, args);
	//emitRust_GenericTypeParams(typeParams);

    emitRust_FunctionArguments(ec,n,depth,false, nullptr, false);
	EMIT("->%s",emitRust_FunctionReturnType_asStr(ec,n,0,0).c_str());


	EMIT(";}\n");
}

fn emitCpp2CShim_GlobalFunctionDecl(EmitCtx& ec, const AstNode&n, EmitContext depth)->void 	{
	if (!shouldEmitFunction(ec,&n)) return;
    auto is_self_mutable=false;
	auto rtnType=emit_FunctionReturnType_asStr(ec,EL_CPP,n,0,0);
	EMIT_INDENT(depth,"extern \"C\" %s %s",rtnType.c_str(), n.cname());
	vector<CpAstNode> args; n.filterByKind(CXCursor_ParmDecl, args);
    emit_FunctionArguments(ec,EL_CPP, n,depth, false, nullptr,is_self_mutable);
	EMIT("{");
	EMIT("return %s(",n.cname());
	apply_separated(args,
		[](CpAstNode n) { EMIT("%s",n->cname());},
		[](CpAstNode n) { EMIT(",");}
	);
	EMIT(");");
//	emitCpp_FunctionArguments(n,depth,nullptr);

	EMIT("};\n");
}


fn emitRust_Constructor(EmitCtx& ec, const AstNode& n, EmitContext depth, const char* selfType,bool emitCShim)->void {
	/*
		// todo - wrappers for constructors with overload underscore qualifiers
	*/
	EMIT_INDENT(depth,"pub fn new");
    emitRust_FunctionArguments(ec,n, depth, true, nullptr,false);
	EMIT("->");
	EMIT("%s",selfType);

	if (selfType && emitCShim) {
		EMIT("{\n");
		EMIT_INDENT(depth+1,"unsafe{ new_%s",n.cname());
        emitRust_CShimArgs(ec,n,depth,selfType,false);
		EMIT("}\n");
		EMIT_INDENT(depth,"}");
	}

	EMIT("\n");
}

fn emitRust_FindDefaultConstructor(EmitCtx& ec, const AstNode& n)->CpAstNode {
	for (auto &sn:n.subNodes) {
		if (sn.nodeKind==CXCursor_Constructor)
			if (sn.count(CXCursor_ParmDecl))
				return &sn;
	}
	return n.findFirst(CXCursor_Constructor);
}

fn emitRust_Destructor(EmitCtx& ec, const AstNode& n, EmitContext depth, const char* selfType, bool emitCShim)->void {
	/*
		// todo - wrappers for constructors with overload underscore qualifiers
	*/
	EMIT_INDENT(depth," fn drop(&mut self)");
	if (emitCShim && selfType) {
		EMIT("{ unsafe { delete_%s(self)} }", selfType);
	}
	EMIT("\n");
}


fn emitRust_InnerDecls(EmitCtx& ec, const AstNode& n, const vector<CpAstNode>& decls, EmitContext depth)->void {
	EMIT_INDENT(depth,"pub mod %s { // inner declarations of %s\n", n.cname(),n.cname());
	emitRustModPrefix(ec,n,depth+1);
	for (auto&sn: decls) {
		emitRustItem(ec,EmitRustMode_Rust, sn, depth+1);
	}
	EMIT_INDENT(depth,"} //mod %s\n",n.cname());
}

// a mod is handled the same as a namespace.
// inner decls can come from a namespace, or a class/struct/..

fn emitRust_Mod(EmitCtx& ec, const AstNode& n, int depth)->void {
	vector<CpAstNode> subNodes;
	for (auto& x: n.subNodes) subNodes.push_back(&x);
	emitRust_InnerDecls(ec, n, subNodes, depth);
}

fn emitRust_Enum(EmitCtx& ec, const AstNode& n, EmitContext depth)->void {
	if (!strlen(n.cname())) { printf("error trying to emit anon enum?!\n"); return; }
	ASSERT(n.name!="vector");
	ec.define(n.name);//.insert(n.name);

	vector<CpAstNode>	decls;
	n.filterByKind(CXCursor_EnumConstantDecl,decls);
	EMIT_INDENT(depth,"pub enum %s {\n",n.cname());
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
fn emitCpp2CShim_ClassTemplate(EmitCtx& ec, const AstNode& n, EmitContext depth)->void {
	vector<CpAstNode> methods;
	n.filterByKind(CXCursor_CXXMethod,methods);
	const char* selfType = n.name.c_str();
	EMIT("//struct %s numMethods=%d\n",n.name.c_str(),(int) methods.size());
	
	for (auto &m:methods) {
		if (!shouldEmitFunction(ec,m)) continue;
		
		EMIT_INDENT(depth,"extern \"C\" %s\t%s_%s",emit_FunctionReturnType_asStr(ec,EL_CPP,*m,0,0).c_str(), selfType, m->cname());
        bool is_self_mutable= true;//clang_isConstQualifiedType(m->cxType); // not sure, does this get our const qualifier?

// todo , what to do with templates...
//		vector<CpAstNode> typeParams; n.filter(CXCursor_TemplateTypeParameter, typeParams);
		vector<CpAstNode> args; m->filterByKind(CXCursor_ParmDecl, args);
//		emitRust_GenericTypeParams(typeParams);
//.		emitRust_FunctionArguments(*m,depth,selfType);
        emit_FunctionArguments(ec,EL_CPP, *m,depth,false, selfType, is_self_mutable);

			

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

// contents of an ast node, but filtered into specific types instead of in declaratin order.
template<typename T,typename NODE>
struct FilteredNodeContents {
	vector<T> typeParams;
	vector<T> fields;
	vector<T> methods;
	vector<T> innerDecls;

	FilteredNodeContents(NODE& n){
		n.filterByKind(CXCursor_TemplateTypeParameter, typeParams);
		n.filterByKind(CXCursor_FieldDecl, fields);
		n.filterByKind(CXCursor_CXXMethod, methods);

		n.filterByKind(CXCursor_StructDecl, innerDecls);
		n.filterByKind(CXCursor_ClassDecl, innerDecls);
		n.filterByKind(CXCursor_ClassTemplate, innerDecls);
		n.filterByKind(CXCursor_EnumDecl, innerDecls);
		n.filterByKind(CXCursor_TypedefDecl, innerDecls);
	}
};
void AstNode::findModClasses(vector<pair<CpAstNode,CpAstNode>>& results) const {
	for (auto &sn : this->subNodes) {
		if (sn.nodeKind != CXCursor_Namespace)
			continue;
		for (auto& ssn : sn.subNodes) {
			if (sn.name!=ssn.name)
				continue;
			auto ssnk=ssn.nodeKind;
			if (!(ssnk==CXCursor_StructDecl || ssnk==CXCursor_ClassDecl || ssnk==CXCursor_ClassTemplate))
				continue;
			// here we are.
			results.push_back(make_pair(&sn,&ssn));
		}
	}
}


fn emitRust_transformNestedClassesToMods(AstNode& n)->void {
	auto nk=n.nodeKind;
	if (	(n.count(CXCursor_StructDecl)+	n.count(CXCursor_ClassDecl)+n.count(CXCursor_ClassTemplate)+n.count(CXCursor_EnumDecl)+n.count(CXCursor_TypedefDecl))>0 &&
			(nk==CXCursor_StructDecl || nk==CXCursor_ClassDecl || nk==CXCursor_ClassTemplate)
		)
	{
		printf("Creating module to hold subnodes %s\n",n.name.c_str());
		FilteredNodeContents<AstNode*,AstNode&> fnc(n);
		AstNode new_this;
		AstNode new_mod;
		new_mod.nodeKind=CXCursor_Namespace; // closest approx to a rust mod is a sepples namespace
		new_mod.parent=&n;
		new_mod.name=n.name; 
		std::transform(new_mod.name.begin(), new_mod.name.begin(), new_mod.name.end(), ::tolower);// life without lambdas, hell.
		new_mod.typeName="";
		for (int i=0; i<fnc.innerDecls.size(); i++) {
			new_mod.subNodes.push_back(*fnc.innerDecls[i]);
		}
		// we hope this is all move semantics, not deepcopy.
		new_this.nodeKind=n.nodeKind;
		new_this.name=n.name;
		new_this.typeName=n.typeName;
		new_this.cxType=n.cxType;
		new_this.resultType=n.resultType;

		for (auto&x : fnc.typeParams) {new_this.subNodes.push_back(*x);}
		for (auto&x : fnc.fields) {new_this.subNodes.push_back(*x);}
		for (auto&x : fnc.methods) {new_this.subNodes.push_back(*x);}

		// 'n' becomes a module we just described, containing a struct that was this
		n=new_mod;
		n.subNodes.push_back(new_this);
		for (int i=0; i<n.subNodes.size();i++) { n.subNodes[i].parent=&n;}		
	}
	// continue scanning..
	for (auto &subnode :n.subNodes) { emitRust_transformNestedClassesToMods(subnode);}
}

fn emitRust_ClassTemplate(EmitCtx& ec,const AstNode& n, EmitContext depth)->void {
	auto cn=n.cname(); if (!strlen(cn)) {printf("error trying to emit nameless structure\n"); return; }

	// filter template params...
//	n.filter([](AstNode& n){ if (n==XCursor_TemplateT)}
	FilteredNodeContents<CpAstNode,const AstNode&>  fnc(n);

/*	if (!(fields.size() || methods.size() || innerDecls.size())) {
		printf("anon struct, r.e. unhandled duplicate/forward decl??");
		return;
	}
*/

	ec.define(n.name);
	auto base = n.findFirst(CXCursor_CXXBaseSpecifier);
#ifdef EMIT_DESTRUCTORS
	auto dtr=n.findFirst(CXCursor_Destructor);
	if (dtr)  {
		EMIT_INDENT(depth,"impl Drop for %s {\n", n.cname());
		emitRust_Destructor(ec,*dtr,depth+1, n.cname(),true);
		EMIT_INDENT(depth,"}\n");
	}
#endif
    // todo - filter what this is really.
	// only make it a rust struct if it has data elements?
	// ..otherwise if its a collection of functions/types it's really trait?

	// other metadata for binding ?
	EMIT_INDENT(depth,"pub struct\t%s", n.cname());
	emitRust_GenericTypeParams(ec,fnc.typeParams);

	if (!fnc.fields.size()) { EMIT(";\n");}
	else {
		EMIT("\t{\n");
		if (base) {
			EMIT_INDENT(depth+1,"tbase:%s,\n",base->cname()+strlen("struct"));
		}
		apply_separated(fnc.fields,
			[&](CpAstNode& s) {
				EMIT_INDENT(depth+1,"%s:", s->cname());
                EMIT("%s",emit_CXTypeRec(ec,EL_RUST, s->cxType,IM_DontCare).c_str());
			},
			[](CpAstNode& s) {
				EMIT(",\n");
			}
		);
		EMIT("\n");
		EMIT_INDENT(depth,"}\n");
	}
	if  (fnc.methods.size()) {
        // todo: gather overloaded methods
		// and emit postfixed types
		EMIT_INDENT(depth,"impl ");
		emitRust_GenericTypeParams(ec,fnc.typeParams);

		EMIT("%s",n.cname());
		emitRust_GenericTypeParams(ec,fnc.typeParams);

		EMIT(" {\n" );
//		auto f=n.findFirst(CXCursor_Constructor);
		auto ctr=emitRust_FindDefaultConstructor(ec,n);
		if (ctr) 
			emitRust_Constructor(ec,*ctr,depth+1, n.cname(),true);
		for (auto &m:fnc.methods) {
            bool isSelfMutable= false;//clang_isConstQualifiedType(m->cxType);
            emitRust_FunctionDecl(ec,*m,depth+1, true, n.cname(),isSelfMutable,  true);
		}
		EMIT_INDENT(depth,"}\n");
		if (ctr) {
			EMIT_INDENT(depth,"extern{ pub fn new_%s",n.cname());
            //auto selfCXType = m.cxType; // not sure, does this get our const qualifier?
            emitRust_FunctionArguments(ec,*ctr,depth,true, nullptr, false);
			EMIT("->*%s;}\n",n.cname());
		}
		// emit C shim prototypes..
		for (auto &m:fnc.methods) {	
			EMIT_INDENT(depth,"extern{ pub fn %s_%s",n.cname(),m->cname());
            emitRust_FunctionArguments(ec,*m,depth,false, n.cname(),false);
			EMIT("->%s;",emitRust_FunctionReturnType_asStr(ec,*m,0,0).c_str());
			EMIT("}\n");
		}
	}
#ifdef EMIT_DESTRUCTORS
	if (dtr) {
		EMIT_INDENT(depth,"extern{ fn delete_%s(self_ptr:*mut %s);}\n",n.cname(),n.cname())
	}
#endif

	// if our transformation worked.. this doesn't happen
	if (fnc.innerDecls.size()>0)
		emitRust_InnerDecls(ec,n,fnc.innerDecls,depth);
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

fn emitRust_GatherFunctionsAsMethodsAndTraits(EmitCtx& ec, const AstNode& n,EmitContext depth)->void{
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
        auto type=emit_CXType(ec,EL_RUST,firstParam->cxType);
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
				emitRust_FunctionDecl(ec,*v.second,depth,nullptr);
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

fn emitRustItem(EmitCtx& ec, EmitRustMode m,CpAstNode n,int depth)->bool
{
//	static set<string> emitted_names;

	if (m==EmitRustMode_Rust) {
		if (ec.emit_this_pass.count(n->name)>0) {
			printf("error duplicate emission, will fail if we had forward decls?!");
			return true;
		}
//		ec.emit_this_pass.insert(n->name);
//		ASSERT(n->name()!="vector");
//		ec.defined_symbols.insert(n->name);
		switch (n->nodeKind) {
			case CXCursor_Namespace:
				emitRust_Mod(ec,*n,depth);
				return true;
			case CXCursor_TypedefDecl:  {
				if (n->subNodes.size()>0) {
					printf("TYPEDEF %s %s{%s}\n", n->name.c_str(), 
					   CXCursorKind_to_str(n->subNodes[0].nodeKind), 
						n->subNodes[0].name.c_str() ); 
//					exit(0);
				} else {
					printf("TYPEDEF %s %s %s \n" ,n->name.c_str(), n->typeName.c_str(), CXType_to_str(n->cxType,EL_RUST));
//					exit(0);
				}
				return true;
			} 
			case CXCursor_EnumDecl:
				emitRust_Enum(ec,*n,depth);
				return	 true;
			case CXCursor_StructDecl:
			case CXCursor_ClassDecl:
			case CXCursor_ClassTemplate:
				emitRust_ClassTemplate(ec,*n,depth);
			break;
			case CXCursor_FunctionDecl:
			case CXCursor_FunctionTemplate:
				emitRust_GlobalFunctionDecl(ec,*n,depth);
				return 	true;
			break;
		}
	} else if (m==EmitRustMode_CppShim) {
		switch (n->nodeKind) {
			case CXCursor_StructDecl:
			case CXCursor_ClassDecl:
			case CXCursor_ClassTemplate:
				emitCpp2CShim_ClassTemplate(ec,*n,depth);
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

fn emitRustRecursive(EmitCtx& ec, EmitRustMode m,const AstNode& n,EmitContext depth)->void {
	#define EMIT_TYPE(T) \
		case CXCursor_ ## T: emitRust_ ## T(n,depth); break;
	bool didEmit=emitRustItem(ec, m,&n,depth);
	if(!didEmit) {
		for (auto& sn: n.subNodes) {
		emitRustRecursive(ec, m,sn,depth);
		}
	}
	emitRust_GatherFunctionsAsMethodsAndTraits(ec, n,depth);
}
void emitXRefs(EmitCtx& ec, const AstNode& n,int depth){
	for (auto sym=ec.referenced_symbols.begin(); sym!=ec.referenced_symbols.end(); ++sym) {
		if (ec.defined_symbols.count(*sym)==0) {
	//printf("XREFD: %s\n", (*sym).c_str());
		EMIT_INDENT(depth,"use %s;\n", (*sym).c_str());
		} else {
	//printf("DEFD: %s\n", (*sym).c_str());

		}
	}
	for (auto sym=ec.defined_symbols.begin(); sym!=ec.defined_symbols.end(); ++sym) {
	//printf("DEFINED IN MODULE: %s\n", (*sym).c_str());
	}
	//printf("DEFINED %d REFERENCED: %d\n", ec.defined_symbols.size(), ec.referenced_symbols.size());
}


fn emitRustModPrefix(EmitCtx& ec, const AstNode& n,EmitContext depth)->void {
	// todo, indent ..
	EMIT_INDENT(depth,"use super::*;\n");// todo - submodules need to be more intelligent, we need nested scopes (EmitCtx)
	EMIT_INDENT(depth,"use std::libc::{c_void, c_char,c_uchar, c_short,c_ushort,c_long,c_ulong,c_int,c_uint, c_float,c_double,c_longlong,c_ulonglong };\n");
	// find 'mod-classes' - C++ classes with inner-classes, 
	// eg class X{ class Y class Z} => namespace X{class X,class Y class Z}=>mod X{struct X struct Y struct Z}

	vector<pair<CpAstNode,CpAstNode>> modClasses; n.findModClasses(modClasses);
	emitXRefs(ec,n,depth);
//	for (auto &mc : modClasses) { EMIT("pub use testoutput::%s::%s;\n", mc.first->name.c_str(),mc.second->name.c_str());}

	EMIT_INDENT(depth,"pub type size_t=uint;\n");
	EMIT_INDENT(depth,"pub type c_bool=bool;\n");
	// TODO: this is going to need template params.
	for (auto &mc : modClasses) { 
		EMIT_INDENT(depth,"pub type %s", mc.second->name.c_str());
		emitRust_TypeParamsOfNode(ec,*mc.second);
		EMIT(" =%s::%s",mc.first->name.c_str(), mc.second->name.c_str());
		emitRust_TypeParamsOfNode(ec,*mc.second);
		EMIT(";\n");
	}
		
//=%s::%s;\n", mc.second->name.c_str(), mc.first->name.c_str(),mc.second->name.c_str());}
//
}

fn emitRustPrefix(EmitCtx& ec, const AstNode& n,EmitContext depth)->void {
	EMIT("#[feature(globs)];\n");
	EMIT("#[allow(non_camel_case_types)];\n");
	EMIT("#[allow(unused_imports)];\n");
	EMIT("#[allow(unused_variable)];\n");

	emitRustModPrefix(ec,n,depth);
/*
	EMIT("pub type c_bool=c_int;\n");
	EMIT("pub type int16_t = i16;\n");
	EMIT("pub type int32_t = i32;\n");
	EMIT("pub type int64_t = i64;\n");
	EMIT("pub type uint16_t = u16;\n");
	EMIT("pub type uint32_t = u32;\n");
	EMIT("pub type uint64_t = u64;\n");
	EMIT("pub type FILE=*c_void;\n");
	EMIT("pub type pconstchar_t=*c_char;\n");
	EMIT("pub struct vector<T,A=allocator<T> > { pub begin:*T, pub end:*T, pub capacity:*T}\n");
	EMIT("pub struct pair<A,B> { pub first:A, pub second:B}\n");
	EMIT("pub struct string { pub data:*c_char }\n");
*/
	// other prefix for the whole file. eg: counting the forward decls?
}

fn emitRust(EmitRustMode mode, const AstNode& root)->void {
	EmitCtx ec;
	// prot pass..


	if (mode==EmitRustMode_Rust) {
	// prototype pass, find everything.
		emitRustRecursive(ec, mode, root,0);
	}
	// reall pass.
	ec.emit_this_pass.clear();
	fseek(gOut,0,SEEK_SET);	
	if (mode==EmitRustMode_Rust)
		emitRustPrefix(ec,root,0);

	emitRustRecursive(ec, mode, root,0);
}

#undef EMIT
