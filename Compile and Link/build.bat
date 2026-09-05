@echo off
llvm-rc ..\resource.rc /fo resource.res
clang++ ..\main.cpp resource.res -o ..\qmx_engine.exe -O3 -std=c++17 -fuse-ld=lld -Xlinker /subsystem:windows -Xlinker -entry:WinMainCRTStartup -l d3d12 -l dxgi -l user32
signtool sign /n "QMX Corporation" /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 qmx_engine.exe

if %errorlevel% equ 0 (
    echo [QMX ENGINE] qmx_engine.exe generated successfully!
) else (
    echo [QMX ENGINE] Check the Errors
)
