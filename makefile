LIBS=-L$(LLVM)/Debug+Asserts/lib
INC=-I$(LLVM)/tools/clang/include -I$(LLVM)/include
CC=clang  $(LIBS) $(INC)  

CPP=clang++ -g -O0 -std=c++11 $(LIBS) $(INC)

main: main.cpp emitrust.cpp clanghelpers.cpp ast.cpp
	$(CPP) main.cpp -lclang -o ./main 

info:
	@echo environment variables:-
	@echo llvm = $(LLVM)

run: main
	./main -d main.cpp -std=c++11 -x c++ -I/usr/include/x86_64-linux-gnu/c++/4.7/ -I/usr/include/clang/3.2/include

debug: main
	gdb --args ./main -d ~/cardemo/ut/ut.h -std=c++11 -x c++ -I/usr/include/x86_64-linux-gnu/c++/4.7/ -I/usr/include/clang/3.2/include

clean:
	rm ./main
