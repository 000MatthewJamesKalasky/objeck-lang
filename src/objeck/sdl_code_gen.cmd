@echo off

cd deploy\bin
obc -src ..\..\..\compiler\test_src\sdl\code_gen\sdl_scanner.obs,..\..\..\compiler\test_src\sdl\code_gen\sdl_parser.obs,..\..\..\compiler\test_src\sdl\code_gen\sdl_emitter.obs -opt s3 -lib collect.obl -dest ..\code_gen.obe
obr ..\code_gen.obe SDL_Window gen_tmpl.dat D:\Temp\SDL2-2.0.3\include\SDL_video.h
cd ..\..
