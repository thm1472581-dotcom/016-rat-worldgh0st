@echo off
setlocal
set "MSBUILD=%ProgramFiles(x86)%\MSBuild\12.0\Bin\MSBuild.exe"
if not exist "%MSBUILD%" set "MSBUILD=%ProgramFiles%\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
if not exist "%MSBUILD%" (echo MSBuild not found. & exit /b 1)
set "XTPROOT=A:\100-programfiles\archive\Xtreme ToolkitPro v15.2.1"
if exist "%~dp0XTP.local.props" for /f "tokens=3 delims=<>" %%A in ('findstr /C:"<XtpRoot>" "%~dp0XTP.local.props"') do set "XTPROOT=%%A"
echo Building ToolkitPro1521vc100S.lib ...
"%MSBUILD%" "%XTPROOT%\Workspace\ToolkitPro\ToolkitProStatic_vc100.vcxproj" /p:Configuration="Static Release" /p:Platform=Win32 /p:PlatformToolset=v120 /m /v:minimal
if errorlevel 1 exit /b 1
if exist "%XTPROOT%\Lib\vc100\ToolkitPro1521vc100S.lib" (echo OK: %XTPROOT%\Lib\vc100\ToolkitPro1521vc100S.lib) else (echo ERROR: library not produced. & exit /b 1)
