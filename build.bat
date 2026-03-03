@echo off
setlocal

set VCVARS="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if exist %VCVARS% call %VCVARS%

if not exist build mkdir build

rc /fo build\OptionsDialog.res src\OptionsDialog.rc

cl /LD /EHsc /O2 /MD /utf-8 /std:c++17 ^
   /I include ^
   /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" ^
   src\dllmain.cpp ^
   src\ZteMifiPlugin.cpp ^
   src\ZteMifiItem.cpp ^
   src\HttpClient.cpp ^
   src\UrlParser.cpp ^
   src\ConfigManager.cpp ^
   src\OptionsDialog.cpp ^
   build\OptionsDialog.res ^
   /link /DLL /OUT:build\ZteMifiPlugin.dll ^
   winhttp.lib user32.lib gdi32.lib comdlg32.lib

del /Q *.obj 2>nul

echo.
if exist build\ZteMifiPlugin.dll (
    echo Build SUCCESS: build\ZteMifiPlugin.dll
) else (
    echo Build FAILED
)

endlocal
