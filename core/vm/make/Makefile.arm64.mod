ARGS=-O3 -Wall -Wno-unused-function -std=c++17 -Wno-strict-overflow -pthread -Wall -D_DEBUGGER -D_OBJECK_NATIVE_LIB_PATH -D_NO_JIT -Wno-int-to-pointer-cast -Wno-unused-variable

CC=g++
SRC=common.o interpreter.o loader.o vm.o posix_main.o 
OBJ_LIBS=jit_arm_a64.a memory.a
MEM_PATH=arch
JIT_PATH=arch/jit/arm64
LIB=vm.a

$(LIB): $(SRC) $(OBJ_LIBS)
	$(AR) -cvq $(LIB) $(SRC)
	cp $(LIB) ../module

memory.a:
	cd $(MEM_PATH); make -f make/Makefile.arm64
	
jit_arm_a64.a:
	cd $(JIT_PATH); make -f make/Makefile.arm64
	
%.o: %.cpp
	$(CC) $(ARGS) -c $<

clean:
	cd $(MEM_PATH); make clean -f make/Makefile.arm64
	cd $(JIT_PATH); make clean -f make/Makefile.arm64
	rm -f $(LIB) *.o *~
