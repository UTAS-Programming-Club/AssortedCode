<!-- : Begin batch script
@ECHO OFF
REM Licensed under MIT.
REM Based on https://github.com/amakvana/Bat-2-Exec/blob/5f75539/Bat2Exe.bat
REM and https://stackoverflow.com/a/9074483

IF "%~1" EQU "" EXIT
IF "%~2" EQU "" EXIT
SET "target_exe=%__cd__%%~n2.exe"
SET "tmp_exe=%target_exe%.tmp"
SET "bat_name=%~nx2"
SET "bat_dir=%~dp2"
SET "sed=%temp%\exe.sed"

(
  ECHO [Version]
  ECHO Class=IEXPRESS
  ECHO SEDVersion=3
  ECHO [Options]
  ECHO PackagePurpose=InstallApp
  ECHO ShowInstallProgramWindow=0
  ECHO HideExtractAnimation=1
  ECHO UseLongFileName=1
  ECHO InsideCompressed=0
  ECHO CAB_FixedSize=0
  ECHO CAB_ResvCodeSigning=0
  ECHO RebootMode=N
  ECHO TargetName=%tmp_exe%
  ECHO AppLaunched=cmd.exe /c "%bat_name%"
  ECHO PostInstallCmd=^<None^>
  ECHO SourceFiles=SourceFiles
  ECHO [SourceFiles]
  ECHO SourceFiles0=%bat_dir%
  ECHO [SourceFiles0]
  ECHO %bat_name%=
)>"%sed%"

CALL iexpress /M /N /Q %sed%

IF "%~1" EQU "-t" (
  CALL cscript //B //Job:FixSubsystem "%~f0?.wsf" "%tmp_exe%" "%target_exe%"
) ELSE (
  COPY "%tmp_exe%" "%target_exe%" >NUL
)

DEL /Q "%sed%" "%tmp_exe%"
EXIT /B

----- Begin wsf script --->
<package>
  <job id="FixSubsystem">
    <script language="VBScript">
      Dim fs, oldExec, newExec
      Set fs = CreateObject("Scripting.FileSystemObject")
      Set oldExec = fs.OpenTextFile(WScript.Arguments(0))
      Set newExec = fs.OpenTextFile(WScript.Arguments(1), 2, True)
      If IsNull(oldExec) or IsNull(newExec) Then
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
    </script>
  </job>
</package>
