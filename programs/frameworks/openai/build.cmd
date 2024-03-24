@echo off

set OBJECK_ROOT=C:\Users\objec\Documents\Code\objeck-lang
REM obc -src %OBJECK_ROOT%\core\compiler\lib_src\net_common.obs,%OBJECK_ROOT%\core\compiler\lib_src\net.obs,%OBJECK_ROOT%\core\compiler\lib_src\net_secure.obs -lib json -tar lib -dest %OBJECK_ROOT%\core\release\deploy64\lib\net.obl
obc -src %OBJECK_ROOT%\core\compiler\lib_src\openai.obs -lib json,net -tar lib -dest %OBJECK_ROOT%\core\release\deploy64\lib\openai.obl

if [%1] == [] goto end
	obc -src %1 -lib net,json,openai -dest %1
	if [%2] == [] goto end
		set API_KEY=%2
		obr %1 %API_KEY% %3 %4 %5 %6 %7
:end