@echo off
"./../../../aftermath/bin/JsonSchemaToHpp.exe" "./config.schema.json"
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
set compilerflags=/std:c++latest /O2 /W4 /WX /wd4505 /EHsc /permissive- /bigobj /I.\..\..\..\aftermath\src /I.\..\..\..\..\include /Fe:simulator.exe
cl.exe %compilerflags% main.cpp
IF EXIST main.obj del main.obj
IF EXIST main.d del main.d
IF EXIST main.o del main.o
