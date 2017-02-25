## Virutal Machine
The Objeck virtual machine is a stack-based VM that can convert bytecode into machine code for faster execution. The VM has a caching "mark and sweep" garbage collector along with wrappers to host API such as networking and file access.

![alt text](../../images/design3.png "Objeck VM")

###Design
The major components of the VM are the interpreter, JIT compiler and memory manager. All 3 components interop with one another. For portability, OS functions for Windows and POSIX environments have been lightly wrapped.

The VM supports the following platforms:

1. Windows (win32, IA-32)
2. macOS (POSIX, AMD64)
3. Linux 64-bit (POSIX, AMD64)
4. Linux 32-bit (POSIX, IA-32)

###Implementation
C++ with STL and machine code