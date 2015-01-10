#/bin/sh

# clean up
rm -rf ~/Documents/tmp

# make directories
mkdir ~/Documents/tmp
mkdir ~/Documents/tmp/src
mkdir ~/Documents/tmp/src/shared
mkdir ~/Documents/tmp/src/utilities
mkdir ~/Documents/tmp/src/compiler
mkdir ~/Documents/tmp/src/vm
mkdir ~/Documents/tmp/src/vm/debugger
mkdir ~/Documents/tmp/src/vm/os
mkdir ~/Documents/tmp/src/vm/os/posix
mkdir ~/Documents/tmp/src/vm/jit
mkdir ~/Documents/tmp/src/vm/jit/amd64
mkdir ~/Documents/tmp/src/vm/jit/ia32

# copy shared files
cp Makefile ~/Documents/tmp/src
cp ../src/shared/*.h ~/Documents/tmp/src/shared

# copy utility files
cp ../src/utilities/*.cpp ~/Documents/tmp/src/utilities
cp ../src/utilities/Makefile.32 ~/Documents/tmp/src/utilities
cp ../src/utilities/Makefile.64 ~/Documents/tmp/src/utilities

# copy compiler files
cp ../src/compiler/*.h ~/Documents/tmp/src/compiler
cp ../src/compiler/*.cpp ~/Documents/tmp/src/compiler
cp ../src/compiler/Makefile.32 ~/Documents/tmp/src/compiler
cp ../src/compiler/Makefile.64 ~/Documents/tmp/src/compiler

# copy vm files
cp ../src/vm/*.h ~/Documents/tmp/src/vm
cp ../src/vm/*.cpp ~/Documents/tmp/src/vm
cp ../src/vm/Makefile.32 ~/Documents/tmp/src/vm
cp ../src/vm/Makefile.64 ~/Documents/tmp/src/vm
cp ../src/vm/Makefile.obd32 ~/Documents/tmp/src/vm
cp ../src/vm/Makefile.obd64 ~/Documents/tmp/src/vm

cp ../src/vm/os/posix/*.h ~/Documents/tmp/src/vm/os/posix
cp ../src/vm/os/posix/*.cpp ~/Documents/tmp/src/vm/os/posix
cp ../src/vm/os/posix/Makefile.32 ~/Documents/tmp/src/vm/os/posix
cp ../src/vm/os/posix/Makefile.64 ~/Documents/tmp/src/vm/os/posix

cp ../src/vm/jit/amd64/*.h ~/Documents/tmp/src/vm/jit/amd64
cp ../src/vm/jit/amd64/*.cpp ~/Documents/tmp/src/vm/jit/amd64
cp ../src/vm/jit/amd64/Makefile.64 ~/Documents/tmp/src/vm/jit/amd64

cp ../src/vm/jit/ia32/*.h ~/Documents/tmp/src/vm/jit/ia32
cp ../src/vm/jit/ia32/*.cpp ~/Documents/tmp/src/vm/jit/ia32
cp ../src/vm/jit/ia32/Makefile.32 ~/Documents/tmp/src/vm/jit/ia32

# copy debugger files
cp ../src/vm/debugger/*.h ~/Documents/tmp/src/vm/debugger
cp ../src/vm/debugger/*.cpp ~/Documents/tmp/src/vm/debugger
cp ../src/vm/debugger/Makefile.32 ~/Documents/tmp/src/vm/debugger
cp ../src/vm/debugger/Makefile.64 ~/Documents/tmp/src/vm/debugger
