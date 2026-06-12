#!/bin/bash
# build.sh — compile + link the Linux Hiss (OpenHoldem) engine.
#
# Three stages:
#   1. poker-eval  (pure C)            -> libpokereval.a
#   2. engine + support + subsystems   -> *.o   (against the compat/ MFC shim)
#   3. link everything                 -> hiss   (with a trivial main for now;
#                                                 the daemon/HTTP entrypoint is next)
set -e
ROOT="$(cd "$(dirname "$0")" && pwd)"
OBJ="$ROOT/build/obj"
CXX="g++ -std=c++17 -fpermissive -w"
CC="gcc -std=c11 -w -O2"
INC="-I $ROOT/compat -I $ROOT/engine/oh"
JOBS="${JOBS:-6}"
mkdir -p "$OBJ"

echo "[1/3] poker-eval -> libpokereval.a"
cd "$ROOT/engine/pokereval"
for c in *.c; do $CC -c -I . "$c" -o "$OBJ/pe_${c%.c}.o"; done
ar rcs "$OBJ/libpokereval.a" "$OBJ"/pe_*.o

echo "[2/3] engine + support + subsystems ($JOBS-way parallel)"
cd "$ROOT/engine/oh"
: > "$OBJ/_fail.list"
export OBJ INC CXX
# filename arrives as a positional arg ("$1") so names with $ (e.g. Chair$Symbols.cpp) are safe
compile_one() { $CXX -include stdafx.h -c $INC "$1" -o "$OBJ/$(basename "$1" .cpp).o" 2>>"$OBJ/_build.log" || echo "$1" >> "$OBJ/_fail.list"; }
export -f compile_one
ls *.cpp | xargs -P "$JOBS" -I {} bash -c 'compile_one "$@"' _ {}
if [ -s "$OBJ/_fail.list" ]; then echo "ABORT: compile failures:"; cat "$OBJ/_fail.list"; exit 1; fi

echo "[2b] WinHTTP-on-libcurl bridge (the API client transport)"
$CXX -c -I "$ROOT/compat" "$ROOT/compat/winhttp_curl.cpp" -o "$OBJ/winhttp_curl.o"

echo "[3/3] link -> hiss"
printf 'int main(){return 0;}\n' > "$OBJ/_main.cpp"
$CXX -c "$OBJ/_main.cpp" -o "$OBJ/_main.o"
$CXX -o "$ROOT/build/hiss" "$OBJ"/*.o "$OBJ/libpokereval.a" -lpthread -lm -lcurl

echo "OK -> $ROOT/build/hiss"
