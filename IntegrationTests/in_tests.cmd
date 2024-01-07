@echo off
setlocal enabledelayedexpansion

goto :begining

REM Function to print a visual divider
:divisor
echo ========
goto :eof


:begining

REM Get the directory of the script
rem set "ScriptPath=%~f0"
set "ScriptPath=%~dp0"

REM Ensure a parameter has been provided
if "%~1"=="" (
    echo To run script provide to script 'bpatch' executable name as parameter
    echo "%~f0  path_to\bpatch.exe"
    exit /b 1
) else (
    echo "This script is located at: %ScriptPath%"
    echo "current folder is %cd%"
    echo "Files will be written here"
    call :divisor
)

REM Set up directories
set "Folder_FA=%ScriptPath%scripts"
set "Folder_FB=%ScriptPath%binaries"
set "Folder_Source=%ScriptPath%sources"
set "Folder_Expected=%ScriptPath%expected"

rem echo "FA is here %Folder_FA%"
rem echo "FB is here %Folder_FB%"
rem echo "sources is here %Folder_Source%"
rem echo "Expected is %Folder_Expected%"

set "ReturnValue=0"

REM Iterate over .test files in source directory
for %%F in ("%Folder_Source%\*.test") do (
    set "Src=%%~nxF"
    set "nameonly=%%~nF"
    set "Act=!nameonly!.json"
    set "Dest=%ScriptPath%!nameonly!.res"
    set "Command=%~1 -s %Folder_Source%\!Src! -a !Act! -w !Dest! -fa %Folder_FA% -fb %Folder_FB%"

    echo To execute: '!Command!'
    call :divisor

REM if we use just !Command! we can get "The system cannot find the path specified."
REM That is why we run command in the separate process
REM    !Command!
    start "executing" /MIN /WAIT !Command!

    REM Compare the output to the expected result
    fc /b %Folder_Expected%\!nameonly!.expected !Dest! >nul
    if errorlevel 1 (
        echo ERROR for !Src!
        set "ReturnValue=1"
    ) else (
        echo OK for !Src!
        del !Dest!
    )
    call :divisor
)

if not !ReturnValue! equ 0 (
    echo ERROR detected
) else (
    echo OK for every test
)
exit /b !ReturnValue!
