#!/bin/bash

cwdir=`pwd`
rootdir=`dirname "$0"`
cd "$rootdir"
rootdir=`pwd`
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:"$rootdir"
cd "$cwdir"
"$rootdir/TexTool" "$1" "$2" "$3"
