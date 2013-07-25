LIBS=-L$(LLVM)/Debug+Asserts/lib
INC=-I$(LLVM)/tools/clang/include -I$(LLVM)/include
CC=clang  $(LIBS) $(INC)  

CPP=clang++ -g -O0 -std=c++11 $(LIBS) $(INC)


# grep  "fn\s*\w*::\w\(.*\).*{" ast.cpp |sed 's/fn\s*\(\w*\)::\(\w*.*\){/\tfn \2;/' |sed 's/\(.*\)=.*\([,/)].*\)/\1\2/' > ast_methods.h && more ast_methods.h

TEST_CMD=./main -w testoutput -dcr $(TEST_OPTS)

demo: main
	$(TEST_CMD)
	@echo output:-
	cat testoutput.rs
	@echo
	@echo "generated testoutput.rs testoutput.cpp from invocation:"
	@echo $(TEST_CMD)

all: main

SRC=main.cpp clanghelpers.cpp ast.cpp emitrust.cpp 
HDR=cpp2rustcommon.h clanghelpers.h ast.h emitrust.h

main: $(SRC)$(HDR) ast_fn.hxx emitrust_fn.hxx AstNode.hxx clanghelpers_fn.hxx 
	$(CPP) $(SRC) -lclang -o ./main 

# TODO- rule for every _methods.h from every .cpp
#ClassName.hxx = member function prototypes for 'classname'
#todo-figure out script/regex to gather ClassName.hxx from *.cpp

GENERATE_METHOD_PROTOTYPES=	grep  "fn\s*\w*::\w\(.*\).*{" $< |sed 's/fn\s*\(\w*\)::\(\w*.*\){/\tfn \2;/' |sed 's/\(.*\)=.*\([,/)].*\)/\1\2/' > $@
GENERATE_FUNCTION_PROTOTYPES=	grep  "fn\s*\w\([^:]*\){" $< |sed 's/fn\s*\(\w*.*\){/\tfn \1;/' |sed 's/\(.*\)=.*\([,/)].*\)/\1\2/' > $@

AstNode.hxx: ast.cpp
	$(GENERATE_METHOD_PROTOTYPES)
clanghelpers_fn.hxx: clanghelpers.cpp
	$(GENERATE_FUNCTION_PROTOTYPES)
emitrust_fn.hxx: emitrust.cpp
	$(GENERATE_FUNCTION_PROTOTYPES)
ast_fn.hxx : ast.cpp
	$(GENERATE_FUNCTION_PROTOTYPES)


info:
	@echo environment variables:-
	@echo llvm = $(LLVM)

TEST_OPTS= main.cpp -std=c++11 -x c++ -I/usr/include/x86_64-linux-gnu/c++/4.7/ -I/usr/include/clang/3.2/include
run: main
	./main -d $(TEST_OPTS)

debug: main
	gdb --args ./main -d $(TEST_OPTS)


clean:
	rm *.hxx
	rm ./main
