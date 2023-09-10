@echo off
set Compiler_Flags= -nologo -GS- -Gs9999999 -MT -O2
set Linker_Flags= kernel32.lib shell32.lib SDL2.lib SDL2_image.lib -incremental:no  -stack:0x100000,0x100000 -subsystem:console -entry:main

clang-cl vim.c %Compiler_Flags% -link %Linker_Flags%
