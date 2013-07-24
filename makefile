LIBS=-L$(LLVM)/Debug+Asserts/lib
INC=-I$(LLVM)/tools/clang/include -I$(LLVM)/include
CC=clang  $(LIBS) $(INC)  

CPP=clang++ -g -O0 -std=c++11 $(LIBS) $(INC)


# grep  "fn\s*\w*::\w\(.*\).*{" ast.cpp |sed 's/fn\s*\(\w*\)::\(\w*.*\){/\tfn \2;/' |sed 's/\(.*\)=.*\([,/)].*\)/\1\2/' > ast_methods.h && more ast_methods.h


demo: main
	./main -w testoutput -dcr $(TEST_OPTS)



main: main.cpp emitrust.cpp clanghelpers.cpp ast.cpp ast.hxx ast.h
	$(CPP) main.cpp -lclang -o ./main 

# TODO- rule for every _methods.h from every .cpp

ast.hxx: ast.cpp
	grep  "fn\s*\w*::\w\(.*\).*{" $< |sed 's/fn\s*\(\w*\)::\(\w*.*\){/\tfn \2;/' |sed 's/\(.*\)=.*\([,/)].*\)/\1\2/' > $@




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
