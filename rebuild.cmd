@echo off
setlocal enabledelayedexpansion


:: Save the initial execution directory
set "INITIALDIR=%cd%"

:: Establish the mode, defaulting to Release if no argument is given
set "MODE=Release"
if not "%~1"=="" set "MODE=%~1"
:: Check if MODE is not Debug, if true set it to Release
if not "%MODE%"=="Debug" set "MODE=Release"

IF "%VS170COMNTOOLS%"=="" (
    echo Environment variable VS170COMNTOOLS not set or is empty
    echo Please set up this variable or run this script from the developer command line of MSVS 2022
    echo PS. This script need to be reworked if you are using later version of MSVS
    exit /b 1
)

SET "CMAKECMD=%VS170COMNTOOLS%..\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"

IF NOT EXIST "%CMAKECMD%" (
    echo Cannot find cmake at the path: "%CMAKECMD%"
    echo Most probably you should install cmake for MSVS
    exit /b 1
) ELSE (
    echo ----------------------------------
    echo  Rebuilding of bpatch %MODE% mode
    echo ----------------------------------
)



:: Establish the directory containing the script
set "PROJECTDIR=%~dp0"

:: rebuilding initiation
cd /d %PROJECTDIR%
if exist "buildWin%MODE%" rmdir /s /q "buildWin%MODE%"
mkdir "buildWin%MODE%" && cd "buildWin%MODE%"
"%CMAKECMD%" -DCMAKE_BUILD_TYPE=%MODE% ..
"%CMAKECMD%" --build . --config %MODE%

echo ----------------------------------

set "FILENAME=%PROJECTDIR%buildWin%MODE%\%MODE%\bpatch.exe"

IF EXIST "%FILENAME%" (
  echo   bpatch.exe built for %MODE% mode is here:
  echo %FILENAME%
) ELSE (
  echo   bpatch.exe built for %MODE% mode could not be found at
  echo %FILENAME%
)

set "FILENAME=%PROJECTDIR%buildWin%MODE%\testbpatch\%MODE%\testbpatch.exe"

IF EXIST "%FILENAME%" (
  echo   Unit tests for bpatch.exe are here:
  echo %FILENAME%
) ELSE (
  echo   Failed to find unit tests for bpatch.exe here:
  echo %FILENAME%
)
echo ----------------------------------


:: Restore the initial execution directory
cd /d %INITIALDIR%

endlocal
