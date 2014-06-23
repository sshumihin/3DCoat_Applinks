@echo off

call "c:\Program Files\Autodesk\Softimage 2014 SP2\Application\bin\setenv.bat"
call "e:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" amd64

set PATH
set "XSISDK_ROOT=c:\Program Files\Autodesk\Softimage 2014 SP2\XSISDK\"

"e:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE\devenv.exe" /useenv

echo on
