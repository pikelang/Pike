'
' $Id: installer.vbs,v 1.2 2004/12/01 21:07:21 grubba Exp $
'
' Companion file to bin/install.pike for custom actions.
'
' 2004-12-01 Henrik Grubbström
'

' At call time the CustomActionData property has been set to [TARGETDIR]
'
' Finalize Pike.
Function FinalizePike()
  Dim WshShell, targetdir
  targetdir = Session.Property("CustomActionData")

  Set WshShell = CreateObject("WScript.Shell")
  WshShell.Run """" & targetdir & "bin\pike""" &_
    " ""-m" & targetdir & "lib\master.pike""" &_
    " """ & targetdir & "bin\install.pike""" &_
    " --finalize BASEDIR=""" & targetdir & """" &_
    " ""TMP_BUILDDIR=" & targetdir & "bin""", 0, True

  FinalizePike = 1
End Function
