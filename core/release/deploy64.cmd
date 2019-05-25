REM clean up
rmdir /s /q deploy64
mkdir deploy64

REM update version information
powershell.exe -executionpolicy remotesigned -file  update_version.ps1

REM build binaries
devenv objeck.sln /rebuild "Release|x64"
mkdir deploy64\bin
copy ..\compiler\Release\win64\*.exe deploy64\bin
copy ..\vm\Release\win64\*.exe deploy64\bin
copy ..\debugger\Release\win64\*.exe deploy64\bin

REM libraries
mkdir deploy64\lib
mkdir deploy64\lib\sdl
mkdir deploy64\lib\sdl\fonts
copy ..\lib\*.obl deploy64\lib
del deploy64\lib\gtk2.obl
del /q deploy64\bin\a.*
copy ..\vm\misc\*.pem deploy64\lib

REM openssl support
mkdir deploy64\lib\native
cd ..\lib\openssl
devenv openssl.sln /rebuild "Release|x64"
copy Release\win64\*.dll ..\..\Release\deploy64\lib\native
cd ..\..\release

REM odbc support
cd ..\lib\odbc
devenv odbc.sln /rebuild "Release|x64"
copy Release\win64\*.dll ..\..\Release\deploy64\lib\native
cd ..\..\release

REM sdl
cd ..\lib\sdl
devenv sdl\sdl.sln /rebuild "Release|x64"
copy sdl\Release\x64\*.dll ..\..\Release\deploy64\lib\native
copy lib\fonts\*.ttf ..\..\Release\deploy64\lib\sdl\fonts
copy lib\x64\*.dll ..\..\Release\deploy64\lib\sdl
cd ..\..\Release

REM copy examples
mkdir deploy64\examples\
mkdir deploy64\examples\doc\
mkdir deploy64\examples\tiny\
set ZIP_BIN="\Program Files\7-Zip"

mkdir deploy64\examples\media\
del  /s /q ..\..\programs\*.obe
xcopy /e ..\..\programs\deploy\*.obs deploy64\examples\
xcopy /e ..\..\programs\deploy\media\*.png deploy64\examples\media\
xcopy /e ..\..\programs\deploy\media\*.wav deploy64\examples\media\
xcopy /e ..\..\programs\doc\* deploy64\examples\doc\
xcopy /e ..\..\programs\tiny\* deploy64\examples\tiny\
del  /s /q ..\..\programs\tiny\*.obe
del  /s /q ..\..\programs\tiny\*.e

REM build and update docs
mkdir deploy64\doc 
copy ..\..\docs\guide\objeck_lang.pdf deploy64\doc 
mkdir deploy64\doc\syntax
xcopy /e ..\..\docs\syntax\* deploy64\doc\syntax
copy ..\..\docs\readme.htm deploy64
call code_doc64.cmd

REM finished
if [%1] NEQ [deploy] goto end
	rmdir /q /s deploy64\examples\doc
	mkdir "%USERPROFILE%\Desktop\objeck-lang-win64"
	xcopy /e deploy64 "%USERPROFILE%\Desktop\objeck-lang-win64"
	mkdir "%USERPROFILE%\Desktop\objeck-lang-win64\doc\icons"
	copy ..\..\images\setup_icons\*.ico "%USERPROFILE%\Desktop\objeck-lang-win64\doc\icons"
	copy ..\..\images\setup_icons\*.jpg "%USERPROFILE%\Desktop\objeck-lang-win64\doc\icons"
	copy ..\..\docs\eula.rtf "%USERPROFILE%\Desktop\objeck-lang-win64\doc"
	copy ..\..\docs\uninstall.vbs "%USERPROFILE%\Desktop\objeck-lang-win64\doc"
	copy ..\..\docs\getting_started.url "%USERPROFILE%\Desktop\objeck-lang-win64\doc"
	copy /y ..\setup64
	devenv setup.sln /rebuild "Release"
	signtool sign /f "\Dropbox\Personal\signing keys\2018\randy_hollines.p12" /p %2 /d "Objeck Toolchain" /t http://timestamp.comodoca.com Release64\setup.msi
	copy Release64\setup.msi "%USERPROFILE%\Desktop\objeck-lang-win64.msi"
	
	rmdir /s /q "%USERPROFILE%\Desktop\Release64"
	mkdir "%USERPROFILE%\Desktop\Release64"
	move "%USERPROFILE%\Desktop\objeck-lang-win64" "%USERPROFILE%\Desktop\Release64"
	%ZIP_BIN%\7z.exe a -r -tzip "%USERPROFILE%\Desktop\Release64\objeck-lang-win64.zip" "%USERPROFILE%\Desktop\Release64\objeck-lang-win64\*"
	move "%USERPROFILE%\Desktop\objeck-lang-win64.msi" "%USERPROFILE%\Desktop\Release64"
:end
