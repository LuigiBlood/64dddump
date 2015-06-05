@echo off
cls
if exist build.z64 del build.z64

set path=%PATH%;C:\n64\bin
set PSYQ_PATH=C:\n64\bin

cmd /c make
echo.
if errorlevel 1 goto failure

del demo.sym > crap
..\util\resize demo.bin 1052672 > crap
..\util\rn64crc -u demo.bin > crap
ren demo.bin build.z64

del crap


echo -------------------------------------------------------------------------------
echo BUILD COMPLETED
goto complete


:failure
echo -------------------------------------------------------------------------------
echo BUILD FAILED !!!!!!!!!!!!!!!!


:complete