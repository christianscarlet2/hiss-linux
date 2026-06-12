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

echo "[2/3] engine + support + subsystems"
cd "$ROOT/engine/oh"
fail=0
for f in *.cpp; do
  $CXX -include stdafx.h -c $INC "$f" -o "$OBJ/${f%.cpp}.o" 2>>"$OBJ/_build.log" || { echo "  ! compile failed: $f (see build/obj/_build.log)"; fail=1; }
done
[ "$fail" = 1 ] && { echo "ABORT: unexpected compile failure"; exit 1; }

echo "[3/3] link -> hiss"
printf 'int main(){return 0;}\n' > "$OBJ/_main.cpp"
$CXX -c "$OBJ/_main.cpp" -o "$OBJ/_main.o"
$CXX -o "$ROOT/build/hiss" "$OBJ"/*.o "$OBJ/libpokereval.a" -lpthread -lm

echo "OK -> $ROOT/build/hiss"
