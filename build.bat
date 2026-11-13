@echo off
rem Compile la ROM (out\rom.bin). "build.bat run" la lance ensuite dans BlastEm.
setlocal
set GDK=D:/code/tools/sgdk
set PATH=D:\code\tools\jre\bin;D:\code\tools\sgdk\bin;%PATH%
cd /d %~dp0
make -f %GDK%/makefile.gen release
if errorlevel 1 exit /b 1
if /i "%1"=="run" start "" D:\code\tools\blastem\blastem.exe "%~dp0out\rom.bin"
