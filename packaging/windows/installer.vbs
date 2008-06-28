'
' $Id: installer.vbs,v 1.4 2008/06/28 19:26:39 mast Exp $
'
' Companion file to bin/install.pike for custom actions.
'
' 2004-12-01 Henrik Grubbström
'

' At call time the CustomActionData property has been set to [TARGETDIR]
Function FinalizePike()
  Dim fso, targetdir, targetdir_unix, source, dest, re
  targetdir = Session.Property("CustomActionData")

  Set fso = CreateObject("Scripting.FileSystemObject")

  Set source = fso.OpenTextFile(targetdir & "lib\master.pike.in", 1, False, 0)

  Set dest = fso.CreateTextFile(targetdir & "lib\master.pike", True)

  source = source.ReadAll

  Set re = New RegExp
  re.Global = True

  re.Pattern = "\\"
  targetdir_unix = re.Replace(targetdir, "/")

  re.Pattern = "¤lib_prefix¤"
  source = re.Replace(source, targetdir_unix & "lib")

  re.Pattern = "¤include_prefix¤"
  source = re.Replace(source, targetdir_unix & "include/pike")

  're.Pattern = "¤share_prefix¤"
  'source = re.Replace(source, "¤share_prefix¤")

  dest.Write(source)
  dest.Close

  ' Warning: It appears to be very difficult to call pike from here to do
  ' finalizing stuff. The problem is that if the MS CRT libs are used, and
  ' they get installed at the same time, then pike can't use them from here.
  ' Many attempts to get around that have failed. /mast

  ' Extra cleanup.
  If fso.FileExists(targetdir & "bin\pike.exe.old") Then
    WshShell.Run "%windir%\system32\cmd /c del bin\pike.exe.old" &_
      " /f", 0, True
  End If

  FinalizePike = 1
End Function
