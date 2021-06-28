@echo off
setlocal enabledelayedexpansion

SET timestamp=%TIME:~3,2%%TIME:~6,2%
SET args=-g -DDEBUG -D_DEBUG -DRENDERER_VULKAN
SET include_path=-I D:\Guigui\Work\Prog\_include\ -I %VULKAN_SDK%\include -I src/

SET linker_options=-L D:\Guigui\Work\Prog\_lib -L %VULKAN_SDK%\lib -Xlinker -incremental:no
SET libs=-lSDL2main.lib -lSDL2.lib -lSDL2_image.lib -lShell32.lib -lvulkan-1.lib

SET module_name=game

PUSHD tmp\
DEL /Q %module_name%_*.pdb  2> NUL
POPD

ECHO Building %module_name% module
clang %args% %include_path% -shared src/game.cpp -o bin/%module_name%.dll %linker_options% %libs% -Xlinker -PDB:tmp/%module_name%_%timestamp%.pdb -Xlinker -IMPLIB:tmp/%module_name%.lib
if !ERRORLEVEL! == 0 ( echo a > bin/%module_name%.meta )

ECHO Ok