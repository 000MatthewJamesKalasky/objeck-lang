@echo off

set OBJECK_ROOT=C:\Users\objec\Documents\Code\objeck-lang
obc -src %OBJECK_ROOT%\core\compiler\lib_src\net_common.obs,%OBJECK_ROOT%\core\compiler\lib_src\net.obs,%OBJECK_ROOT%\core\compiler\lib_src\net_secure.obs -lib json -tar lib -dest %OBJECK_ROOT%\core\release\deploy64\lib\net.obl
obc -src %OBJECK_ROOT%\core\compiler\lib_src\openai.obs -lib json,net,misc -tar lib -dest %OBJECK_ROOT%\core\release\deploy64\lib\openai.obl

if [%1] == [] goto end
	set a=%1:~0,-3%
	echo %a%
	obc -src %1 -lib net,json,misc,openai -dest 
	if [%2] == [] goto end
		obr %1 %2 %3 %4 %5
:end