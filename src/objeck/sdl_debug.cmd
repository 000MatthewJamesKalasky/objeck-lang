cd deploy\bin
obc -src '..\..\..\compiler\lib_src\sdl.obs' -lib collect.obl -tar lib -dest sdl.obl
copy ..\..\..\lib\sdl\lib\x86\sdl2.dll .
devenv /rebuild Debug ..\..\..\lib\sdl\sdl\sdl.sln
copy ..\..\..\lib\sdl\sdl\Debug\libobjk_sdl.dll ..\lib\objeck-lang
cd ..\..