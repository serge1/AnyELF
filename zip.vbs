' Syntax: cscript zip.vbs target.zip file1 file2 ... fileN

Set fso = CreateObject("Scripting.FileSystemObject")
sSourceFile = fso.GetAbsolutePathName(WScript.Arguments.Item(0))

Set objArgs = WScript.Arguments
ZipFile = fso.GetAbsolutePathName(objArgs(0))

' Create empty ZIP file and open for adding
CreateObject("Scripting.FileSystemObject").CreateTextFile(ZipFile, True).Write "PK" & Chr(5) & Chr(6) & String(18, vbNullChar)
Set zip = CreateObject("Shell.Application").NameSpace(ZipFile)

' Add all files/directories to the .zip file
For i = 1 To objArgs.count-1
  zip.CopyHere(fso.GetAbsolutePathName(objArgs(i)))
  'Keep script waiting until compression is done
  Do Until zip.Items.Count = i
    WScript.Sleep 200
  Loop
Next
