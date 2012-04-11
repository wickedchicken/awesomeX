CC=clang
CFLAGS=`llvm-config --cflags`
LD=clang
LDFLAGS=`llvm-config --libs --cflags --ldflags core analysis executionengine jit interpreter native` -lstdc++

all: stockprint jitprint

# --------- regular program

stockprint: print.o main.o
	$(LD) print.o main.o -o stockprint

print.o: print.c print.h
	$(CC) print.c -c

main.o: main.c
	$(CC) -c main.c


# --------- jitified

awesomex.cpp: print.c
	$(CC) -emit-llvm -o - -c print.c | llc -march=cpp | awk '/int main/,/}/ { next } 1' | awk '/Module\*\ makeLLVMModule\(\);/ { print "extern \"C\""; next } 1' > awesomex.cpp

awesomex.h:
	echo "#include <llvm-c/Core.h>" > awesomex.h
	echo "LLVMModuleRef makeLLVMModule();" >> awesomex.h

jitprint: awesomex.o main.o jitprint.o
	$(LD) awesomex.o main.o jitprint.o $(LDFLAGS) -o jitprint

awesomex.o: awesomex.cpp
	$(CC) $(CFLAGS) -c awesomex.cpp

jitprint.o: jitprint.c awesomex.h
	$(CC) $(CFLAGS) -c jitprint.c

jitprint.c: codegen
	./codegen > jitprint.c

codegen: awesomex.o codegen.o
	$(LD) awesomex.o codegen.o $(LDFLAGS) -o codegen

codegen.o: codegen.c awesomex.h
	$(CC) $(CFLAGS) -c codegen.c

.PHONY: clean
clean:
	-rm -rf awesomex.cpp awesomex.h jitprint.c awesomex.o codegen.o jitprint.o main.o print.o stockprint jitprint codegen
