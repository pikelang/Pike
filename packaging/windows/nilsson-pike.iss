; Configuration file for Inno Setup 4
; By Martin Nilsson

[Setup]
InternalCompressLevel=ultra
SolidCompression=true
Compression=lzma/normal
SetupIconFile=pikeinstall.ico
UninstallIconFile=icons\pike_red.ico
OutputBaseFilename=Pike-7.6.12-win32-nilsson-beta4

LicenseFile=Copying.txt
AllowNoIcons=true

; These are dependent of the Pike version
VersionInfoVersion=7.6.13.4
AppVersion=7.6.13.4
AppName=Pike 7.6
AppVerName=Pike 7.6.13

DefaultDirName={pf}\Pike
DefaultGroupName=Pike
AppPublisherURL=http://pike.ida.liu.se/

[Files]

; Base installation
;; /
Source: ~piketmp\build\pike.exe; DestDir: {app}; Flags: ignoreversion; Components: base
Source: ~piketmp\build\pike.syms; DestDir: {app}; Flags: ignoreversion; DestName: pike.exe.syms; Components: base
Source: ~piketmp\build\COPYING; DestDir: {app}; Flags: ignoreversion; DestName: Copying.txt; Components: base
Source: ~piketmp\build\COPYRIGHT; DestDir: {app}; Flags: ignoreversion; DestName: Copyright.txt; Components: base
;; /lib
Source: ~piketmp\build\lib\modules\*; Excludes: ___GTK.so,SDL.so; DestDir: {app}\lib\modules\; Flags: ignoreversion recursesubdirs; Components: base
Source: ~piketmp\lib\*; DestDir: {app}\lib\; Flags: recursesubdirs ignoreversion; Components: base
;; /icons
Source: icons\*; DestDir: {app}\icons\; Components: base
;; tmp
Source: finalizer.pike; DestDir: {tmp}; Flags: deleteafterinstall ignoreversion; Components: base
Source: finalizer.bat; DestDir: {tmp}; Flags: deleteafterinstall ignoreversion; Components: base

; Debug
Source: ~piketmp\build\pike.pdb; DestDir: {app}; Flags: ignoreversion; Components: debug

; GTK
Source: ~piketmp\build\lib\modules\___GTK.so; DestDir: {app}\lib\modules\; Flags: ignoreversion; Components: gtk
Source: dlls\gdk-1.3.dll; DestDir: {app}; Flags: ignoreversion restartreplace sharedfile; Components: gtk
Source: dlls\glib-1.3.dll; DestDir: {app}; Flags: ignoreversion restartreplace sharedfile; Components: gtk
Source: dlls\gmodule-1.3.dll; DestDir: {app}; Flags: ignoreversion restartreplace sharedfile; Components: gtk
Source: dlls\gnu-intl.dll; DestDir: {app}; Flags: ignoreversion restartreplace sharedfile; Components: gtk
Source: dlls\gtk-1.3.dll; DestDir: {app}; Flags: ignoreversion restartreplace sharedfile; Components: gtk
Source: dlls\iconv-1.3.dll; DestDir: {app}; Flags: ignoreversion restartreplace sharedfile; Components: gtk

; SDL
Source: ~piketmp\build\lib\modules\SDL.so; DestDir: {app}\lib\modules\; Flags: ignoreversion; Components: sdl
Source: dlls\SDL.dll; DestDir: {app}; Flags: restartreplace ignoreversion sharedfile; Components: sdl


[Components]
Name: base; Description: Pike base files; Flags: fixed; Types: custom compact full; ExtraDiskSpaceRequired: 6500000
Name: debug; Description: Debug symbols; Types: full
Name: gtk; Description: GTK+; Types: full
Name: sdl; Description: SDL; Types: custom full

[Tasks]
Name: associate; Description: Associate .pike and .pmod extensions with Pike; Components: base

[Run]
Filename: {tmp}\finalizer.bat; Parameters: """{app}\pike.exe"" ""{tmp}\finalizer.pike"" ""{app}\lib\master.pike""; Flags: runhidden"; Flags: runhidden

[Registry]
Root: HKCR; Subkey: .pike; ValueType: string; ValueData: pike_file; Tasks: associate
Root: HKCR; Subkey: .pike; ValueType: string; ValueName: ContentType; ValueData: text/x-pike-code; Tasks: associate
Root: HKCR; Subkey: .pmod; ValueType: string; ValueData: pike_module; Tasks: associate
Root: HKCR; Subkey: .pmod; ValueType: string; ValueName: ContentType; ValueData: text/x-pike-code; Tasks: associate
Root: HKCR; Subkey: pike_file; ValueType: string; ValueData: Pike program file; Tasks: associate
;Root: HKCR; Subkey: pike_file\DefaultIcon; ValueType: string; ValueData: {app}\pike.ico,0; Tasks: associate
Root: HKCR; Subkey: pike_file\DefaultIcon; ValueType: string; ValueData: {app}\icons\pike_black.ico,0; Tasks: associate
Root: HKCR; Subkey: pike_file\Shell\Open\Command; ValueType: string; ValueData: """{app}\pike.exe"" ""%1"""; Flags: uninsdeletevalue; Tasks: associate
Root: HKCR; Subkey: pike_file\Shell\Edit\Command; ValueType: string; ValueData: """notepad.exe"" ""%1"""; Flags: createvalueifdoesntexist; Tasks: associate
Root: HKCR; Subkey: pike_module; ValueType: string; ValueData: Pike module file; Tasks: associate
;Root: HKCR; Subkey: pike_module\DefaultIcon; ValueType: string; ValueData: {app}\pike.ico,0; Tasks: associate
Root: HKCR; Subkey: pike_module\DefaultIcon; ValueType: string; ValueData: {app}\icons\pike_blue.ico,0; Tasks: associate
Root: HKCR; Subkey: pike_module\Shell\Edit\Command; ValueType: string; ValueData: """notepad.exe"" ""%1"""; Flags: createvalueifdoesntexist; Tasks: associate

[Icons]
Name: {group}\Pike; Filename: {app}\pike.exe; IconFilename: {app}\icons\icon_magenta.ico

[UninstallDelete]
Type: filesandordirs; Name: {app}\lib
Type: filesandordirs; Name: {app}\pike.exe

[Messages]
LicenseLabel=Please read the following unimportant information before continuing.

; End
