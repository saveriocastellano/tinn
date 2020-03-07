# TINN - 
TINN stands for TINN-Is-Not-NodeJS. 

TINN is a Javascript application toolkit based on the D8 Javascript shell from google-v8. TINN modifies D8 through a patch
file in order to add to it the capability of dynamically loading external C++ modules. 

## Features

* ...
* ...
* ...

## Compile and Install

```sh

git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH=$PATH:`pwd`/depot_tools
mkdir ~/v8
cd ~/v8
fetch v8
cd v8
gclient sync
./build/install-build-deps.sh
git checkout 7.9.1
tools/dev/gm.py x64.release




```
