#!/bin/bash
DIR="$(cd "$(dirname "$0")" && pwd)"
export DYLD_INSERT_LIBRARIES="$DIR/libObscura.dylib"
exec "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/ld" "$@"
