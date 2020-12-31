@echo off

call c:\Program_Files\Autodesk\Softimage_2015_SP1\Application\bin\setenv.bat
call "c:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64

set PATH
set XSISDK_ROOT=c:\Program_Files\Autodesk\Softimage_2015_SP1\XSISDK

"c:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\devenv.exe" /useenv

echo on
