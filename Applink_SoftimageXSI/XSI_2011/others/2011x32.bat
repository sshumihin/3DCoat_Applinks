@echo off

call "c:\Program Files (x86)\Autodesk\Softimage 2011 SP1\Application\bin\setenv.bat"
call "c:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\vcvarsall.bat" x86

set PATH
set XSISDK_ROOT="c:\Program Files (x86)\Autodesk\Softimage 2011 SP1\XSISDK"

"c:\Program Files (x86)\Microsoft Visual Studio 9.0\Common7\IDE\devenv.exe" /useenv

echo on
