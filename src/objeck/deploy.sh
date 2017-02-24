#!/bin/sh

# setup directories
rm -rf deploy
mkdir deploy
mkdir deploy/bin
mkdir deploy/lib
mkdir deploy/lib/native
mkdir deploy/doc

rm -rf deploy_fcgi
mkdir deploy_fcgi
mkdir deploy_fcgi/bin
mkdir deploy_fcgi/lib
mkdir deploy_fcgi/lib/native
mkdir deploy_fcgi/doc

# build compiler
cd ../compiler
if [ ! -z "$1" ] && [ "$1" = "32" ]; then
	cp make/Makefile.32 Makefile
elif [ ! -z "$1" ] && [ "$1" = "osx" ]; then
	cp make/Makefile.OSX.64 Makefile
else
	cp make/Makefile.64 Makefile
fi
make clean; make -j3 OBJECK_LIB_PATH=\\\".\\\"
cp obc ../objeck/deploy/bin
cp ../lib/*.obl ../objeck/deploy/lib
cp ../vm/misc/*.pem ../objeck/deploy/lib
rm ../objeck/deploy/lib/gtk2.obl
rm ../objeck/deploy/lib/sdl.obl
rm ../objeck/deploy/lib/db.obl

# build VM
cd ../vm
if [ ! -z "$1" ] && [ "$1" = "32" ]; then
	cp Makefile.32 make/Makefile
elif [ ! -z "$1" ] && [ "$1" = "osx" ]; then
	cp Makefile.OSX.64 make/Makefile
else 
	cp Makefile.64 make/Makefile
fi
make clean; make -j3
cp obr ../objeck/deploy/bin

if [ ! -z "$1" ] && [ "$1" = "32" ]; then
	cp make/Makefile.FCGI32 Makefile
elif [ ! -z "$1" ] && [ "$1" = "64" ]; then	
	cp make/Makefile.FCGI64 Makefile
fi
make clean; make -j3

# build debugger
cd ../debugger
if [ ! -z "$1" ] && [ "$1" = "32" ]; then
	cp make/Makefile.32 Makefile
elif [ ! -z "$1" ] && [ "$1" = "osx" ]; then
	cp make/Makefile.OSX.64 Makefile
else
	cp make/Makefile.64 Makefile
fi
make clean; make -j3
cp obd ../objeck/deploy/bin

# build libraries
cd ../lib/odbc
if [ ! -z "$1" ] && [ "$1" = "osx" ]; then
	./build_osx_x64.sh odbc
	cp odbc.dylib ../../objeck/deploy/lib/native/libobjk_odbc.dylib
else
	./build_linux.sh odbc
	cp odbc.so ../../objeck/deploy/lib/native/libobjk_odbc.so
fi

cd ../openssl

if [ ! -z "$1" ] && [ "$1" = "osx" ]; then
	./build_osx_x64.sh openssl
	cp openssl.dylib ../../objeck/deploy/lib/native/libobjk_openssl.dylib
else
	./build_linux.sh openssl
	cp openssl.so ../../objeck/deploy/lib/native/libobjk_openssl.so
fi

cd ../fcgi
./build_linux.sh

# copy docs
cd ../../..
cp docs/guide/objeck_lang.pdf src/objeck/deploy/doc
cp -R docs/syntax src/objeck/deploy/doc/syntax
cp docs/readme.htm src/objeck/deploy
unzip docs/api.zip -d src/objeck/deploy/doc

# copy examples
mkdir src/objeck/deploy/examples
cp src/compiler/programs/deploy/*.obs src/objeck/deploy/examples
cp -aR src/compiler/programs/doc src/objeck/deploy/examples
cp -aR src/compiler/programs/tiny src/objeck/deploy/examples

# create and build fcgi
cd src/objeck
cp ../lib/fcgi.obl deploy/lib
cp -Rfu deploy/* deploy_fcgi
rm deploy_fcgi/bin/obc
rm deploy_fcgi/bin/obd
rm -rf deploy_fcgi/doc
rm -rf deploy_fcgi/examples
cp ../vm/obr_fcgi deploy_fcgi/bin
cp ../lib/fcgi/*.so deploy_fcgi/lib/native
mkdir deploy_fcgi/examples
cp -R ../compiler/programs/web/* deploy_fcgi/examples
cp ../../docs/fcgi_readme.htm deploy_fcgi/readme.htm
mkdir deploy_fcgi/fcgi_readme_files
cp ../../docs/fcgi_readme_files/* deploy_fcgi/fcgi_readme_files

# deploy
if [ ! -z "$2" ] && [ "$2" = "deploy" ]; then
	rm -rf ~/Desktop/objeck-lang
	cp -rf ../objeck/deploy ~/Desktop/objeck-lang
	cd ~/Desktop
	
	rm -f objeck.tar objeck.tgz
	tar cf objeck.tar objeck-lang
	gzip objeck.tar
	mv objeck.tar.gz objeck.tgz	
fi;
