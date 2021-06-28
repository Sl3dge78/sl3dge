@echo off
setlocal enabledelayedexpansion

SET timestamp=%TIME:~3,2%%TIME:~6,2%
SET args=-g -DDEBUG -D_DEBUG -DRENDERER_VULKAN
SET include_path=-I D:\Guigui\Work\Prog\_include\ -I %VULKAN_SDK%\include -I src/

SET linker_options=-L D:\Guigui\Work\Prog\_lib -L %VULKAN_SDK%\lib -Xlinker -incremental:no
SET libs=-lSDL2main.lib -lSDL2.lib -lSDL2_image.lib -lShell32.lib -lvulkan-1.lib

PUSHD tmp\
DEL /Q win32_*.pdb 2> NUL
POPD

ECHO Building win32.exe
clang %args% %include_path% src/platform/platform_win32.cpp -o bin/win32.exe %linker_options% %libs% -Xlinker -SUBSYSTEM:WINDOWS -Xlinker -PDB:tmp/win32.pdb
ECHO Ok