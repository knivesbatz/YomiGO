@echo off
rem Builds YomiGo for Windows (the Rust app, which is also the TV app), the updater and the helpers, and runs the tests.
setlocal
set ROOT=%~dp0
set PYTHONUTF8=1
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" for /f "usebackq tokens=*" %%i in (`call "%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS=%%i"
if "%VS%"=="" if exist "%ProgramFiles%\Microsoft Visual Studio\18\Community\VC" set "VS=%ProgramFiles%\Microsoft Visual Studio\18\Community"
if "%VS%"=="" ( echo Visual Studio with C++ tools not found & exit /b 1 )
cd /d "%ROOT%"

python tools\build_content.py || exit /b 1
python tools\gen_version.py || exit /b 1
if not exist dist mkdir dist

rem The app: Rust and the shared web UI. One program; named YomiGoHTPC.exe it opens in TV mode.
cargo test --release --manifest-path desktop\Cargo.toml || exit /b 1
cargo build --release --manifest-path desktop\Cargo.toml || exit /b 1
copy /y desktop\target\release\yomigo.exe dist\YomiGo.exe >nul || exit /b 1
copy /y desktop\target\release\yomigo.exe dist\YomiGoHTPC.exe >nul || exit /b 1

call "%VS%\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul || exit /b 1
rem The updater needs WinHTTP with TLS 1.2.
rem The manifest matters: without it, Windows sees "Update" in the name and demands admin rights.
cl /nologo /utf-8 /W3 /O1 /GS- /Gs999999 /GR- /EHs-c- /Ibuild\gen /Isrc\updater /Fobuild\ /Fedist\YomiGoUpdate.exe src\updater\yg_update.c /link /NODEFAULTLIB /ENTRY:yg_entry /SUBSYSTEM:WINDOWS /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" kernel32.lib user32.lib advapi32.lib shell32.lib winhttp.lib || exit /b 1
rem The text reader uses the OCR in Windows 10 and later, through C++/WinRT from the Windows SDK
cl /nologo /std:c++20 /EHsc /O1 /MT /utf-8 /W3 /D_CRT_SECURE_NO_WARNINGS /Fobuild\ /Fedist\YomiGoOCR.exe src\ocr\yg_ocr.cpp /link WindowsApp.lib || exit /b 1
rem The transcriber and the conversation partner, with whisper.cpp and llama.cpp from the submodules
call tools\build_x64.bat "%VS%" || exit /b 1
echo Build OK: dist\YomiGo.exe dist\YomiGoHTPC.exe dist\YomiGoUpdate.exe dist\YomiGoOCR.exe dist\YomiGoListen.exe dist\YomiGoTalk.exe
