LIBS=-L$(LLVM)/Release/lib
INC=-I$(LLVM)/tools/clang/include -I$(LLVM)/include
CC=clang  $(LIBS) $(INC)  

CPP=clang++ -g -O0 -std=c++11 $(LIBS) $(INC)


# grep  "fn\s*\w*::\w\(.*\).*{" ast.cpp |sed 's/fn\s*\(\w*\)::\(\w*.*\){/\tfn \2;/' |sed 's/\(.*\)=.*\([,/)].*\)/\1\2/' > ast_methods.h && more ast_methods.h

TEST_CMD=./main -w testoutput -dcr $(TEST_OPTS)


testoutput: test_testoutput.cpp demo libtestinput.a
	echo "compiliong CPP shim (wraps methods as externC)"
	$(CPP) $< emitrust.cpp main.cpp clanghelpers.cpp ast.cpp testinput.cpp -lclang -o ./trash -Wno-return-type
	echo "compiliong rust test that includes generated mod 'testoutput.rs"
	rustc test.rs -L. -C link-args="-ltestinput -lstdc++"
	./test

demo: main
	echo  $(LLVM)
	$(TEST_CMD)
	@echo output:-
	cat testoutput.cpp
	cat testoutput.rs
	@echo
	@echo "generated testoutput.rs testoutput.cpp from invocation:"
	@echo $(TEST_CMD)

all: main

# clang++ main.cpp clanghelpers.cpp ast.cpp emitrust.cpp -L  -lclang

SRC=main.cpp clanghelpers.cpp ast.cpp emitrust.cpp 
HDR=cpp2rustcommon.h clanghelpers.h ast.h emitrust.h

main: $(SRC)$(HDR) ast_fn.hxx emitrust_fn.hxx AstNode.hxx clanghelpers_fn.hxx 
	$(CPP) $(SRC) $(LIBS) -lclang -o ./main 

testoutput.o: testoutput.cpp test_testoutput.cpp
	$(CPP) test_testoutput.cpp -o testoutput.o -c

testinput.o: testinput.cpp
	$(CPP) $^ -o $@ -c

libtestinput.a:testinput.o testoutput.o
	ar rcs $@ $^
#testinput.o testoutput.o


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

TEST_OPTS= testinput.cpp -std=c++11 -x c++ -I/usr/include/x86_64-linux-gnu/c++/4.7/ -I/usr/include/clang/3.2/include
run: main
	./main -d $(TEST_OPTS)

debug: main
	gdb --args ./main -d $(TEST_OPTS)


clean:
	rm *.hxx
	rm ./main
