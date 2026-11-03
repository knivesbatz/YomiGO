@echo off
rem Builds the 64-bit helpers: YomiGoListen.exe (transcription, whisper.cpp) and YomiGoTalk.exe (conversation, llama.cpp).
rem They're 64-bit because the models need the memory and the speed. usage: build_x64.bat "<Visual Studio folder>"   (build.bat calls this)
setlocal
call "%~1\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul || exit /b 1
cd /d "%~dp0.."
if not exist third_party\whisper.cpp\include\whisper.h ( echo whisper.cpp is missing. Run: git submodule update --init & exit /b 1 )
if not exist third_party\llama.cpp\include\llama.h ( echo llama.cpp is missing. Run: git submodule update --init & exit /b 1 )
rem Each engine is built once. AVX2 without -march=native, so they run on any PC from about 2013 on, not just this one.
set GG=-A x64 -DBUILD_SHARED_LIBS=OFF -DGGML_NATIVE=OFF -DGGML_AVX=ON -DGGML_AVX2=ON -DGGML_FMA=ON -DGGML_F16C=ON -DGGML_OPENMP=OFF -DCMAKE_POLICY_DEFAULT_CMP0091=NEW -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded
if not exist build\whisper\src\Release\whisper.lib (
  cmake -S third_party\whisper.cpp -B build\whisper %GG% -DWHISPER_BUILD_EXAMPLES=OFF -DWHISPER_BUILD_TESTS=OFF -DWHISPER_BUILD_SERVER=OFF || exit /b 1
  cmake --build build\whisper --config Release -j 8 || exit /b 1
)
if not exist build\llama\src\Release\llama.lib (
  cmake -S third_party\llama.cpp -B build\llama %GG% -DLLAMA_BUILD_TESTS=OFF -DLLAMA_BUILD_EXAMPLES=OFF -DLLAMA_BUILD_SERVER=OFF -DLLAMA_BUILD_TOOLS=OFF -DLLAMA_BUILD_COMMON=OFF -DLLAMA_CURL=OFF || exit /b 1
  cmake --build build\llama --config Release -j 8 --target llama || exit /b 1
)
if not exist build\l mkdir build\l
if not exist build\t mkdir build\t
set CXF=/nologo /std:c++17 /EHsc /O2 /MT /utf-8 /W3 /D_CRT_SECURE_NO_WARNINGS
set SYS=winhttp.lib bcrypt.lib ole32.lib advapi32.lib
set W=build\whisper
cl %CXF% /Ithird_party\whisper.cpp\include /Ithird_party\whisper.cpp\ggml\include /Fobuild\l\ /Fedist\YomiGoListen.exe src\listen\yg_listen.cpp /link %W%\src\Release\whisper.lib %W%\ggml\src\Release\ggml.lib %W%\ggml\src\Release\ggml-base.lib %W%\ggml\src\Release\ggml-cpu.lib mfplat.lib mfreadwrite.lib mfuuid.lib comdlg32.lib %SYS% || exit /b 1
set L=build\llama
cl %CXF% /Ithird_party\llama.cpp\include /Ithird_party\llama.cpp\ggml\include /Fobuild\t\ /Fedist\YomiGoTalk.exe src\talk\yg_talk.cpp /link %L%\src\Release\llama.lib %L%\ggml\src\Release\ggml.lib %L%\ggml\src\Release\ggml-base.lib %L%\ggml\src\Release\ggml-cpu.lib %SYS% || exit /b 1
