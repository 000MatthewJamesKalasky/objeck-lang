@echo off

if "%~1"=="" goto blank
for %%i in (%1) do (
	set fn=%%~ni
	obc -src ..\..\..\core\compiler\lib_src\%fn%.obs -lib gen_collect.obl -tar lib -dest ..\..\..\core\release\deploy64\lib\%fn%.obl
	goto end
)

:blank
echo missing args

:end
