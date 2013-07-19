LIBS=-L$(LLVM)/Debug+Asserts/lib
INC=-I$(LLVM)/tools/clang/include -I$(LLVM)/include
CC=clang  $(LIBS) $(INC)  

CPP=clang++ -g -O0 -std=c++11 $(LIBS) $(INC)

main: main.cpp emitrust.cpp clanghelpers.cpp ast.cpp
	$(CPP) main.cpp -lclang -o ./main 

info:
	@echo environment variables:-
	@echo llvm = $(LLVM)

TEST_OPTS= main.cpp -std=c++11 -x c++ -I/usr/include/x86_64-linux-gnu/c++/4.7/ -I/usr/include/clang/3.2/include
run: main
	./main -d $(TEST_OPTS)

debug: main
	gdb --args ./main -d $(TEST_OPTS)

demo: main
	./main $(TEST_OPTS)

clean:
	rm ./main
