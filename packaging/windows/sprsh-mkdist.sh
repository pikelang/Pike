#!/bin/bash
# Creates Inno Setup dist via sprsh
# Feed it the burk file name.
#
# Example:
# ./sprshdist.sh ../../build/windows_xp-5.1.2600-i686/Pike-v7.6.102-Win32-Windows-NT-5.1.2600-i86pc.burk
#
# Unpack burk to "~piketmp". "~" does not imply user here.
echo "Unarchiving burk..."
rm -rf \~piketmp
pike unburk.pike $1 || exit 1
#
# Fetch the demo applications
echo "Setting up demos..."
mkdir demo
cp /famine/win32build/win32-pike/extras/gcolor.pike demo/
#
# Get DLL
echo "Setting up DLLs..."
mkdir dlls
for x in gdk-1.3.dll glib-1.3.dll gmodule-1.3.dll gnu-intl.dll gtk-1.3.dll iconv-1.3.dll SDL.dll SDL_mixer.dll; do
    cp /famine/win32build/win32-pike/dlls/$x dlls/ || exit 1 
done
#
# Compile installation package
echo "Compiling Inno Setup package..."
PROGRAMFILES=`sprsh cmd /c echo %PROGRAMFILES%`
sprsh "$PROGRAMFILES\Inno Setup 5\iscc.exe" pike.iss || exit 1
#
echo "Done."
