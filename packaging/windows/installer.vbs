'
' $Id: installer.vbs,v 1.3 2004/12/03 18:20:19 grubba Exp $
'
' Companion file to bin/install.pike for custom actions.
'
' 2004-12-01 Henrik Grubbström
'

' At call time the CustomActionData property has been set to [TARGETDIR]
'
' First make a new master.
'
' Then start a pike to Finalize Pike.
Function FinalizePike()
  Dim fso, targetdir, source, dest, re
  targetdir = Session.Property("CustomActionData")

  Set fso = CreateObject("Scripting.FileSystemObject")

  Set source = fso.OpenTextFile(targetdir & "lib\master.pike.in", ForReading)

  Set dest = fso.CreateTextFile(targetdir & "lib\master.pike", True)

  source = source.ReadAll

  Set re = New RegExp
  re.Flobal = True

  re.Pattern = "¤lib_prefix¤"
  source = re.Replace(source, target & "lib")

  re.Pattern = "¤include_prefix¤"
  source = re.Replace(source, target & "include\pike")

  're.Pattern = "¤share_prefix¤"
  'source = re.Replace(source, "¤share_prefix¤")

  dest.Write(source)
  dest.Close

  ' FIXME: Is there no support for binary files in vbs?

  Dim WshShell

  Set WshShell = CreateObject("WScript.Shell")
  WshShell.CurrentDirectory = targetdir
  WshShell.Run "bin\pike" &_
    " -mlib\master.pike" &_
    " bin\install.pike" &_
    " --finalize BASEDIR=." &_
    " TMP_BUILDDIR=bin", 0, True

  ' Extra cleanup.
  fso.DeleteFile(targetdir & "bin\pike.exe.old")

  FinalizePike = 1
End Function
