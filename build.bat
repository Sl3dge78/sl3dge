@echo off
setlocal enabledelayedexpansion

SET timestamp=%TIME:~3,2%%TIME:~6,2%
SET args=-g -DDEBUG
SET include_path=-I D:\Guigui\Work\Prog\_include\ -I %VULKAN_SDK%\include

SET linker_options=-L D:\Guigui\Work\Prog\_lib -L %VULKAN_SDK%\lib -Xlinker -incremental:no
SET libs=-lSDL2main.lib -lSDL2.lib -lSDL2_image.lib -lShell32.lib -lvulkan-1.lib

DEL /Q tmp > NUL 2> NUL
MKDIR tmp > NUL 2> NUL

ECHO win32.cpp
clang %args% %include_path% src/win32.cpp -o bin/win32.exe %linker_options% %libs% -Xlinker -SUBSYSTEM:WINDOWS -Xlinker -PDB:tmp/win32.pdb
FOR %%G IN (game, vulkan) do (
    ECHO %%G.cpp
    clang %args% %include_path% -shared src/%%G.cpp -o bin/%%G.dll %linker_options% %libs% -Xlinker -PDB:tmp/%%G_%timestamp%.pdb -Xlinker -IMPLIB:tmp/%%G.lib
    if !ERRORLEVEL! == 0 ( echo a > bin/%%G.meta )
)

ECHO Done