#/bin/sh
rm -rf *.o *.dylib
clang++ -D_X64 -D_OSX -fPIC -c -O3 -Wall -F/Library/Frameworks -I/Library/Frameworks/SDL2.framework/Headers -I/Library/Frameworks/SDL2_ttf.framework/Headers -I/Library/Frameworks/SDL2_mixer.framework/Headers -I/Library/Frameworks/SDL2_image.framework/Headers -I/Library/Frameworks/SDL2_mixer.framework/Headers -I./../openssl/macos/include -I/usr/local/include $1.cpp SDL2_framerate.c SDL2_gfxPrimitives.c SDL2_imageFilter.c SDL2_rotozoom.c  -Wno-unused-function -Wno-deprecated-declarations -Wno-unused-but-set-variable
clang++ -shared -dynamiclib -F/Library/Frameworks -framework SDL2 -framework SDL2_ttf -framework SDL2_mixer -framework SDL2_image -L./../openssl/macos/x64 -lssl -Wl,-headerpad_max_install_names,-undefined,dynamic_lookup,-compatibility_version,1.0,-current_version,1.0 `sdl2-config --cflags --libs` -o $1.dylib $1.o SDL2_framerate.o SDL2_gfxPrimitives.o SDL2_imageFilter.o SDL2_rotozoom.o -v