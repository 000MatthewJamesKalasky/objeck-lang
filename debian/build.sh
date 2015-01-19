#/bin/sh

BUILDDIR="/tmp/objeck"
CWD=$(pwd)

# clean up
rm -rf $BUILDDIR

# make directories
mkdir $BUILDDIR
mkdir $BUILDDIR/src
mkdir $BUILDDIR/src/man
mkdir $BUILDDIR/src/lib
mkdir $BUILDDIR/src/lib/odbc
mkdir $BUILDDIR/src/lib/openssl
mkdir $BUILDDIR/src/objk_lib
mkdir $BUILDDIR/src/shared
mkdir $BUILDDIR/src/utilities
mkdir $BUILDDIR/src/compiler
mkdir $BUILDDIR/src/vm
mkdir $BUILDDIR/src/vm/debugger
mkdir $BUILDDIR/src/vm/os
mkdir $BUILDDIR/src/vm/os/posix
mkdir $BUILDDIR/src/vm/jit
mkdir $BUILDDIR/src/vm/jit/amd64
mkdir $BUILDDIR/src/vm/jit/ia32

# copy shared files
if [ ! -z "$1" ] && [ "$1" = "32" ]; then
    cp Makefile.32 $BUILDDIR/src/Makefile
else
    cp Makefile.64 $BUILDDIR/src/Makefile
fi

cp ../src/shared/*.h $BUILDDIR/src/shared


# copy utility files
cp ../src/utilities/*.cpp $BUILDDIR/src/utilities
if [ ! -z "$1" ] && [ "$1" = "32" ]; then
	cp ../src/utilities/Makefile.32 $BUILDDIR/src/utilities/Makefile
else
	cp ../src/utilities/Makefile.64 $BUILDDIR/src/utilities/Makefile
fi

# copy compiler files
cp ../src/compiler/*.h $BUILDDIR/src/compiler
cp ../src/compiler/*.cpp $BUILDDIR/src/compiler
if [ ! -z "$1" ] && [ "$1" = "32" ]; then
	cp ../src/compiler/Makefile.Debian.32 $BUILDDIR/src/compiler/Makefile
else
	cp ../src/compiler/Makefile.Debian.64 $BUILDDIR/src/compiler/Makefile
fi

# copy vm files
cp ../src/vm/*.h $BUILDDIR/src/vm
cp ../src/vm/*.cpp $BUILDDIR/src/vm
if [ ! -z "$1" ] && [ "$1" = "32" ]; then
	cp ../src/vm/Makefile.Debian.32 $BUILDDIR/src/vm/Makefile
else
	cp ../src/vm/Makefile.Debian.64 $BUILDDIR/src/vm/Makefile
fi
cp ../src/vm/Makefile.obd32 $BUILDDIR/src/vm
cp ../src/vm/Makefile.obd64 $BUILDDIR/src/vm

cp ../src/vm/os/posix/*.h $BUILDDIR/src/vm/os/posix
cp ../src/vm/os/posix/*.cpp $BUILDDIR/src/vm/os/posix
if [ ! -z "$1" ] && [ "$1" = "32" ]; then
	cp ../src/vm/os/posix/Makefile.32 $BUILDDIR/src/vm/os/posix/Makefile
else
	cp ../src/vm/os/posix/Makefile.64 $BUILDDIR/src/vm/os/posix/Makefile
fi

if [ ! -z "$1" ] && [ "$1" = "32" ]; then
	cp ../src/vm/jit/ia32/*.h $BUILDDIR/src/vm/jit/ia32
	cp ../src/vm/jit/ia32/*.cpp $BUILDDIR/src/vm/jit/ia32
	cp ../src/vm/jit/ia32/Makefile.32 $BUILDDIR/src/vm/jit/ia32/Makefile
else
	cp ../src/vm/jit/amd64/*.h $BUILDDIR/src/vm/jit/amd64
	cp ../src/vm/jit/amd64/*.cpp $BUILDDIR/src/vm/jit/amd64
	cp ../src/vm/jit/amd64/Makefile.64 $BUILDDIR/src/vm/jit/amd64/Makefile
fi

# copy debugger files
cp ../src/vm/debugger/*.h $BUILDDIR/src/vm/debugger
cp ../src/vm/debugger/*.cpp $BUILDDIR/src/vm/debugger
if [ ! -z "$1" ] && [ "$1" = "32" ]; then
	cp ../src/vm/debugger/Makefile.Debian.32 $BUILDDIR/src/vm/debugger/Makefile
else
	cp ../src/vm/debugger/Makefile.Debian.64 $BUILDDIR/src/vm/debugger/Makefile
fi

# copy odbc library
cp ../src/lib/odbc/*.h $BUILDDIR/src/lib/odbc
cp ../src/lib/odbc/*.cpp $BUILDDIR/src/lib/odbc
cp ../src/lib/odbc/Makefile.Debian $BUILDDIR/src/lib/odbc/Makefile

# copy openssl library
cp ../src/lib/openssl/*.cpp $BUILDDIR/src/lib/openssl
cp ../src/lib/openssl/Makefile.Debian $BUILDDIR/src/lib/openssl/Makefile

# copy language libraries
cp ../src/compiler/lang.obl $BUILDDIR/src/objk_lib
cp ../src/compiler/collect.obl $BUILDDIR/src/objk_lib
cp ../src/compiler/regex.obl $BUILDDIR/src/objk_lib
cp ../src/compiler/encrypt.obl $BUILDDIR/src/objk_lib
cp ../src/compiler/odbc.obl $BUILDDIR/src/objk_lib

# man pages
cp ../docs/man/*1 $BUILDDIR/src/man

# create upstream archive
cd $BUILDDIR
tar cf objeck-lang.tar *
gzip objeck-lang.tar
bzr dh-make objeck-lang 3.3.5-2 objeck-lang.tar.gz 
cd objeck-lang
rm debian/*ex debian/*EX debian/README.Debian debian/README.source
cp -rf $CWD/files/* debian
bzr add debian/source/format
bzr commit -m "Initial commit"
bzr builddeb -- -us -uc
cp -f ../*.deb $CWD
lesspipe *.deb
