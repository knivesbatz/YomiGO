@echo off
rem Builds YomiGo Desktop, HTPC and the updater, and runs the core tests.
setlocal
set ROOT=%~dp0
set PYTHONUTF8=1
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" for /f "usebackq tokens=*" %%i in (`call "%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS=%%i"
if "%VS%"=="" if exist "%ProgramFiles%\Microsoft Visual Studio\18\Community\VC" set "VS=%ProgramFiles%\Microsoft Visual Studio\18\Community"
if "%VS%"=="" ( echo Visual Studio with C++ tools not found & exit /b 1 )
call "%VS%\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul || exit /b 1
cd /d "%ROOT%"

python tools\build_content.py || exit /b 1
if not exist build\gen\font_gen.h python tools\build_font.py || exit /b 1
python tools\gen_vectors.py || exit /b 1
if not exist dist mkdir dist
if not exist build\d mkdir build\d
if not exist build\h mkdir build\h
set /p VER=<VERSION
python tools\gen_version.py || exit /b 1

rem Core tests use the normal C runtime
cl /nologo /utf-8 /W3 /O1 /Ibuild\gen /Isrc /Fobuild\ /Febuild\test_core.exe tests\test_core.c src\core.c /link /STACK:2097152 || exit /b 1
build\test_core.exe || exit /b 1

rem The apps: no C runtime, x87 only, ANSI imports, so they can load on Windows 95
set CF=/nologo /utf-8 /W3 /O1 /Oy- /GS- /Gs999999 /GR- /EHs-c- /arch:IA32 /Zc:threadSafeInit- /Ibuild\gen /Isrc
set LF=/link /NODEFAULTLIB /ENTRY:yg_entry /SUBSYSTEM:WINDOWS,5.01 /FIXED /DYNAMICBASE:NO /NXCOMPAT:NO /SAFESEH:NO /MERGE:.rdata=.text /OPT:REF /STACK:1048576 kernel32.lib user32.lib gdi32.lib winmm.lib shell32.lib advapi32.lib
cl %CF% /Fobuild\d\ /Fedist\YomiGo.exe src\core.c src\ui.c src\screens.c src\plat_win.c %LF% || exit /b 1
cl %CF% /DYG_HTPC=1 /Fobuild\h\ /Fedist\YomiGoHTPC.exe src\core.c src\ui.c src\screens.c src\plat_win.c %LF% || exit /b 1
python tools\check_pe.py dist\YomiGo.exe dist\YomiGoHTPC.exe || exit /b 1

rem The updater needs WinHTTP with TLS 1.2, so Windows 7 or later.
rem The manifest matters: without it, Windows sees "Update" in the name and demands admin rights.
cl /nologo /utf-8 /W3 /O1 /GS- /Gs999999 /GR- /EHs-c- /Ibuild\gen /Isrc\updater /Fobuild\ /Fedist\YomiGoUpdate.exe src\updater\yg_update.c /link /NODEFAULTLIB /ENTRY:yg_entry /SUBSYSTEM:WINDOWS /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" kernel32.lib user32.lib advapi32.lib shell32.lib winhttp.lib || exit /b 1
rem The text reader uses the OCR in Windows 10 and later, through C++/WinRT from the Windows SDK
cl /nologo /std:c++20 /EHsc /O1 /MT /utf-8 /W3 /D_CRT_SECURE_NO_WARNINGS /Fobuild\ /Fedist\YomiGoOCR.exe src\ocr\yg_ocr.cpp /link WindowsApp.lib || exit /b 1
rem The transcriber, with whisper.cpp from the submodule
call tools\build_listen.bat "%VS%" || exit /b 1
echo Build OK: dist\YomiGo.exe dist\YomiGoHTPC.exe dist\YomiGoUpdate.exe dist\YomiGoOCR.exe dist\YomiGoListen.exe
