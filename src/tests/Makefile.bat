@echo off
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
set compilerflags=/std:c++20 /O2 /W4 /wd4710 /wd4711 /wd4514 /EHsc /permissive- /I.\..\..\..\aftermath\src /I.\..\..\..\..\include
set linkerflags=/OUT:tests.exe
cl.exe %compilerflags% main.cpp /link %linkerflags%
del main.obj
