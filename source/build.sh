#!/bin/sh
# Builds XINPUT9_1_0.dll with clang + lld (no Windows SDK or MSVC needed).
set -e
llvm-dlltool -m i386 -k -d kernel32.def -l kernel32.lib
llvm-dlltool -m i386 -k -d user32.def -l user32.lib
clang --target=i686-pc-windows-msvc -O2 -msse2 -fno-builtin -ffreestanding \
      -fno-stack-protector -mno-stack-arg-probe -c freecam.c -o freecam.obj
lld-link /dll /out:XINPUT9_1_0.dll /entry:DllMain@12 /nodefaultlib /def:exports.def \
         /safeseh:no /machine:x86 freecam.obj kernel32.lib user32.lib
echo "Built XINPUT9_1_0.dll"
