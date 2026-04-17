Set fs = CreateObject("Scripting.FileSystemObject")
Set oldExec = fs.OpenTextFile("run.exe.tmp")
Set newExec = fs.OpenTextFile("run.exe", 2, True)
If IsNull(oldExec) Then
  WScript.Quit
End If

newExec.Write(oldExec.Read(324))
newExec.Write(chr(3))
oldExec.Skip(1)
Do While oldExec.AtEndOfStream <> True
  newExec.Write(oldExec.Read(100))
Loop

oldExec.Close()
newExec.Close()
