#/bin/sh

rm -rf *.o
rm -rf *.dll

g++ -O3 -Wall -std=c++17 -fPIC -c *$1.cpp -Wno-int-to-pointer-cast -Wno-unused-function
g++ -O3 -shared -Wl,-soname,$1.dll.1 -o $1.dll *.o -lodbc