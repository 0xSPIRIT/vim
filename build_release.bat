@echo off
set Compiler_Flags= -nologo -GS- -Gs9999999 -MT -O2
set Linker_Flags= kernel32.lib SDL2.lib SDL2_image.lib -incremental:no  -stack:0x100000,0x100000 -subsystem:windows -entry:main

cl vim.c %Compiler_Flags% -link %Linker_Flags%