@echo off
set Compiler_Flags= -nologo -Z7 -GS- -Gs9999999 -W4 -MT
set Linker_Flags= kernel32.lib shell32.lib SDL2.lib SDL2_image.lib -incremental:no -stack:0x100000,0x100000 -nodefaultlib -subsystem:console -entry:main

cl vim.c %Compiler_Flags% -link %Linker_Flags%
