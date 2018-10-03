#/bin/sh
rm -rf *.o *.dylib
g++ -D_X64 -D_OSX -shared -Wno-unused-function -Wno-deprecated-declarations -fPIC -c -O3 -Wall -F/Library/Frameworks -I/Library/Frameworks/SDL2.framework/Headers -I/Library/Frameworks/SDL2_ttf.framework/Headers -I/Library/Frameworks/SDL2_mixer.framework/Headers -I/Library/Frameworks/SDL2_image.framework/Headers -I/Library/Frameworks/SDL2_mixer.framework/Headers -I/usr/local/ssl/include $1.cpp
g++ -D_X64 -D_OSX -dynamiclib -F/Library/Frameworks -framework SDL2 -framework SDL2_ttf -framework SDL2_mixer -framework SDL2_image -L/usr/local/ssl/lib -lssl -Wl,-headerpad_max_install_names,-undefined,dynamic_lookup,-compatibility_version,1.0,-current_version,1.0 `sdl2-config --cflags --libs` -o $1.dylib $1.o -v
