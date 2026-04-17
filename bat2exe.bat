@ECHO OFF
REM Based on https://github.com/amakvana/Bat-2-Exec/blob/5f75539/Bat2Exe.bat

IF "%~1" EQU "" EXIT
IF "%~2" EQU "" EXIT
SET "target_exe=%__cd__%%~n2.exe"
SET "tmp_exe=%target_exe%.tmp"
SET "bat_name=%~nx2"
SET "bat_dir=%~dp2"
SET "sed=exe.sed"

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

CALL iexpress /n /q /m %sed%

IF "%~1" EQU "-t" (
  CALL cscript //b fixexec.vbs
) ELSE (
  COPY "%tmp_exe%" "%target_exe%" >NUL
)

DEL /q /f "%sed%" "%tmp_exe%"
