#!/bin/sh

BUILD_DIR=$(dirname "$0")

if [ ! -f "$BUILD_DIR/V8-VERSION" ]; then
    echo "V8-VERSION is missing...? Please reinstall" 
	exit 1
fi


V8_VER=`cat $BUILD_DIR/V8-VERSION`

if [ $# -lt 1 ]; then 
	echo "\nSyntax is: ./tinn.sh v8_directory [build_directory]"
	echo "\n when not given build_directory defaults to out/x64.release"
	echo " e.g. ./tinn.sh /opt/v8"
	echo "\n"	
	exit 1
fi

V8_DIR=$1

if [ $# -eq 1 ]; then 
	OUT_DIR=$V8_DIR/out/x64.release
else
	OUT_DIR=$V8_DIR/$2
fi 

if [ ! -d "$V8_DIR" ]; then
    echo "v8_directory $V8_DIR does not exist"
	exit 1
fi

if [ ! -d "$V8_DIR/include" ]; then
    echo "include directory not found in $V8_DIR/include"
	exit 1
fi

V8_VER_H=$V8_DIR/include/v8-version.h

if [ ! -f "$V8_VER_H" ]; then
    echo "v8-version.h not found in $V8_VER_H" 
	exit 1
fi

V8_MAJ_V=`fgrep "V8_MAJOR_VERSION" $V8_VER_H | cut -d ' ' -f 3`
V8_MIN_V=`fgrep "V8_MINOR_VERSION" $V8_VER_H | cut -d ' ' -f 3`
V8_BN=`fgrep "V8_BUILD_NUMBER" $V8_VER_H | cut -d ' ' -f 3`
V8_VER_FOUND="$V8_MAJ_V.$V8_MIN_V.$V8_BN"

if [ "$V8_VER_FOUND" != "$V8_VER" ]; then
	echo "Needed v8 version is $V8_VER, v8 version found in $V8_DIR is $V8_VER_FOUND"
	exit 1
fi

D8_CC=$V8_DIR/src/d8/d8.cc
D8_H=$V8_DIR/src/d8/d8.h
D8_NINJA=$OUT_DIR/obj/d8.ninja
NINJA=`which ninja`

if [ ! -f "${NINJA}" ]; then
	echo "'ninja' was not found. Make sure that depot_tools is in PATH"
	exit 1 
fi

if [ ! -f "$D8_NINJA" ]; then
    echo "d8.ninja not found in $OUT_DIR/obj, make sure that v8 was built and you provided the correct build_directory" 
	exit 1
fi

if [ ! -f "${D8_CC}.orig" ]; then
	cp ${D8_CC} ${D8_CC}.orig
	cp ${D8_H} ${D8_H}.orig
	echo "backed up original d8 sources"
fi

if [ ! -f "${D8_NINJA}.orig" ]; then
	cp ${D8_NINJA} ${D8_NINJA}.orig
	echo "backed up original d8.ninja"	
fi

sed -i 's/-lpthread\s-lrt/-lpthread -rdynamic -lrt/' $OUT_DIR/obj/d8.ninja
cp $BUILD_DIR/tinn_$V8_VER/tinn.cc $D8_CC
cp $BUILD_DIR/tinn_$V8_VER/tinn.h $D8_H

D8=$OUT_DIR/d8

if [ ! -f "${D8}" ]; then
	mv ${D8} ${D8}.orig
fi

$NINJA -C $OUT_DIR

if [ ! -f "${D8}" ]; then
	echo "building TINN failed"
	exit 1
fi

cp ${D8} $BUILD_DIR/../tinn
cp $OUT_DIR/natives_blob.bin $BUILD_DIR/..
cp $OUT_DIR/snapshot_blob.bin $BUILD_DIR/..








