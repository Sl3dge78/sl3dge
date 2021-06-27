@echo off
setlocal enabledelayedexpansion

SET timestamp=%TIME:~3,2%%TIME:~6,2%
SET args=-g -DDEBUG -D_DEBUG -DRENDERER_VULKAN
SET include_path=-I D:\Guigui\Work\Prog\_include\ -I %VULKAN_SDK%\include -I src/

SET linker_options=-L D:\Guigui\Work\Prog\_lib -L %VULKAN_SDK%\lib -Xlinker -incremental:no
SET libs=-lSDL2main.lib -lSDL2.lib -lSDL2_image.lib -lShell32.lib -lvulkan-1.lib

DEL /Q tmp > NUL 2> NUL
MKDIR tmp > NUL 2> NUL

ECHO Building win32.exe
clang %args% %include_path% src/platform/platform_win32.cpp -o bin/win32.exe %linker_options% %libs% -Xlinker -SUBSYSTEM:WINDOWS -Xlinker -PDB:tmp/win32.pdb


SET module_name=renderer
ECHO Building %module_name% module
clang %args% %include_path% -shared src/%module_name%/%module_name%.cpp -o bin/%module_name%.dll %linker_options% %libs% -Xlinker -PDB:tmp/%module_name%_%timestamp%.pdb -Xlinker -IMPLIB:tmp/%module_name%.lib
if !ERRORLEVEL! == 0 ( echo a > bin/%module_name%.meta )

ECHO Building game module
clang %args% %include_path% -shared src/game.cpp -o bin/game.dll %linker_options% %libs% -Xlinker -PDB:tmp/game_%timestamp%.pdb -Xlinker -IMPLIB:tmp/game.lib
if !ERRORLEVEL! == 0 ( echo a > bin/game.meta )

ECHO Done