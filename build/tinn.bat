
SET DIR=%~dp0

copy %DIR%\tinn_7.9.2\tinn.cc c:\work\v8\src\d8\d8.cc
copy %DIR%\tinn_7.9.2\tinn.h c:\work\v8\src\d8\d8.h
copy %DIR%\includes\win32\linux.h c:\work\v8\src\d8\linux.h
copy %DIR%\includes\win32\dirent.h c:\work\v8\src\d8\dirent.h

ninja -C c:\work\v8\out\x64.release

copy c:\work\v8\out\x64.release\d8.exe %DIR%\..\tinn.exe