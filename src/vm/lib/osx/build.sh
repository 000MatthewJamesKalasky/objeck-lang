#/bin/sh
rm -rf *.o *.dylib
g++ -fPIC -g -c -Wno-deprecated -Wall $1.cpp
g++ -dynamiclib -Wl,-headerpad_max_install_names,-undefined,dynamic_lookup,-compatibility_version,1.0,-current_version,1.0 -o $1.1.dylib $1.o
