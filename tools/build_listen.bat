@echo off
rem Builds YomiGoListen.exe, the transcriber. It's 64-bit, as whisper.cpp needs the memory and the speed.
rem usage: build_listen.bat "<Visual Studio folder>"   (build.bat calls this)
setlocal
call "%~1\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul || exit /b 1
cd /d "%~dp0.."
if not exist third_party\whisper.cpp\include\whisper.h ( echo whisper.cpp is missing. Run: git submodule update --init & exit /b 1 )
rem whisper.cpp is built once. AVX2 without -march=native, so it runs on any PC from about 2013 on, not just this one.
if not exist build\whisper\src\Release\whisper.lib (
  cmake -S third_party\whisper.cpp -B build\whisper -A x64 -DBUILD_SHARED_LIBS=OFF -DWHISPER_BUILD_EXAMPLES=OFF -DWHISPER_BUILD_TESTS=OFF -DWHISPER_BUILD_SERVER=OFF -DGGML_NATIVE=OFF -DGGML_AVX=ON -DGGML_AVX2=ON -DGGML_FMA=ON -DGGML_F16C=ON -DGGML_OPENMP=OFF -DCMAKE_POLICY_DEFAULT_CMP0091=NEW -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded || exit /b 1
  cmake --build build\whisper --config Release -j 8 || exit /b 1
)
if not exist build\l mkdir build\l
set W=build\whisper
cl /nologo /std:c++17 /EHsc /O2 /MT /utf-8 /W3 /D_CRT_SECURE_NO_WARNINGS /Ithird_party\whisper.cpp\include /Ithird_party\whisper.cpp\ggml\include /Fobuild\l\ /Fedist\YomiGoListen.exe src\listen\yg_listen.cpp /link %W%\src\Release\whisper.lib %W%\ggml\src\Release\ggml.lib %W%\ggml\src\Release\ggml-base.lib %W%\ggml\src\Release\ggml-cpu.lib mfplat.lib mfreadwrite.lib mfuuid.lib winhttp.lib bcrypt.lib ole32.lib comdlg32.lib advapi32.lib || exit /b 1
