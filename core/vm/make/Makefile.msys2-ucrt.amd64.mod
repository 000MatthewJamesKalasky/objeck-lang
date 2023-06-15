ARGS=-O3 -Wall -std=c++17 -Wno-strict-overflow -pthread -Wall -D_WIN32 -D_MSYS2 -D_X64 -D_DEBUGGER -D_OBJECK_NATIVE_LIB_PATH -D_NO_JIT -Wno-dangling-pointer -Wno-maybe-uninitialized -Wno-unknown-pragmas -Wno-unused-function -Wno-unused-variable -Wno-int-to-pointer-cast -Wno-unknown-pragmas

CC=g++
SRC=common.o interpreter.o loader.o vm.o posix_main.o 
OBJ_LIBS=jit_amd_lp64.a memory.a
MEM_PATH=arch
JIT_PATH=arch/jit/amd64
LIB=vm.a

$(LIB): $(SRC) $(OBJ_LIBS)
	$(AR) -cvq $(LIB) $(SRC)

memory.a:
	cd $(MEM_PATH); make -f make/Makefile.msys2-ucrt.amd64
	
jit_amd_lp64.a:
	cd $(JIT_PATH); make -f make/Makefile.msys2-ucrt.amd64
	
%.o: %.cpp
	$(CC) -m64 $(ARGS) -c $< 

clean:
	cd $(MEM_PATH); make clean -f make/Makefile.msys2-ucrt.amd64
	cd $(JIT_PATH); make clean -f make/Makefile.msys2-ucrt.amd64
	rm -f $(LIB) *.o *~
