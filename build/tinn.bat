@echo off
SET BUILD_DIR=%~dp0

if not exist %BUILD_DIR%/V8-VERSION (
	echo 
	echo V8-VERSION is missing...? Please reinstall
	goto eof
)

@set /p V8_VER=<%BUILD_DIR%\V8-VERSION

SET /A ARGS_COUNT=0    
FOR %%A in (%*) DO SET /A ARGS_COUNT+=1    

	 
if %ARGS_COUNT% lss 1 (
	echo. 
	echo. Syntax is: tinn.bat v8_directory [build_directory]
	echo. when not given build_directory defaults to out\x64.release
	echo. e.g. tinn.bat c:\v8
	echo.
	
	goto eof
)

set V8_DIR=%1

if %ARGS_COUNT% equ 1 (
	set OUT_DIR=%V8_DIR%\out\x64.release
) else (
	set OUT_DIR=%V8_DIR%\%2
)

if not exist %V8_DIR% (
    echo.
	echo. v8_directory %V8_DIR% does not exist
	echo.
	goto eof
)

if not exist %V8_DIR%\include (
    echo.
	echo. include directory not found in %V8_DIR%\include
	echo.
	goto eof
)


set V8_VER_H=%V8_DIR%\include\v8-version.h

if not exist %V8_VER_H% (
	echo.
	echo. v8-version.h not found in %V8_VER_H%
	echo.
	goto eof
)

for /F "tokens=1,2,3 delims=." %%a in ("%V8_VER%") do (
   set V8_VER_MAJOR=%%a
   set V8_VER_MINOR=%%b
   set V8_VER_BUILD_NUM=%%c
)


set V8_MAJ_V=
set V8_MIN_V=
set V8_BUILD_M=
FOR /F "tokens=* USEBACKQ" %%g IN (`findstr /c:"V8_MAJOR_VERSION %V8_VER_MAJOR%" %V8_VER_H%`) do (SET "V8_MAJ_V=%%g")
FOR /F "tokens=* USEBACKQ" %%g IN (`findstr /c:"V8_MINOR_VERSION %V8_VER_MINOR%" %V8_VER_H%`) do (SET "V8_MIN_V=%%g")
FOR /F "tokens=* USEBACKQ" %%g IN (`findstr /c:"V8_BUILD_NUMBER %V8_VER_BUILD_NUM%" %V8_VER_H%`) do (SET "V8_BUILD_M=%%g")

set VALID_VER=1
IF "%V8_MAJ_V%" == "" set VALID_VER=0
IF "%V8_MIN_V%" == "" set VALID_VER=0
IF "%V8_BUILD_M%" == "" set VALID_VER=0

if %VALID_VER% equ 0 (
	echo.
	echo. v8-version.h not found in %V8_VER_H%
	echo. Needed v8 version is %V8_VER%, different v8 version found in %V8_DIR%
	goto eof
)


@WHERE ninja > nul 2>&1
IF %ERRORLEVEL% NEQ 0 (
	echo.
	echo. 'ninja' was not found. Make sure that depot_tools is in PATH
	goto eof

)

set D8_CC=%V8_DIR%\src\d8\d8.cc
set D8_H=%V8_DIR%\src\d8\d8.h
set D8_NINJA=%OUT_DIR%\obj\d8.ninja

if not exist %D8_NINJA% (
    echo.
	echo. d8.ninja not found in %OUT_DIR%\obj, make sure that v8 was built and you provided the correct build_directory
	goto eof

)

rem check that v8 was built..

set VALID_BUILT=1
IF not exist %OUT_DIR%\d8.exe set VALID_BUILT=0
IF not exist %OUT_DIR%\natives_blob.bin set VALID_BUILT=0
IF not exist %OUT_DIR%\snapshot_blob.bin set VALID_BUILT=0
IF not exist %OUT_DIR%\icui18n.dll set VALID_BUILT=0
IF not exist %OUT_DIR%\icuuc.dll set VALID_BUILT=0
IF not exist %OUT_DIR%\libc++.dll set VALID_BUILT=0
IF not exist %OUT_DIR%\v8.dll set VALID_BUILT=0
IF not exist %OUT_DIR%\v8_libbase.dll set VALID_BUILT=0
IF not exist %OUT_DIR%\v8_libplatform.dll set VALID_BUILT=0

if %VALID_BUILT% equ 0 (
	echo.
	echo. Some binary files were not found in %OUT_DIR%
	echo. You need to build v8 first
	echo. If v8 was built then make sure that the following files exist in %OUT_DIR%: 
	echo. d8.exe, natives_blob.bin, snapshot_blob.bin, icui18n.dll, icuuc.dll, libc.dll, v8.dll, v8_libbase.dll, v8_libplatform.dll
	goto eof
)


if not exist %D8_CC%.orig (
	copy %D8_CC% %D8_CC%.orig 1> nul
	copy %D8_H% %D8_H%.orig 1> nul
	echo. backed up original d8 sources
)

copy %BUILD_DIR%\tinn_7.9.2\tinn.cc %D8_CC% 1> nul
copy %BUILD_DIR%\tinn_7.9.2\tinn.h %D8_H% 1> nul
copy %BUILD_DIR%\includes\win32\linux.h %V8_DIR%\src\d8\linux.h 1> nul
copy %BUILD_DIR%\includes\win32\dirent.h %V8_DIR%\src\d8\dirent.h 1> nul

ninja -C %OUT_DIR%

copy %OUT_DIR%\d8.exe %BUILD_DIR%\..\tinn.exe 1> nul
copy %OUT_DIR%\natives_blob.bin %BUILD_DIR%\.. 1> nul
copy %OUT_DIR%\snapshot_blob.bin %BUILD_DIR%\.. 1> nul

copy %OUT_DIR%\icui18n.dll %BUILD_DIR%\.. 1> nul
copy %OUT_DIR%\icuuc.dll %BUILD_DIR%\.. 1> nul
copy %OUT_DIR%\libc++.dll %BUILD_DIR%\.. 1> nul
copy %OUT_DIR%\v8.dll %BUILD_DIR%\.. 1> nul
copy %OUT_DIR%\v8_libbase.dll %BUILD_DIR%\.. 1> nul
copy %OUT_DIR%\v8_libplatform.dll %BUILD_DIR%\.. 1> nul

:eof