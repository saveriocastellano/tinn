pushd "%~dp0..\..\modules\%1"

set /p V8-VERSION=<..\..\V8-VERSION
set V8_HOME=..\..\includes\v8_%V8-VERSION%

nmake /D "V8_HOME=%V8_HOME%" "V8_VERSION=%V8-VERSION%" "ARCH=x64" "ARCH_FLAG=-m64" -f Makefile.win %2

popd