@echo off

del gens\*.obl gens\*.obs gens\*.cpp gens\*.txt

obc -src gtk3_binder.obs -lib xml

if [%1] NEQ [] (
	obr gtk3_binder %1
	copy /y gens\objk_code.txt gens\gtk3_binder.obs
	obc -src gens\objk_code.obs -tar lib

	copy /y gens\cxx_code.txt gens\gtk3_binder.cpp
	REM compiler
)