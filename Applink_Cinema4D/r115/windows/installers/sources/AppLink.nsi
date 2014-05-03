!include "x64.nsh"

; The name of the installer
Name "AppLink installer"
Icon "Icons\3D-Coat.ico"
UninstallIcon "Icons\3DCoatUninst32.ico"
DirText "Please choose the folder where Maxon Cinema 4D R11.5 is installed to copy 3D-Coat AppLink plugin files there."  

!macro BIMAGE IMAGE PARMS
	Push $0
	GetTempFileName $0
	File /oname=$0 "${IMAGE}"
	SetBrandingImage $0
	Delete $0
	Pop $0
!macroend

RequestExecutionLevel admin

; The file to write
OutFile "ApplinkCinema4DR115.exe"

ReserveFile "${NSISDIR}\Plugins\InstallOptions.dll"

XPStyle on

AddBrandingImage top 120

RequestExecutionLevel admin

; IMPORTANT: The default installation directory - you need to write your application install dir there
;InstallDir "$INSTDIR\MAXON\CINEMA 4D R12\plugins"

; IMPORTANT: or you may get it from come registry key if need, if there is no nregistery key, the one above will be used
;InstallDirRegKey HKCU "Software\3D-Coat-V3" ""

Function dirImage
	!insertmacro BIMAGE "C4DR115-header.bmp" /RESIZETOFIT
FunctionEnd

Function .onInit
	${If} ${RunningX64}
	StrCpy $INSTDIR "$PROGRAMFILES64\MAXON\CINEMA 4D R11.5\plugins"
	${Else}
	StrCpy $INSTDIR "$PROGRAMFILES32\MAXON\CINEMA 4D R11.5\plugins"
	${EndIf}
	
  	InitPluginsDir
	# the plugins dir is automatically deleted when the installer exits
	InitPluginsDir
	File /oname=$PLUGINSDIR\splash.bmp "3DC-V3-loading.bmp"

	advsplash::show 1000 200 200 0x04025C $PLUGINSDIR\splash

FunctionEnd

Function un.dirImage
	!insertmacro BIMAGE "C4DR115-header.bmp" /RESIZETOFIT
FunctionEnd

;--------------------------------

; Pages
Page directory dirImage
Page instfiles


; The stuff to install
Section "" ;No components page, name is not important
  !insertmacro BIMAGE "C4DR115-header.bmp" /RESIZETOFIT  

  SetOutPath $INSTDIR
  File /r files\*.*  
SectionEnd ; end the section