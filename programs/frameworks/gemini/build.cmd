@echo off

set OBJECK_ROOT=C:\Users\objec\Documents\Code\objeck-lang
REM obc -src %OBJECK_ROOT%\core\compiler\lib_src\net_common.obs,%OBJECK_ROOT%\core\compiler\lib_src\net.obs,%OBJECK_ROOT%\core\compiler\lib_src\net_secure.obs -lib json -tar lib -dest %OBJECK_ROOT%\core\release\deploy64\lib\net.obl

del /q *.obe

if [%1] == [] goto end
	obc -src %1 -lib net,json,encrypt -dest %1
	obr %1 %2 %3 %4 %5
:end