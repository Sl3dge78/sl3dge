@echo off
setlocal enabledelayedexpansion

SET timestamp=%TIME:~3,2%%TIME:~6,2%

REM SET rdr_arg=-DRENDERER_VULKAN -I %VULKAN_SDK%\include -L %VULKAN_SDK%\lib -lvulkan-1.lib
SET rdr_arg=-DRENDERER_OPENGL -lOpenGL32.lib -lGdi32.lib

SET args= -std=c17 -D__WIN32__ -g -DDEBUG -D_DEBUG -debug -D_CRT_SECURE_NO_WARNINGS -Werror -Wall -Wno-unused-function -Wgnu-empty-initializer
SET include_path=-I include/ -I src/
SET linker_options=-Xlinker -incremental:no
SET libs=

SET start_time=%TIME%

PUSHD tmp\
DEL /Q *.pdb 2> NUL
POPD

SET module_name=game

ECHO Building %module_name% module
clang %args% %include_path%  %rdr_arg% -shared src/game.c -o bin/%module_name%.dll %linker_options% %libs% -Xlinker -PDB:tmp/%module_name%_%timestamp%.pdb -Xlinker -IMPLIB:tmp/%module_name%.lib
IF !ERRORLEVEL! == 0 (
    ECHO a > bin/%module_name%.meta
    ECHO BUILD OK
)

ECHO Building win32.exe
clang %args% %include_path%  src/platform/platform_win32.c -o bin/win32.exe  %linker_options% %libs% -lShell32.lib -lUser32.lib -Xlinker -SUBSYSTEM:WINDOWS -Xlinker -PDB:tmp/win32.pdb
IF !ERRORLEVEL! == 0 (
    ECHO BUILD OK
)

REM SET module_name=renderer

REM ECHO Building %module_name% module
REM clang %args% %include_path% -shared src/%module_name%/%module_name%.c -o bin/%module_name%.dll %linker_options% %libs% -Xlinker -PDB:tmp/%module_name%_%timestamp%.pdb -Xlinker -IMPLIB:tmp/%module_name%.lib
REM if !ERRORLEVEL! == 0 (
REM     ECHO a > bin/%module_name%.meta
REM     ECHO BUILD OK
REM )



SET end_time=%TIME%
ECHO ----
REM ECHO START TIME : [%start_time%]
REM ECHO END TIME : [%end_time%]
