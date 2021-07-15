@echo off
setlocal enabledelayedexpansion

SET timestamp=%TIME:~3,2%%TIME:~6,2%
SET args= -std=c17 -g -DDEBUG -D_DEBUG -DRENDERER_VULKAN -debug -D_CRT_SECURE_NO_WARNINGS -Werror -Wall -Wno-unused-function -Wgnu-empty-initializer
SET include_path=-I D:\Guigui\Work\Prog\_include\ -I %VULKAN_SDK%\include -I src/

SET linker_options=-L D:\Guigui\Work\Prog\_lib -L %VULKAN_SDK%\lib -Xlinker -incremental:no
SET libs= -lvulkan-1.lib

SET start_time=%TIME%

PUSHD tmp\
DEL /Q *.pdb 2> NUL
POPD

ECHO Building win32.exe
clang %args% %include_path% src/platform/platform_win32.c -o bin/win32.exe %linker_options% %libs% -lShell32.lib -lUser32.lib -Xlinker -SUBSYSTEM:WINDOWS -Xlinker -PDB:tmp/win32.pdb
IF !ERRORLEVEL! == 0 (
    ECHO BUILD OK
)

SET module_name=renderer

ECHO Building %module_name% module
clang %args% %include_path% -shared src/%module_name%/%module_name%.c -o bin/%module_name%.dll %linker_options% %libs% -Xlinker -PDB:tmp/%module_name%_%timestamp%.pdb -Xlinker -IMPLIB:tmp/%module_name%.lib
if !ERRORLEVEL! == 0 (
    ECHO a > bin/%module_name%.meta
    ECHO BUILD OK
)

SET module_name=game

ECHO Building %module_name% module
clang %args% %include_path% -shared src/game.c -o bin/%module_name%.dll %linker_options% %libs% -Xlinker -PDB:tmp/%module_name%_%timestamp%.pdb -Xlinker -IMPLIB:tmp/%module_name%.lib
IF !ERRORLEVEL! == 0 (
    ECHO a > bin/%module_name%.meta
    ECHO BUILD OK
)

SET end_time=%TIME%
ECHO ----
ECHO [%start_time%]
ECHO [%end_time%]