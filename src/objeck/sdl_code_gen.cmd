@echo off

cd deploy\bin
obc -src ..\..\..\compiler\test_src\sdl\code_gen\sdl_scanner.obs,..\..\..\compiler\test_src\sdl\code_gen\sdl_parser.obs,..\..\..\compiler\test_src\sdl\code_gen\sdl_emitter.obs -opt s3 -lib collect.obl -dest ..\code_gen.obe
REM obr ..\code_gen.obe Surface window ..\..\..\lib\sdl\include\SDL_surface.h
cd ..\..
